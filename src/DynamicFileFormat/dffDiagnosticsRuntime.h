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

#ifndef BIFROSTUSD_DYNAMIC_FILE_FORMAT_DIAGNOSTICS_RUNTIME_H
#define BIFROSTUSD_DYNAMIC_FILE_FORMAT_DIAGNOSTICS_RUNTIME_H

#include "dffDiagnosticsRuntimeExport.h"

#include <pxr/pxr.h>

#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Architecture overview
// ---------------------
//
//  ┌──────────────────────┐
//  │  DffDiagnosticScope  │  RAII guard; pushes phase and bucketId onto
//  │  (nestable)          │  this thread's scope stack for its lifetime;
//  └──────────┬───────────┘  restores the previous values on exit.
//             │
//             │ writes thread-local phase + bucketId
//             ▼
//  ┌──────────────────────────────────────────────┐
//  │          recordDffDiagnosticMsg()            │
//  │  reads phase + bucketId from thread-local    │
//  │  state, builds a DffDiagnosticRecord, and    │
//  │  dispatches it to every registered sink      │
//  └────────┬─────────────────────┬───────────────┘
//           │ builds              │ dispatches to
//           ▼                     ▼
//  ┌─────────────────────┐  ┌───────────────────────┐
//  │ DffDiagnosticRecord │  │  DffDiagnosticSink    │
//  │ - severity          │  │  (abstract)           │
//  │ - phase             │  └──────────┬────────────┘
//  │ - bucketId          │             │ implemented by
//  │ - context + message │             ▼
//  │ - threadId          │  ╔═══════════════════════╗
//  └─────────────────────┘  ║ DffDiagnosticCollector║  ← start here
//                           ║  (in-memory store)    ║
//                           ╚═══════════════════════╝

/// \brief Name of the USD attribute that assigns a diagnostics bucket id to a
/// prim carrying a Bifrost DFF payload (e.g.
/// <tt>string bifrost:dff:diagnostics:id = "my_bucket"</tt>).
///
/// Author this attribute on the prim that owns the \c bifrostCompound
/// file-format field (the prim whose payload the DFF plugin resolves). The
/// plugin reads it at composition time and at read time to route all
/// diagnostics for that layer to the named bucket. When this attribute is
/// absent, the layer identifier is used as the bucket id instead.
///
/// \par Timing
/// The attribute must be present before the DFF evaluation you want to
/// observe:
/// - If authored in the \c .usda source before \c UsdStage::Open, it takes
///   effect immediately on Open.
/// - If authored programmatically on a live stage, it requires an explicit
///   \c stage->Reload() to trigger recomposition and activate the new key.
///
/// \par GTest usage
/// \code
///   DffDiagnosticCollector collector;
///   constexpr auto bucketId = "MyTest_some_scenario";
///
///   auto stage = UsdStage::Open("my_layer.usda");
///   // Bucket not yet active, so no records for this bucket:
///   EXPECT_TRUE(dffHasNoError(collector, bucketId));
///
///   // Author the attribute on the DFF payload prim, then reload so the
///   // plugin picks up the new bucket id on the next evaluation:
///   auto prim = stage->GetPrimAtPath(SdfPath{"/Root"});
///   prim.CreateAttribute(TfToken{kDffDiagnosticsBucketAttr.data()},
///                        SdfValueTypeNames->String, /*custom=*/false)
///       .Set(std::string{bucketId});
///   stage->Reload();
///
///   // Use the collector to inspect what the reload produced:
///   EXPECT_TRUE(dffHasError(collector, bucketId, "expected message"));
/// \endcode
inline constexpr std::string_view kDffDiagnosticsBucketAttr =
    "bifrost:dff:diagnostics:id";

/// \enum DffDiagnosticSeverity
/// \brief Severity levels recorded by the Dynamic File Format diagnostics
/// runtime.
enum class DffDiagnosticSeverity { Error, Warning, Status };

/// \enum DffDiagnosticPhase
/// \brief Top-level DFF entry points that can record diagnostics.
enum class DffDiagnosticPhase {
    /// No DFF scope is currently active on the calling thread.
    Unknown,

    /// Diagnostic recorded while composing file format arguments for a payload.
    ComposeFieldsForFileFormatArguments,

    /// Diagnostic recorded while reading or generating a dynamic layer.
    Read
};

/// \struct DffDiagnosticRecord
/// \brief Fully formatted diagnostic event captured by the DFF runtime.
///
/// Records are emitted by dffReportError(), dffReportWarning(), and
/// dffReportStatus() after the active DffDiagnosticScope has annotated the
/// current thread with a phase and bucket id.
struct DffDiagnosticRecord {
    /// Severity associated with this message.
    DffDiagnosticSeverity severity = DffDiagnosticSeverity::Error;

    /// DFF phase active on the recording thread when the record was created.
    DffDiagnosticPhase phase = DffDiagnosticPhase::Unknown;

    /// Correlation key used to group diagnostics for one logical source.
    std::string bucketId;

    /// Context prefix supplied by the DFF reporting helper.
    std::string context;

    /// Raw message body supplied by the DFF reporting helper.
    std::string message;

    /// Concatenation of \ref context and \ref message.
    std::string formattedMessage;

    /// Thread that recorded this diagnostic.
    std::thread::id threadId;
};

/// \class DffDiagnosticSink
/// \brief Abstract consumer interface for DFF diagnostic records.
///
/// Hosts such as tests or DCC adapters can register one or more sinks with the
/// runtime to receive every recorded record.
class BIFROSTUSD_DFF_DIAGNOSTICS_RUNTIME_DECL DffDiagnosticSink {
public:
    /// Virtual destructor for sink implementations.
    virtual ~DffDiagnosticSink();

    /// Handle a newly recorded diagnostic \p record.
    /// This method is called by \c recordDffDiagnosticMsg for each registered
    /// sink.
    ///
    /// \note Re-entry is not allowed: if a sink's \c handle() method calls
    /// \c recordDffDiagnosticMsg (directly or indirectly), the runtime will
    /// drop the new record silently to avoid self-deadlock.
    ///
    virtual void handle(const DffDiagnosticRecord& record) = 0;
};

/// \class DffDiagnosticCollector
/// \brief Thread-safe in-memory sink used to capture DFF diagnostics.
///
/// Constructing a collector automatically registers it with the runtime;
/// destroying it unregisters it.
class BIFROSTUSD_DFF_DIAGNOSTICS_RUNTIME_DECL DffDiagnosticCollector
    : public DffDiagnosticSink {
public:
    /// Construct and register this collector with the runtime.
    DffDiagnosticCollector();

    /// Unregister this collector from the runtime.
    ///
    /// If another thread is currently inside \c recordDffDiagnosticMsg, this
    /// destructor will block until that call returns.
    ~DffDiagnosticCollector() override;

    /// Append the given \p record to this collector.
    ///
    /// \note Re-entry is not allowed: if a sink's \c handle() method calls
    /// \c recordDffDiagnosticMsg (directly or indirectly), the runtime will
    /// drop the new record silently to avoid self-deadlock.
    ///
    void handle(const DffDiagnosticRecord& record) override;

    /// Return a snapshot of all collected records.
    std::vector<DffDiagnosticRecord> getSnapshot() const;

    /// Return only records whose bucket id matches \p bucketId.
    std::vector<DffDiagnosticRecord> getSnapshotForBucket(
        std::string_view bucketId) const;

    /// Remove all currently collected records.
    void clear();

private:
    mutable std::mutex               m_mutex;
    std::vector<DffDiagnosticRecord> m_records;
};

/// \class DffDiagnosticScope
/// \brief RAII guard that pushes a diagnostics context onto the calling
/// thread's scope stack for the duration of its lifetime.
///
/// The constructor records the current thread-local \c phase (see
/// \c DffDiagnosticPhase) and \c bucketId, then replaces them with the supplied
/// values. The destructor restores the saved values, so scopes nest correctly
/// at any depth, regardless of how many scopes are live simultaneously.
///
/// Every \c DffDiagnosticRecord created by \c recordDffDiagnosticMsg on this
/// thread while a scope is active inherits the innermost scope's phase and
/// bucket id. When no scope is active, the record's phase is
/// \c DffDiagnosticPhase::Unknown and its bucket id is empty.
///
/// \c setBucketId() updates only the active (innermost) scope's bucket id; it
/// does not affect what will be restored when this scope exits.
class BIFROSTUSD_DFF_DIAGNOSTICS_RUNTIME_DECL DffDiagnosticScope {
public:
    /// Push a new diagnostics context for \p phase and \p bucketId onto this
    /// thread's scope stack.
    DffDiagnosticScope(DffDiagnosticPhase phase, std::string bucketId);

    /// Pop this scope and restore the phase and bucket id that were active
    /// before this scope was constructed.
    ~DffDiagnosticScope();

    /// Replace the bucket id of the currently active (innermost) scope.
    void setBucketId(std::string bucketId);

private:
    DffDiagnosticPhase m_previousPhase = DffDiagnosticPhase::Unknown;
    std::string        m_previousBucketId;
    bool               m_hadPreviousScope = false;
};

/// Register \p sink to receive future DFF diagnostic records.
BIFROSTUSD_DFF_DIAGNOSTICS_RUNTIME_DECL
void registerDffDiagnosticSink(DffDiagnosticSink* sink);

/// Unregister \p sink from the DFF diagnostics runtime.
BIFROSTUSD_DFF_DIAGNOSTICS_RUNTIME_DECL
void unregisterDffDiagnosticSink(DffDiagnosticSink* sink);

/// Record one diagnostic message in the DFF diagnostics runtime.
///
/// This function creates a \c DffDiagnosticRecord and delivers it to all
/// registered \c DffDiagnosticSink instances through their \c handle() method.
/// The current thread's active \c DffDiagnosticScope, if any, contributes the
/// \c phase and \c bucketId stored in the built \c DffDiagnosticRecord.
BIFROSTUSD_DFF_DIAGNOSTICS_RUNTIME_DECL
void recordDffDiagnosticMsg(DffDiagnosticSeverity severity,
                            std::string_view      context,
                            std::string_view      message);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // BIFROSTUSD_DYNAMIC_FILE_FORMAT_DIAGNOSTICS_RUNTIME_H
