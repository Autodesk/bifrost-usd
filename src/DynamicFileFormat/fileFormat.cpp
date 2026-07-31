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
#include "dffDiagnostics.h"

// Amino
#include <Amino/Core/Any.h>

// Bifrost
#include <Amino/Core/Ptr.h>
#include <Amino/Core/StringView.h>
#include <Bifrost/Object/Object.h>

// Bifrost USD
#include <BifrostUsd/GraphExecutor/GraphExecutor.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorConstants.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorFactory.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorTypes.h>
#include <BifrostUsd/Stage.h>
#include <BifrostUsd/UsdTranslator/ObjectToStage.h>
#include <bifusd/config/CfgWarningMacros.h>

// Open USD base headers
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/pcp/dynamicFileFormatContext.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#if ((PXR_MINOR_VERSION == 25) && (PXR_PATCH_VERSION >= 8)) || \
    (PXR_MINOR_VERSION > 25)
#include <pxr/usd/sdf/usdaFileFormat.h>
#else
#include <pxr/usd/sdf/textFileFormat.h>
#endif

// C++ Standard Library
#include <algorithm>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

using namespace BifrostUsd::DynamicPayload;
using namespace BifrostUsd::GraphExecutor;

PXR_NAMESPACE_OPEN_SCOPE

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

namespace {

// Function to process terminal output and transfer content to a layer
Amino::Ptr<BifrostUsd::Stage> createStageFromTerminalOutput(
    GraphExecutorPtr& executor, TerminalType terminalType) {
    auto terminalOutput = executor->extractTerminalOutput(terminalType);
    if (terminalOutput.isValid()) {
        auto flattened = terminalOutput.getFlattened();
        using T        = Amino::Ptr<Amino::Array<Amino::Ptr<Bifrost::Object>>>;
        using ArrayT   = Amino::Ptr<Amino::Array<T>>;

        auto const& outArrayOfObjectsPtr = flattened.get<ArrayT>();

        Amino::MutablePtr<Amino::Array<Amino::Ptr<Bifrost::Object>>> objectsPtr;
        size_t totalCount = 0;

        for (const auto& arrayPtr : *outArrayOfObjectsPtr) {
            totalCount += arrayPtr->size();
        }

        objectsPtr =
            Amino::newMutablePtr<Amino::Array<Amino::Ptr<Bifrost::Object>>>(
                totalCount);
        size_t idx = 0;

        for (const auto& arrayPtr : *outArrayOfObjectsPtr) {
            for (const auto& objPtr : *arrayPtr) {
                (*objectsPtr)[idx++] = objPtr;
            }
        }

        BifrostUsd::ImageablePurpose purpose =
            BifrostUsd::ImageablePurpose::Default;
        switch (terminalType) {
            case TerminalType::eFinal:
                purpose = BifrostUsd::ImageablePurpose::Render;
                break;
            case TerminalType::eProxy:
                purpose = BifrostUsd::ImageablePurpose::Proxy;
                break;
            case TerminalType::eDiagnostic:
                purpose = BifrostUsd::ImageablePurpose::Guide;
                break;
        }

        // *******************************************************************
        // ************************ Execute the graph ************************
        // *******************************************************************
        return objects_to_stage(objectsPtr.toImmutable(),
                                         "terminal_layer.usda", purpose);
    }

    return Amino::Ptr<BifrostUsd::Stage>{};
}

bool executeGraph(GraphExecutorPtr&    executor,
                  const std::string&   compoundName,
                  const VerbosityLevel verbosity,
                  double frame = std::numeric_limits<double>::max()) {
    if (frame < std::numeric_limits<double>::max()) {
        dffReportStatus(kCtxDFFExecuteGraph,
                        "Set frame " + std::to_string(frame) + " in graph \"" +
                            compoundName + "\".",
                        verbosity);
        executor->setFrame(frame);
    }

    StringArray messages;
    if (!executor->execute(messages, verbosity)) {
        dffReportError(kCtxDFFExecuteGraph,
                       "Failed to execute graph \"" + compoundName + "\".",
                       verbosity);
        for (const auto& msg : messages) {
            dffReportError(kCtxDFFExecuteGraph, std::string{msg.c_str()},
                           verbosity);
        }
        return false;
    }
    return true;
}

const Amino::Ptr<BifrostUsd::Stage> createStageFromGraphOutput(
    GraphExecutorPtr&      executor,
    const std::string&     compoundName,
    const std::string&     outputName,
    Amino::ExecutionState& translatorState,
    const VerbosityLevel   verbosity,
    bool                   use_frame        = false,
    float                  frame            = 0.0f,
    bool                   varying_topology = false) {
    auto closure = executor->extractOutputClosure(outputName);

    if (!closure) {
        dffReportError(kCtxDFFCreateStageFromGraphOutput,
                       "The graph \"" + compoundName +
                           "\" does not have an \"" + outputName + "\" output.",
                       verbosity);

        return nullptr;
    }

    auto const& outAny = closure.getAny();

    auto const& outStagePtr =
        Amino::any_cast<Amino::Ptr<BifrostUsd::Stage>>(&outAny);

    auto const& outObjectPtr =
        Amino::any_cast<Amino::Ptr<Bifrost::Object>>(&outAny);

    auto const& outObjectArrayPtr =
        Amino::any_cast<Amino::Ptr<Amino::Array<Amino::Ptr<Bifrost::Object>>>>(
            &outAny);

    if (!outStagePtr && !outObjectPtr && !outObjectArrayPtr) {
        dffReportError(kCtxDFFCreateStageFromGraphOutput,
                       "The graph \"" + compoundName +
                           "\" did not produce a valid \"" + outputName +
                           "\" output. Expecting a Stage, an Object or an array"
                           " of Objects.",
                       verbosity);
        return nullptr;
    }

    if (outStagePtr) {
        // A Stage output must have a default prim; without it the DFF plugin
        // cannot locate the root of the generated scene.
        if (!(*outStagePtr) || !(*outStagePtr)->isValid() ||
            !(*outStagePtr)->get().HasDefaultPrim()) {
            dffReportError(
                kCtxDFFCreateStageFromGraphOutput,
                "The graph \"" + compoundName +
                    "\" produced a Stage with no default prim on output port "
                    "\"" +
                    outputName +
                    "\". Set a default prim on the output Stage (e.g. using "
                    "\"USD::Layer::set_layer_default_prim\"), or use an Object "
                    "or Object[] output port instead (the default prim is then "
                    "set automatically).",
                verbosity);
            return nullptr;
        }
        return *outStagePtr;
    }

    if (outObjectPtr) {
        dffReportStatus(kCtxDFFCreateStageFromGraphOutput,
                        "The graph \"" + compoundName +
                            "\" has produced an Object. Converting to USD...",
                        verbosity);

        auto objectsArrayMutablePtr =
            Amino::newMutablePtr<Amino::Array<Amino::Ptr<Bifrost::Object>>>(1);
        (*objectsArrayMutablePtr)[0] = *outObjectPtr;

        Amino::Ptr<BifrostUsd::Stage> outStageFromObjectPtr;

        if (use_frame) {
            outStageFromObjectPtr = objects_to_stage(
                objectsArrayMutablePtr.toImmutable(), translatorState,
                Amino::String{"objects.usd"},
                BifrostUsd::ImageablePurpose::Default, frame, varying_topology);

        } else {
            outStageFromObjectPtr =
                objects_to_stage(objectsArrayMutablePtr.toImmutable(),
                                 Amino::String{"objects.usd"},
                                 BifrostUsd::ImageablePurpose::Default);
        }

        if (outStageFromObjectPtr) {
            return outStageFromObjectPtr;
        } else {
            dffReportError(
                kCtxDFFCreateStageFromGraphOutput,
                "Failed to create a Stage from an Object with graph \"" +
                    compoundName + "\".",
                verbosity);
            return nullptr;
        }
    }

    if (outObjectArrayPtr) {
        dffReportStatus(kCtxDFFCreateStageFromGraphOutput,
                        "The graph \"" + compoundName +
                            "\" has produced an array of Objects. Converting to"
                            " USD...",
                        verbosity);

        auto const& outStageFromObjectArrayPtr =
            objects_to_stage(*outObjectArrayPtr);
        if (outStageFromObjectArrayPtr) {
            return outStageFromObjectArrayPtr;
        } else {
            dffReportError(kCtxDFFCreateStageFromGraphOutput,
                           "Failed to create a Stage from an array of Objects"
                           " with graph \"" +
                               compoundName + "\".",
                           verbosity);
            return nullptr;
        }
    }

    return nullptr;
}

TfToken getDefaultPrimName(const SdfLayerRefPtr& layer) {
    TfToken name;
    auto    stage = UsdStage::Open(layer);
    if (stage && stage->HasDefaultPrim()) {
        name = stage->GetDefaultPrim().GetName();
    }
    return name;
};

void addSubLayerToLayer(SdfLayerHandle&    subLayer,
                        const std::string& msg,
                        SdfLayerHandle&    outLayer,
                        TfToken&           outDefaultPrimName) {
    subLayer->SetComment(msg);
    outLayer->GetSubLayerPaths().push_back(subLayer->GetIdentifier());

    if (TfToken value = getDefaultPrimName(subLayer); !value.IsEmpty()) {
        outDefaultPrimName = value;
    }
};

} // namespace

BifrostDynamicFileFormat::BifrostDynamicFileFormat()
    : SdfFileFormat(BifrostDffPluginTokens->Id,
                    BifrostDffPluginTokens->Version,
                    BifrostDffPluginTokens->Target,
                    BifrostDffPluginTokens->Extension) {}

BifrostDynamicFileFormat::~BifrostDynamicFileFormat() {}

bool BifrostDynamicFileFormat::CanRead(const std::string& /*filePath*/) const {
    return false;
}

bool BifrostDynamicFileFormat::Read(
    SdfLayer*                           layer,
    [[maybe_unused]] const std::string& resolvedPath,
    [[maybe_unused]] bool               metadataOnly) const {
    if (!layer) {
        return false;
    }

    const FileFormatArguments& args = layer->GetFileFormatArguments();
    DffDiagnosticScope         diagnosticScope(
                DffDiagnosticPhase::Read,
                getDiagnosticsBucketArg(args).value_or(layer->GetIdentifier()));

    // Extract the verbosity level first so that subsequent calls can use it.
    std::optional<VerbosityLevel> verbosityOpt;
    if (!getVerbosityLevelArg(args, verbosityOpt)) {
        return false;
    }
    auto verbosity = verbosityOpt.value();

    std::optional<std::string> compoundNameOpt;
    if (!getCompoundNameArg(args, verbosity, compoundNameOpt)) {
        return false;
    }
    const std::string compoundName = compoundNameOpt.value();

    std::optional<std::string> outputNameOpt;
    if (!getOutputNameArg(args, verbosity, outputNameOpt)) {
        return false;
    }
    std::string outputName =
        outputNameOpt.has_value() ? outputNameOpt.value() : "";

    std::optional<TimelineSettings> timelineSettingsOpt;
    std::optional<double>           fpsOpt;
    if (!getTimelineSettingsArgs(args, verbosity, timelineSettingsOpt)) {
        return false;
    }
    bool useTimeline = timelineSettingsOpt.has_value();
    if (useTimeline && !getFpsArg(args, verbosity, fpsOpt)) {
        return false;
    }

    GraphExecutorPtr executor =
        makeGraphExecutor(Amino::String{compoundName.c_str()});
    if (!executor) {
        dffReportError(kCtxDFFRead,
                       "Failed to create a GraphExecutor for graph \"" +
                           compoundName + "\".",
                       verbosity);
        return false;
    }

    // ******************************************************************

    // Since a graph can have more than one output (the BifrostDynamicFileFormat
    // plugin is currently supporting one regular graph output and many terminal
    // outputs), it can create many stages. We put those stage's root layers
    // under the "main.usda" root layer of an in-memory stage
    // created here. This is used to compose all the graph outputs below one
    // stage that will be flattened. The flattened layer is then transferred to
    // the in/out layer of the BifrostDynamicFileFormat::Read function member.
    //
    // For example, if the graph uses every supported outputs, the "mainStage"
    // layer hierarchy will look like that:
    //
    // |- main.usda
    // `--- terminal_final.usda
    // `--- terminal_proxy.usda
    // `--- terminal_diagnostic.usda
    // `--- graph_output.usda
    //
    auto mainStage = UsdStage::CreateInMemory("main.usda");
    auto mainLayer = mainStage->GetRootLayer();

    // We try to find the default prim name in this order:
    // 1: from stage created by terminal "final" output.
    // 2: from stage created by terminal "proxy" output
    // 3: from stage created by terminal "diagnostic" output
    // 4: from stage created by graph's output
    TfToken defaultPrimName;

    // Use the root layer from a BifrostUsd stage created from a terminal output
    // and add it as a sublayer of the main stage.
    auto addSubLayerFromTerminal =
        [&mainLayer, &defaultPrimName](
            Amino::Ptr<BifrostUsd::Stage>& bifrostStage,
            TerminalType                   terminalType) {
            std::string terminalTypeStr;
            switch (terminalType) {
                case TerminalType::eFinal: terminalTypeStr = "final"; break;
                case TerminalType::eProxy: terminalTypeStr = "proxy"; break;
                case TerminalType::eDiagnostic:
                    terminalTypeStr = "diagnostic";
                    break;
            }

            if (bifrostStage) {
                auto subLayer = (*bifrostStage)->GetRootLayer();
                addSubLayerToLayer(subLayer,
                                   "Layer generated by Bifrost Graph from a " +
                                       terminalTypeStr +
                                       " terminal output port",
                                   mainLayer, defaultPrimName);
            }
        };

    // Use the root layer from a BifrostUsd stage created from a graph output
    // and add it as a sublayer of the main stage.
    auto addSubLayerFromOutput =
        [&mainLayer, &defaultPrimName,
         &outputName](Amino::Ptr<BifrostUsd::Stage>& bifrostStage) {
            if (bifrostStage) {
                auto subLayer = (*bifrostStage)->GetRootLayer();
                addSubLayerToLayer(subLayer,
                                   "Layer generated by Bifrost Graph from the "
                                   "regular output port \"" +
                                       outputName + "\"",
                                   mainLayer, defaultPrimName);
            }
        };

    // *******************************************************************
    // ************************ Execute the graph ************************
    // *******************************************************************

    Amino::Ptr<BifrostUsd::Stage> terminalFinalStage;
    Amino::Ptr<BifrostUsd::Stage> terminalProxyStage;
    Amino::Ptr<BifrostUsd::Stage> terminalDiagnosticStage;
    Amino::Ptr<BifrostUsd::Stage> graphOutputStage;

    bool hasFinal = executor->hasTerminalPort(TerminalType::eFinal);
    bool hasProxy = executor->hasTerminalPort(TerminalType::eProxy);
    bool hasDiagn = executor->hasTerminalPort(TerminalType::eDiagnostic);
    bool hasOutput = !outputName.empty();
    if (!hasFinal && !hasProxy && !hasDiagn && !hasOutput) {
        dffReportError(
            kCtxDFFRead,
            "The graph \"" + compoundName +
                "\" produced no output: specify an output port via the \"" +
                tokenToText(BifrostDffGroupingTokens->OutputsField) +
                "\" field or a \"" +
                tokenToText(BifrostDffGroupingTokens->OutputsAttrPrefix) +
                "<portName>\" attribute, or enable a terminal port (\"final\", "
                "\"proxy\", or \"diagnostic\").",
            verbosity);
        return false;
    }

    if (useTimeline) {
        auto usdTranslatorGraphState = Amino::ExecutionState{};

        executor->setTimelineSettings(timelineSettingsOpt.value());
        executor->setFps(fpsOpt.value());

        double startFrame = timelineSettingsOpt.value().startFrame;
        double endFrame   = timelineSettingsOpt.value().endFrame;

        for (double frame = startFrame; frame < endFrame + 1; frame += 1) {
            if (!setGraphInputs(executor, args, verbosity)) {
                return false;
            }
            if (!executeGraph(executor, compoundName, verbosity, frame)) {
                return false;
            }

            if (hasFinal) {
                terminalFinalStage = createStageFromTerminalOutput(
                    executor, TerminalType::eFinal);
            }
            if (hasProxy) {
                terminalProxyStage = createStageFromTerminalOutput(
                    executor, TerminalType::eProxy);
            }
            if (hasDiagn) {
                terminalDiagnosticStage = createStageFromTerminalOutput(
                    executor, TerminalType::eDiagnostic);
            }
            if (hasOutput) {
                graphOutputStage = createStageFromGraphOutput(
                    executor, compoundName, outputName, usdTranslatorGraphState,
                    verbosity, true, static_cast<float>(frame));
            }
        }
    } else {
        if (!setGraphInputs(executor, args, verbosity)) {
            return false;
        }
        if (!executeGraph(executor, compoundName, verbosity)) {
            return false;
        }

        if (hasFinal) {
            terminalFinalStage =
                createStageFromTerminalOutput(executor, TerminalType::eFinal);
        }
        if (hasProxy) {
            terminalProxyStage =
                createStageFromTerminalOutput(executor, TerminalType::eProxy);
        }
        if (hasDiagn) {
            terminalDiagnosticStage = createStageFromTerminalOutput(
                executor, TerminalType::eDiagnostic);
        }
        if (hasOutput) {
            auto unusedState = Amino::ExecutionState{};
            graphOutputStage = createStageFromGraphOutput(
                executor, compoundName, outputName, unusedState, verbosity);
        }
    }

    if (terminalFinalStage) {
        addSubLayerFromTerminal(terminalFinalStage, TerminalType::eFinal);
    }
    if (terminalProxyStage) {
        addSubLayerFromTerminal(terminalProxyStage, TerminalType::eProxy);
    }
    if (terminalDiagnosticStage) {
        addSubLayerFromTerminal(terminalDiagnosticStage,
                                TerminalType::eDiagnostic);
    }
    if (graphOutputStage) {
        addSubLayerFromOutput(graphOutputStage);
    }

    if (mainLayer->GetSubLayerPaths().empty()) {
        // All configured outputs were attempted but none produced a usable
        // stage. Build a list of the terminal ports that were enabled but
        // yielded nothing.
        // Note: createStageFromGraphOutput() already emits its own specific
        // error for graph output port failures, so we only need to report
        // terminal port failures here.
        std::string failedPorts;
        auto        appendPort = [&failedPorts](const std::string& portDesc) {
            if (!failedPorts.empty()) failedPorts += ", ";
            failedPorts += portDesc;
        };
        if (hasFinal) appendPort("\"final\"");
        if (hasProxy) appendPort("\"proxy\"");
        if (hasDiagn) appendPort("\"diagnostic\"");
        if (!failedPorts.empty()) {
            dffReportError(kCtxDFFRead,
                           "The graph \"" + compoundName +
                               "\" was executed but the following terminal"
                               " output port(s) produced no usable stage: " +
                               failedPorts + ".",
                           verbosity);
        }
        return false;
    }

    // *******************************************************************
    // *******************************************************************
    // *******************************************************************

    auto defaultPrim = mainStage->GetPrimAtPath(
        SdfPath::AbsoluteRootPath().AppendChild(defaultPrimName));
    if (defaultPrim) {
        mainStage->SetDefaultPrim(defaultPrim);
    } else {
        // This should never happen: every sublayer added to mainStage is
        // produced either by objects_to_stage (which always sets "/root" as
        // the default prim) or by a Stage output whose default prim was
        // validated in createStageFromGraphOutput(). If we reach here, an
        // unexpected situation was encountered.
        dffReportError(kCtxDFFRead,
                       "The graph \"" + compoundName +
                           "\" produced sublayers but none carries an"
                           " identifiable default prim. This situation was"
                           " not expected.",
                       verbosity);
        return false;
    }

    auto flattenedTerminalsLayerRefPtr = mainStage->Flatten();

    layer->TransferContent(flattenedTerminalsLayerRefPtr);

    if (layer) {
        // Enforce that the procedural layer is read only.
        layer->SetPermissionToSave(false);
        layer->SetPermissionToEdit(false);
        return true;
    }

    return false;
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
