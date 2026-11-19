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

#include "dffTestLayerHelpers.h"

#include "dffDiagnosticsRuntime.h"

#include <gtest/gtest.h>

// Open USD
#include <pxr/base/vt/array.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/payload.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/sdf/reference.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/points.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
std::map<VerbosityLevel, std::string> VerbosityLevelToStrMap{
    {VerbosityLevel::eSilent, "Silent"},
    {VerbosityLevel::eErrorsOnly, "ErrorsOnly"},
    {VerbosityLevel::eErrorsAndWarnings, "ErrorsAndWarnings"},
    {VerbosityLevel::eAllMessages, "AllMessages"}};

void createRootLayerWithDffImpl(
    const std::string&                             diagnosticsBucketKey,
    BifrostUsd::TestUtils::UniqueTestOutputSubdir& outputDir,
    SdfLayerRefPtr&                                out_rootLayer,
    Amino::String&                                 out_layerPath) {
    const auto* testInfo =
        ::testing::UnitTest::GetInstance()->current_test_info();
    const std::string layerName = testInfo ? testInfo->name() : "unknown_test";
    out_layerPath =
        outputDir.getPath_abs(Amino::String{layerName.data()} + ".usda");

    // Dynamic Payload works only with usda file format.
    // TODO LATER: Investigate why.
    auto fileFormat = SdfFileFormat::FindByExtension(".usda");
    out_rootLayer   = SdfLayer::New(fileFormat, out_layerPath.c_str());

    auto rootPrimSpec =
        SdfPrimSpec::New(out_rootLayer, "Root", SdfSpecifierDef);

    if (!diagnosticsBucketKey.empty()) {
        auto diagnosticsBucketAttr = SdfAttributeSpec::New(
            rootPrimSpec, std::string{kDffDiagnosticsBucketAttr},
            SdfValueTypeNames->String, SdfVariabilityVarying,
            /*custom=*/false);
        diagnosticsBucketAttr->SetDefaultValue(VtValue{diagnosticsBucketKey});
    }

    auto payload     = SdfPayload("anon:autodesk:bifrost.bifrostDynamicFile");
    auto payloadList = rootPrimSpec->GetPayloadList();
    payloadList.Append(payload);

    auto dynFffPrimSpec = SdfPrimSpec::New(
        out_rootLayer, "DynamicFileFormatField", SdfSpecifierDef);

    auto internalReference =
        SdfReference(std::string(), dynFffPrimSpec->GetPath());

    auto referenceList = rootPrimSpec->GetReferenceList();
    referenceList.Append(internalReference);
}
} // namespace

namespace DffTestHelpers {

Amino::String createRootLayerWithDff(
    const std::string&                             diagnosticsBucketKey,
    BifrostUsd::TestUtils::UniqueTestOutputSubdir& outputDir) {
    SdfLayerRefPtr layer;
    Amino::String  exportPath;
    createRootLayerWithDffImpl(diagnosticsBucketKey, outputDir, layer,
                               exportPath);
    layer->Save();
    return exportPath;
}

Amino::String createRootLayerWithDefaultDffFields(
    const std::string&                             diagnosticsBucketKey,
    BifrostUsd::TestUtils::UniqueTestOutputSubdir& outputDir,
    const std::string&                             compoundName,
    const std::string&                             outputName) {
    SdfLayerRefPtr layer;
    Amino::String  exportPath;

    createRootLayerWithDffImpl(diagnosticsBucketKey, outputDir, layer,
                               exportPath);

    layer->SetFieldDictValueByKey(SdfPath{"/DynamicFileFormatField"},
                                  TfToken{"bifrostCompound"}, TfToken("name"),
                                  TfToken{compoundName.data()});

    if (outputName.empty()) {
        layer->SetField(SdfPath{"/DynamicFileFormatField"},
                        TfToken{"bifrostOutputs"}, VtValue{VtArray<TfToken>{}});
    } else {
        layer->SetField(SdfPath{"/DynamicFileFormatField"},
                        TfToken{"bifrostOutputs"},
                        VtValue{VtArray<TfToken>{TfToken{outputName.data()}}});
    }

    layer->SetFieldDictValueByKey(SdfPath{"/DynamicFileFormatField"},
                                  TfToken{"bifrostSettings"},
                                  TfToken("reloadLibrary"), false);
    layer->SetFieldDictValueByKey(
        SdfPath{"/DynamicFileFormatField"}, TfToken{"bifrostSettings"},
        TfToken("verbosityLevel"),
        VerbosityLevelToStrMap[VerbosityLevel::eErrorsAndWarnings]);

    layer->Save();

    return exportPath;
}

void setVerbosityLevelField(UsdStageRefPtr& stage, VerbosityLevel level) {
    stage->GetRootLayer()->SetFieldDictValueByKey(
        SdfPath{"/DynamicFileFormatField"}, TfToken{"bifrostSettings"},
        TfToken("verbosityLevel"), VerbosityLevelToStrMap[level]);
}

void setReloadLibraryField(UsdStageRefPtr& stage, bool reload) {
    stage->GetRootLayer()->SetFieldDictValueByKey(
        SdfPath{"/DynamicFileFormatField"}, TfToken{"bifrostSettings"},
        TfToken{"reloadLibrary"}, reload);
}

void setGlobalsField(UsdStageRefPtr& stage, const VtDictionary& paramsDict) {
    stage->GetRootLayer()->SetField(SdfPath{"/DynamicFileFormatField"},
                                    TfToken{"bifrostGlobals"}, paramsDict);
}

void setInputsField(UsdStageRefPtr& stage, const VtDictionary& paramsDict) {
    stage->GetRootLayer()->SetField(SdfPath{"/DynamicFileFormatField"},
                                    TfToken{"bifrostInputs"}, paramsDict);
}

void setOutputsField(UsdStageRefPtr&                 stage,
                     const std::vector<std::string>& outputPortNames) {
    VtArray<TfToken> outputs;
    outputs.reserve(outputPortNames.size());
    for (const auto& name : outputPortNames) {
        outputs.push_back(TfToken{name});
    }
    stage->GetRootLayer()->SetField(SdfPath{"/DynamicFileFormatField"},
                                    TfToken{"bifrostOutputs"}, outputs);
}

void setAttributeOverrides(
    UsdStageRefPtr&                           stage,
    const std::vector<AttributeNameAndValue>& attributeVector) {
    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});

    for (const auto& [attrName, attrTypeAndValue] : attributeVector) {
        auto attr = rootPrim.GetAttribute(attrName);
        if (!attr) {
            attr = rootPrim.CreateAttribute(attrName, attrTypeAndValue.first,
                                            /*custom*/ false);
        }
        attr.Set(attrTypeAndValue.second);
    }
}

void setCompoundNameAttrAndSave(const SdfLayerRefPtr& layer,
                                const std::string&    overrideName) {
    auto stage = UsdStage::Open(layer);
    ASSERT_TRUE(stage) << "Failed to open stage for attribute override setup";

    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim) << "Root prim not found";

    auto attr = rootPrim.CreateAttribute(TfToken{"bifrost:compound:name"},
                                         SdfValueTypeNames->String,
                                         /*custom=*/false);
    ASSERT_TRUE(attr) << "Failed to create bifrost:compound:name attribute";
    attr.Set(overrideName);

    layer->Save();
}

PXR_NS::UsdStageRefPtr createStageWithCompoundPayload(
    const std::string& fullyQualifiedCompoundName,
    const std::string& outputName,
    const std::string&                             bucketKey,
    BifrostUsd::TestUtils::UniqueTestOutputSubdir& outputDir) {
    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, outputDir,
        fullyQualifiedCompoundName, outputName);
    return UsdStage::Open(rootLayerPath.c_str());
}

PXR_NS::SdfLayerHandle getGeneratedPayloadLayer(
    const PXR_NS::UsdStageRefPtr& stage) {
    if (!stage) {
        return {};
    }
    // The DFF plugin is a runtime-loaded USD MODULE that tests do not link
    // against, so match on the file format id string rather than the plugin's
    // BifrostDffPluginTokens->Id symbol.
    static const TfToken kDffFormatId{"bifrostDynamicFile"};
    for (const auto& layer : stage->GetUsedLayers()) {
        if (layer && layer->GetFileFormat() &&
            layer->GetFileFormat()->GetFormatId() == kDffFormatId) {
            return layer;
        }
    }
    return {};
}

PXR_NS::UsdStageRefPtr createMoveUpPointSimulationStage(
    const std::string&                             bucketKey,
    BifrostUsd::TestUtils::UniqueTestOutputSubdir& outputDir) {
    return createStageWithCompoundPayload(
        "Test::DynamicFileFormat::move_up_point_simulation", "point", bucketKey,
        outputDir);
}

PXR_NS::UsdAttribute setUpMoveUpPointSimulationFields(
    PXR_NS::UsdStageRefPtr& stage,
    float                   verticalStep,
    double                  startFrame,
    double                  endFrame,
    double                  fps) {
    using namespace PXR_NS;
    setGlobalsField(stage,
                    VtDictionary{
                        {"timeline_info_start_frame", VtValue{startFrame}},
                        {"timeline_info_end_frame", VtValue{endFrame}},
                        {"time_fps", VtValue{fps}},
                    });
    setInputsField(stage,
                   VtDictionary{{"vertical_step", VtValue{verticalStep}}});

    auto pointsPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/points"});
    auto pointsGeom = UsdGeomPoints{pointsPrim};
    return pointsGeom.GetPointsAttr();
}

std::optional<PXR_NS::GfVec3f> firstPointAt(
    const PXR_NS::UsdAttribute& pointsAttr, PXR_NS::UsdTimeCode time) {
    using namespace PXR_NS;
    VtVec3fArray points;
    EXPECT_TRUE(pointsAttr.Get(&points, time));
    if (points.empty()) {
        return std::nullopt;
    }
    return points[0];
}

void checkMoveUpPointSimulation(const UsdStageRefPtr& stage,
                                double                startFrame,
                                double                endFrame,
                                double                fps,
                                float                 verticalStep) {
    EXPECT_GT(fps, 0.0);
    for (double frame = startFrame; frame <= endFrame; ++frame) {
        auto expectedY =
            static_cast<float>((frame - startFrame) * (static_cast<double>(verticalStep) / fps));
        ASSERT_EQ(firstPointAt(stage->GetPrimAtPath(SdfPath{"/Root/geo/points"})
                                   .GetAttribute(TfToken{"points"}),
                               UsdTimeCode{frame}),
                  GfVec3f(0.f, expectedY, 0.f));
    }
}
} // namespace DffTestHelpers
