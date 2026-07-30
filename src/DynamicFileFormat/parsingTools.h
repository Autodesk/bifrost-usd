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

#ifndef BIFROSTUSD_DYNAMIC_FILE_FORMAT_PARSING_TOOLS_H
#define BIFROSTUSD_DYNAMIC_FILE_FORMAT_PARSING_TOOLS_H

#include <Bifrost/Math/Types.h>

#include <array>
#include <cctype>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace BifrostUsd {
namespace DynamicPayload {

/// \brief Boolean parser that accepts "0", "1", "true", "false", "yes" and "no"
/// (case-insensitive) as valid boolean representations, with optional
/// leading and trailing whitespace.
/// \return An optional containing the parsed boolean value if parsing
/// succeeded; std::nullopt otherwise.
std::optional<bool> maybeStob(const std::string& str);

//------------------------------------------------------------------------------
// Non-throwing replacements for std::stoi, std::stoul, std::stof etc.
//------------------------------------------------------------------------------
// Notes:
// - Accept leading whitespace            (e.g. "   3.14" is valid).
// - Accept trailing whitespace           (e.g. "3.14   " is valid).
// - Reject any other trailing characters (e.g. "3.14abc" is invalid).
// - Reject out-of-range results.
// - Floating-point conversions reject NaN and Infinity.

std::optional<float>  maybeStof(const std::string& str);
std::optional<double> maybeStod(const std::string& str);

/// \brief Non-throwing integral parser for any integral target type T.
///
/// Uses std::from_chars directly on T, so range enforcement is native to the
/// target type without a wider intermediate or manual min/max checks at call
/// sites.
template <typename T>
std::optional<T> maybeStoIntegral(std::string_view str) {
    static_assert(std::is_integral_v<T>,
                  "maybeStoIntegral requires an integral type");

    const char* first = str.data();
    const char* last  = first + str.size();

    // Skip leading whitespace:
    while (first < last && std::isspace(static_cast<unsigned char>(*first))) {
        ++first;
    }
    if (first == last) {
        return std::nullopt; // Empty or all-whitespace. Not a valid number.
    }

    // Reject negative sign for unsigned types:
    if constexpr (std::is_unsigned_v<T>) {
        if (*first == '-') {
            return std::nullopt; // Negative sign not valid for unsigned types.
        }
    }

    // Parse the number using std::from_chars, which parses characters directly
    // into the target type T and provides native range checking.
    T value{};
    const auto [ptr, ec] = std::from_chars(first, last, value, 10 /*base-10*/);
    if (ec != std::errc{}) {
        return std::nullopt; // No pattern matched or value is out of range.
    }

    // Skip potential trailing whitespace:
    const char* trailing = ptr;
    while (trailing < last &&
           std::isspace(static_cast<unsigned char>(*trailing))) {
        ++trailing;
    }
    if (trailing != last) {
        return std::nullopt; // Invalid non-whitespace trailing characters.
    }

    return value;
}

//------------------------------------------------------------------------------
// Helpers to parse array of strings
//------------------------------------------------------------------------------

using StringVec2Opt = std::optional<std::array<std::string, 2>>;
using StringVec3Opt = std::optional<std::array<std::string, 3>>;
using StringVec4Opt = std::optional<std::array<std::string, 4>>;

/// \brief Parse an input string "(<a>,<b>)" into an array of substrings
/// ["<a>","<b>"].
///
/// Notes on the expected format:
/// - Leading/trailing whitespace around the whole "(<a>,<b>)" expression is
///   allowed.
/// - The captured substrings are returned verbatim (whitespace is NOT
///   stripped), so " 3.14" is captured as-is when the input is "(1, 3.14)".
/// - Each component must contain at least one character that is not ',', '(',
///   or ')'.
///
/// \return An optional containing the array of substrings if parsing succeeded;
/// std::nullopt otherwise.
StringVec2Opt maybeGetStringVec2(const std::string& str);

/// \brief Parse an input string "(<a>,<b>,<c>)" into an array of substrings
/// ["<a>","<b>","<c>"].
///
/// See also the notes on the expected format in \c maybeGetStringVec2, which
/// apply similarly to this function.
///
/// \return An optional containing the array of substrings if parsing succeeded;
/// std::nullopt otherwise.
StringVec3Opt maybeGetStringVec3(const std::string& str);

/// \brief Parse an input string "(<a>,<b>,<c>,<d>)" into an array of
/// substrings ["<a>","<b>","<c>","<d>"].
///
/// See also the notes on the expected format in \c maybeGetStringVec2, which
/// apply similarly to this function.
///
/// \return An optional containing the array of substrings if parsing succeeded;
/// std::nullopt otherwise.
StringVec4Opt maybeGetStringVec4(const std::string& str);

//------------------------------------------------------------------------------
// Integer vector parsers
//------------------------------------------------------------------------------

/// \brief Parse a "(x, y)" string into a \c Bifrost::Math::int2.
/// \return An optional containing the parsed \c Bifrost::Math::int2 if parsing
/// succeeded and x and y are valid integers; std::nullopt otherwise.
std::optional<Bifrost::Math::int2> maybeGetInt2FromString(
    const std::string& str);

/// \brief Parse a "(x, y, z)" string into a \c Bifrost::Math::int3.
/// \return An optional containing the parsed \c Bifrost::Math::int3 if parsing
/// succeeded and x, y, and z are valid integers; std::nullopt otherwise.
std::optional<Bifrost::Math::int3> maybeGetInt3FromString(
    const std::string& str);

/// \brief Parse a "(x, y, z, w)" string into a \c Bifrost::Math::int4.
/// \return An optional containing the parsed \c Bifrost::Math::int4 if parsing
/// succeeded and x, y, z, and w are valid integers; std::nullopt otherwise.
std::optional<Bifrost::Math::int4> maybeGetInt4FromString(
    const std::string& str);

//------------------------------------------------------------------------------
// Float vector parsers
//------------------------------------------------------------------------------

/// \brief Parse a "(x, y)" string into a \c Bifrost::Math::float2.
/// \return An optional containing the parsed \c Bifrost::Math::float2 if
/// parsing succeeded and x and y are valid floats; std::nullopt otherwise.
std::optional<Bifrost::Math::float2> maybeGetFloat2FromString(
    const std::string& str);

/// \brief Parse a "(x, y, z)" string into a \c Bifrost::Math::float3.
/// \return An optional containing the parsed \c Bifrost::Math::float3 if
/// parsing succeeded and x, y, and z are valid floats; std::nullopt otherwise.
std::optional<Bifrost::Math::float3> maybeGetFloat3FromString(
    const std::string& str);

/// \brief Parse a "(x, y, z, w)" string into a \c Bifrost::Math::float4.
/// \return An optional containing the parsed \c Bifrost::Math::float4 if
/// parsing succeeded and x, y, z, and w are valid floats; std::nullopt
/// otherwise.
std::optional<Bifrost::Math::float4> maybeGetFloat4FromString(
    const std::string& str);

//------------------------------------------------------------------------------
// Double vector parsers
//------------------------------------------------------------------------------

/// \brief Parse a "(x, y)" string into a \c Bifrost::Math::double2.
/// \return An optional containing the parsed \c Bifrost::Math::double2 if
/// parsing succeeded and x and y are valid doubles; std::nullopt otherwise.
std::optional<Bifrost::Math::double2> maybeGetDouble2FromString(
    const std::string& str);

/// \brief Parse a "(x, y, z)" string into a \c Bifrost::Math::double3.
/// \return An optional containing the parsed \c Bifrost::Math::double3 if
/// parsing succeeded and x, y, and z are valid doubles; std::nullopt otherwise.
std::optional<Bifrost::Math::double3> maybeGetDouble3FromString(
    const std::string& str);

/// \brief Parse a "(x, y, z, w)" string into a \c Bifrost::Math::double4.
/// \return An optional containing the parsed \c Bifrost::Math::double4 if
/// parsing succeeded and x, y, z, and w are valid doubles; std::nullopt
/// otherwise.
std::optional<Bifrost::Math::double4> maybeGetDouble4FromString(
    const std::string& str);

} // namespace DynamicPayload
} // namespace BifrostUsd

#endif // BIFROSTUSD_DYNAMIC_FILE_FORMAT_PARSING_TOOLS_H
