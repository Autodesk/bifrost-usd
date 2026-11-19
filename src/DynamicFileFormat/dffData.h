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

/// \file dffData.h
///
/// \brief A lazy \c SdfAbstractData implementation for the BifrostUsd Dynamic
/// File Format.
///
/// Instead of baking the whole timeline into the payload layer when the layer
/// is read, this data implementation evaluates the Bifrost graph lazily, at the
/// time sample(s) USD actually queries. Structural data (prim/property specs,
/// type names, default values) is discovered once by evaluating the graph at
/// the timeline start frame; per-time values for animated attributes are
/// produced on demand in \ref BifrostDffData::QueryTimeSample.

#ifndef BIFROSTUSD_DYNAMIC_FILE_FORMAT_DATA_H
#define BIFROSTUSD_DYNAMIC_FILE_FORMAT_DATA_H

#include <BifrostUsd/GraphExecutor/GraphExecutorConstants.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorFactory.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorTypes.h>

#include <pxr/base/tf/declarePtrs.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/abstractData.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>

#include <memory>
#include <optional>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \brief The parameters needed to (re)configure and execute a Bifrost graph
/// as a Dynamic Payload.
///
/// This is the decoded form of the \ref SdfFileFormat::FileFormatArguments that
/// \ref BifrostDynamicFileFormat::ComposeFieldsForFileFormatArguments produced.
/// It is built once in \ref BifrostDynamicFileFormat::InitData and carried by
/// the data implementation for the lifetime of the layer.
struct BifrostDffDataParams {
    /// The name of the compound definition to execute as a graph.
    std::string compoundName;
    /// The regular output port name to read (empty if only terminal ports are
    /// used).
    std::string outputName;
    /// The timeline settings (start/end frame). Absent when the payload is not
    /// animated, in which case the data is a single static evaluation.
    std::optional<BifrostUsd::GraphExecutor::TimelineSettings> timeline;
    /// The frame rate. Present whenever \ref timeline is present.
    std::optional<double> fps;
    /// The verbosity level used when reporting graph execution diagnostics.
    BifrostUsd::GraphExecutor::VerbosityLevel verbosity =
        BifrostUsd::GraphExecutor::defaultVerbosity;
    /// The diagnostics bucket id used to group diagnostics emitted while
    /// (re)evaluating the graph. Because evaluation happens lazily (outside
    /// \c Read), the bucket must be carried here so a \c DffDiagnosticScope can
    /// be re-established around each evaluation.
    std::string diagnosticsBucket;
    /// The raw file format arguments, kept so the graph inputs can be set
    /// before every execution via \c setGraphInputs.
    SdfFileFormat::FileFormatArguments rawArgs;

    /// Whether the arguments decoded and validated successfully. When false,
    /// the payload is treated as producing no content (mirroring the previous
    /// behavior where \c Read aborted on invalid arguments).
    bool valid = true;

    /// \brief Whether the payload is animated (i.e. has time samples).
    bool useTimeline() const { return timeline.has_value(); }

    /// \brief Decode a \ref BifrostDffDataParams from file format arguments.
    static BifrostDffDataParams FromArgs(
        const SdfFileFormat::FileFormatArguments& args);
};

class BifrostDffDataImpl;

TF_DECLARE_WEAK_AND_REF_PTRS(BifrostDffData);

/// \brief Lazy \c SdfAbstractData for the BifrostUsd Dynamic Payload.
///
/// Structural queries are answered from a topology discovered once at the
/// timeline start frame. Time sample queries evaluate the graph at the queried
/// frame on demand (caching the result). The implementation lives in
/// \ref BifrostDffDataImpl, held behind a shared pointer so that the \c const
/// \c SdfAbstractData query methods can mutate the lazily-populated caches.
class BifrostDffData : public SdfAbstractData {
public:
    /// \brief Factory. The data is always created with an explicit params
    /// object decoded from the file format arguments.
    static BifrostDffDataRefPtr New(const BifrostDffDataParams& params);

    ~BifrostDffData() override;

    /// \brief Whether the file format arguments decoded and validated
    /// successfully at \c InitData time (see \ref BifrostDffDataParams::valid).
    /// When false, this data produces no content, and \c Read reports that as
    /// a failure just as it did before argument decoding moved out of it.
    bool IsValid() const;

    /// \name SdfAbstractData structural overrides
    /// \{
    bool        StreamsData() const override;
    bool        IsEmpty() const override;
    void        CreateSpec(const SdfPath& path, SdfSpecType specType) override;
    bool        HasSpec(const SdfPath& path) const override;
    void        EraseSpec(const SdfPath& path) override;
    void        MoveSpec(const SdfPath& oldPath,
                         const SdfPath& newPath) override;
    SdfSpecType GetSpecType(const SdfPath& path) const override;

    bool Has(const SdfPath&        path,
             const TfToken&        fieldName,
             SdfAbstractDataValue* value) const override;
    bool Has(const SdfPath& path,
             const TfToken& fieldName,
             VtValue*       value = nullptr) const override;
    VtValue Get(const SdfPath& path, const TfToken& fieldName) const override;
    void    Set(const SdfPath& path,
                const TfToken& fieldName,
                const VtValue& value) override;
    void    Set(const SdfPath&                 path,
                const TfToken&                 fieldName,
                const SdfAbstractDataConstValue& value) override;
    void    Erase(const SdfPath& path, const TfToken& fieldName) override;
    std::vector<TfToken> List(const SdfPath& path) const override;
    /// \}

    /// \name SdfAbstractData time sample overrides
    /// \{
    std::set<double> ListAllTimeSamples() const override;
    std::set<double> ListTimeSamplesForPath(const SdfPath& path) const override;
    bool             GetBracketingTimeSamples(double  time,
                                              double* tLower,
                                              double* tUpper) const override;
    size_t           GetNumTimeSamplesForPath(const SdfPath& path) const override;
    bool             GetBracketingTimeSamplesForPath(const SdfPath& path,
                                                     double         time,
                                                     double*        tLower,
                                                     double* tUpper) const override;
    bool QueryTimeSample(const SdfPath&        path,
                         double                time,
                         SdfAbstractDataValue* optionalValue) const override;
    bool QueryTimeSample(const SdfPath& path,
                         double         time,
                         VtValue*       value) const override;
    void SetTimeSample(const SdfPath& path,
                       double         time,
                       const VtValue& value) override;
    void EraseTimeSample(const SdfPath& path, double time) override;
    /// \}

protected:
    void _VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const override;

private:
    /// Construct around a (possibly shared) implementation. \ref New decides
    /// whether to reuse an existing impl for the same graph-input signature or
    /// create a fresh one.
    explicit BifrostDffData(std::shared_ptr<BifrostDffDataImpl> impl);

    std::shared_ptr<BifrostDffDataImpl> _impl;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // BIFROSTUSD_DYNAMIC_FILE_FORMAT_DATA_H
