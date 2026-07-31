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

#ifndef BIFROSTUSD_DYNAMIC_FILE_FORMAT_STRING_TO_ANY_H
#define BIFROSTUSD_DYNAMIC_FILE_FORMAT_STRING_TO_ANY_H

#include <Amino/Core/Any.h>
#include <string>

namespace BifrostUsd {
namespace DynamicPayload {

/// Enum to indicate status of a type conversion.
enum class ConversionStatus : unsigned int {
    // Conversion succeeded:
    kSuccess,

    // Conversion failed because the input is empty or invalid:
    kFailure_InvalidInput,

    // Conversion failed because the input type is unsupported:
    kFailure_UnsupportedType
};

/// Struct used to call stringToAny and return both the result and the status.
struct StringToAnyResult {
    Amino::Any       value;
    ConversionStatus status;
};

/// \brief Helper function to convert a string to an \c Amino::Any of a
/// supported type.
///
/// Supported types include bool, int, float, double, string, and vector types
/// (e.g. float3, double4, etc.).
///
/// \param       typeName The name of the Amino type to convert to, as a string.
/// \param       str      The string to convert.
/// \return The converted value wrapped in an \c Amino::Any and the conversion
/// status; if the conversion succeeded, the returned \c Amino::Any contains
/// the converted value and the status is \c ConversionStatus::kSuccess;
/// otherwise, the returned \c Amino::Any is empty and the status indicates
/// the reason for the failure.
StringToAnyResult stringToAny(const std::string& typeName,
                              const std::string& str);

} // namespace DynamicPayload
} // namespace BifrostUsd

#endif // BIFROSTUSD_DYNAMIC_FILE_FORMAT_STRING_TO_ANY_H
