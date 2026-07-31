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

#ifndef BIFROSTUSD_GRAPH_EXECUTOR_TYPES_H
#define BIFROSTUSD_GRAPH_EXECUTOR_TYPES_H

#include <cstdint>
#include <type_traits>

namespace BifrostUsd::GraphExecutor {

/// \brief Types of terminal ports in the graph.
enum class TerminalType : uint8_t {
    /// Final terminal output.
    eFinal = 0,
    /// Proxy terminal output.
    eProxy = 1,
    /// Diagnostic terminal output.
    eDiagnostic = 2
};


/// \brief Settings for timeline configuration in graph execution.
struct TimelineSettings {
    /// Start frame of the timeline.
    double startFrame = 1.0;
    /// End frame of the timeline.
    double endFrame = 1.0;
    /// Minimum frame value.
    double minFrame = 1.0;
    /// Maximum frame value.
    double maxFrame = 1.0;
    /// Step between frames.
    double frameStep = 1.0;
};

/// \enum VerbosityLevel
/// \brief Specifies the verbosity level for logging or message output.
enum class VerbosityLevel {
    eSilent,            ///< Neither messages, warnings nor errors are output.
    eErrorsOnly,        ///< Only error messages are output.
    eErrorsAndWarnings, ///< Error messages and warnings are output.
    eAllMessages        ///< All messages, including informational, warning and
                        ///< error messages, are output.
};

/// Ordering operators for VerbosityLevel. Higher levels produce more output.
constexpr bool operator<(VerbosityLevel a, VerbosityLevel b) noexcept {
    using U = std::underlying_type_t<VerbosityLevel>;
    return static_cast<U>(a) < static_cast<U>(b);
}
constexpr bool operator>(VerbosityLevel a, VerbosityLevel b) noexcept {
    return b < a;
}
constexpr bool operator<=(VerbosityLevel a, VerbosityLevel b) noexcept {
    return !(b < a);
}
constexpr bool operator>=(VerbosityLevel a, VerbosityLevel b) noexcept {
    return !(a < b);
}
constexpr bool operator==(VerbosityLevel a, VerbosityLevel b) noexcept {
    using U = std::underlying_type_t<VerbosityLevel>;
    return static_cast<U>(a) == static_cast<U>(b);
}
constexpr bool operator!=(VerbosityLevel a, VerbosityLevel b) noexcept {
    return !(a == b);
}

} // namespace BifrostUsd::GraphExecutor

#endif // BIFROSTUSD_GRAPH_EXECUTOR_TYPES_H
