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

#ifndef BIFROSTUSD_DYNAMIC_FILE_FORMAT_TYPE_NAMES_H
#define BIFROSTUSD_DYNAMIC_FILE_FORMAT_TYPE_NAMES_H

/// \file dffTypeNames.h
///
/// Single source of truth for the Amino type name strings that are produced by
/// \c fileFormat.cpp (via \c UsdToAminoTypeName) and consumed by \c
/// stringToAny.cpp.
///
/// Including this header from both translation units guarantees that a rename
/// or addition in one place is a compile error in the other.
///
/// This header has no USD or Amino dependencies so it can also be included
/// from unit tests that have no USD linkage.

#include <array>
#include <string_view>

namespace BifrostUsd {
namespace DynamicPayload {
namespace DffTypeNames {

// clang-format off

// Scalar primitive types
inline constexpr std::string_view kBool     = "bool";
inline constexpr std::string_view kChar     = "char";
inline constexpr std::string_view kUchar    = "uchar";
inline constexpr std::string_view kShort    = "short";
inline constexpr std::string_view kUshort   = "ushort";
inline constexpr std::string_view kInt      = "int";
inline constexpr std::string_view kUint     = "uint";
inline constexpr std::string_view kLong     = "long";
inline constexpr std::string_view kUlong    = "ulong";
inline constexpr std::string_view kFloat    = "float";
inline constexpr std::string_view kDouble   = "double";
inline constexpr std::string_view kString   = "string";

// Math vector types (float)
inline constexpr std::string_view kFloat2   = "Math::float2";
inline constexpr std::string_view kFloat3   = "Math::float3";
inline constexpr std::string_view kFloat4   = "Math::float4";

// Math vector types (double)
inline constexpr std::string_view kDouble2  = "Math::double2";
inline constexpr std::string_view kDouble3  = "Math::double3";
inline constexpr std::string_view kDouble4  = "Math::double4";

// Math vector types (int)
inline constexpr std::string_view kInt2     = "Math::int2";
inline constexpr std::string_view kInt3     = "Math::int3";
inline constexpr std::string_view kInt4     = "Math::int4";

/// All unique Amino type names that stringToAny() is expected to support.
/// This is the authoritative list: fileFormat.cpp maps USD types to these
/// names and stringToAny() must handle every entry.
inline constexpr std::array kAllTypeNames = {
    kBool,
    kChar,    kUchar,
    kShort,   kUshort,
    kInt,     kUint,
    kLong,    kUlong,
    kFloat,   kDouble,
    kString,
    kFloat2,  kFloat3,  kFloat4,
    kDouble2, kDouble3, kDouble4,
    kInt2,    kInt3,    kInt4
};

// clang-format on

} // namespace DffTypeNames
} // namespace DynamicPayload
} // namespace BifrostUsd

#endif // BIFROSTUSD_DYNAMIC_FILE_FORMAT_TYPE_NAMES_H
