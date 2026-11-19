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

#include "dffExecuteGraphFrameToLayer.h"

#include "dffArguments.h"
#include "dffConstants.h"
#include "dffData.h"
#include "dffDiagnostics.h"
#include "fileFormat.h"

// Amino
#include <Amino/Core/Any.h>
#include <Amino/Core/Ptr.h>
#include <Amino/Core/StringView.h>

// Bifrost
#include <Bifrost/Object/Object.h>

// Bifrost USD
#include <BifrostUsd/GraphExecutor/GraphExecutor.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorConstants.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorFactory.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorTypes.h>
#include <BifrostUsd/Stage.h>
#include <BifrostUsd/UsdTranslator/ObjectToStage.h>

// Open USD base headers
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

// C++ Standard Library
#include <limits>
#include <string>

using namespace BifrostUsd::DynamicPayload;
using namespace BifrostUsd::GraphExecutor;

PXR_NAMESPACE_OPEN_SCOPE

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
        // ******************* Convert Objects to USD Prims ******************
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

// Set graph inputs and execute at `frame` (frame is ignored for a
// non-animated, single evaluation). Shared by executeGraphFrameToLayer and
// advanceGraphFrame, which both need this identical preamble before either
// extracting outputs or simply discarding them.
bool setInputsAndExecute(GraphExecutorPtr&           executor,
                         const BifrostDffDataParams& params,
                         double                      frame) {
    if (!setGraphInputs(executor, params.rawArgs, params.verbosity)) {
        return false;
    }
    return params.useTimeline()
               ? executeGraph(executor, params.compoundName, params.verbosity,
                              frame)
               : executeGraph(executor, params.compoundName, params.verbosity);
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

// Evaluate the Bifrost graph for a single frame and return the resulting
// flattened layer. Factored out of the previous timeline-baking Read so the
// lazy BifrostDffData can call it once per queried frame. The graph's
// Simulation::time is set from `frame`, but the resulting USD attribute values
// are authored as defaults so a single frame's scene can be read uniformly.
SdfLayerRefPtr executeGraphFrameToLayer(GraphExecutorPtr&           executor,
                                        const BifrostDffDataParams& params,
                                        double                      frame) {
    const auto         verbosity    = params.verbosity;
    const std::string& compoundName = params.compoundName;
    const std::string& outputName   = params.outputName;

    // Evaluation happens lazily, outside Read, so re-establish the diagnostics
    // scope here to keep graph diagnostics grouped by bucket.
    DffDiagnosticScope diagnosticScope(DffDiagnosticPhase::Read,
                                       params.diagnosticsBucket);

    if (!executor) {
        dffReportError(kCtxDFFRead,
                       "Failed to create a GraphExecutor for graph \"" +
                           compoundName + "\".",
                       verbosity);
        return nullptr;
    }

    if (!setInputsAndExecute(executor, params, frame)) {
        return nullptr;
    }

    const bool hasFinal  = executor->hasTerminalPort(TerminalType::eFinal);
    const bool hasProxy  = executor->hasTerminalPort(TerminalType::eProxy);
    const bool hasDiagn  = executor->hasTerminalPort(TerminalType::eDiagnostic);
    const bool hasOutput = !outputName.empty();
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
        return nullptr;
    }

    Amino::Ptr<BifrostUsd::Stage> terminalFinalStage;
    Amino::Ptr<BifrostUsd::Stage> terminalProxyStage;
    Amino::Ptr<BifrostUsd::Stage> terminalDiagnosticStage;
    Amino::Ptr<BifrostUsd::Stage> graphOutputStage;

    if (hasFinal) {
        terminalFinalStage =
            createStageFromTerminalOutput(executor, TerminalType::eFinal);
    }
    if (hasProxy) {
        terminalProxyStage =
            createStageFromTerminalOutput(executor, TerminalType::eProxy);
    }
    if (hasDiagn) {
        terminalDiagnosticStage =
            createStageFromTerminalOutput(executor, TerminalType::eDiagnostic);
    }
    if (hasOutput) {
        // Author values as defaults (use_frame = false): the graph was already
        // evaluated at the requested frame, so we capture that single-frame
        // scene rather than per-frame time samples.
        auto unusedState = Amino::ExecutionState{};
        graphOutputStage = createStageFromGraphOutput(
            executor, compoundName, outputName, unusedState, verbosity);
    }

    // Compose all the produced stages' root layers under a single in-memory
    // stage, mirroring the previous Read assembly.
    auto    mainStage = UsdStage::CreateInMemory("main.usda");
    auto    mainLayer = mainStage->GetRootLayer();
    TfToken defaultPrimName;

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

    addSubLayerFromTerminal(terminalFinalStage, TerminalType::eFinal);
    addSubLayerFromTerminal(terminalProxyStage, TerminalType::eProxy);
    addSubLayerFromTerminal(terminalDiagnosticStage, TerminalType::eDiagnostic);

    if (graphOutputStage) {
        auto subLayer = (*graphOutputStage)->GetRootLayer();
        addSubLayerToLayer(subLayer,
                           "Layer generated by Bifrost Graph from the regular "
                           "output port \"" +
                               outputName + "\"",
                           mainLayer, defaultPrimName);
    }

    if (mainLayer->GetSubLayerPaths().empty()) {
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
        return nullptr;
    }

    auto defaultPrim = mainStage->GetPrimAtPath(
        SdfPath::AbsoluteRootPath().AppendChild(defaultPrimName));
    if (defaultPrim) {
        mainStage->SetDefaultPrim(defaultPrim);
    } else {
        dffReportError(kCtxDFFRead,
                       "The graph \"" + compoundName +
                           "\" produced sublayers but none carries an"
                           " identifiable default prim. This situation was"
                           " not expected.",
                       verbosity);
        return nullptr;
    }

    return mainStage->Flatten();
}

bool advanceGraphFrame(GraphExecutorPtr&           executor,
                       const BifrostDffDataParams& params,
                       double                      frame) {
    // Evaluation happens lazily, outside Read, so re-establish the diagnostics
    // scope here to keep graph diagnostics grouped by bucket.
    DffDiagnosticScope diagnosticScope(DffDiagnosticPhase::Read,
                                       params.diagnosticsBucket);

    if (!executor) {
        dffReportError(kCtxDFFRead,
                       "Failed to create a GraphExecutor for graph \"" +
                           params.compoundName + "\".",
                       params.verbosity);
        return false;
    }

    return setInputsAndExecute(executor, params, frame);
}

PXR_NAMESPACE_CLOSE_SCOPE
