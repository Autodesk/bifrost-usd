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

#include "parsingTools.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <regex>

namespace {

template <typename T>
std::optional<T> maybeStrToFloatingImpl(const char* str) {
    static_assert(std::is_floating_point_v<T>);

    if (!str) {
        return std::nullopt;
    }

    // Extract the floating point number using std::strtold which has the widest
    // range and can be used for range checking for all floating point types.
    errno              = 0;
    char*       strEnd = nullptr;
    long double num    = std::strtold(str, &strEnd);
    if (str == strEnd) {
        return std::nullopt; // No conversion can be performed.
    }
    if (errno == ERANGE) {
        return std::nullopt; // The value is out of range for a long double.
    }

    // std::strtold accepts strings 'inf', '-inf', 'nan' and '-nan'
    // (without producing ERANGE error) but we consider them as invalid
    // input data:
    if (std::isnan(num) || std::isinf(num)) {
        return std::nullopt;
    }

    // Check if the number is within the representable range of the type T.
    const auto lowest =
        static_cast<long double>(std::numeric_limits<T>::lowest());
    const auto highest =
        static_cast<long double>(std::numeric_limits<T>::max());
    if (num < lowest || num > highest) {
        return std::nullopt; // The value is out of range for type T.
    }

    // Skip potential trailing white spaces:
    while (std::isspace(static_cast<unsigned char>(*strEnd))) {
        ++strEnd;
    }
    if (*strEnd != '\0') {
        return std::nullopt; // Invalid non-whitespace trailing characters.
    }

    return static_cast<T>(num);
}

} // namespace

namespace BifrostUsd {
namespace DynamicPayload {

std::optional<bool> maybeStob(const std::string& str) {
    const char* begin = str.c_str();
    const char* end   = begin + str.size();

    // Skip leading and trailing whitespace:
    while (begin < end && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }

    std::string lowerStr;
    lowerStr.reserve(static_cast<size_t>(end - begin));
    for (const char* p = begin; p < end; ++p) {
        lowerStr +=
            static_cast<char>(std::tolower(static_cast<unsigned char>(*p)));
    }

    // Accepts the same boolean string tokens as OpenUSD's Sdf_BoolFromString
    // (pxr/usd/sdf/parserHelpers.cpp), matched case-insensitively.
    if (lowerStr == "0" || lowerStr == "false" || lowerStr == "no") {
        return false;
    }
    if (lowerStr == "1" || lowerStr == "true" || lowerStr == "yes") {
        return true;
    }
    return std::nullopt; // Invalid boolean string.
}

//------------------------------------------------------------------------------
// Non-throwing replacements for std::stof and std::stod.
//------------------------------------------------------------------------------

std::optional<float> maybeStof(const std::string& str) {
    return maybeStrToFloatingImpl<float>(str.c_str());
}

std::optional<double> maybeStod(const std::string& str) {
    return maybeStrToFloatingImpl<double>(str.c_str());
}

//------------------------------------------------------------------------------
// Helpers to parse array of strings
//------------------------------------------------------------------------------

StringVec2Opt maybeGetStringVec2(const std::string& str) {
    static const std::regex re(R"(\s*\(([^,()]+),([^,()]+)\)\s*)");
    std::smatch             match;
    if (std::regex_match(str, match, re)) {
        return std::array<std::string, 2>{match[1].str(), match[2].str()};
    }
    return std::nullopt;
}

StringVec3Opt maybeGetStringVec3(const std::string& str) {
    static const std::regex re(R"(\s*\(([^,()]+),([^,()]+),([^,()]+)\)\s*)");
    std::smatch             match;
    if (std::regex_match(str, match, re)) {
        return std::array<std::string, 3>{match[1].str(), match[2].str(),
                                          match[3].str()};
    }
    return std::nullopt;
}

StringVec4Opt maybeGetStringVec4(const std::string& str) {
    static const std::regex re(
        R"(\s*\(([^,()]+),([^,()]+),([^,()]+),([^,()]+)\)\s*)");
    std::smatch match;
    if (std::regex_match(str, match, re)) {
        return std::array<std::string, 4>{match[1].str(), match[2].str(),
                                          match[3].str(), match[4].str()};
    }
    return std::nullopt;
}

//------------------------------------------------------------------------------
// Integer vector parsers
//------------------------------------------------------------------------------

std::optional<Bifrost::Math::int2> maybeGetInt2FromString(
    const std::string& str) {
    auto strVec = maybeGetStringVec2(str);
    if (strVec.has_value()) {
        const auto x = maybeStoIntegral<int>((*strVec)[0]);
        const auto y = maybeStoIntegral<int>((*strVec)[1]);
        if (x && y) {
            return Bifrost::Math::int2{*x, *y};
        }
    }
    return std::nullopt;
}

std::optional<Bifrost::Math::int3> maybeGetInt3FromString(
    const std::string& str) {
    auto strVec = maybeGetStringVec3(str);
    if (strVec.has_value()) {
        const auto x = maybeStoIntegral<int>((*strVec)[0]);
        const auto y = maybeStoIntegral<int>((*strVec)[1]);
        const auto z = maybeStoIntegral<int>((*strVec)[2]);
        if (x && y && z) {
            return Bifrost::Math::int3{*x, *y, *z};
        }
    }
    return std::nullopt;
}

std::optional<Bifrost::Math::int4> maybeGetInt4FromString(
    const std::string& str) {
    auto strVec = maybeGetStringVec4(str);
    if (strVec.has_value()) {
        const auto x = maybeStoIntegral<int>((*strVec)[0]);
        const auto y = maybeStoIntegral<int>((*strVec)[1]);
        const auto z = maybeStoIntegral<int>((*strVec)[2]);
        const auto w = maybeStoIntegral<int>((*strVec)[3]);
        if (x && y && z && w) {
            return Bifrost::Math::int4{*x, *y, *z, *w};
        }
    }
    return std::nullopt;
}

//------------------------------------------------------------------------------
// Float vector parsers
//------------------------------------------------------------------------------

std::optional<Bifrost::Math::float2> maybeGetFloat2FromString(
    const std::string& str) {
    auto strVec = maybeGetStringVec2(str);
    if (strVec.has_value()) {
        const auto x = maybeStof((*strVec)[0]);
        const auto y = maybeStof((*strVec)[1]);
        if (x && y) {
            return Bifrost::Math::float2{*x, *y};
        }
    }
    return std::nullopt;
}

std::optional<Bifrost::Math::float3> maybeGetFloat3FromString(
    const std::string& str) {
    auto strVec = maybeGetStringVec3(str);
    if (strVec.has_value()) {
        const auto x = maybeStof((*strVec)[0]);
        const auto y = maybeStof((*strVec)[1]);
        const auto z = maybeStof((*strVec)[2]);
        if (x && y && z) {
            return Bifrost::Math::float3{*x, *y, *z};
        }
    }
    return std::nullopt;
}

std::optional<Bifrost::Math::float4> maybeGetFloat4FromString(
    const std::string& str) {
    auto strVec = maybeGetStringVec4(str);
    if (strVec.has_value()) {
        const auto x = maybeStof((*strVec)[0]);
        const auto y = maybeStof((*strVec)[1]);
        const auto z = maybeStof((*strVec)[2]);
        const auto w = maybeStof((*strVec)[3]);
        if (x && y && z && w) {
            return Bifrost::Math::float4{*x, *y, *z, *w};
        }
    }
    return std::nullopt;
}

//------------------------------------------------------------------------------
// Double vector parsers
//------------------------------------------------------------------------------

std::optional<Bifrost::Math::double2> maybeGetDouble2FromString(
    const std::string& str) {
    auto strVec = maybeGetStringVec2(str);
    if (strVec.has_value()) {
        const auto x = maybeStod((*strVec)[0]);
        const auto y = maybeStod((*strVec)[1]);
        if (x && y) {
            return Bifrost::Math::double2{*x, *y};
        }
    }
    return std::nullopt;
}

std::optional<Bifrost::Math::double3> maybeGetDouble3FromString(
    const std::string& str) {
    auto strVec = maybeGetStringVec3(str);
    if (strVec.has_value()) {
        const auto x = maybeStod((*strVec)[0]);
        const auto y = maybeStod((*strVec)[1]);
        const auto z = maybeStod((*strVec)[2]);
        if (x && y && z) {
            return Bifrost::Math::double3{*x, *y, *z};
        }
    }
    return std::nullopt;
}

std::optional<Bifrost::Math::double4> maybeGetDouble4FromString(
    const std::string& str) {
    auto strVec = maybeGetStringVec4(str);
    if (strVec.has_value()) {
        const auto x = maybeStod((*strVec)[0]);
        const auto y = maybeStod((*strVec)[1]);
        const auto z = maybeStod((*strVec)[2]);
        const auto w = maybeStod((*strVec)[3]);
        if (x && y && z && w) {
            return Bifrost::Math::double4{*x, *y, *z, *w};
        }
    }
    return std::nullopt;
}

} // namespace DynamicPayload
} // namespace BifrostUsd
