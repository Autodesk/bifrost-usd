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

#ifndef BIFROSTUSD_DYNAMICFILEFORMAT_CONSTANTS_H
#define BIFROSTUSD_DYNAMICFILEFORMAT_CONSTANTS_H

#include <string_view>

namespace BifrostUsd::DynamicPayload {

// clang-format off

// DynamicFileFormat Log Prefix Strings
#define BIFROSTUSD_DFF_CTX_ "[BifrostUsd::DynamicFileFormat"
inline constexpr std::string_view kCtxDFF                           = BIFROSTUSD_DFF_CTX_ "] ";
inline constexpr std::string_view kCtxDFFGetGraphArgs               = BIFROSTUSD_DFF_CTX_ "::getGraphArgs] ";
inline constexpr std::string_view kCtxDFFExecuteGraph               = BIFROSTUSD_DFF_CTX_ "::executeGraph] ";
inline constexpr std::string_view kCtxDFFCreateStageFromGraphOutput = BIFROSTUSD_DFF_CTX_ "::createStageFromGraphOutput] ";
inline constexpr std::string_view kCtxDFFRead                       = BIFROSTUSD_DFF_CTX_ "::Read] ";
#undef BIFROSTUSD_DFF_CTX_

// clang-format on

} // namespace BifrostUsd::DynamicPayload

#endif // BIFROSTUSD_DYNAMICFILEFORMAT_CONSTANTS_H
