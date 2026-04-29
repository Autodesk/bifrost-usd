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

#ifndef BIFROSTUSD_GRAPH_EXECUTOR_CONSTANTS_H
#define BIFROSTUSD_GRAPH_EXECUTOR_CONSTANTS_H

#include <string_view>

namespace BifrostUsd::GraphExecutor {

// clang-format off

// Terminal Type Name Strings
inline constexpr std::string_view kTerminalFinal      = "Core::Graph::terminal::final";
inline constexpr std::string_view kTerminalProxy      = "Core::Graph::terminal::proxy";
inline constexpr std::string_view kTerminalDiagnostic = "Core::Graph::terminal::diagnostic";

// GraphExecutor Log Prefix Strings
#define BIFROSTUSD_GEF_CTX_ "[BifrostUsd::GraphExecutorFactory"
inline constexpr std::string_view kCtxGExecFact                  = BIFROSTUSD_GEF_CTX_ "] ";
inline constexpr std::string_view kCtxGExecFactLoadConfigFile    = BIFROSTUSD_GEF_CTX_ "::loadConfigFile] ";
inline constexpr std::string_view kCtxGExecFactMakeGraphExecutor = BIFROSTUSD_GEF_CTX_ "::makeGraphExecutor] ";
#undef BIFROSTUSD_GEF_CTX_
#define BIFROSTUSD_GE_CTX_ "[BifrostUsd::GraphExecutor"
inline constexpr std::string_view kCtxGExec        = BIFROSTUSD_GE_CTX_ "] ";
inline constexpr std::string_view kCtxGExecExecute = BIFROSTUSD_GE_CTX_ "::execute] ";
#undef BIFROSTUSD_GE_CTX_

// clang-format on

} // namespace BifrostUsd::GraphExecutor

#endif // BIFROSTUSD_GRAPH_EXECUTOR_CONSTANTS_H
