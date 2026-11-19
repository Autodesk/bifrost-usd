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

#include "fileFormat.h"

#include "dffArguments.h"
#include "dffConstants.h"
#include "dffData.h"
#include "dffDiagnostics.h"

// Amino
#include <Amino/Core/StringView.h>

// Bifrost USD
#include <BifrostUsd/GraphExecutor/GraphExecutor.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorConstants.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorFactory.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorTypes.h>
#include <bifusd/config/CfgWarningMacros.h>

// Open USD base headers
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/pcp/dynamicFileFormatContext.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/common.h>

#if ((PXR_MINOR_VERSION == 25) && (PXR_PATCH_VERSION >= 8)) || \
    (PXR_MINOR_VERSION > 25)
#include <pxr/usd/sdf/usdaFileFormat.h>
#else
#include <pxr/usd/sdf/textFileFormat.h>
#endif

// C++ Standard Library
#include <algorithm>
#include <atomic>
#include <optional>
#include <string>
#include <string_view>

using namespace BifrostUsd::DynamicPayload;
using namespace BifrostUsd::GraphExecutor;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Internal-only argument key: not read by any decoder in dffArguments.*, its
// sole purpose is to make the composed arguments differ across a
// reloadLibrary() call (see its use in ComposeFieldsForFileFormatArguments).
constexpr std::string_view kReloadGenerationArg =
    "bifrost:dff:internal:reloadGeneration";

// Monotonic counter incremented every time this file format actually calls
// reloadLibrary() while composing a prim's arguments.
std::atomic<uint64_t> g_reloadGeneration{0};

} // namespace

// clang-format off
TF_DEFINE_PUBLIC_TOKENS(BifrostDffPluginTokens, BIFROST_DFF_PLUGIN_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(BifrostDffGroupingTokens, BIFROST_DFF_GROUPING_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(BifrostDffCompoundFieldTokens, BIFROST_DFF_COMPOUND_FIELD_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(BifrostDffCompoundAttrTokens, BIFROST_DFF_COMPOUND_ATTR_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(BifrostDffGlobalsFieldTokens, BIFROST_DFF_GLOBALS_FIELD_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(BifrostDffGlobalsAttrTokens, BIFROST_DFF_GLOBALS_ATTR_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(BifrostDffSettingsFieldTokens, BIFROST_DFF_SETTINGS_FIELD_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(BifrostDffSettingsAttrTokens, BIFROST_DFF_SETTINGS_ATTR_TOKENS);
/* clang-format on */

// clang-format off
BIFUSD_WARNING_PUSH
// Silence USD warning that we cast from 'void (*)(TfType *)' to 'void (*)()'
BIFUSD_WARNING_DISABLE_CLANG_160(-Wcast-function-type-strict)
TF_REGISTRY_FUNCTION(TfType)
{
    BIFUSD_WARNING_POP
    SDF_DEFINE_FILE_FORMAT(BifrostDynamicFileFormat, SdfFileFormat);
}
// clang-format on

BifrostDynamicFileFormat::BifrostDynamicFileFormat()
    : SdfFileFormat(BifrostDffPluginTokens->Id,
                    BifrostDffPluginTokens->Version,
                    BifrostDffPluginTokens->Target,
                    BifrostDffPluginTokens->Extension) {}

BifrostDynamicFileFormat::~BifrostDynamicFileFormat() {}

bool BifrostDynamicFileFormat::CanRead(const std::string& /*filePath*/) const {
    return false;
}

SdfAbstractDataRefPtr BifrostDynamicFileFormat::InitData(
    const FileFormatArguments& args) const {
    // The layer's data is a lazy implementation that evaluates the Bifrost
    // graph at the time sample(s) USD queries. All the configuration needed to
    // (re)execute the graph is decoded from the composed file format arguments
    // here and carried by the data for the lifetime of the layer.
    return BifrostDffData::New(BifrostDffDataParams::FromArgs(args));
}

bool BifrostDynamicFileFormat::Read(
    SdfLayer*                           layer,
    [[maybe_unused]] const std::string& resolvedPath,
    [[maybe_unused]] bool               metadataOnly) const {
    if (!layer) {
        return false;
    }

    // The payload contents are produced lazily by the BifrostDffData created
    // in InitData, which evaluates the Bifrost graph at the time sample(s) USD
    // queries. There is no asset to read here, so report the same failure
    // InitData recorded when the file format arguments failed to
    // decode/validate, mirroring the previous behavior where Read itself
    // aborted (returned false) on invalid arguments -- otherwise USD would
    // silently treat the layer as successfully read but empty, hiding the
    // failure from callers that do not inspect the diagnostics collector.
    auto data = TfDynamic_cast<BifrostDffDataConstPtr>(_GetLayerData(*layer));
    if (!data || !data->IsValid()) {
        return false;
    }

    // Otherwise we only enforce that the generated layer is read only.
    layer->SetPermissionToSave(false);
    layer->SetPermissionToEdit(false);
    return true;
}

bool BifrostDynamicFileFormat::WriteToString(const SdfLayer&    layer,
                                             std::string*       str,
                                             const std::string& comment) const {
    // Write the generated contents in usda text format.
#if ((PXR_MINOR_VERSION == 25) && (PXR_PATCH_VERSION >= 8)) || \
    (PXR_MINOR_VERSION > 25)
    auto fileFormatPtr = SdfFileFormat::FindById(SdfUsdaFileFormatTokens->Id);
#else
    auto fileFormatPtr = SdfFileFormat::FindById(SdfTextFileFormatTokens->Id);
#endif
    return fileFormatPtr->WriteToString(layer, str, comment);
}

bool BifrostDynamicFileFormat::WriteToStream(const SdfSpecHandle& spec,
                                             std::ostream&        out,
                                             size_t indent) const {
    // Write the generated contents in usda text format.
#if ((PXR_MINOR_VERSION == 25) && (PXR_PATCH_VERSION >= 8)) || \
    (PXR_MINOR_VERSION > 25)
    auto fileFormatPtr = SdfFileFormat::FindById(SdfUsdaFileFormatTokens->Id);
#else
    auto fileFormatPtr = SdfFileFormat::FindById(SdfTextFileFormatTokens->Id);
#endif
    return fileFormatPtr->WriteToStream(spec, out, indent);
}

void BifrostDynamicFileFormat::ComposeFieldsForFileFormatArguments(
    const std::string&                 assetPath,
    const PcpDynamicFileFormatContext& context,
    FileFormatArguments*               args,
    [[maybe_unused]] VtValue*          contextDependencyData) const {
    FileFormatArguments outArgs;
    bool                hasError = false;

    // The BucketId is used to group all diagnostics related to the same asset
    // together in the DFF Diagnostics. By default, when no BucketId field
    // was authored, we use the assetPath.
    DffDiagnosticScope diagnosticScope(
        DffDiagnosticPhase::ComposeFieldsForFileFormatArguments,
        assetPath /*bucketId*/);
    if (const auto diagnosticsBucket = composeDiagnosticsBucketField(
            context, BifrostUsd::GraphExecutor::defaultVerbosity, &outArgs)) {
        diagnosticScope.setBucketId(*diagnosticsBucket);
    }

    // Compose "settings" field before processing the remaining groups since
    // these settings values would affect the behavior of the DFF plugin itself
    // (e.g. verbosity level).
    // Note: We use a minimum of eErrorsAndWarnings level so that errors in
    //       these "settings" are always surfaced regardless of what the default
    //       verbosity says.
    VerbosityLevel tempVerbosity =
        std::max(VerbosityLevel::eErrorsAndWarnings,
                 BifrostUsd::GraphExecutor::defaultVerbosity);
    if (!composeGroupOfFieldsToArgs(
            context, tempVerbosity, BifrostDffGroupingTokens->SettingsField,
            &BifrostDffSettingsFieldTokens->allTokens,
            nullptr, // no typeIds to validate against
            BifrostDffGroupingTokens->SettingsAttrPrefix,
            hasError ? nullptr : &outArgs)) {
        hasError = true;
    }

    // Probe "settings" attributes that may be present on the primitive, and
    // when one is present, it overrides the corresponding field value (which
    // may be present or not).
    // Note: These calls are made unconditionally (even when hasError is already
    //       set) so that PCP always records a dependency on each attribute via
    //       context.ComposeAttributeDefaultValue(). Otherwise, we would leave
    //       PCP unaware of the attribute and prevent recomposition when its
    //       value subsequently changes.
    for (const TfToken& attrToken : BifrostDffSettingsAttrTokens->allTokens) {
        bool success = applyAttrOverrideToFileFormatArgument(
            context, tempVerbosity, tokenToText(attrToken), &outArgs);
        if (!success) hasError = true;
    }

    // Extract the actual desired verbosity level that has been set in content
    // so that all subsequent composition calls can use it. Both the field and
    // the attribute override have been written to outArgs, so the extracted
    // value is the final composed verbosity.
    VerbosityLevel verbosity = BifrostUsd::GraphExecutor::defaultVerbosity;
    std::optional<VerbosityLevel> verbosityOpt;
    if (getVerbosityLevelArg(outArgs, verbosityOpt)) {
        verbosity = verbosityOpt.value();
    } else {
        hasError = true;
    }

    // If reloadLibrary=true (from the field, the attribute override, or both),
    // reload the Bifrost library before creating the GraphExecutor. This
    // ensures that compounds added, removed, or whose graph inputs/outputs
    // changed since the last load are picked up here, so the correct port
    // names are used when composing the remaining DFF plugin arguments
    // (inputs, outputs, etc.).
    std::optional<bool> reloadLibraryOpt;
    if (!getReloadLibraryArg(outArgs, verbosity, reloadLibraryOpt)) {
        hasError = true;
    } else if (reloadLibraryOpt.value_or(false)) {
        reloadLibrary();

        // Force this composition's arguments to differ from whatever was
        // previously composed for this prim. Without this, if reload is
        // triggered by an edit that otherwise leaves the composed arguments
        // unchanged, PCP's dynamic-file-format caching would reuse the
        // existing BifrostDffData -- whose persistent GraphExecutor
        // (dffData.cpp) is only rebuilt on a backward scrub -- so an
        // already-open animated payload mid-playback would keep stepping the
        // pre-reload library forward indefinitely instead of picking up the
        // reload.
        outArgs[std::string{kReloadGenerationArg}] =
            std::to_string(++g_reloadGeneration);
    }

    // Compose fields from all remaining *known* grouping tokens.
    if (!composeGroupOfFieldsToArgs(
            context, verbosity, BifrostDffGroupingTokens->CompoundField,
            &BifrostDffCompoundFieldTokens->allTokens,
            nullptr, // no typeIds to validate against
            BifrostDffGroupingTokens->CompoundAttrPrefix,
            hasError ? nullptr : &outArgs)) {
        hasError = true;
    }
    if (!composeGroupOfFieldsToArgs(context, verbosity,
                                    BifrostDffGroupingTokens->GlobalsField,
                                    &BifrostDffGlobalsFieldTokens->allTokens,
                                    nullptr, // no typeIds to validate against
                                    BifrostDffGroupingTokens->GlobalsAttrPrefix,
                                    hasError ? nullptr : &outArgs)) {
        hasError = true;
    }

    // Probe the "compound" attributes that may be present on the primitive. If
    // the compoundName attribute is present, it overrides the corresponding
    // field value (which may not be present).
    for (const TfToken& attrToken : BifrostDffCompoundAttrTokens->allTokens) {
        if (!applyAttrOverrideToFileFormatArgument(
                context, verbosity, tokenToText(attrToken), &outArgs)) {
            hasError = true;
        }
    }

    // At this point, if the compoundName is known, we can use it to create
    // a temporary GraphExecutor to query the graph input and output port names,
    // which cannot be known in advance like other fields and attributes.
    GraphExecutorPtr           executor;
    std::optional<std::string> compoundNameOpt;
    if (!getCompoundNameArg(outArgs, verbosity, compoundNameOpt)) {
        hasError = true;
    } else {
        const std::string compoundName = compoundNameOpt.value();
        executor = makeGraphExecutor(Amino::String{compoundName.c_str()});
        if (!executor) {
            dffReportError(kCtxDFFComposingValues,
                           "Failed to create a GraphExecutor for graph \"" +
                               compoundName + "\".",
                           verbosity);
            hasError = true;
        }
    }

    StringArray validInputPortNames;
    StringArray validOutputPortNames;
    if (executor) {
        // Compose the "inputs" field, and validate them against the valid
        // input port names of the compound to execute.
        validInputPortNames = executor->getInputPortNames();
        if (!composeInputsFieldToArgs(
                context, verbosity, BifrostDffGroupingTokens->InputsField,
                validInputPortNames, BifrostDffGroupingTokens->InputsAttrPrefix,
                hasError ? nullptr : &outArgs))
            hasError = true;

        // The graph output ports are listed as a token[] array.
        // Like inputs, we validate the field entries and probe attributes
        // against the graph's known output port names, enabling attribute-only
        // output port specification with no "outputs" field present.
        validOutputPortNames = executor->getOutputPortNames();
        if (!composeOutputsFieldToArgs(
                context, verbosity, BifrostDffGroupingTokens->OutputsField,
                validOutputPortNames,
                BifrostDffGroupingTokens->OutputsAttrPrefix,
                hasError ? nullptr : &outArgs))
            hasError = true;
    }

    if (!hasError) {
        // Probe "globals" attributes that may be present on the
        // primitive, and when one is present, it overrides the corresponding
        // field value (which may be present or not).
        for (const TfToken& attrToken :
             BifrostDffGlobalsAttrTokens->allTokens) {
            bool success = applyAttrOverrideToFileFormatArgument(
                context, verbosity, tokenToText(attrToken), &outArgs);
            if (!success) hasError = true;
        }

        // Do the same for graph "inputs" attributes whose names match names of
        // graph "inputs" field, so they can be used as graph input overrides.
        if (executor) {
            const std::string attrPrefix =
                tokenToText(BifrostDffGroupingTokens->InputsAttrPrefix);
            for (const auto& portName : validInputPortNames) {
                const std::string attrName = attrPrefix + portName.c_str();
                bool success = applyAttrOverrideToFileFormatArgument(
                    context, verbosity, attrName, &outArgs);
                if (!success) hasError = true;
            }
        }

        // If any "outputs" attributes are present on the prim, they replace the
        // entire output port list set by the "outputs" field (all-or-nothing
        // semantics). This also enables attribute-only output specification
        // when no "outputs" field is authored.
        if (executor) {
            applyOutputsAttrOverridesToArgs(
                context, verbosity, validOutputPortNames,
                BifrostDffGroupingTokens->OutputsAttrPrefix, &outArgs);
        }
    }

    if (!hasError) {
        *args = std::move(outArgs);
    }
}

bool BifrostDynamicFileFormat::CanFieldChangeAffectFileFormatArguments(
    [[maybe_unused]] const TfToken& field,
    [[maybe_unused]] const VtValue& oldValue,
    [[maybe_unused]] const VtValue& newValue,
    [[maybe_unused]] const VtValue& contextDependencyData) const {
    return true;
}

bool BifrostDynamicFileFormat::_ShouldSkipAnonymousReload() const {
    // Anonymous layers need to be reloadable for mute/unmute of the
    // layer to work.
    return false;
}

bool BifrostDynamicFileFormat::_ShouldReadAnonymousLayers() const {
    // Anonymous layers of this format are allowed to call Read because
    // Read doesn't read from an actual asset path. This allows payloads
    // to target anonymous layers of this format.
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
