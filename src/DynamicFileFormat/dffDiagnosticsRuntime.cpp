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

#include "dffDiagnosticsRuntime.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Per-thread context pushed by DffDiagnosticScope and consumed by
// recordDffDiagnosticMsg(). DffDiagnosticScope's constructor writes the three
// fields and saves the previous values; its destructor restores them, giving
// RAII nesting semantics. recordDffDiagnosticMsg() reads the fields to
// populate the phase and bucketId of every DffDiagnosticRecord it dispatches.
struct DffDiagnosticScopeState {
    // DFF entry point currently active on this thread.
    DffDiagnosticPhase phase = DffDiagnosticPhase::Unknown;

    // Bucket id supplied by the innermost active DffDiagnosticScope.
    std::string bucketId = {};

    // False when no DffDiagnosticScope is active on this thread.
    bool isActive = false;
};

// Active scope state for the current thread.
thread_local DffDiagnosticScopeState g_scopeState;

// Set to true while recordDffDiagnosticMsg() is dispatching on this thread.
// This prevents a virtual handle() override from re-entering the recording
// path recordDffDiagnosticMsg() -> handle()->recordDffDiagnosticMsg()->...
// and deadlocking on g_sinksMutex, which is not a recursive mutex.
thread_local bool g_isRecording = false;

// Guards g_sinks. Also provides the lifetime guarantee for registered sinks:
// see the LOCK ORDER comment in recordDffDiagnosticMsg().
std::mutex g_sinksMutex;

// All currently registered sinks. Protected by g_sinksMutex.
std::vector<DffDiagnosticSink*> g_sinks;

} // namespace

DffDiagnosticSink::~DffDiagnosticSink() = default;

DffDiagnosticCollector::DffDiagnosticCollector() {
    registerDffDiagnosticSink(this);
}

DffDiagnosticCollector::~DffDiagnosticCollector() {
    unregisterDffDiagnosticSink(this);
}

void DffDiagnosticCollector::handle(const DffDiagnosticRecord& record) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_records.push_back(record);
}

std::vector<DffDiagnosticRecord> DffDiagnosticCollector::getSnapshot() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_records;
}

std::vector<DffDiagnosticRecord> DffDiagnosticCollector::getSnapshotForBucket(
    std::string_view bucketId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<DffDiagnosticRecord> records;
    for (const auto& record : m_records) {
        if (record.bucketId == bucketId) {
            records.push_back(record);
        }
    }
    return records;
}

void DffDiagnosticCollector::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_records.clear();
}

DffDiagnosticScope::DffDiagnosticScope(DffDiagnosticPhase phase,
                                       std::string        bucketId)
    : m_previousPhase(g_scopeState.phase),
      m_previousBucketId(g_scopeState.bucketId),
      m_hadPreviousScope(g_scopeState.isActive) {
    g_scopeState.phase    = phase;
    g_scopeState.bucketId = std::move(bucketId);
    g_scopeState.isActive = true;
}

DffDiagnosticScope::~DffDiagnosticScope() {
    g_scopeState.phase    = m_previousPhase;
    g_scopeState.bucketId = std::move(m_previousBucketId);
    g_scopeState.isActive = m_hadPreviousScope;
}

void DffDiagnosticScope::setBucketId(std::string bucketId) {
    g_scopeState.bucketId = std::move(bucketId);
}

void registerDffDiagnosticSink(DffDiagnosticSink* sink) {
    if (!sink) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_sinksMutex);
    if (std::find(g_sinks.begin(), g_sinks.end(), sink) == g_sinks.end()) {
        g_sinks.push_back(sink);
    }
}

void unregisterDffDiagnosticSink(DffDiagnosticSink* sink) {
    std::lock_guard<std::mutex> lock(g_sinksMutex);
    g_sinks.erase(std::remove(g_sinks.begin(), g_sinks.end(), sink),
                  g_sinks.end());
}

void recordDffDiagnosticMsg(DffDiagnosticSeverity severity,
                            std::string_view      context,
                            std::string_view      message) {
    // Re-entry guard: g_sinksMutex is non-recursive. A virtual handle()
    // override that calls recordDffDiagnosticMsg() (directly or indirectly)
    // would deadlock. Drop the record silently to avoid that:
    if (g_isRecording) {
        return;
    }

    DffDiagnosticRecord record;
    record.severity = severity;
    record.phase    = g_scopeState.isActive ? g_scopeState.phase
                                            : DffDiagnosticPhase::Unknown;
    record.bucketId =
        g_scopeState.isActive ? g_scopeState.bucketId : std::string{};
    record.context          = std::string{context};
    record.message          = std::string{message};
    record.formattedMessage = record.context + record.message;
    record.threadId         = std::this_thread::get_id();

    // LOCK ORDER: g_sinksMutex is always acquired before delegating to any
    // DffDiagnosticSink handle() methods, which may in turn acquire a
    // DffDiagnosticSink-internal lock (e.g. DffDiagnosticCollector::m_mutex).
    // Holding the lock on g_sinksMutex for the entire iteration gives a
    // lifetime guarantee: a concurrent DffDiagnosticCollector destructor that
    // calls unregisterDffDiagnosticSink will block until all handle() calls
    // return, preventing use-after-free without requiring shared ownership.
    g_isRecording = true;
    {
        std::lock_guard<std::mutex> lock(g_sinksMutex);
        for (auto* sink : g_sinks) {
            if (sink) {
                sink->handle(record);
            }
        }
    }
    g_isRecording = false;
}

PXR_NAMESPACE_CLOSE_SCOPE
