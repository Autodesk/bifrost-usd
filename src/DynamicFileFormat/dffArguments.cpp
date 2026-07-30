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

#include "dffArguments.h"

// DynamicFileFormat
#include "dffConstants.h"
#include "dffDiagnostics.h"
#include "dffDiagnosticsRuntime.h"
#include "dffTypeNames.h"
#include "fileFormat.h"
#include "parsingTools.h"
#include "stringToAny.h"

// Bifrost USD
#include <BifrostUsd/GraphExecutor/GraphExecutor.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorConstants.h>

// Open USD base headers
#include <pxr/base/gf/vec2d.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec2i.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec3i.h>
#include <pxr/base/gf/vec4d.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/gf/vec4i.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/pcp/dynamicFileFormatContext.h>
#include <pxr/usd/sdf/assetPath.h>

// C++ Standard Library
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace BifrostUsd::DynamicPayload;
using namespace BifrostUsd::GraphExecutor;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

/// \brief Maps a USD C++ type to the corresponding Amino type name string.
///
/// Specializations cover every USD value type that has a matching Amino
/// built-in or Math type.
template <typename T>
struct UsdToAminoTypeName {
    static_assert(!std::is_same_v<T, T>,
                  "UsdToAminoTypeName is not defined for this USD type");
};
// clang-format off

// Scalar primitive types - the template parameter is the C++ type that VtValue actually holds;
// the mapped string is the Amino type name used for encoding at composition time and decoding
// on the read side (stringToAny()).
template <> struct UsdToAminoTypeName<bool>            { static constexpr std::string_view value = DffTypeNames::kBool; };
template <> struct UsdToAminoTypeName<char>            { static constexpr std::string_view value = DffTypeNames::kChar; };
template <> struct UsdToAminoTypeName<unsigned char>   { static constexpr std::string_view value = DffTypeNames::kUchar; };
template <> struct UsdToAminoTypeName<short>           { static constexpr std::string_view value = DffTypeNames::kShort; };
template <> struct UsdToAminoTypeName<unsigned short>  { static constexpr std::string_view value = DffTypeNames::kUshort; };
template <> struct UsdToAminoTypeName<int>             { static constexpr std::string_view value = DffTypeNames::kInt; };
template <> struct UsdToAminoTypeName<unsigned int>    { static constexpr std::string_view value = DffTypeNames::kUint; };

// Special case for 64-bit integers:
//      int64_t = long on Linux/macOS (LP64), int64_t = long long on Windows (LLP64).
//      Using int64_t/uint64_t here ensures typeid() matches what VtValue holds on every platform,
//      unlike Amino::long_t (always long long) which diverges on LP64 systems where VtValue stores long.
template <> struct UsdToAminoTypeName<int64_t>         { static constexpr std::string_view value = DffTypeNames::kLong; };
template <> struct UsdToAminoTypeName<uint64_t>        { static constexpr std::string_view value = DffTypeNames::kUlong; };

template <> struct UsdToAminoTypeName<float>           { static constexpr std::string_view value = DffTypeNames::kFloat; };
template <> struct UsdToAminoTypeName<double>          { static constexpr std::string_view value = DffTypeNames::kDouble; };

// String-like USD types all map to Amino "string"
template <> struct UsdToAminoTypeName<std::string>     { static constexpr std::string_view value = DffTypeNames::kString; };
template <> struct UsdToAminoTypeName<TfToken>         { static constexpr std::string_view value = DffTypeNames::kString; };
template <> struct UsdToAminoTypeName<SdfAssetPath>    { static constexpr std::string_view value = DffTypeNames::kString; };

// Math vector types (float)
template <> struct UsdToAminoTypeName<GfVec2f>         { static constexpr std::string_view value = DffTypeNames::kFloat2; };
template <> struct UsdToAminoTypeName<GfVec3f>         { static constexpr std::string_view value = DffTypeNames::kFloat3; };
template <> struct UsdToAminoTypeName<GfVec4f>         { static constexpr std::string_view value = DffTypeNames::kFloat4; };

// Math vector types (double)
template <> struct UsdToAminoTypeName<GfVec2d>         { static constexpr std::string_view value = DffTypeNames::kDouble2; };
template <> struct UsdToAminoTypeName<GfVec3d>         { static constexpr std::string_view value = DffTypeNames::kDouble3; };
template <> struct UsdToAminoTypeName<GfVec4d>         { static constexpr std::string_view value = DffTypeNames::kDouble4; };

// Math vector types (int)
template <> struct UsdToAminoTypeName<GfVec2i>         { static constexpr std::string_view value = DffTypeNames::kInt2; };
template <> struct UsdToAminoTypeName<GfVec3i>         { static constexpr std::string_view value = DffTypeNames::kInt3; };
template <> struct UsdToAminoTypeName<GfVec4i>         { static constexpr std::string_view value = DffTypeNames::kInt4; };
// clang-format on

// clang-format off
using SupportedTypes =
    std::tuple<
        bool,
        char,
        unsigned char,
        short,
        unsigned short,
        int,
        unsigned int,
        int64_t,
        uint64_t,
        float,
        double,
        std::string,
        TfToken,
        SdfAssetPath,
        GfVec2f,
        GfVec3f,
        GfVec4f,
        GfVec2d,
        GfVec3d,
        GfVec4d,
        GfVec2i,
        GfVec3i,
        GfVec4i
    >;
// clang-format on

using TypeAndValuePair   = std::pair<std::string, std::string>;
using ParamNameToTypeMap = std::map<std::string, std::string>;

const std::string kColon{":"};

const std::map<std::string, VerbosityLevel> StrToVerbosityLevelMap{
    {"Silent", VerbosityLevel::eSilent},
    {"ErrorsOnly", VerbosityLevel::eErrorsOnly},
    {"ErrorsAndWarnings", VerbosityLevel::eErrorsAndWarnings},
    {"AllMessages", VerbosityLevel::eAllMessages}};

// Implementation of the function to apply an operation to each type in a tuple
template <typename Tuple, typename Func, std::size_t... I>
constexpr void for_each_type_impl(Func&& f, std::index_sequence<I...>) {
    // Fold expression to call f for each type
    (f(std::tuple_element_t<I, Tuple>{}), ...);
}

// Function to apply an operation to each type in a tuple
template <typename Tuple, typename Func>
constexpr void for_each_type(Func&& f) {
    constexpr std::size_t N = std::tuple_size<Tuple>::value;
    for_each_type_impl<Tuple>(std::forward<Func>(f),
                              std::make_index_sequence<N>{});
}

/// \brief Encode the type and value input strings using length-prefixing.
///
/// This function encodes the type and value input strings using
/// length-prefixing of the form "<typeNameLen>:<typeName><value>" to make the
/// parsing of the <typeName> and <value> deterministic, whatever special
/// characters any of them could contain now or in the future, even ':', '.' or
/// spaces.
///
/// \param [in] typeName The input string containing the name of the type.
/// \param [in] value The input string containing the stringified value.
/// \return A String containing the encoded type and value.
std::string encodeTypeAndValue(const std::string& typeName,
                               const std::string& value) {
    if (typeName.empty()) {
        dffReportError(kCtxDFFComposingValues,
                       "Invalid empty type string passed to encodeTypeAndValue."
                       " Falling back to \"bool\" type with \"0\" value.",
                       VerbosityLevel::eErrorsAndWarnings);
        return encodeTypeAndValue("bool", "0");
    }
    return std::to_string(typeName.size()) + kColon + typeName + value;
}

/// \brief Decode the type and value from the encoded string produced by
/// \c encodeTypeAndValue. It expects the input string to be in the format
/// "<typeNameLen>:<typeName><value>".
///
/// \param [in] encoded The encoded string containing the type and value.
/// \return A \c TypeAndValuePair pair containing the decoded type and value.
TypeAndValuePair decodeTypeAndValue(const std::string& encoded) {
    auto fail = [&encoded]() -> TypeAndValuePair {
        dffReportError(kCtxDFFRead,
                       "Invalid encoded type/value string passed to"
                       " decodeTypeAndValue: \"" +
                           encoded + "\"",
                       VerbosityLevel::eErrorsAndWarnings);
        return {"", ""};
    };
    auto isAllDigits = [](const std::string& s) {
        return !s.empty() &&
               std::all_of(s.begin(), s.end(), [](unsigned char c) {
                   return c >= '0' && c <= '9';
               });
    };

    const size_t colonPos = encoded.find(kColon);
    if (colonPos == std::string::npos) {
        return fail();
    }

    const std::string prefix = encoded.substr(0, colonPos);
    if (!isAllDigits(prefix)) {
        return fail();
    }

    const auto typeLenOpt = maybeStoIntegral<size_t>(prefix);
    if (!typeLenOpt.has_value()) {
        return fail();
    }
    const size_t typeNameLen   = *typeLenOpt;
    const size_t typeNameBegin = colonPos + 1;
    const size_t valueBegin    = typeNameBegin + typeNameLen;
    const size_t size          = encoded.size();
    // typeNameBegin must be < size and typeNameLen must be > 0 to have a valid
    // type, but we allow valueBegin == size to support an empty value:
    if (typeNameLen == 0 || typeNameBegin >= size || valueBegin > size) {
        return fail();
    }

    return {encoded.substr(typeNameBegin, typeNameLen),
            encoded.substr(valueBegin)};
}

/// \brief Helper function to report a missing required argument error.
void reportMissingArg(const std::string& argName, VerbosityLevel verbosity) {
    dffReportError(kCtxDFFRead,
                   "The required \"" + argName + "\" argument is missing.",
                   verbosity);
}
void reportMissingArg(const TfToken& token, VerbosityLevel verbosity) {
    reportMissingArg(tokenToText(token), verbosity);
}

/// \brief Helper function to report that an argument has the wrong type.
void reportInvalidArgType(const std::string& argName,
                          std::string_view   expectedTypeName,
                          const std::string& currentTypeName,
                          VerbosityLevel     verbosity) {
    dffReportError(kCtxDFFRead,
                   "The \"" + argName + "\" argument has type \"" +
                       currentTypeName + "\", expected \"" +
                       std::string{expectedTypeName} + "\".",
                   verbosity);
}
void reportInvalidArgType(const TfToken&     token,
                          std::string_view   expectedTypeName,
                          const std::string& currentTypeName,
                          VerbosityLevel     verbosity) {
    reportInvalidArgType(tokenToText(token), expectedTypeName, currentTypeName,
                         verbosity);
}

/// \brief Helper function to report that an argument has an invalid value.
void reportInvalidArgValue(const std::string& argName,
                           const std::string& currentValue,
                           VerbosityLevel     verbosity) {
    if (currentValue.empty()) {
        dffReportError(kCtxDFFRead,
                       "The \"" + argName + "\" argument has an empty value.",
                       verbosity);
    } else {
        dffReportError(kCtxDFFRead,
                       "The \"" + argName +
                           "\" argument has an invalid value \"" +
                           currentValue + "\".",
                       verbosity);
    }
}
void reportInvalidArgValue(const TfToken&     token,
                           const std::string& currentValue,
                           VerbosityLevel     verbosity) {
    reportInvalidArgValue(tokenToText(token), currentValue, verbosity);
}

/// \brief Find an argument, decode it and validate its type.
///
/// Output \p value is the decoded value string if the argument is found and
/// has the expected type; it is set to \c std::nullopt otherwise.
/// Function logs an error when the argument is required but missing.
/// Function logs an error when the argument is found but has the wrong type.
///
/// \return true if argument is missing and not required, or is present and has
/// the right type; false if argument is missing and required, or if argument
/// has the wrong type.
bool findArg(const SdfFileFormat::FileFormatArguments& args,
             VerbosityLevel                            verbosity,
             const TfToken&                            token,
             std::string_view                          expectedType,
             bool                                      required,
             std::optional<std::string>&               value) {
    value   = std::nullopt;
    auto it = args.find(tokenToText(token));

    if (it == args.end()) {
        if (required) {
            reportMissingArg(token, verbosity);
            return false; // not found and required => error
        }
        return true; // not found but not required => not an error
    }

    const auto [typeName, valueStr] = decodeTypeAndValue(it->second);
    if (typeName != expectedType) {
        reportInvalidArgType(token, std::string{expectedType}, typeName,
                             verbosity);
        return false; // invalid type is always an error
    }

    value = valueStr;
    return true;
}

} // namespace

/// \brief Returns a static map from VtValue type index to encoder function,
/// built once at startup by iterating SupportedTypes.
/// Each encoder converts a VtValue to an encoded <typeName>+<value> arg string.
///
/// \c for_each_type<SupportedTypes> runs exactly once during static
/// initialization, never on the hot path.
// clang-format off
const std::unordered_map<std::type_index, std::function<std::string(const VtValue&)>>&
getTypeAndValueEncoderMap() {
    static const auto map = []() {
        std::unordered_map<std::type_index, std::function<std::string(const VtValue&)>> m;
        for_each_type<SupportedTypes>([&m](auto placeholder) {
            using T = std::decay_t<decltype(placeholder)>;
            m.emplace(std::type_index(typeid(T)), [](const VtValue& val) -> std::string {
                constexpr std::string_view typeName = UsdToAminoTypeName<T>::value;
                // FileFormatArguments is std::map<std::string, std::string>, a plain str-to-str map
                // baked into the USD API. Everything must be encoded as a string at that level.
                // We call GetResolvedPath() here, at composition time, to obtain an absolute path,
                // doing the resolution at the only point in the pipeline where the full asset
                // resolver context is guaranteed to be available.
                if constexpr (std::is_same_v<T, SdfAssetPath>) {
                    const SdfAssetPath& ap       = val.UncheckedGet<T>();
                    const std::string&  resolved = ap.GetResolvedPath();
                    return encodeTypeAndValue(std::string{typeName},
                        resolved.empty() ? ap.GetAssetPath() : resolved);
                } else {
                    return encodeTypeAndValue(std::string{typeName},
                        TfStringify(val.UncheckedGet<T>()));
                }
            });
        });
        return m;
    }();
    return map;
}
// clang-format on

/// \brief Returns a static map from VtValue type index to Amino type name,
/// built once at startup by iterating SupportedTypes.
///
/// Useful for producing human-readable type names in error messages without
/// relying on mangled \c std::type_index::name() output.
const std::unordered_map<std::type_index, std::string_view>&
getTypeIndexToAminoTypeNameMap() {
    static const auto map = []() {
        std::unordered_map<std::type_index, std::string_view> m;
        for_each_type<SupportedTypes>([&m](auto placeholder) {
            using T = std::decay_t<decltype(placeholder)>;
            m.emplace(std::type_index(typeid(T)), UsdToAminoTypeName<T>::value);
        });
        return m;
    }();
    return map;
}

/// \brief Compose a group of fields from a \c VtDictionary composed value
/// and accumulate the encoded entries into \p args. Errors are logged.
/// If any error is detected then no entries are written to \p args.
///
/// Looks up \p groupFieldToken in the composition context. If found and holding
/// a \c VtDictionary, iterates over its entries, validates each key against
/// \p validTokens (if supplied), validates each value against \p validTypeIds
/// (if supplied), encodes the value via the encoder map (see
/// \c getTypeAndValueEncoderMap), and writes the encoded entry into \p args
/// keyed as "<attrPrefixToken>" + <field name>.
/// If \c attrNames is supplied, also appends the full attribute name to the
/// vector when an entry is successfully composed.
///
/// \param [in]  context            The dynamic file format composition context.
/// \param [in]  verbosity          The verbosity to use when logging messages.
/// \param [in]  groupFieldToken    The field token whose composed value is the
///                                 dictionary of fields to be composed.
/// \param [in]  validTokens        The valid field-name tokens for this group.
///                                 If nullptr, no validation is performed on
///                                 the field names.
/// \param [in]  validTypeIds       The valid type indices for this group.
///                                 If nullptr, no validation is performed on
///                                 the value types.
/// \param [in]  attrPrefixToken    Prefix prepended to each field name to form
///                                 the output argument key.
/// \param [out] args               If supplied, the FileFormatArguments map to
///                                 write the encoded entries into.
/// \return true if all entries were processed without error; false otherwise.
bool composeGroupOfFieldsToArgs(
    const PcpDynamicFileFormatContext&         context,
    VerbosityLevel                             verbosity,
    const TfToken&                             groupFieldToken,
    const std::vector<TfToken>*                validTokens,
    const std::unordered_set<std::type_index>* validTypeIds,
    const TfToken&                             attrPrefixToken,
    SdfFileFormat::FileFormatArguments*        args) {
    SdfFileFormat::FileFormatArguments tempArgs;
    VtValue                            val;

    if (!context.ComposeValue(groupFieldToken, &val)) {
        return true; // field not present, not an error
    }
    if (!val.IsHolding<VtDictionary>()) {
        dffReportError(kCtxDFFComposingValues,
                       "\"" + std::string{tokenToText(groupFieldToken)} +
                           "\" must be a dictionary.",
                       verbosity);
        return false;
    }

    const auto&         encoderMap = getTypeAndValueEncoderMap();
    const VtDictionary& dict       = val.UncheckedGet<VtDictionary>();
    bool                hasError   = false;

    for (const auto& item : dict) {
        const std::string& name  = item.first;
        const VtValue&     value = item.second;
        const bool         isValidToken =
            !validTokens || // if nullptr, no validation: all tokens are valid
            std::any_of(validTokens->begin(), validTokens->end(),
                        [&name](const TfToken& token) {
                            return tokenToText(token) == name;
                        });
        const bool isValidType =
            !validTypeIds || // if nullptr, all supported types are valid
            validTypeIds->count(std::type_index(value.GetTypeid())) > 0;

        if (!isValidToken) {
            dffReportError(kCtxDFFComposingValues,
                           "\"" + std::string{tokenToText(groupFieldToken)} +
                               "\" has unknown field \"" + name + "\".",
                           verbosity);
            hasError = true;
            continue; // Keep processing to report all errors at once.
        }

        const auto encoderIt =
            encoderMap.find(std::type_index(value.GetTypeid()));
        if (encoderIt == encoderMap.end()) {
            dffReportError(kCtxDFFComposingValues,
                           "Field \"" + name + "\" in \"" +
                               std::string{tokenToText(groupFieldToken)} +
                               "\" has unsupported type \"" +
                               value.GetTypeName() + "\".",
                           verbosity);
            hasError = true;
            continue; // Keep processing to report all errors at once.
        }

        if (!isValidType) {
            const auto& typeMap = getTypeIndexToAminoTypeNameMap();
            std::string validTypes;
            bool        first = true;
            for (const auto& typeId : *validTypeIds) {
                const auto it = typeMap.find(typeId);
                if (!first) validTypes += ", ";
                validTypes += (it != typeMap.end()) ? it->second : "<unknown>";
                first = false;
            }
            dffReportError(
                kCtxDFFComposingValues,
                "Field \"" + name + "\" in \"" +
                    std::string{tokenToText(groupFieldToken)} +
                    "\" has invalid type \"" + value.GetTypeName() +
                    "\". The valid types for this field are: " + validTypes,
                verbosity);
            hasError = true;
            continue; // Keep processing to report all errors at once.
        }

        const std::string fullName = tokenToText(attrPrefixToken) + name;
        if (args) {
            const std::string encoded = encoderIt->second(value);
            tempArgs[fullName]        = encoded;
        }
    }

    if (!hasError) {
        if (args) {
            for (auto& [k, v] : tempArgs) {
                (*args)[k] = std::move(v);
            }
        }
    }
    return !hasError;
}

/// \brief Compose the Bifrost inputs fields from a \c VtDictionary composed
/// value, validate each entry against a list of valid input port names, and
/// accumulate the encoded entries into \p args.
bool composeInputsFieldToArgs(
    const PcpDynamicFileFormatContext&        context,
    BifrostUsd::GraphExecutor::VerbosityLevel verbosity,
    const TfToken&                            inputsFieldToken,
    const StringArray&                        validInputPortNames,
    const TfToken&                            attrPrefixToken,
    SdfFileFormat::FileFormatArguments*       args) {
    SdfFileFormat::FileFormatArguments tempArgs;
    VtValue                            val;

    if (!context.ComposeValue(inputsFieldToken, &val)) {
        return true; // group of inputs field not present, not an error
    }
    if (!val.IsHolding<VtDictionary>()) {
        dffReportError(kCtxDFFComposingValues,
                       "\"" + std::string{tokenToText(inputsFieldToken)} +
                           "\" must be a dictionary.",
                       verbosity);
        return false;
    }

    const auto&         encoderMap = getTypeAndValueEncoderMap();
    const VtDictionary& dict       = val.UncheckedGet<VtDictionary>();
    bool                hasError   = false;

    // Build a set of valid port names for O(1) lookup.
    std::unordered_set<std::string> portNameSet;
    for (const auto& name : validInputPortNames)
        portNameSet.insert(name.c_str());

    for (const auto& item : dict) {
        const std::string& name  = item.first;
        const VtValue&     value = item.second;
        const bool         isValidPortName = portNameSet.count(name) > 0;

        if (!isValidPortName) {
            dffReportError(kCtxDFFComposingValues,
                           "\"" + std::string{tokenToText(inputsFieldToken)} +
                               "\" has unknown input port \"" + name + "\".",
                           verbosity);
            hasError = true;
            continue; // Keep processing to report all errors at once.
        }

        const auto encoderIt =
            encoderMap.find(std::type_index(value.GetTypeid()));
        if (encoderIt == encoderMap.end()) {
            dffReportError(kCtxDFFComposingValues,
                           "Input port \"" + name + "\" in \"" +
                               std::string{tokenToText(inputsFieldToken)} +
                               "\" has unsupported type \"" +
                               value.GetTypeName() + "\".",
                           verbosity);
            hasError = true;
            continue; // Keep processing to report all errors at once.
        }

        const std::string fullName = tokenToText(attrPrefixToken) + name;
        if (args) {
            const std::string encoded = encoderIt->second(value);
            tempArgs[fullName]        = encoded;
        }
    }

    if (!hasError) {
        if (args) {
            for (auto& [k, v] : tempArgs) {
                (*args)[k] = std::move(v);
            }
        }
    }
    return !hasError;
}

/// \brief Compose the Bifrost outputs field from a \c VtArray<TfToken> value,
/// validate each entry against \p validOutputPortNames, and accumulate one
/// entry per valid port name into \p args.
///
/// Looks up \p outputsFieldToken in the composition context. If found and
/// holding a \c VtArray<TfToken>, iterates over the array, validates each name
/// against \p validOutputPortNames, and writes one entry per valid port into
/// \p args.
/// If the field is absent, returns true (not an error).
/// If the field holds the wrong type or contains an unknown/empty port name,
/// logs an error and returns false.
///
/// \param [in]  context               The dynamic file format composition ctx.
/// \param [in]  verbosity             The verbosity to use when logging.
/// \param [in]  outputsFieldToken     The token for the Bifrost outputs field.
/// \param [in]  validOutputPortNames  Port names from
///                                    \c GraphExecutor::getOutputPortNames().
/// \param [in]  attrPrefixToken       Prefix prepended to each port name to
///                                    form the output argument key.
/// \param [out] args                  FileFormatArguments map to write into.
/// \return true if all entries were processed without error; false otherwise.
bool composeOutputsFieldToArgs(const PcpDynamicFileFormatContext& context,
                               VerbosityLevel                     verbosity,
                               const TfToken&     outputsFieldToken,
                               const StringArray& validOutputPortNames,
                               const TfToken&     attrPrefixToken,
                               SdfFileFormat::FileFormatArguments* args) {
    VtValue val;
    if (!context.ComposeValue(outputsFieldToken, &val)) {
        return true; // field not present, not an error
    }
    if (!val.IsHolding<VtArray<TfToken>>()) {
        dffReportError(kCtxDFFComposingValues,
                       "\"" + std::string{tokenToText(outputsFieldToken)} +
                           "\" must be a token[] array.",
                       verbosity);
        return false;
    }

    const auto&       portNames = val.UncheckedGet<VtArray<TfToken>>();
    const std::string prefix    = tokenToText(attrPrefixToken);
    bool              hasError  = false;

    // Build a set of valid port names for O(1) lookup.
    std::unordered_set<std::string> portNameSet;
    for (const auto& name : validOutputPortNames)
        portNameSet.insert(name.c_str());

    // We use a default value placeholder for entries. This value is not used,
    // getOutputNameArg reads the keys only.
    const std::string valuePlaceholder = encodeTypeAndValue("string", "");

    SdfFileFormat::FileFormatArguments tempArgs;

    for (const TfToken& portName : portNames) {
        if (portName.IsEmpty()) {
            dffReportError(kCtxDFFComposingValues,
                           "\"" +
                               std::string{tokenToText(outputsFieldToken)} +
                               "\" contains an empty output port name.",
                           verbosity);
            hasError = true;
            continue; // Keep processing to report all errors at once.
        }
        if (portNameSet.count(portName.GetString()) == 0) {
            dffReportError(kCtxDFFComposingValues,
                           "\"" + std::string{tokenToText(outputsFieldToken)} +
                               "\" has unknown output port \"" +
                               portName.GetString() + "\".",
                           verbosity);
            hasError = true;
            continue; // Keep processing to report all errors at once.
        }
        if (args) {
            tempArgs[prefix + portName.GetString()] = valuePlaceholder;
        }
    }

    if (!hasError && args) {
        for (auto& [k, v] : tempArgs) {
            (*args)[k] = std::move(v);
        }
    }
    return !hasError;
}

/// \brief Apply an attribute override to a FileFormatArgument.
///
/// Calls \c ComposeAttributeDefaultValue and encodes the value via the
/// encoder map (see \c getTypeAndValueEncoderMap). If \p attrName already has
/// an entry in \p args (written from a field), its encoded type is compared
/// to the attribute's type.
///
/// If the attribute has an unsupported type, or if there is a type mismatch,
/// an error is logged and \c false is returned.
///
/// \param [in]  context    The dynamic file format composition context.
/// \param [in]  verbosity  The verbosity to use when logging messages.
/// \param [in]  attrName   The name of the attribute to override.
/// \param [in,out] args    The FileFormatArguments map to query and write into.
/// \return \c true if there is no attribute override, or if the override was
/// applied successfully; \c false if the override's type mismatches the field's
/// type, or if the override's type is unsupported.
bool applyAttrOverrideToFileFormatArgument(
    const PcpDynamicFileFormatContext&  context,
    VerbosityLevel                      verbosity,
    const std::string&                  attrName,
    SdfFileFormat::FileFormatArguments* args) {
    VtValue value;
    if (!context.ComposeAttributeDefaultValue(TfToken(attrName.c_str()),
                                              &value)) {
        return true; // no attribute override => not an error
    }

    // Check for an already existing entry for this attrName:
    const auto argIt      = args->find(attrName);
    const bool isOverride = (argIt != args->end());

    // Retrieve the encoder function:
    const auto& encoderMap = getTypeAndValueEncoderMap();
    const auto  encoderIt = encoderMap.find(std::type_index(value.GetTypeid()));
    if (encoderIt == encoderMap.end()) {
        std::string msg = std::string("Attribute ") +
                          (isOverride ? "override for " : "") +
                          "\"" + attrName + "\" has unsupported type \"" +
                          value.GetTypeName() + "\".";
        if (isOverride) {
            msg += " Cannot apply override.";
        }
        dffReportError(kCtxDFFComposingValues, msg, verbosity);
        return false; // unsupported type => error
    }
    // Encode the <typeName> and <value> into a string:
    const std::string encoded = encoderIt->second(value);

    // If there was an existing entry for this attrName, compare types:
    if (isOverride) {
        const std::string previousType =
            decodeTypeAndValue(argIt->second).first;
        const std::string newType = decodeTypeAndValue(encoded).first;
        assert(!previousType.empty() && !newType.empty());
        if (previousType != newType) {
            dffReportError(kCtxDFFComposingValues,
                           "Attribute override for \"" + attrName +
                               "\" has type \"" + newType +
                               "\" but the field declared type is \"" +
                               previousType + "'. Cannot apply override.",
                           verbosity);
            return false; // type mismatch => error
        }
    }
    // Write the override into the args, potentially overwriting a previous
    // field value (when types match):
    (*args)[attrName] = encoded;
    return true; // attribute override applied successfully
}

std::optional<std::string> composeDiagnosticsBucketField(
    const PcpDynamicFileFormatContext&  context,
    VerbosityLevel                      verbosity,
    SdfFileFormat::FileFormatArguments* args) {
    VtValue value;
    if (!context.ComposeAttributeDefaultValue(
            TfToken(kDffDiagnosticsBucketAttr.data()), &value)) {
        return std::nullopt;
    }

    std::string bucketId;
    if (value.IsHolding<std::string>()) {
        bucketId = value.UncheckedGet<std::string>();
    } else if (value.IsHolding<TfToken>()) {
        bucketId = tokenToText(value.UncheckedGet<TfToken>());
    } else {
        dffReportError(kCtxDFFComposingValues,
                       "Field \"" + std::string{kDffDiagnosticsBucketAttr} +
                           "\" must be a string or token.",
                       verbosity);
        return std::nullopt;
    }

    if (bucketId.empty()) {
        return std::nullopt;
    }

    (*args)[std::string{kDffDiagnosticsBucketAttr}] = bucketId;
    return bucketId;
}

/// \brief Apply the attribute override to Bifrost output port names in \p args.
///
/// Probes \c bifrost:out:<portName> attributes for every port in
/// \p validOutputPortNames via \c ComposeAttributeDefaultValue.
/// If **any** attribute is present, all field-derived output entries (those
/// whose key starts with the prefix of \p attrPrefixToken) are removed from
/// \p args and replaced by entries for the attribute-confirmed port names only.
///
/// This all-or-nothing semantics means that when you author even a single
/// \c bifrost:out: attribute on the prim, you take full ownership of the
/// output port list - the \c bifrostOutputs field value is discarded entirely.
/// This also enables attribute-only output port specification with no field
/// present: any \c bifrost:out:<portName> attribute whose port name is in
/// \p validOutputPortNames will be discovered and added to the output list.
///
/// \param [in]  context               Composition context.
/// \param [in]  verbosity             The verbosity to use when logging.
/// \param [in]  validOutputPortNames  All valid output port names from
///                                    \c GraphExecutor::getOutputPortNames().
///                                    Only attributes whose port name appears
///                                    here are probed.
/// \param [in]  attrPrefixToken       Prefix that identifies output entries in
///                                    \p args.
/// \param [in,out] args               The FileFormatArguments map to update.
void applyOutputsAttrOverridesToArgs(const PcpDynamicFileFormatContext& context,
                                     [[maybe_unused]] VerbosityLevel verbosity,
                                     const StringArray& validOutputPortNames,
                                     const TfToken&     attrPrefixToken,
                                     SdfFileFormat::FileFormatArguments* args) {
    const std::string prefix = tokenToText(attrPrefixToken);

    // Probe each valid output port name for a "bifrost:out:<portName>"
    // attribute override.
    std::vector<std::string> presentAttrNames;
    for (const auto& portName : validOutputPortNames) {
        const std::string attrName = prefix + portName.c_str();
        VtValue           value;
        if (context.ComposeAttributeDefaultValue(TfToken(attrName.c_str()),
                                                 &value)) {
            presentAttrNames.push_back(attrName);
        }
    }
    if (presentAttrNames.empty()) {
        return; // No attribute override: keep field-derived entries as-is.
    }

    // At least one attribute is present: remove all field-derived output
    // entries from args, then re-add only attribute-confirmed port names.
    auto it = args->lower_bound(prefix);
    while (it != args->end() &&
           it->first.compare(0, prefix.size(), prefix) == 0) {
        it = args->erase(it);
    }

    // We use a default value placeholder for entries. This value is not used,
    // getOutputNameArg reads the keys only.
    const std::string valuePlaceholder = encodeTypeAndValue("string", "");

    for (const auto& name : presentAttrNames) {
        (*args)[name] = valuePlaceholder;
    }
}

/// \brief Retrieve and validate the compound name argument.
///
/// The compound name is required. If present and a non-empty string,
/// \p compoundName is set to the string value and true is returned.
/// If absent or invalid, an error is logged, \p compoundName is set to
/// \c std::nullopt and false is returned.
///
/// \param [in]  args          The File Format Arguments from the SdfLayer.
/// \param [in]  verbosity     The verbosity to use when logging messages.
/// \param [out] compoundName  Set to the non-empty \c std::string if valid;
///                            set to \c std::nullopt otherwise.
/// \return true if the arg is present and valid; false on any error.
bool getCompoundNameArg(const SdfFileFormat::FileFormatArguments& args,
                        VerbosityLevel                            verbosity,
                        std::optional<std::string>& compoundName) {
    const TfToken& token = BifrostDffCompoundAttrTokens->CompoundNameAttr;

    if (!findArg(args, verbosity, token, DffTypeNames::kString,
                 /*required=*/true, compoundName)) {
        compoundName = std::nullopt;
        return false; // missing arg and invalid type reported by findArg()
    }
    if (compoundName->empty()) {
        reportInvalidArgValue(token, *compoundName, verbosity);
        compoundName = std::nullopt;
        return false;
    }
    return true;
}

/// \brief Retrieve and validate the timeline start and end frame arguments.
///
/// Both arguments are optional, but if either is present, both must be present.
/// If neither is present, \p settings is set to \c std::nullopt and true is
/// returned (no timeline).
/// If both are present, each must decode as \c double, and startFrame must
/// be <= endFrame.
/// Otherwise an error is logged,\p settings is set to \c std::nullopt and
/// false is returned.
///
/// \param [in]  args      The File Format Arguments from the SdfLayer.
/// \param [in]  verbosity The verbosity to use when logging messages.
/// \param [out] settings  Set to parsed \c TimelineSettings if both are valid;
///                        set to \c std::nullopt if both are absent or if either
///                        argument is invalid.
/// \return true if the args are both absent or both valid; false otherwise.
bool getTimelineSettingsArgs(const SdfFileFormat::FileFormatArguments& args,
                             VerbosityLevel                   verbosity,
                             std::optional<TimelineSettings>& settings) {
    const TfToken& startToken = BifrostDffGlobalsAttrTokens->StartFrameAttr;
    const TfToken& endToken   = BifrostDffGlobalsAttrTokens->EndFrameAttr;

    const bool startPresent = args.count(tokenToText(startToken)) > 0;
    const bool endPresent   = args.count(tokenToText(endToken)) > 0;

    settings = std::nullopt;

    if (!startPresent && !endPresent) {
        return true; // both absent: no timeline, not an error
    }
    if (startPresent && !endPresent) {
        reportMissingArg(endToken, verbosity);
        return false;
    }
    if (!startPresent /* && endPresent */) {
        reportMissingArg(startToken, verbosity);
        return false;
    }

    // Both are present: validate their types
    std::optional<std::string> startStrOpt, endStrOpt;

    bool startTypeOk =
        findArg(args, verbosity, startToken, DffTypeNames::kDouble,
                /*required=*/false, startStrOpt);
    bool endTypeOk = findArg(args, verbosity, endToken, DffTypeNames::kDouble,
                             /*required=*/false, endStrOpt);

    if (!startTypeOk || !endTypeOk) {
        return false; // type error(s) reported by findArg()
    }

    // Parse into doubles
    const auto startOpt = maybeStod(*startStrOpt);
    if (!startOpt) {
        reportInvalidArgValue(startToken, *startStrOpt, verbosity);
        return false;
    }
    const auto endOpt = maybeStod(*endStrOpt);
    if (!endOpt) {
        reportInvalidArgValue(endToken, *endStrOpt, verbosity);
        return false;
    }

    // Validate range
    if (*startOpt > *endOpt) {
        dffReportError(kCtxDFFRead,
                       "The \"" + std::string{tokenToText(startToken)} +
                           "\" value (" + std::to_string(*startOpt) +
                           ") must be smaller or equal to the \"" +
                           std::string{tokenToText(endToken)} + "\" value (" +
                           std::to_string(*endOpt) + ").",
                       verbosity);
        return false;
    }

    TimelineSettings ts;
    ts.startFrame = *startOpt;
    ts.endFrame   = *endOpt;
    settings      = ts;
    return true;
}

/// \brief Retrieves and validates the fps argument.
///
/// fps is optional. If absent, \p fps is set to the default fps and true is
/// returned. If argument is present, it must decode as \c double and be greater
/// than 0; otherwise an error is logged, \p fps is set to \c std::nullopt and
/// false is returned.
///
/// \param [in]  args      The File Format Arguments from the SdfLayer.
/// \param [in]  verbosity The verbosity to use when logging messages.
/// \param [out] fps   Set to the parsed \c double if present and valid; set to
///                    the default fps if absent; set to \c std::nullopt upon
///                    error.
/// \return true if the arg is absent or valid; false on any error.
bool getFpsArg(const SdfFileFormat::FileFormatArguments& args,
               VerbosityLevel                            verbosity,
               std::optional<double>&                    fps) {
    fps                  = std::nullopt;
    const TfToken& token = BifrostDffGlobalsAttrTokens->FpsAttr;

    std::optional<std::string> strOpt;
    if (!findArg(args, verbosity, token, DffTypeNames::kDouble,
                 /*required=*/false, strOpt)) {
        return false; // invalid type reported by findArg()
    }
    if (!strOpt) {
        fps = BifrostUsd::GraphExecutor::defaultFps; // absent => use default
        return true;
    }
    const auto fpsOpt = maybeStod(*strOpt);
    if (!fpsOpt || *fpsOpt <= 0.0) {
        reportInvalidArgValue(token, *strOpt, verbosity);
        return false;
    }
    fps = *fpsOpt; // present and valid
    return true;
}

/// \brief Retrieves and validates the verbosity level argument.
///
/// verbosity level is optional. If absent, \p verbosityLevel is set to the
/// default verbosity level and true is returned. If argument is present, it
/// must decode as \c string and have one of the valid values; otherwise an
/// error is logged, \p verbosityLevel is set to \c std::nullopt and
/// false is returned.
///
/// \param [in]  args  The File Format Arguments from the SdfLayer.
/// \param [out] verbosityLevel Set to \c std::nullopt if absent, or to the
///                            parsed \c GraphExecutor::VerbosityLevel if valid.
/// \return true if the arg is absent or valid; false on any error.
bool getVerbosityLevelArg(const SdfFileFormat::FileFormatArguments& args,
                          std::optional<VerbosityLevel>& verbosityLevel) {
    verbosityLevel       = std::nullopt;
    const TfToken& token = BifrostDffSettingsAttrTokens->VerbosityLevelAttr;

    // Use hardcoded verbosity when we attempt to retrieve the verbosity level
    // itself, otherwise user could not know when something goes wrong or not
    // when they attempt to set it (and if default level is silent):
    std::optional<std::string> strOpt;
    if (!findArg(args, VerbosityLevel::eErrorsAndWarnings, token,
                 DffTypeNames::kString, /*required=*/false, strOpt)) {
        return false; // invalid type reported by findArg()
    }
    if (!strOpt) {
        // If not set explicitly, use the default verbosity level:
        verbosityLevel = BifrostUsd::GraphExecutor::defaultVerbosity;
        return true;
    }

    auto it = StrToVerbosityLevelMap.find(*strOpt);
    if (it == StrToVerbosityLevelMap.end()) {
        reportInvalidArgValue(token, *strOpt,
                              VerbosityLevel::eErrorsAndWarnings);
        std::stringstream validValues;
        bool              first = true;
        for (const auto& pair : StrToVerbosityLevelMap) {
            if (!first) validValues << ", ";
            validValues << pair.first;
            first = false;
        }
        dffReportError(
            kCtxDFFRead,
            "Valid verbosity level values are: " + validValues.str() + ".",
            VerbosityLevel::eErrorsAndWarnings);
        return false;
    }
    verbosityLevel = it->second;
    return true;
}

/// \brief Retrieve and validate the reload library argument.
///
/// The reload library argument is optional. If present and a boolean value,
/// \p reloadLibrary is set to the boolean value and true is returned.
/// If invalid, an error is logged, \p reloadLibrary is set to \c std::nullopt
/// and false is returned.
///
/// \param [in]  args           The File Format Arguments from the SdfLayer.
/// \param [in]  verbosity      The verbosity to use when logging messages.
/// \param [out] reloadLibrary  Set to the boolean value if present and valid;
///                             set to \c std::nullopt otherwise.
/// \return true if the arg is absent or valid; false on any error.
bool getReloadLibraryArg(const SdfFileFormat::FileFormatArguments& args,
                         VerbosityLevel                            verbosity,
                         std::optional<bool>& reloadLibrary) {
    reloadLibrary        = std::nullopt;
    const TfToken& token = BifrostDffSettingsAttrTokens->ReloadLibraryAttr;
    auto           it    = args.find(tokenToText(token));
    if (it == args.end()) {
        return true; // absent => not reloading, not an error
    }
    const auto [typeName, valueStr] = decodeTypeAndValue(it->second);
    const auto boolOpt              = maybeStob(valueStr);
    if (!boolOpt) {
        reportInvalidArgValue(token, valueStr, verbosity); // bad type or value
        return false;
    }
    reloadLibrary = *boolOpt;
    return true;
}

/// \brief Set the Bifrost graph inputs from given USD FileFormatArguments.
///
/// Called by \ref BifrostDynamicFileFormat::Read.
///
/// Iterate given FileFormatArguments and call GraphExecutor::setInput() for
/// each bifrost:in:<name> entry. The value of each input port argument is
/// decoded, then parsed and converted into an Amino::Any value, suitable
/// for setting the corresponding graph input port's value.
///
/// \param [in] args      The file format arguments from the SdfLayer.
/// \param [in] verbosity The verbosity to use when logging messages.
/// \return true if all inputs were set successfully; false otherwise.
bool setGraphInputs(BifrostUsd::GraphExecutor::GraphExecutorPtr& executor,
                    const SdfFileFormat::FileFormatArguments&    args,
                    BifrostUsd::GraphExecutor::VerbosityLevel    verbosity) {
    // The input port names are encoded in the arguments KEYs as
    // "<InputsAttrPrefix><portName>". Arguments are sorted by attribute names.
    // Attempt to jump to the first input port attribute:
    const std::string& kInputsPrefix =
        tokenToText(BifrostDffGroupingTokens->InputsAttrPrefix);
    auto it = args.lower_bound(std::string{kInputsPrefix});

    bool success = true;
    for (; it != args.end() &&
           it->first.compare(0, kInputsPrefix.size(), kInputsPrefix) == 0;
         ++it) {
        const auto&            attrName = it->first;
        const TypeAndValuePair decoded  = decodeTypeAndValue(it->second);
        const std::string&     typeName = decoded.first;
        const std::string&     valueStr = decoded.second;

        // Extract the port name by stripping the prefix:
        const std::string portName = attrName.substr(kInputsPrefix.size());

        auto [valueAny, status] = stringToAny(typeName, valueStr);
        if (portName.empty()) {
            dffReportError(kCtxDFFSetGraphInputs,
                           "The \"" + kInputsPrefix +
                               "\" argument has an empty port name.",
                           verbosity);
            success = false;
        } else if (status == ConversionStatus::kFailure_UnsupportedType) {
            dffReportError(kCtxDFFSetGraphInputs,
                           "Unsupported type \"" + typeName +
                               "\" for argument \"" + attrName + "\".",
                           verbosity);
            success = false;
        } else if (status == ConversionStatus::kFailure_InvalidInput) {
            dffReportError(kCtxDFFSetGraphInputs,
                           "Unable to parse \"" + valueStr + "\" as type \"" +
                               typeName + "\" for parameter \"" + attrName +
                               "\".",
                           verbosity);
            success = false;
        } else {
            StringArray messages;
            if (!executor->setInput(portName, valueAny, messages)) {
                for (const auto& msg : messages) {
                    dffReportError(kCtxDFFSetGraphInputs,
                                   std::string{msg.c_str()}, verbosity);
                }
                success = false;
            }
        }
    }

    return success;
}

/// \brief Retrieve and validate the output name argument.
///
/// The output name is optional. If present and a non-empty string,
/// \p outputName is set to the string value and true is returned.
/// If invalid, an error is logged, \p outputName is set to \c std::nullopt
/// and false is returned.
///
/// \param [in]  args        The File Format Arguments from the SdfLayer.
/// \param [in]  verbosity   The verbosity to use when logging messages.
/// \param [out] outputName  Set to the non-empty output port name if valid;
///                          set to \c std::nullopt otherwise.
/// \return true if the arg is absent or valid; false on any error.
bool getOutputNameArg(const SdfFileFormat::FileFormatArguments& args,
                      VerbosityLevel                            verbosity,
                      std::optional<std::string>&               outputName) {
    outputName = std::nullopt;

    // The output port names are encoded in the arguments KEYs as
    // "<OutputsAttrPrefix><portName>". The values of the arguments are ignored.
    // Arguments are sorted by attribute names. Attempt to jump to the first
    // output port attribute:
    const std::string kOutputsPrefix{
        tokenToText(BifrostDffGroupingTokens->OutputsAttrPrefix)};
    auto it = args.lower_bound(kOutputsPrefix);

    for (; it != args.end() &&
           it->first.compare(0, kOutputsPrefix.size(), kOutputsPrefix) == 0;
         ++it) {
        // Extract the port name by stripping the prefix:
        const std::string portName = it->first.substr(kOutputsPrefix.size());
        if (portName.empty()) {
            dffReportError(kCtxDFFRead,
                           "The \"" + kOutputsPrefix +
                               "\" argument has an empty port name.",
                           verbosity);
            return false;
        } else if (outputName) {
            dffReportWarning(
                kCtxDFFRead,
                "Output port argument \"" + it->first +
                    "\" is ignored. Only the first output port argument (\"" +
                    kOutputsPrefix + *outputName + "\") is used (limitation).",
                verbosity);
        } else {
            outputName = portName;
        }
    }
    // Absent output port is not an error; graph may output via Terminals.
    return true;
}

/// \brief Retrieve the diagnostics bucket argument, if present and valid.
///
/// The diagnostics bucket argument is optional. If present and a non-empty
/// string, it is returned as a \c std::string. If absent or invalid,
/// \c std::nullopt is returned.
std::optional<std::string> getDiagnosticsBucketArg(
    const SdfFileFormat::FileFormatArguments& args) {
    const auto it = args.find(std::string{kDffDiagnosticsBucketAttr});
    if (it == args.end() || it->second.empty()) {
        return std::nullopt;
    }
    return it->second;
}

void printArgs(const SdfFileFormat::FileFormatArguments* args,
               const std::string&                        msg) {
    std::string output = "****** FileFormatArguments from " + msg + " ******\n";
    for (const auto& pair : *args) {
        output += pair.first + ": " + pair.second + '\n';
    }
    output += "***************************************************************";
    recordDffDiagnosticMsg(DffDiagnosticSeverity::Status, kCtxDFF, output);
}

PXR_NAMESPACE_CLOSE_SCOPE
