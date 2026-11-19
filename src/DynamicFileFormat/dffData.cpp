//-
// Copyright 2026 Autodesk, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//+

#include "dffData.h"

#include "dffArguments.h"
#include "dffDiagnosticsRuntime.h"
#include "dffExecuteGraphFrameToLayer.h"

#include <Amino/Core/String.h>

#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/changeBlock.h>
#include <pxr/usd/sdf/schema.h>
#include <pxr/usd/sdf/types.h>

#include <algorithm>
#include <iterator>
#include <limits>
#include <mutex>
#include <set>
#include <vector>

using namespace BifrostUsd::GraphExecutor;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

/// Build a stable fallback diagnostics bucket id from the raw file format
/// arguments, used when no explicit bucket was authored. Serializing every
/// key/value pair (map iteration is already key-sorted) keeps diagnostics
/// separated per distinct combination of arguments -- e.g. two prims that
/// share a compound but differ in inputs/settings get different buckets --
/// mirroring the fallback \c Read used before evaluation moved out of it:
/// \c SdfLayer::GetIdentifier(), which for a Dynamic File Format's generated
/// anonymous layer already encodes these same composed arguments.
std::string fallbackDiagnosticsBucket(
    const SdfFileFormat::FileFormatArguments& args) {
    std::string result;
    for (const auto& [key, value] : args) {
        result += key;
        result += '=';
        result += value;
        result += ';';
    }
    return result;
}

} // namespace

//==============================================================================
// BifrostDffDataParams
//==============================================================================

BifrostDffDataParams BifrostDffDataParams::FromArgs(
    const SdfFileFormat::FileFormatArguments& args) {
    BifrostDffDataParams params;
    params.rawArgs = args;

    // Decoding/validation used to happen in Read; now it happens here, at
    // InitData time. Re-establish a diagnostics scope so argument errors are
    // grouped under the right bucket, and abort (mark invalid) on any failure
    // exactly as Read used to return false. Fall back to a per-argument-
    // combination id (rather than just the compound name) so that two prims
    // sharing a compound but differing in inputs/settings do not collapse
    // into the same diagnostics bucket.
    params.diagnosticsBucket =
        getDiagnosticsBucketArg(args).value_or(fallbackDiagnosticsBucket(args));
    DffDiagnosticScope diagnosticScope(DffDiagnosticPhase::Read,
                                       params.diagnosticsBucket);

    // Verbosity first, so the remaining decoders can report at the right level.
    std::optional<VerbosityLevel> verbosityOpt;
    if (!getVerbosityLevelArg(args, verbosityOpt)) {
        params.valid = false;
        return params;
    }
    if (verbosityOpt.has_value()) {
        params.verbosity = verbosityOpt.value();
    }
    const auto verbosity = params.verbosity;

    std::optional<std::string> compoundNameOpt;
    if (!getCompoundNameArg(args, verbosity, compoundNameOpt) ||
        !compoundNameOpt.has_value()) {
        params.valid = false;
        return params;
    }
    params.compoundName = compoundNameOpt.value();

    std::optional<std::string> outputNameOpt;
    if (!getOutputNameArg(args, verbosity, outputNameOpt)) {
        params.valid = false;
        return params;
    }
    params.outputName = outputNameOpt.value_or("");

    std::optional<TimelineSettings> timelineOpt;
    if (!getTimelineSettingsArgs(args, verbosity, timelineOpt)) {
        params.valid = false;
        return params;
    }
    params.timeline = timelineOpt;
    if (params.timeline.has_value()) {
        std::optional<double> fpsOpt;
        if (!getFpsArg(args, verbosity, fpsOpt)) {
            params.valid = false;
            return params;
        }
        params.fps = fpsOpt;
    }

    return params;
}

//==============================================================================
// BifrostDffDataImpl
//==============================================================================

/// \brief Holds the lazily-populated state behind \ref BifrostDffData.
///
/// All the work happens here so that the \c const \c SdfAbstractData query
/// methods (which forward to this impl through a shared pointer) can populate
/// the topology and per-frame caches on demand.
///
/// A single mutex serializes all queries. Public methods acquire it once and
/// then call lock-free \c _l helpers, so graph execution and cache mutation are
/// thread-safe.
/// The graph is driven through a single persistent executor that run when a
/// frame is stepped forward: this is what lets stateful simulations (feedback
/// loops) accumulate correctly.
/// Scrubbing backward re-creates the executor and replays from the start frame
/// (if the requested frames are not already cached).
class BifrostDffDataImpl {
public:
    explicit BifrostDffDataImpl(const BifrostDffDataParams& params)
        : m_params(params) {
        // The set of time sample times is the discrete integer frames in the
        // configured range.
        if (m_params.useTimeline()) {
            // TODO(laforgg): consider using a more robust frame stepping that
            // respects the frame step rather than assuming a 1:1.
            const double startFrame = m_params.timeline->startFrame;
            const double endFrame   = m_params.timeline->endFrame;
            for (double frame = startFrame; frame < endFrame + 1; frame += 1) {
                m_animatedFrames.insert(frame);
            }
        }
    }

    bool isEmpty() {
        std::lock_guard<std::mutex> lock(m_mutex);
        ensureTopology_l();
        return !m_topologyLayer || m_topologyLayer->IsEmpty();
    }

    // m_params is set once at construction and never mutated afterward, so
    // this is safe to read without holding m_mutex.
    bool isValid() const { return m_params.valid; }

    SdfSpecType getSpecType(const SdfPath& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        ensureTopology_l();
        return m_topologyLayer ? m_topologyLayer->GetSpecType(path)
                               : SdfSpecTypeUnknown;
    }

    bool hasSpec(const SdfPath& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        ensureTopology_l();
        return m_topologyLayer && m_topologyLayer->HasSpec(path);
    }

    bool has(const SdfPath& path, const TfToken& field, VtValue* value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return has_l(path, field, value);
    }

    VtValue get(const SdfPath& path, const TfToken& field) {
        std::lock_guard<std::mutex> lock(m_mutex);
        VtValue                     value;
        has_l(path, field, &value);
        return value;
    }

    std::vector<TfToken> list(const SdfPath& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        ensureTopology_l();
        std::vector<TfToken> fields = m_topologyLayer
                                          ? m_topologyLayer->ListFields(path)
                                          : std::vector<TfToken>{};
        // Per-frame attributes do not carry a TimeSamples field in the
        // start-frame topology layer (their values are plain defaults there);
        // advertise it so USD treats them as time-varying. Graph-baked
        // attributes already list TimeSamples via the topology layer.
        if (isPerFrameAnimated_l(path) &&
            std::find(fields.begin(), fields.end(),
                      SdfFieldKeys->TimeSamples) == fields.end()) {
            fields.push_back(SdfFieldKeys->TimeSamples);
        }
        return fields;
    }

    void visitSpecs(const SdfAbstractData&      self,
                    SdfAbstractDataSpecVisitor* visitor) {
        if (!visitor) {
            return;
        }
        // Collect the paths while holding the lock, but invoke the visitor
        // after releasing it. Generic SdfAbstractDataSpecVisitor
        // implementations (e.g. SdfAbstractData_CopySpecs, used by
        // SdfAbstractData::CopyFrom and hence SdfLayer::TransferContent
        // whenever a source's StreamsData() is true, which ours always is)
        // call back into this same object's List()/Get() for every visited
        // spec. Since m_mutex is not recursive, calling the visitor while
        // still holding the lock would self-deadlock on that re-entrant
        // call.
        std::vector<SdfPath> paths;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            ensureTopology_l();
            if (!m_topologyLayer) {
                return;
            }
            m_topologyLayer->Traverse(
                SdfPath::AbsoluteRootPath(),
                [&paths](const SdfPath& path) { paths.push_back(path); });
        }
        for (const SdfPath& path : paths) {
            if (!visitor->VisitSpec(self, path)) {
                break;
            }
        }
    }

    bool queryTimeSample(const SdfPath& path, double time, VtValue* value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return queryTimeSample_l(path, time, value);
    }

    std::set<double> listAllTimeSamples() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return mergedTimeSamples_l();
    }

    std::set<double> listTimeSamplesForPath(const SdfPath& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (isPerFrameAnimated_l(path)) {
            return m_animatedFrames;
        }
        return m_topologyLayer ? m_topologyLayer->ListTimeSamplesForPath(path)
                               : std::set<double>{};
    }

    size_t numTimeSamplesForPath(const SdfPath& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (isPerFrameAnimated_l(path)) {
            return m_animatedFrames.size();
        }
        return m_topologyLayer ? m_topologyLayer->GetNumTimeSamplesForPath(path)
                               : 0;
    }

    bool getBracketingTimeSamples(double time, double* tLower, double* tUpper) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return bracketWithin(mergedTimeSamples_l(), time, tLower, tUpper);
    }

    bool getBracketingTimeSamplesForPath(const SdfPath& path,
                                         double         time,
                                         double*        tLower,
                                         double*        tUpper) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (isPerFrameAnimated_l(path)) {
            return bracketWithin(m_animatedFrames, time, tLower, tUpper);
        }
        return m_topologyLayer &&
               m_topologyLayer->GetBracketingTimeSamplesForPath(path, time,
                                                                tLower, tUpper);
    }

private:
    // --- Lock-free helpers (callers hold m_mutex) ----------------------------

    bool has_l(const SdfPath& path, const TfToken& field, VtValue* value) {
        ensureTopology_l();
        if (!m_topologyLayer) {
            return false;
        }

        // Time samples for per-frame attributes are generated lazily by
        // stepping the graph. Graph-baked attributes (whose start-frame layer
        // already carries time samples) and all other fields are served from
        // the topology layer directly.
        //
        // Note: deliberately NOT advertising stage timing metadata
        // (startTimeCode/endTimeCode/framesPerSecond/timeCodesPerSecond) on the
        // pseudo-root. Through a payload arc those do not affect the composed
        // stage's metadata, and a payload timeCodesPerSecond that differs from
        // the stage's would scale (misalign) our time samples during
        // composition.
        if (field == SdfFieldKeys->TimeSamples && isPerFrameAnimated_l(path) &&
            m_params.useTimeline()) {
            if (value) {
                // Callers that ask for this field's value (as opposed to a
                // plain existence check) need the complete sample set: e.g.
                // UsdStage::Flatten and SdfLayer::TransferContent (used
                // whenever a source's StreamsData() is true, which ours
                // always is) fetch the whole SdfTimeSampleMap this way to
                // merge/copy it verbatim, rather than through
                // QueryTimeSample/ListTimeSamplesForPath. Ensure every frame
                // is computed (in ascending order, so stateful simulations
                // accumulate correctly) instead of only reporting the start
                // frame, which would silently truncate flattened/exported
                // animation to a single sample.
                SdfTimeSampleMap samples;
                for (double frame : m_animatedFrames) {
                    VtValue sampleValue;
                    if (queryTimeSample_l(path, frame, &sampleValue)) {
                        samples[frame] = sampleValue;
                    }
                }

                *value = VtValue(samples);
            }
            return true;
        }

        return m_topologyLayer->HasField(path, field, value);
    }

    bool queryTimeSample_l(const SdfPath& path, double time, VtValue* value) {
        ensureTopology_l();
        if (!m_topologyLayer) {
            return false;
        }
        if (isPerFrameAnimated_l(path)) {
            // Time codes map 1:1 to graph frames: ensure the graph has been
            // stepped to this frame (accumulating simulation state) so its
            // per-frame value has been stamped into the accumulating layer,
            // then read it back as a time sample.
            if (!m_accumLayer || !ensureFrameComputed_l(time)) {
                return false;
            }
            return m_accumLayer->QueryTimeSample(path, time, value);
        }
        // Graph-baked animation: the start-frame execution already produced the
        // full set of time samples; serve them directly.
        return m_topologyLayer->QueryTimeSample(path, time, value);
    }

    /// Bracket \p time within the sorted \p times set. Robust for arbitrary
    /// (possibly sparse or non-integer) sample sets.
    static bool bracketWithin(const std::set<double>& times,
                              double                  time,
                              double*                 tLower,
                              double*                 tUpper) {
        if (times.empty()) {
            return false;
        }
        if (time <= *times.begin()) {
            *tLower = *tUpper = *times.begin();
            return true;
        }
        if (time >= *times.rbegin()) {
            *tLower = *tUpper = *times.rbegin();
            return true;
        }
        auto upper = times.lower_bound(time); // first element >= time
        *tUpper    = *upper;
        // When *upper is exactly `time`, it brackets to itself; otherwise the
        // lower bound is the preceding sample. Comparing with > (not ==) keeps
        // this float-safe.
        *tLower = (*upper > time) ? *std::prev(upper) : *upper;
        return true;
    }

    /// The union of the topology layer's own time samples (graph-baked) and the
    /// configured per-frame sample times.
    std::set<double> mergedTimeSamples_l() {
        ensureTopology_l();
        std::set<double> times;
        if (m_topologyLayer) {
            times = m_topologyLayer->ListAllTimeSamples();
        }
        if (m_params.useTimeline()) {
            times.insert(m_animatedFrames.begin(), m_animatedFrames.end());
        }
        return times;
    }

    /// Discover the prim/property structure once, by evaluating (stepping to)
    /// the timeline start frame. The resulting layer also serves as the default
    /// (untimed) value source and is held separately so it is never evicted.
    void ensureTopology_l() {
        if (m_topologyReady) {
            return;
        }
        m_topologyReady = true;
        // Invalid arguments were already reported in FromArgs; produce no
        // content, just as the previous Read aborted on invalid arguments.
        if (!m_params.valid) {
            return;
        }

        // Evaluate the start frame once to discover the structure. This also
        // positions the persistent executor at the start frame so per-frame
        // stepping continues forward from here (stateful sims accumulate).
        resetExecutor_l();
        if (!m_executor) {
            return;
        }
        const double startFrame = topologyFrame();
        m_topologyLayer =
            executeGraphFrameToLayer(m_executor, m_params, startFrame);
        m_hasCurrent   = true;
        m_currentFrame = startFrame;
        if (!m_topologyLayer || !m_params.useTimeline()) {
            return;
        }

        // Freeze the per-frame classification BEFORE any sample is stamped: an
        // attribute spec that carries no graph-baked time samples in the
        // start-frame layer is animated by per-frame stepping. Capturing it now
        // keeps the classification stable even though _accumLayer will gain
        // time samples for these paths below.
        m_topologyLayer->Traverse(
            SdfPath::AbsoluteRootPath(), [this](const SdfPath& path) {
                if (path.IsPrimPropertyPath() &&
                    m_topologyLayer->GetSpecType(path) ==
                        SdfSpecTypeAttribute &&
                    m_topologyLayer->GetNumTimeSamplesForPath(path) == 0) {
                    m_perFrameAnimatedPaths.insert(path);
                }
            });
        if (m_perFrameAnimatedPaths.empty()) {
            return;
        }

        // Seed the accumulating layer with the topology structure so the
        // attribute specs exist for SetTimeSample/QueryTimeSample, then stamp
        // the start frame's per-frame values (which are the topology defaults)
        // and mark the start frame computed so it is never executed twice.
        m_accumLayer = SdfLayer::CreateAnonymous("bifrost_dff_accum.usda");
        m_accumLayer->TransferContent(m_topologyLayer);
        stampFrameSamples_l(m_topologyLayer, startFrame);
        m_computedFrames.insert(startFrame);
    }

    /// A property path is "per-frame animated" when the payload has a timeline,
    /// the path resolves to an attribute spec in the discovered topology, and
    /// that attribute does NOT already carry graph-baked time samples in the
    /// start-frame layer. Such attributes get their per-frame values by
    /// stepping the graph; attributes the graph already time-sampled in a
    /// single execution are served directly from the topology layer.
    ///
    /// The classifying set is captured once in \ref ensureTopology_l, before
    /// any per-frame sample is stamped, so this stays a pure lookup that is
    /// unaffected by the time samples \ref _accumLayer later accumulates.
    bool isPerFrameAnimated_l(const SdfPath& path) {
        ensureTopology_l();
        return m_perFrameAnimatedPaths.find(path) !=
               m_perFrameAnimatedPaths.end();
    }

    double topologyFrame() const {
        return m_params.useTimeline() ? m_params.timeline->startFrame : 0.0;
    }

    /// (Re)create and configure the persistent executor. Resets the stepping
    /// cursor so the next evaluation starts from the timeline start frame.
    /// The computed-frame cache (m_computedFrames) and the accumulating layer
    /// are intentionally preserved;
    /// only the executor and the stepping cursor are reset.
    void resetExecutor_l() {
        m_executor =
            makeGraphExecutor(Amino::String{m_params.compoundName.c_str()});
        if (m_executor && m_params.useTimeline()) {
            m_executor->setTimelineSettings(m_params.timeline.value());
            if (m_params.fps.has_value()) {
                m_executor->setFps(m_params.fps.value());
            }
        }
        m_hasCurrent = false;
        m_currentFrame = -std::numeric_limits<double>::infinity();
    }

    /// Ensure the graph has been evaluated at \p frame and its per-frame
    /// attribute values stamped into \ref _accumLayer.
    ///
    /// Steps the persistent executor forward one frame at a time from its
    /// current position so stateful simulations accumulate, stamping each
    /// stepped frame's values as time samples. An already-computed frame
    /// returns immediately without re-executing, so a revisit (including a
    /// backward scrub to a frame already accumulated) is free. The executor is
    /// reset and replayed from the start frame only if it is missing or sits
    /// ahead of an uncomputed frame. That is, forward stepping keeps every frame
    /// up to the cursor computed. A fresh executor has no memory of prior frames,
    /// so replaying still has to run every frame from the start up to the cursor
    /// to rebuild its internal simulation state -- but any of those frames already
    /// cached in \ref m_computedFrames only needs its output discarded
    /// (via \ref advanceGraphFrame), not re-extracted and re-converted to USD.
    bool ensureFrameComputed_l(double frame) {
        // Do nothing when the frame is already computed.
        if (m_computedFrames.find(frame) != m_computedFrames.end()) {
            return true;
        }

        if (!m_executor || !m_hasCurrent || frame <= m_currentFrame) {
            resetExecutor_l();
        }
        if (!m_executor) {
            return false;
        }

        double nextFrame = m_hasCurrent ? m_currentFrame + 1.0 : topologyFrame();

        // The requested frame may be several frames ahead of the cursor, so
        // step through every intermediate frame (not just one) to keep
        // stateful simulations accumulating correctly.
        while (nextFrame <= frame) {
            if (m_computedFrames.find(nextFrame) != m_computedFrames.end()) {
                // Already computed and stamped -- reachable after a reset
                // above replays from the start frame, which (along with any
                // other previously-computed frame in the replayed range) is
                // already cached. The executor still needs to be run through
                // it to rebuild its internal simulation state, but its output
                // is already known, so skip the (about to be discarded)
                // output extraction/USD conversion that executeGraphFrameToLayer
                // performs.
                if (!advanceGraphFrame(m_executor, m_params, nextFrame)) {
                    return false;
                }
                m_hasCurrent   = true;
                m_currentFrame = nextFrame;
                nextFrame += 1.0;
                continue;
            }

            SdfLayerRefPtr layer =
                executeGraphFrameToLayer(m_executor, m_params, nextFrame);
            if (!layer) {
                // Leave the cursor at the last successfully computed frame
                // (do not advance m_currentFrame/m_hasCurrent to a failed
                // frame): otherwise a later retry of this exact frame would
                // find frame == m_currentFrame, skip the reset branch above,
                // and fall through the loop without ever re-attempting
                // execution, silently treating the failed frame as having no
                // time sample instead of retrying it.
                return false;
            }
            m_hasCurrent   = true;
            m_currentFrame = nextFrame;
            stampFrameSamples_l(layer, nextFrame);
            m_computedFrames.insert(nextFrame);

            nextFrame += 1.0;
        }

        return true;
    }

    /// Append the per-frame attributes' values from \p frameLayer (where they
    /// are authored as defaults) into \ref _accumLayer as time samples at
    /// \p frame. The accumulating layer is seeded with the topology structure
    /// in \ref ensureTopology_l, so the attribute specs already exist.
    void stampFrameSamples_l(const SdfLayerRefPtr& frameLayer, double frame) {
        if (!m_accumLayer || !frameLayer) {
            return;
        }
        SdfChangeBlock block;
        for (const SdfPath& path : m_perFrameAnimatedPaths) {
            const VtValue value =
                frameLayer->GetField(path, SdfFieldKeys->Default);
            if (!value.IsEmpty()) {
                m_accumLayer->SetTimeSample(path, frame, value);
            }
        }
    }

    BifrostDffDataParams m_params;

    std::mutex m_mutex;

    bool           m_topologyReady = false;
    SdfLayerRefPtr m_topologyLayer;

    // Single accumulating layer of per-frame time samples.
    // Per-frame attribute values are appended here as time samples as the
    // executor is stepped forward; it is never evicted, so a revisit of an
    // already-computed frame
    //  never re-executes the graph.
    SdfLayerRefPtr m_accumLayer;
    // The per-frame-animated attribute paths, captured once during topology
    // discovery (before any sample is stamped) so the classification stays
    // stable even after _accumLayer gains time samples.
    std::set<SdfPath> m_perFrameAnimatedPaths;
    // The frames already evaluated and stamped into _accumLayer.
    std::set<double> m_computedFrames;

    std::set<double> m_animatedFrames;

    // Persistent executor and the forward-stepping cursor.
    GraphExecutorPtr m_executor;
    // m_hasCurrent is false when the executor is just created or reset, but
    // not executed yet. That answers "does m_currentFrame mean anything right now?".
    bool             m_hasCurrent   = false;
    double           m_currentFrame = -std::numeric_limits<double>::infinity();
};

//==============================================================================
// BifrostDffData
//==============================================================================

BifrostDffDataRefPtr BifrostDffData::New(const BifrostDffDataParams& params) {
    return TfCreateRefPtr(
        new BifrostDffData(std::make_shared<BifrostDffDataImpl>(params)));
}

BifrostDffData::BifrostDffData(std::shared_ptr<BifrostDffDataImpl> impl)
    : _impl(std::move(impl)) {}

BifrostDffData::~BifrostDffData() = default;

bool BifrostDffData::IsValid() const { return _impl->isValid(); }

bool BifrostDffData::StreamsData() const {
    // Values are generated on demand rather than held fully in memory.
    return true;
}

bool BifrostDffData::IsEmpty() const { return _impl->isEmpty(); }

SdfSpecType BifrostDffData::GetSpecType(const SdfPath& path) const {
    return _impl->getSpecType(path);
}

bool BifrostDffData::HasSpec(const SdfPath& path) const {
    return _impl->hasSpec(path);
}

bool BifrostDffData::Has(const SdfPath& path,
                         const TfToken& fieldName,
                         VtValue*       value) const {
    return _impl->has(path, fieldName, value);
}

bool BifrostDffData::Has(const SdfPath&        path,
                         const TfToken&        fieldName,
                         SdfAbstractDataValue* value) const {
    if (!value) {
        return _impl->has(path, fieldName, nullptr);
    }
    VtValue vtValue;
    if (!_impl->has(path, fieldName, &vtValue)) {
        return false;
    }
    return value->StoreValue(vtValue);
}

VtValue BifrostDffData::Get(const SdfPath& path,
                            const TfToken& fieldName) const {
    return _impl->get(path, fieldName);
}

std::vector<TfToken> BifrostDffData::List(const SdfPath& path) const {
    return _impl->list(path);
}

void BifrostDffData::_VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const {
    _impl->visitSpecs(*this, visitor);
}

// Time samples ---------------------------------------------------------------

std::set<double> BifrostDffData::ListAllTimeSamples() const {
    return _impl->listAllTimeSamples();
}

std::set<double> BifrostDffData::ListTimeSamplesForPath(
    const SdfPath& path) const {
    return _impl->listTimeSamplesForPath(path);
}

bool BifrostDffData::GetBracketingTimeSamples(double  time,
                                              double* tLower,
                                              double* tUpper) const {
    return _impl->getBracketingTimeSamples(time, tLower, tUpper);
}

size_t BifrostDffData::GetNumTimeSamplesForPath(const SdfPath& path) const {
    return _impl->numTimeSamplesForPath(path);
}

bool BifrostDffData::GetBracketingTimeSamplesForPath(const SdfPath& path,
                                                     double         time,
                                                     double*        tLower,
                                                     double* tUpper) const {
    return _impl->getBracketingTimeSamplesForPath(path, time, tLower, tUpper);
}

bool BifrostDffData::QueryTimeSample(const SdfPath& path,
                                     double         time,
                                     VtValue*       value) const {
    return _impl->queryTimeSample(path, time, value);
}

bool BifrostDffData::QueryTimeSample(
    const SdfPath&        path,
    double                time,
    SdfAbstractDataValue* optionalValue) const {
    if (!optionalValue) {
        return _impl->queryTimeSample(path, time, nullptr);
    }
    VtValue vtValue;
    if (!_impl->queryTimeSample(path, time, &vtValue)) {
        return false;
    }
    return optionalValue->StoreValue(vtValue);
}

// Write operations ------------------------------------------------------------
//
// The payload layer is generated and read-only; USD does not author into it
// during composition. These overrides are therefore intentional no-ops.

void BifrostDffData::CreateSpec(const SdfPath& /*path*/,
                                SdfSpecType /*specType*/) {}
void BifrostDffData::EraseSpec(const SdfPath& /*path*/) {}
void BifrostDffData::MoveSpec(const SdfPath& /*oldPath*/,
                              const SdfPath& /*newPath*/) {}
void BifrostDffData::Set(const SdfPath& /*path*/,
                         const TfToken& /*fieldName*/,
                         const VtValue& /*value*/) {}
void BifrostDffData::Set(const SdfPath& /*path*/,
                         const TfToken& /*fieldName*/,
                         const SdfAbstractDataConstValue& /*value*/) {}
void BifrostDffData::Erase(const SdfPath& /*path*/,
                           const TfToken& /*fieldName*/) {}
void BifrostDffData::SetTimeSample(const SdfPath& /*path*/,
                                   double /*time*/,
                                   const VtValue& /*value*/) {}
void BifrostDffData::EraseTimeSample(const SdfPath& /*path*/, double /*time*/) {
}

PXR_NAMESPACE_CLOSE_SCOPE
