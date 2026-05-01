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
#include "DynamicFileFormatConstants.h"

// Amino
#include <Amino/Core/Any.h>
#include <Amino/Core/BuiltInTypes.h>

// Bifrost
#include <Amino/Core/Ptr.h>
#include <Amino/Core/StringView.h>
#include <Bifrost/Geometry/GeoProperty.h>
#include <Bifrost/Geometry/GeometryTypes.h>
#include <Bifrost/Geometry/Primitives.h>
#include <Bifrost/Math/Types.h>
#include <Bifrost/Object/Object.h>

// Bifrost USD
#include <BifrostUsd/GraphExecutor/GraphExecutor.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorFactory.h>
#include <BifrostUsd/GraphExecutor/Types.h>
#include <BifrostUsd/Stage.h>
#include <BifrostUsd/UsdTranslator/ObjectToStage.h>
#include <bifusd/config/CfgWarningMacros.h>

// Open USD base headers
#include <pxr/base/gf/vec3i.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
// Open USD sdf headers
#include <pxr/usd/sdf/declareHandles.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/sdf/reference.h>
// Open USD usd headers
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/stage.h>

#if ((PXR_MINOR_VERSION == 25) && (PXR_PATCH_VERSION >= 8)) || \
    (PXR_MINOR_VERSION > 25)
#include <pxr/usd/sdf/usdaFileFormat.h>
#else
#include <pxr/usd/sdf/textFileFormat.h>
#endif

#include <pxr/usd/pcp/dynamicFileFormatContext.h>

// C++ Standard Library
#include <algorithm>
#include <array>
#include <cstdio>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>


using namespace BifrostUsd::DynamicPayload;
using namespace BifrostUsd::GraphExecutor;

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PUBLIC_TOKENS(
    BifrostDynamicFileFormatTokens,
    BIFROST_DYNAMIC_FILE_FORMAT_TOKENS);

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

// Constants for BifrostDynamicFileFormat arguments as std::string constructed
// from TfToken::GetText() instead of TfToken::GetString() to avoid ABI
// interface issue.
const std::string kCompoundName{
    BifrostDynamicFileFormatTokens->CompoundName.GetText()};
const std::string kOptions{BifrostDynamicFileFormatTokens->Options.GetText()};
const std::string kGlobals{BifrostDynamicFileFormatTokens->Globals.GetText()};
const std::string kReloadLibrary{
    BifrostDynamicFileFormatTokens->ReloadLibrary.GetText()};
const std::string kOutputName{
    BifrostDynamicFileFormatTokens->OutputName.GetText()};

// Other constants
const std::string kStart_frame{"timeline_info_start_frame"};
const std::string kEnd_frame{"timeline_info_end_frame"};
const std::string kFps{"time_fps"};

const std::string kSpace{" "};
const std::string kDot{"."};
const std::string kColon{":"};
const std::string kVerbosity_level{"verbosity_level"};

constexpr std::string_view kUnknownType_sv{"UnknownType UnknownValue"};

// clang-format off
using SupportedTypes =
    std::tuple<
        Amino::int_t,
        Amino::uint_t,
        Amino::long_t,
        Amino::bool_t,
        Amino::float_t,
        Amino::double_t,
        GfVec3i,
        GfVec3f,
        TfToken,
        SdfAssetPath,
        std::string
    >;
// clang-format on

std::map<std::string, VerbosityLevel> VerbosityLevelMap{
    {"Silent", VerbosityLevel::eSilent},
    {"ErrorsOnly", VerbosityLevel::eErrorsOnly},
    {"AllMessages", VerbosityLevel::eAllMessages}};

using ParamNameToTypeMap = std::map<std::string, std::string>;
using NameAndValueStr    = std::pair<std::string, std::string>;
using BifrostFloat3      = Bifrost::Math::float3;
using BifrostInt3        = Bifrost::Math::int3;

// Function to apply an operation to each type in a tuple
template <typename Tuple, typename Func, std::size_t... I>
constexpr void for_each_type_impl(Func&& f, std::index_sequence<I...>) {
    // Fold expression to call f for each type
    (f(std::tuple_element_t<I, Tuple>{}), ...);
}

template <typename Tuple, typename Func>
constexpr void for_each_type(Func&& f) {
    constexpr std::size_t N = std::tuple_size<Tuple>::value;
    for_each_type_impl<Tuple>(std::forward<Func>(f),
                              std::make_index_sequence<N>{});
}

// If the value matches T then add its name and typeName to the input/output map
// and set the output string with <typeName>{space}<value>.
template <typename T>
void getArgImpl(const VtValue&      value,
                const std::string&  name,
                ParamNameToTypeMap& nameToTypesMap,
                std::string&        arg) {
    if (arg != kUnknownType_sv) {
        return;
    }

    if (value.IsHolding<T>()) {
        std::string typeName = value.GetTypeName();
        nameToTypesMap[name] = typeName;
        auto strVal          = TfStringify(value.UncheckedGet<T>());
        if (typeName == "unsigned int") {
            typeName = "uint";
        } else if (typeName == "long long") {
            typeName = "long";
        }

        arg = typeName + kSpace + strVal;
    }
}

std::string getArg(const VtValue&      value,
                   const std::string&  name,
                   ParamNameToTypeMap& nameToTypeMap) {
    std::string arg{kUnknownType_sv};

    // Iterate over types at compile time
    for_each_type<SupportedTypes>(
        [&value, &name, &nameToTypeMap, &arg](auto valueType) {
            using T = std::decay_t<decltype(valueType)>;
            getArgImpl<T>(value, name, nameToTypeMap, arg);
        }

    );

    return arg;
}

TfToken resolveArgName(const std::string& name) {
    auto attrName = name;

    if (attrName == kOptions + kDot + kVerbosity_level) {
        attrName = kOptions + kColon + kVerbosity_level;
    } else if (attrName == kGlobals + kDot + kStart_frame) {
        attrName = kGlobals + kColon + kStart_frame;
    } else if (attrName == kGlobals + kDot + kEnd_frame) {
        attrName = kGlobals + kColon + kEnd_frame;
    } else if (attrName == kGlobals + kDot + kFps) {
        attrName = kGlobals + kColon + kFps;
    }

    return TfToken(attrName.c_str());
}

template <typename T>
bool getArgOverrideImpl(const PcpDynamicFileFormatContext&  context,
                        const std::string&                  name,
                        SdfFileFormat::FileFormatArguments* args) {
    auto attrName = resolveArgName(name);

    VtValue value;
    if (context.ComposeAttributeDefaultValue(attrName, &value) &&
        value.IsHolding<T>()) {
        std::string typeName = value.GetTypeName();

        if constexpr (std::is_same_v<T, SdfAssetPath>) {
            const auto& assetPath = value.UncheckedGet<SdfAssetPath>();
            (*args)[name] = typeName + kSpace + assetPath.GetAssetPath();
        } else {
            if (typeName == "unsigned int") {
                typeName = "uint";
            } else if (typeName == "long long") {
                typeName = "long";
            }

            auto strVal   = TfStringify(value.UncheckedGet<T>());
            (*args)[name] = typeName + kSpace + strVal;
        }
        return true;
    }
    return false;
}

void getArgOverride(const PcpDynamicFileFormatContext&  context,
                    const std::string&                  name,
                    SdfFileFormat::FileFormatArguments* args) {
    // Iterate over types at compile time
    for_each_type<SupportedTypes>([&context, &name, &args](auto valueType) {
        using T = std::decay_t<decltype(valueType)>;
        getArgOverrideImpl<T>(context, name, args);
    }

    );
}

[[maybe_unused]] void printArgs(const SdfFileFormat::FileFormatArguments* args,
                                const std::string& msg = "") {
    std::cout << "****** FileFormatArguments from " << msg << " ******" << '\n';
    for (const auto& pair : *args) {
        std::cout << pair.first << ": " << pair.second << '\n';
    }
    std::cout
        << "***************************************************************"
        << std::endl;
}

NameAndValueStr getTypeAndValue(const std::string& input) {
    std::string part1, part2;

    size_t pos = input.find(kSpace);
    if (pos != std::string::npos) {
        part1 = input.substr(0, pos);
        part2 = input.substr(pos + 1);
    }

    return {part1, part2};
}

using StringVec3 = std::optional<std::array<std::string, 3>>;
StringVec3 getStringVec3(const std::string& str) {
    std::array<std::string, 3> vec;

    size_t leftParenthesis  = str.find('(');
    size_t rightParenthesis = str.find(')');
    if (leftParenthesis != std::string::npos &&
        rightParenthesis != std::string::npos) {
        std::string strValues = str.substr(
            leftParenthesis + 1, rightParenthesis - leftParenthesis - 1);
        size_t firstComma  = strValues.find(',');
        size_t secondComma = strValues.find(',', firstComma + 1);

        if (firstComma != std::string::npos &&
            secondComma != std::string::npos) {
            vec[0] = strValues.substr(0, firstComma);
            vec[1] =
                strValues.substr(firstComma + 1, secondComma - firstComma - 1);
            vec[2] = strValues.substr(secondComma + 1);

            return vec;
        }
    }

    return std::nullopt;
}

BifrostFloat3 getFloat3FromString(const std::string& str) {
    BifrostFloat3 vec;

    auto strVec = getStringVec3(str);

    if (strVec.has_value()) {
        vec.x = std::stof((*strVec)[0]);
        vec.y = std::stof((*strVec)[1]);
        vec.z = std::stof((*strVec)[2]);
    }

    return vec;
}

BifrostInt3 getInt3FromString(const std::string& str) {
    BifrostInt3 vec;

    auto strVec = getStringVec3(str);

    if (strVec.has_value()) {
        vec.x = std::stoi((*strVec)[0]);
        vec.y = std::stoi((*strVec)[1]);
        vec.z = std::stoi((*strVec)[2]);
    }

    return vec;
}

GraphArgs getGraphArgs(const SdfFileFormat::FileFormatArguments& args) {
    auto startsWithPrefix = [](const std::string& str,
                               const std::string& prefix) -> bool {
        if (prefix.size() > str.size()) {
            return false;
        }
        return std::equal(prefix.begin(), prefix.end(), str.begin());
    };

    SdfFileFormat::FileFormatArguments filteredArgs;
    for (const auto& [name, value] : args) {
        if (name == SdfFileFormatTokens->TargetArg.GetText()) {
            // When SdfFileFormat creates the new layer, it adds a "usd" target
            // to the args. We don't need it.
            continue;
        }

        if (name == kCompoundName) {
            continue;
        }
        if (startsWithPrefix(name, kOptions + kDot)) {
            continue;
        }

        if (startsWithPrefix(name, kGlobals + kDot)) {
            continue;
        }

        if (name == kReloadLibrary) {
            continue;
        }

        if (name == kOutputName) {
            continue;
        }

        if (value != kUnknownType_sv) {
            filteredArgs[name] = value;
        } else {
            std::cerr << kCtxDFFGetGraphArgs
                      << "Warning: "
                         "Skipping unsupported type for argument '"
                      << name << "'." << std::endl;
        }
    }

    GraphArgs graphArgs;
    for (const auto& pair : filteredArgs) {
        Amino::String name = pair.first.c_str();

        auto [typeName, valueStr] = getTypeAndValue(pair.second);

        if (typeName == "int") {
            graphArgs[name] = std::stoi(valueStr);
        } else if (typeName == "uint") {
            graphArgs[name] = Amino::uint_t(std::stoi(valueStr));
        } else if (typeName == "bool") {
            graphArgs[name] = (valueStr == "1" || valueStr == "true");
        } else if (typeName == "long") {
            graphArgs[name] = Amino::long_t(std::stoi(valueStr));
        } else if (typeName == "float") {
            graphArgs[name] = std::stof(valueStr);
        } else if (typeName == "double") {
            graphArgs[name] = std::stod(valueStr);
        } else if (typeName == "GfVec3i") {
            graphArgs[name] = getInt3FromString(valueStr);
        } else if (typeName == "GfVec3f") {
            graphArgs[name] = getFloat3FromString(valueStr);
        } else if (typeName == "TfToken" || typeName == "string" ||
                   typeName == "SdfAssetPath") {
            graphArgs[name] = Amino::String{valueStr.c_str()};
        } else {
            std::cerr
                << kCtxDFFGetGraphArgs << "Error: Unsupported type '"
                << typeName << "' for parameter '" << name.c_str() << "'."
                << std::endl;
        }
    }

    return graphArgs;
}

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
        return array_of_objects_to_stage(objectsPtr.toImmutable(),
                                         "terminal_layer.usda", purpose);
    }

    return Amino::Ptr<BifrostUsd::Stage>{};
}

void executeGraph(GraphExecutorPtr&    executor,
                  const std::string&   compoundName,
                  const GraphArgs&     graphArgs,
                  const VerbosityLevel verbLevel,
                  double frame = std::numeric_limits<double>::max()) {
    if (!executor->setGraphInputs(graphArgs)) {
        std::cerr << kCtxDFFExecuteGraph
                  << "Error: Failed to set "
                     "inputs for graph '"
                  << compoundName << "'." << std::endl;
        throw std::runtime_error("Graph inputs error");
    }

    if (frame < std::numeric_limits<double>::max()) {
        std::cout << kCtxDFFExecuteGraph << "Set frame " << frame
                  << " in graph '" << compoundName << "'." << std::endl;
        executor->setFrame(frame);
    }

    if (!executor->execute(verbLevel)) {
        std::cerr << kCtxDFFExecuteGraph
                  << "Error: Failed to "
                     "execute graph '"
                  << compoundName << "'." << std::endl;
        throw std::runtime_error("Graph execution error");
    }
}

const Amino::Ptr<BifrostUsd::Stage> createStageFromGraphOutput(
    GraphExecutorPtr&         executor,
    const std::string&        compoundName,
    const std::string&        outputName,
    Amino::ExecutionState&    translatorState,
    const VerbosityLevel      verbosityLevel   = VerbosityLevel::eErrorsOnly,
    bool                      use_frame        = false,
    float                     frame            = 0.0f,
    bool                   varying_topology = false) {
    auto closure = executor->extractOutputClosure(outputName);

    if (!closure) {
        std::cerr << kCtxDFFCreateStageFromGraphOutput
                  << "Error: The graph '"
                  << compoundName << "' does not have an '" << outputName
                  << "' output." << std::endl;

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
        std::cerr << kCtxDFFCreateStageFromGraphOutput
                  << "Error: The graph '"
                  << compoundName << "' did not produce a valid '" << outputName
                  << "' output. Expecting a Stage, an Object or an array "
                     "of Objects."
                  << std::endl;
        return nullptr;
    }

    if (outStagePtr) {
        return *outStagePtr;
    }

    if (outObjectPtr) {
        if (verbosityLevel == VerbosityLevel::eAllMessages) {
            std::cout << kCtxDFFCreateStageFromGraphOutput << "The graph '"
                    << compoundName
                    << "' has produced an Object. Converting to USD..."
                    << std::endl;
        }

        Amino::Ptr<BifrostUsd::Stage> outStageFromObjectPtr;

        if (use_frame) {
            outStageFromObjectPtr = object_to_stage(
                *outObjectPtr, translatorState, Amino::String{"objects.usd"},
                BifrostUsd::ImageablePurpose::Default, frame, varying_topology);

        } else {
            outStageFromObjectPtr =
                object_to_stage(*outObjectPtr, Amino::String{"objects.usd"},
                                BifrostUsd::ImageablePurpose::Default);
        }

        if (outStageFromObjectPtr) {
            return outStageFromObjectPtr;
        } else {
            std::cout << kCtxDFFCreateStageFromGraphOutput
                      << "Error: Failed to create a Stage from an Object with "
                         "graph '"
                      << compoundName << "'." << std::endl;
            return nullptr;
        }
    }

    if (outObjectArrayPtr) {
        if (verbosityLevel == VerbosityLevel::eAllMessages) {
            std::cout
                << kCtxDFFCreateStageFromGraphOutput << "The graph '"
                << compoundName
                << "' has produced an array of Objects. Converting to USD..."
                << std::endl;
        }

        auto const& outStageFromObjectArrayPtr =
            array_of_objects_to_stage(*outObjectArrayPtr);
        if (outStageFromObjectArrayPtr) {
            return outStageFromObjectArrayPtr;
        } else {
            std::cout << kCtxDFFCreateStageFromGraphOutput
                      << "Error: Failed to create a Stage from an array of "
                         "Objects with graph '"
                      << compoundName << "'." << std::endl;
            return nullptr;
        }
    }

    return nullptr;
}

std::optional<TimelineSettings> getTimelineSettings(
    const SdfFileFormat::FileFormatArguments& args) {
    NameAndValueStr startFrameStr;
    auto findTimelineInfoStartFrame = args.find(kGlobals + kDot + kStart_frame);
    if (findTimelineInfoStartFrame != args.end()) {
        startFrameStr = getTypeAndValue(findTimelineInfoStartFrame->second);
    }

    NameAndValueStr endFrameStr;
    auto findTimelineInfoEndFrame = args.find(kGlobals + kDot + kEnd_frame);
    if (findTimelineInfoEndFrame != args.end()) {
        endFrameStr = getTypeAndValue(findTimelineInfoEndFrame->second);
    }

    if (!startFrameStr.first.empty() && !endFrameStr.first.empty()) {
        TimelineSettings timelineSettings;
        timelineSettings.startFrame = std::stod(startFrameStr.second);
        timelineSettings.endFrame   = std::stod(endFrameStr.second);
        return timelineSettings;
    }
    return std::nullopt;
}

double getFps(const SdfFileFormat::FileFormatArguments& args) {
    NameAndValueStr fpsStr;
    auto findTimeFps = args.find(kGlobals + kDot + kFps);
    if (findTimeFps != args.end()) {
        fpsStr = getTypeAndValue(findTimeFps->second);
        if(!fpsStr.first.empty()) {
            double fps = std::stod(fpsStr.second);
            if (fps > 0.0) return fps;
        }
    }
    return BifrostUsd::GraphExecutor::defaultFps;
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
    : SdfFileFormat(BifrostDynamicFileFormatTokens->Id,
                    BifrostDynamicFileFormatTokens->Version,
                    BifrostDynamicFileFormatTokens->Target,
                    BifrostDynamicFileFormatTokens->Extension) {}

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

    const auto& args = layer->GetFileFormatArguments();

    auto findCompoundName = args.find(kCompoundName);
    if (findCompoundName == args.end()) {
        std::cerr << kCtxDFFRead << "Error: The required '"
                  << BifrostDynamicFileFormatTokens->CompoundName.GetText()
                  << "' argument is missing." << std::endl;
        return false;
    }

    auto findOutputName = args.find(kOutputName);
    if (findOutputName == args.end()) {
        std::cerr << kCtxDFFRead << "Error: The required '"
                  << BifrostDynamicFileFormatTokens->OutputName.GetText()
                  << "' argument is missing." << std::endl;
        return false;
    }

    auto findReloadLibrary = args.find(kReloadLibrary);
    if (findReloadLibrary != args.end()) {
        auto reloadLibStr = getTypeAndValue(findReloadLibrary->second).second;
        if (reloadLibStr == "true") {
            reloadLibrary();
        }
    }

    auto compoundName = getTypeAndValue(findCompoundName->second).second;
    GraphExecutorPtr executor = makeGraphExecutor(Amino::String{compoundName.c_str()});
    if (!executor) {
        std::cerr << kCtxDFFRead
                  << "Error: Failed to create a GraphExecutor for graph '"
                  << compoundName << "'." << std::endl;
        return false;
    }
    auto graphArgs = getGraphArgs(args);

    auto verbLevel          = VerbosityLevel::eSilent;
    auto findVerbosityLevel = args.find(kOptions + kDot + kVerbosity_level);
    if (findVerbosityLevel != args.end()) {
        auto verbosityLevelStr =
            getTypeAndValue(findVerbosityLevel->second).second;
        verbLevel = VerbosityLevelMap[verbosityLevelStr];
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
                                       terminalTypeStr + " terminal output port",
                                   mainLayer, defaultPrimName);
            }
        };

    auto getGraphOutputName = [&findOutputName]() {
        using namespace Amino::StringViewLiterals;
        return getTypeAndValue(findOutputName->second).second;
    };

    // Use the root layer from a BifrostUsd stage created from a graph output
    // and add it as a sublayer of the main stage.
    auto addSubLayerFromOutput = [&mainLayer, &defaultPrimName](
                                     Amino::Ptr<BifrostUsd::Stage>&
                                                        bifrostStage,
                                     const std::string& outputName) {
        if (bifrostStage) {
            auto subLayer = (*bifrostStage)->GetRootLayer();
            addSubLayerToLayer(subLayer,
                               "Layer generated by Bifrost Graph from the "
                               "regular output port '" +
                                   outputName + "'",
                               mainLayer, defaultPrimName);
        }
    };
    // ******************************************************************

    auto timelineSettings = getTimelineSettings(args);

    // *******************************************************************
    // ************************ Execute the graph ************************
    // *******************************************************************

    Amino::Ptr<BifrostUsd::Stage> terminalFinalStage;
    Amino::Ptr<BifrostUsd::Stage> terminalProxyStage;
    Amino::Ptr<BifrostUsd::Stage> terminalDiagnosticStage;
    Amino::Ptr<BifrostUsd::Stage> graphOutputStage;

    bool        useFrame = timelineSettings.has_value();
    std::string outputName;

    if (useFrame) {
        auto usdTranslatorGraphState = Amino::ExecutionState{};

        executor->setTimelineSettings(timelineSettings.value());
        executor->setFps(getFps(args));

        bool hasFinal = executor->hasTerminalPort(TerminalType::eFinal);
        bool hasProxy = executor->hasTerminalPort(TerminalType::eProxy);
        bool hasDiagn = executor->hasTerminalPort(TerminalType::eDiagnostic);

        double startFrame = timelineSettings.value().startFrame;
        double endFrame   = timelineSettings.value().endFrame;

        for (double frame = startFrame; frame < endFrame + 1; frame += 1) {
            executeGraph(executor, compoundName, graphArgs, verbLevel, frame);

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

            if (outputName = getGraphOutputName(); !outputName.empty()) {
                graphOutputStage = createStageFromGraphOutput(
                    executor, compoundName, outputName, usdTranslatorGraphState,
                    verbLevel, true, static_cast<float>(frame));
            }
        }
    } else {
        executeGraph(executor, compoundName, graphArgs, verbLevel);

        if (executor->hasTerminalPort(TerminalType::eFinal)) {
            terminalFinalStage =
                createStageFromTerminalOutput(executor, TerminalType::eFinal);
        }
        if (executor->hasTerminalPort(TerminalType::eProxy)) {
            terminalProxyStage =
                createStageFromTerminalOutput(executor, TerminalType::eProxy);
        }
        if (executor->hasTerminalPort(TerminalType::eDiagnostic)) {
            terminalDiagnosticStage = createStageFromTerminalOutput(
                executor, TerminalType::eDiagnostic);
        }

        if (outputName = getGraphOutputName(); !outputName.empty()) {
            auto dummyState = Amino::ExecutionState{};
            graphOutputStage = createStageFromGraphOutput(
                executor, compoundName, outputName, dummyState, verbLevel);
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
        addSubLayerFromOutput(graphOutputStage, outputName);
    }

    // *******************************************************************
    // *******************************************************************
    // *******************************************************************

    auto defaultPrim = mainStage->GetPrimAtPath(
        SdfPath::AbsoluteRootPath().AppendChild(defaultPrimName));
    if (defaultPrim) {
        mainStage->SetDefaultPrim(defaultPrim);
    } else {
        std::cout << kCtxDFFRead << "Error: Unable to find the "
                     "default prim in the generated Stage from graph '"
                  << compoundName << "'." << std::endl;
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
    [[maybe_unused]] const std::string& assetPath,
    const PcpDynamicFileFormatContext&  context,
    FileFormatArguments*                args,
    [[maybe_unused]] VtValue*           contextDependencyData) const {
    // Get BifrostGraph_Params dictionary.
    VtValue            val;
    ParamNameToTypeMap nameToTypesMap;

    if (context.ComposeValue(BifrostDynamicFileFormatTokens->CompoundName,
                             &val) &&
        val.IsHolding<std::string>()) {
        (*args)[kCompoundName] = "string " + val.UncheckedGet<std::string>();
    }

    if (context.ComposeValue(BifrostDynamicFileFormatTokens->ReloadLibrary,
                             &val) &&
        val.IsHolding<Amino::bool_t>()) {
        nameToTypesMap[kReloadLibrary] = "bool";
        (*args)[kReloadLibrary] =
            TfStringify(val.UncheckedGet<Amino::bool_t>());
    } else {
        val = VtValue{};
    }

    if (context.ComposeValue(BifrostDynamicFileFormatTokens->Options, &val) &&
        val.IsHolding<VtDictionary>()) {
        const VtDictionary& dict = val.UncheckedGet<VtDictionary>();

        for (const auto& [name, value] : dict) {
            std::string fullName = kOptions + "." + name;
            (*args)[fullName]    = getArg(value, fullName, nameToTypesMap);
        }
    } else {
        val = VtValue{};
    }

    if (context.ComposeValue(BifrostDynamicFileFormatTokens->Globals, &val) &&
        val.IsHolding<VtDictionary>()) {
        const VtDictionary& dict = val.UncheckedGet<VtDictionary>();

        for (const auto& [name, value] : dict) {
            std::string fullName = kGlobals + "." + name;
            (*args)[fullName]    = getArg(value, fullName, nameToTypesMap);
        }
    } else {
        val = VtValue{};
    }

    if (context.ComposeValue(BifrostDynamicFileFormatTokens->Params, &val) &&
        val.IsHolding<VtDictionary>()) {
        const VtDictionary& dict = val.UncheckedGet<VtDictionary>();

        for (const auto& [name, value] : dict) {
            (*args)[name] = getArg(value, name, nameToTypesMap);
        }
    } else {
        val = VtValue{};
    }

    if (context.ComposeValue(BifrostDynamicFileFormatTokens->OutputName,
                             &val) &&
        val.IsHolding<std::string>()) {
        (*args)[kOutputName] = "string " + val.UncheckedGet<std::string>();
    } else {
        val = VtValue{};
    }

    // Override BifrostGraph_Params dictionary if there are some attributes
    // matching the param nane and type.
    // TODO (BIFROST-13584): use "bifrost" namespace to retrieve atttributes
    // from the context that are used to set graph inputs.
    for (const auto& [name, typeName] : nameToTypesMap) {
        getArgOverride(context, name, args);
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
