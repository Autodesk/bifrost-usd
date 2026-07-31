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
}

namespace DffTestHelpers {

Amino::String createRootLayerWithDff(
    const std::string&                             diagnosticsBucketKey,
    BifrostUsd::TestUtils::UniqueTestOutputSubdir& outputDir) {
    SdfLayerRefPtr layer;
    Amino::String  exportPath;
    createRootLayerWithDffImpl(diagnosticsBucketKey, outputDir, layer, exportPath);
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

    createRootLayerWithDffImpl(diagnosticsBucketKey, outputDir, layer, exportPath);

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

void setLiveVerbosityLevelField(UsdStageRefPtr& stage,
                                VerbosityLevel  level) {
    stage->GetRootLayer()->SetFieldDictValueByKey(
        SdfPath{"/DynamicFileFormatField"}, TfToken{"bifrostSettings"},
        TfToken("verbosityLevel"), VerbosityLevelToStrMap[level]);
}

void setLiveReloadLibraryField(UsdStageRefPtr& stage, bool reload) {
    stage->GetRootLayer()->SetFieldDictValueByKey(
        SdfPath{"/DynamicFileFormatField"}, TfToken{"bifrostSettings"},
        TfToken{"reloadLibrary"}, reload);
}

void setLiveGlobalsField(UsdStageRefPtr&     stage,
                         const VtDictionary& paramsDict) {
    stage->GetRootLayer()->SetField(SdfPath{"/DynamicFileFormatField"},
                                    TfToken{"bifrostGlobals"}, paramsDict);
}

void setLiveInputsField(UsdStageRefPtr&     stage,
                        const VtDictionary& paramsDict) {
    stage->GetRootLayer()->SetField(SdfPath{"/DynamicFileFormatField"},
                                    TfToken{"bifrostInputs"}, paramsDict);
}

void setLiveOutputsField(UsdStageRefPtr&                stage,
                         const std::vector<std::string>& outputPortNames) {
    VtArray<TfToken> outputs;
    outputs.reserve(outputPortNames.size());
    for (const auto& name : outputPortNames) {
        outputs.push_back(TfToken{name});
    }
    stage->GetRootLayer()->SetField(SdfPath{"/DynamicFileFormatField"},
                                    TfToken{"bifrostOutputs"}, outputs);
}

void setLiveAttributeOverrides(
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

void setOfflineCompoundNameAttr(const SdfLayerRefPtr& layer,
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

} // namespace DffTestHelpers
