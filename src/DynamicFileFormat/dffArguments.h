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

#ifndef BIFROSTUSD_DYNAMIC_FILE_FORMAT_ARGUMENTS_H
#define BIFROSTUSD_DYNAMIC_FILE_FORMAT_ARGUMENTS_H

#include <BifrostUsd/GraphExecutor/GraphExecutor.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorFactory.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorTypes.h>

#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/pcp/dynamicFileFormatContext.h>
#include <pxr/usd/sdf/fileFormat.h>

#include <functional>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \brief Helper function to convert a Token to a string for error messages,
/// without risking undefined behavior.
/// Use `const char* GetText()` instead of `const std::string& GetString()`.
/// If USD and this plugin are compiled with different std::string ABIs,
/// any layout-dependent operation on that string reference owned by USD is
/// undefined behavior.
inline const char* tokenToText(const TfToken& token) { return token.GetText(); }

/// \brief Compose a group of fields from a \c VtDictionary composed value
/// and accumulate the encoded entries into \p args.
bool composeGroupOfFieldsToArgs(
    const PcpDynamicFileFormatContext&         context,
    BifrostUsd::GraphExecutor::VerbosityLevel  verbosity,
    const TfToken&                             groupFieldToken,
    const std::vector<TfToken>*                validTokens,
    const std::unordered_set<std::type_index>* validTypeIds,
    const TfToken&                             attrPrefixToken,
    SdfFileFormat::FileFormatArguments*        args);

/// \brief Compose the Bifrost inputs fields from a \c VtDictionary composed
/// value, validate each entry against a list of valid input port names, and
/// accumulate the encoded entries into \p args.
bool composeInputsFieldToArgs(
    const PcpDynamicFileFormatContext&        context,
    BifrostUsd::GraphExecutor::VerbosityLevel verbosity,
    const TfToken&                            inputsFieldToken,
    const StringArray&                        validInputPortNames,
    const TfToken&                            attrPrefixToken,
    SdfFileFormat::FileFormatArguments*       args);

/// \brief Compose the Bifrost outputs field from a token[] and accumulate one
/// entry per valid port name into \p args.
///
/// Validates each listed port name against \p validOutputPortNames. Unknown
/// port names are reported as errors.
/// If the field is absent, returns true (not an error).
bool composeOutputsFieldToArgs(
    const PcpDynamicFileFormatContext&        context,
    BifrostUsd::GraphExecutor::VerbosityLevel verbosity,
    const TfToken&                            outputsFieldToken,
    const StringArray&                        validOutputPortNames,
    const TfToken&                            attrPrefixToken,
    SdfFileFormat::FileFormatArguments*       args);

/// \brief Compose the optional diagnostics bucket id into \p args.
std::optional<std::string> composeDiagnosticsBucketField(
    const PcpDynamicFileFormatContext&        context,
    BifrostUsd::GraphExecutor::VerbosityLevel verbosity,
    SdfFileFormat::FileFormatArguments*       args);

/// \brief Apply the Bifrost outputs attribute override to the output port name
/// entries in \p args.
///
/// Probes \c bifrost:out:<portName> attributes for every port in
/// \p validOutputPortNames. If **any** such attribute is present, all
/// field-derived output entries are removed from \p args and replaced by the
/// attribute-confirmed port names only (all-or-nothing semantics). This also
/// enables attribute-only output specification with no field present.
void applyOutputsAttrOverridesToArgs(
    const PcpDynamicFileFormatContext&        context,
    BifrostUsd::GraphExecutor::VerbosityLevel verbosity,
    const StringArray&                        validOutputPortNames,
    const TfToken&                            attrPrefixToken,
    SdfFileFormat::FileFormatArguments*       args);

/// \brief Apply an attribute override to a FileFormatArgument.
bool applyAttrOverrideToFileFormatArgument(
    const PcpDynamicFileFormatContext&        context,
    BifrostUsd::GraphExecutor::VerbosityLevel verbosity,
    const std::string&                        attrName,
    SdfFileFormat::FileFormatArguments*       args);

/// \brief Retrieve and validate the compound name argument.
bool getCompoundNameArg(const SdfFileFormat::FileFormatArguments& args,
                        BifrostUsd::GraphExecutor::VerbosityLevel verbosity,
                        std::optional<std::string>&               compoundName);

/// \brief Retrieve and validate the timeline start and end frame arguments.
bool getTimelineSettingsArgs(
    const SdfFileFormat::FileFormatArguments&                   args,
    BifrostUsd::GraphExecutor::VerbosityLevel                   verbosity,
    std::optional<BifrostUsd::GraphExecutor::TimelineSettings>& settings);

/// \brief Retrieve and validate the fps argument.
bool getFpsArg(const SdfFileFormat::FileFormatArguments& args,
               BifrostUsd::GraphExecutor::VerbosityLevel verbosity,
               std::optional<double>&                    fps);

/// \brief Retrieve and validate the verbosity level argument.
bool getVerbosityLevelArg(
    const SdfFileFormat::FileFormatArguments&                 args,
    std::optional<BifrostUsd::GraphExecutor::VerbosityLevel>& verbosityLevel);

/// \brief Retrieve and validate the reload library argument.
bool getReloadLibraryArg(const SdfFileFormat::FileFormatArguments& args,
                         BifrostUsd::GraphExecutor::VerbosityLevel verbosity,
                         std::optional<bool>& reloadLibrary);

/// \brief Set the Bifrost graph inputs from given USD FileFormatArguments.
bool setGraphInputs(BifrostUsd::GraphExecutor::GraphExecutorPtr& executor,
                    const SdfFileFormat::FileFormatArguments&    args,
                    BifrostUsd::GraphExecutor::VerbosityLevel    verbosity);

/// \brief Retrieve and validate the output name argument.
bool getOutputNameArg(const SdfFileFormat::FileFormatArguments& args,
                      BifrostUsd::GraphExecutor::VerbosityLevel verbosity,
                      std::optional<std::string>&               outputName);

/// \brief Retrieve the optional diagnostics bucket id argument.
std::optional<std::string> getDiagnosticsBucketArg(
    const SdfFileFormat::FileFormatArguments& args);

/// \brief Returns a static map from VtValue type index to encoder function,
/// built once at startup by iterating SupportedTypes.
///
/// Each encoder converts a VtValue to an encoded <typeName>+<value> arg string.
const std::unordered_map<std::type_index,
                         std::function<std::string(const VtValue&)>>&
getTypeAndValueEncoderMap();

/// \brief Returns a static map from VtValue type index to Amino type name,
/// built once at startup by iterating SupportedTypes.
const std::unordered_map<std::type_index, std::string_view>&
getTypeIndexToAminoTypeNameMap();

/// \brief Helper function to print the FileFormatArguments for debugging.
void printArgs(const SdfFileFormat::FileFormatArguments* args,
               const std::string&                        msg = "");

PXR_NAMESPACE_CLOSE_SCOPE

#endif // BIFROSTUSD_DYNAMIC_FILE_FORMAT_ARGUMENTS_H
