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

#ifndef BIFROSTUSD_DYNAMIC_FILE_FORMAT_DIAGNOSTICS_H
#define BIFROSTUSD_DYNAMIC_FILE_FORMAT_DIAGNOSTICS_H

#include "dffDiagnosticsRuntime.h"

#include <BifrostUsd/GraphExecutor/GraphExecutorTypes.h>
#include <pxr/pxr.h>

#include <string>
#include <string_view>

PXR_NAMESPACE_OPEN_SCOPE

/// \brief Record an error-level message in the DFF diagnostic system.
///
/// The message is recorded only when \p verbosity allows error messages.
inline void dffReportError(
    std::string_view                          ctx,
    const std::string&                        msg,
    BifrostUsd::GraphExecutor::VerbosityLevel verbosity) {
    if (verbosity >= BifrostUsd::GraphExecutor::VerbosityLevel::eErrorsOnly) {
        recordDffDiagnosticMsg(DffDiagnosticSeverity::Error, ctx, msg);
    }
}

/// \brief Record a warning-level message in the DFF diagnostic system.
///
/// The message is recorded only when \p verbosity allows warning messages.
inline void dffReportWarning(
    std::string_view                          ctx,
    const std::string&                        msg,
    BifrostUsd::GraphExecutor::VerbosityLevel verbosity) {
    if (verbosity >=
        BifrostUsd::GraphExecutor::VerbosityLevel::eErrorsAndWarnings) {
        recordDffDiagnosticMsg(DffDiagnosticSeverity::Warning, ctx, msg);
    }
}

/// \brief Record a status-level (verbose) message in the DFF diagnostic system.
///
/// The message is recorded only when \p verbosity allows status messages.
inline void dffReportStatus(
    std::string_view                          ctx,
    const std::string&                        msg,
    BifrostUsd::GraphExecutor::VerbosityLevel verbosity) {
    if (verbosity >= BifrostUsd::GraphExecutor::VerbosityLevel::eAllMessages) {
        recordDffDiagnosticMsg(DffDiagnosticSeverity::Status, ctx, msg);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // BIFROSTUSD_DYNAMIC_FILE_FORMAT_DIAGNOSTICS_H
