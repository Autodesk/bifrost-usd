//-
// Copyright 2023 Autodesk, Inc.
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
#include <Amino/Core/String.h>
#include <Bifrost/FileUtils/FileUtils.h>

#include <bifusd/config/CfgWarningMacros.h>
#include <gtest/gtest.h>
#include <nodedefs/usd_pack/usd_layer_nodedefs.h>
#include <nodedefs/usd_pack/usd_stage_nodedefs.h>
#include <nodedefs/usd_pack/usd_utils.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdUtils/stageCache.h>
#include <utils/test/testUtils.h>

#include <fstream>
#include <regex>
#include <string>

// Note: To silence warnings coming from USD library
#include <bifusd/config/CfgWarningMacros.h>
BIFUSD_WARNING_PUSH
BIFUSD_WARNING_DISABLE_MSC(4003)
#include <pxr/usd/usd/modelAPI.h>
BIFUSD_WARNING_POP

using namespace BifrostUsd::TestUtils;

namespace {
Amino::String getThisTestOutputDir() {
    return Bifrost::FileUtils::filePath(getTestOutputDir(),
                                        "testStageNodeDefs");
}
Amino::String getThisTestOutputPath(const Amino::String& filename) {
    return Bifrost::FileUtils::filePath(getThisTestOutputDir(), filename);
}
} // namespace

TEST(StageNodeDefs, initial_cleanup) {
    ASSERT_TRUE(Bifrost::FileUtils::removeAll(getThisTestOutputDir()));
}

TEST(StageNodeDefs, set_edit_layer) {
    const Amino::String rootName = "helloworld.usd";

    // Case 1: test with no sublayers
    {
        std::string title("\t Case 1: Test when root has no sublayers - ");

        // Open an SdfLayer with no sublayer in it:
        PXR_NS::SdfLayerRefPtr sdfRootLayer = PXR_NS::SdfLayer::FindOrOpen(rootName.c_str());
        ASSERT_NE(sdfRootLayer, nullptr);

        // Create a Bifrost root layer and stage from the SdfLayer:
        BifrostUsd::Layer layer{sdfRootLayer, true /*editable*/};
        EXPECT_TRUE(layer.isValid());
        EXPECT_EQ(layer.getSubLayers().size(), 0);
        BifrostUsd::Stage stage{layer};
        EXPECT_TRUE(stage.isValid());

        if(layer && stage) {
            // Initially, should be Root layer:
            EXPECT_EQ(stage.getEditLayerIndex(), -1);

            // Test with multiple layer indices, including root layer index -1.
            // Edit layer must always remain the root layer
            for(int i = -3; i < 2; ++i) {
                
                // Try changing it
                USD::Stage::set_edit_layer(stage, i);

                // Check current edit layer through Pixar interface
                std::string id = stage.get().GetEditTarget().GetLayer()->GetIdentifier();
                EXPECT_NE(id.find(rootName.c_str()), std::string::npos)
                    << title
                    << "After an attempt to change the edit layer to index=" << i
                    << ", the USD::Stage's EditTarget now has identifier=`" << id
                    << "` but we expect it to contain string=`" << rootName.c_str() << "`\n";

                // Check current edit layer on BifrostUsd::Stage
                EXPECT_EQ(stage.getEditLayerIndex(), -1)
                    << title
                    << "After an attempt to change the edit layer to index=" << i
                    << ", the BifrostUsd::Stage's EditLayerIndex was expected to remain -1\n";
            }
        }
    }

    {
        // Open an SdfLayer with some sublayers in it:
        const Amino::Array<Amino::String> subNames = {
            "Grass1.usd", "Mushroom1.usd", "Tree1.usd"}; // weakest to strongest
        const int numSubNames = static_cast<int>(subNames.size());
        PXR_NS::SdfLayerRefPtr sdfRootLayer = PXR_NS::SdfLayer::FindOrOpen(rootName.c_str());
        ASSERT_NE(sdfRootLayer, nullptr);
        Amino::String errorMsg;
        addSubLayers(sdfRootLayer, subNames, errorMsg);
        EXPECT_STREQ("", errorMsg.c_str());
        checkSdfSublayerPaths(*sdfRootLayer, subNames, errorMsg);
        EXPECT_STREQ("", errorMsg.c_str());

        // Create a Bifrost root layer and stage from the SdfLayer:
        BifrostUsd::Layer layer{sdfRootLayer, true /*editable*/};
        EXPECT_TRUE(layer.isValid());
        EXPECT_EQ(layer.getSubLayers().size(), static_cast<size_t>(numSubNames));
        BifrostUsd::Stage stage{layer};
        EXPECT_TRUE(stage.isValid());

        Amino::String expectedName;
        int lastReversedEditIndex;

        // Case 2: test with multiple sublayers and valid layer indices:
        if(layer && stage) {
            std::string title("\t Case 2: Test with multiple sublayers and VALID layer indices - ");

            // Initially, should be Root layer:
            EXPECT_EQ(stage.getEditLayerIndex(), -1);

            // Note: we mix other valid indices with the special case of
            //       index -1 (root layer), but end with non-root layer
            std::vector<int> indices{0, -1, 1, -1, 2};

            for(auto it = indices.begin(); it < indices.end(); ++it) {
                // Set edit layer through our interface
                USD::Stage::set_edit_layer(stage, *it);

                // Check current edit layer through Pixar interface
                std::string id = stage.get().GetEditTarget().GetLayer()->GetIdentifier();
                expectedName = (*it == -1) ? rootName : subNames[*it];
                EXPECT_NE(id.find(expectedName.c_str()), std::string::npos)
                    << title
                    << "After changing the edit layer to index=" << (*it)
                    << ", the USD::Stage's EditTarget now has identifier=`" << id
                    << "` but we expect it to contain string=`" << expectedName.c_str() << "`\n";

                // USD::Stage::set_edit_layer() uses a WEAKEST to STRONGEST index
                // ordering, while BifrostUsd::Stage::getEditLayerIndex() uses the
                // opposite ordering (like Pixar layer stack):
                lastReversedEditIndex = (*it == -1) ? -1 :
                    USDUtils::reversedSublayerIndex(*it, numSubNames);

                // Check current edit layer on BifrostUsd::Stage
                EXPECT_EQ(stage.getEditLayerIndex(), lastReversedEditIndex)
                    << title
                    << "After changing the edit layer to index=" << (*it)
                    << ", the BifrostUsd::Stage's EditLayerIndex was expected to be "
                    << "reverse(" << (*it) << ")=" << lastReversedEditIndex << "\n";
            }
        }

        // Case 3: test with multiple sublayers and INVALID layer indices:
        if(layer && stage) {
            std::string title("\t Case 3: Test with multiple sublayers and INVALID layer indices - ");

            std::vector<int> indices{-3, -2, numSubNames, numSubNames+1};
            for(auto it = indices.begin(); it < indices.end(); ++it) {
                // Try changing it for an invalid index
                // Edit layer must be left unchanged to the last valid index we set.
                USD::Stage::set_edit_layer(stage, *it);

                // Check current edit layer through Pixar interface
                std::string id = stage.get().GetEditTarget().GetLayer()->GetIdentifier();
                EXPECT_NE(id.find(expectedName.c_str()), std::string::npos)
                    << title
                    << "After an attempt to change the edit layer to INVALID index=" << (*it)
                    << ", the USD::Stage's EditTarget now has identifier=`" << id
                    << "` but we expect it to contain string=`" << expectedName.c_str() << "`\n";

                // Check current edit layer on BifrostUsd::Stage
                EXPECT_EQ(stage.getEditLayerIndex(), lastReversedEditIndex)
                    << title
                    << "After an attempt to change the edit layer to INVALID index=" << (*it)
                    << ", the BifrostUsd::Stage's EditLayerIndex was expected to remain "
                    << lastReversedEditIndex << "\n";
            }
        }
    }
}

TEST(StageNodeDefs, set_stage_metadata) {
    auto stage = Amino::newMutablePtr<BifrostUsd::Stage>(
        getResourcePath("helloworld.usd").c_str());
    ASSERT_TRUE(stage);
    ASSERT_TRUE(*stage);

    // Set and get string metadata
    Amino::String key = "documentation";
    Amino::String doc = "This is my documentation";
    USD::Stage::set_stage_metadata(*stage, key, doc);
    ASSERT_TRUE(*stage);
    ASSERT_TRUE(stage->get().HasMetadata(PXR_NS::SdfFieldKeys->Documentation));

    Amino::String strDefault = "Oups an error!";
    Amino::String strResult;
    USD::Stage::get_stage_metadata(*stage, key, strDefault, strResult);
    ASSERT_EQ(strResult, doc);
    // Check error - must have base type different than for "documentation"
    // Documentation is string and TimeCodesPerSecond are not...
    Amino::String bogusKey = "timeCodesPerSecond";
    USD::Stage::get_stage_metadata(*stage, bogusKey, strDefault, strResult);
    ASSERT_EQ(strResult, strDefault);

    // Set and get double value
    key             = "endTimeCode";
    double dblValue = 1001.0;
    USD::Stage::set_stage_metadata(*stage, key, dblValue);
    ASSERT_TRUE(*stage);
    ASSERT_TRUE(stage->get().HasMetadata(PXR_NS::SdfFieldKeys->EndTimeCode));

    double dblDefault = 123.0;
    double dblResult;
    USD::Stage::get_stage_metadata(*stage, key, dblDefault, dblResult);
    ASSERT_EQ(dblResult, dblValue);
    // Must have base type different than "EndTimeCode"
    // EndTimeCode is double and comment is string
    bogusKey = "comment";
    USD::Stage::get_stage_metadata(*stage, bogusKey, dblDefault, dblResult);
    ASSERT_EQ(dblResult, dblDefault);

    // Set and get Amino Object
    key                    = "customLayerData";
    auto          objValue = Bifrost::createObject();
    Amino::String strValue("foo");
    objValue->setProperty("my_string", strValue);
    objValue->setProperty("my_double", dblValue);
    objValue->setProperty("my_bool", true);
    USD::Stage::set_stage_metadata(*stage, key, *objValue);
    ASSERT_TRUE(*stage);
    ASSERT_TRUE(stage->get().HasMetadata(PXR_NS::SdfFieldKeys->CustomLayerData));

    Amino::Ptr<Bifrost::Object> objDefault = Bifrost::createObject();
    Amino::Ptr<Bifrost::Object> objResult;
    USD::Stage::get_stage_metadata(*stage, key, objDefault, objResult);

    /// \note Bool Set and Get not yet supported
    ASSERT_TRUE(objResult != nullptr);
    Amino::Any any = objResult->getProperty("my_double");
    {
        auto dblPtr = Amino::any_cast<double>(&any);
        ASSERT_TRUE(dblPtr != nullptr);
        ASSERT_DOUBLE_EQ(*dblPtr, dblValue);
    }

    any = objResult->getProperty("my_string");
    {
        auto strPtr = Amino::any_cast<Amino::String>(&any);
        ASSERT_TRUE(strPtr != nullptr);
        ASSERT_EQ(*strPtr, strValue);
    }

    // Must have base type different than "CustomLayerData"
    // CustomLayerData is anything and comment is string
    bogusKey = "comment";
    USD::Stage::get_stage_metadata(*stage, bogusKey, objDefault, objResult);
    ASSERT_TRUE(objResult->empty());
}

TEST(StageNodeDefs, set_default_prim) {
    BifrostUsd::Stage stage{getResourcePath("helloworld.usd").c_str()};
    ASSERT_TRUE(stage);
    // Path needs to be at root, for example "/hello/world" won't work
    const Amino::String primPath = "/hello";
    auto prim = stage->GetPrimAtPath(PXR_NS::SdfPath(primPath.c_str()));
    ASSERT_TRUE(prim.IsValid());
    USD::Stage::set_default_prim(stage, primPath);
    ASSERT_TRUE(stage);
    ASSERT_TRUE(stage->HasDefaultPrim());
    auto defaultPrim = stage->GetDefaultPrim();
    ASSERT_TRUE(defaultPrim.IsValid());
    ASSERT_EQ(primPath, defaultPrim.GetPath().GetText());
}

TEST(StageNodeDefs, get_default_prim) {
    BifrostUsd::Stage stage{getResourcePath("helloworld.usd").c_str()};
    ASSERT_TRUE(stage);
    // Path needs to be at root, for example "/hello/world" won't work
    const Amino::String primPath = "/hello";
    auto prim = stage->GetPrimAtPath(PXR_NS::SdfPath(primPath.c_str()));
    ASSERT_TRUE(prim.IsValid());
    stage->SetDefaultPrim(prim);
    Amino::String defaultPrimPath;
    USD::Stage::get_default_prim(stage, defaultPrimPath);
    ASSERT_TRUE(stage);
    ASSERT_TRUE(stage->HasDefaultPrim());
    ASSERT_EQ(defaultPrimPath, primPath);
}

TEST(StageNodeDefs, set_stage_up_axis) {
    BifrostUsd::Stage stage{getResourcePath("helloworld.usd").c_str()};
    ASSERT_TRUE(stage);

    // Z Axis
    bool success = USD::Stage::set_stage_up_axis(stage, BifrostUsd::UpAxis::Z);
    ASSERT_TRUE(success);
    PXR_NS::TfToken zaxis = PXR_NS::UsdGeomGetStageUpAxis(stage.getStagePtr());
    ASSERT_EQ(zaxis, PXR_NS::UsdGeomTokens->z);

    // Y Axis
    success = USD::Stage::set_stage_up_axis(stage, BifrostUsd::UpAxis::Y);
    ASSERT_TRUE(success);
    PXR_NS::TfToken yaxis = PXR_NS::UsdGeomGetStageUpAxis(stage.getStagePtr());
    ASSERT_EQ(yaxis, PXR_NS::UsdGeomTokens->y);
}

TEST(StageNodeDefs, set_stage_time_code) {
    BifrostUsd::Stage stage{getResourcePath("helloworld.usd").c_str()};
    ASSERT_TRUE(stage);

    const float start = 1.7f;
    const float end   = 84.1f;

    USD::Stage::set_stage_time_code(stage, start, end);
    ASSERT_TRUE(stage);

    auto stageStart = static_cast<float>(stage->GetStartTimeCode());
    ASSERT_FLOAT_EQ(start, stageStart);

    auto stageEnd = static_cast<float>(stage->GetEndTimeCode());
    ASSERT_FLOAT_EQ(end, stageEnd);
}

TEST(StageNodeDefs, save_stage) {
    const char*         helloworldContent = R"usda(#usda 1.0

def Xform "hello"
{
    def Sphere "world"
    {
        int testAttr = 123
    }
}

)usda";
    BifrostUsd::Stage stage{getResourcePath("helloworld.usd")};
    ASSERT_TRUE(stage);
    auto filepath = getThisTestOutputPath("testSaveStage.usda");

    bool success = USD::Stage::save_stage(stage, filepath);
    ASSERT_TRUE(success);
    std::ifstream     filestream(filepath.c_str());
    std::stringstream filebuffer;
    filebuffer << filestream.rdbuf();
    ASSERT_STREQ(filebuffer.str().c_str(), helloworldContent);
}

TEST(StageNodeDefs, open_stage_from_layer) {
    {
        Amino::MutablePtr<BifrostUsd::Stage> stage;
        BifrostUsd::Layer layer{getResourcePath("helloworld.usd"), ""};
        Amino::Array<Amino::String> mask;
        auto load = BifrostUsd::InitialLoadSet::LoadAll;

        USD::Stage::open_stage_from_layer(layer, mask, load, -1, stage);

        ASSERT_TRUE(*stage);
        auto prim = stage->get().GetPrimAtPath(PXR_NS::SdfPath("/hello/world"));
        ASSERT_TRUE(prim.IsValid());
    }
    {
        // Test opening read only SdfLayer.
        auto pxr_layer =
            PXR_NS::SdfLayer::FindOrOpen("kitchen_set/kitchen_props.usd");

        Amino::MutablePtr<BifrostUsd::Stage> stage;
        BifrostUsd::Layer                    layer{pxr_layer, false};
        Amino::Array<Amino::String>            mask;
        auto load = BifrostUsd::InitialLoadSet::LoadAll;

        USD::Stage::open_stage_from_layer(layer, mask, load, -1, stage);

        auto bottle =
            stage->get().GetPrimAtPath(PXR_NS::SdfPath("/Props_grp/bottle"));
        ASSERT_TRUE(bottle);
        ASSERT_EQ(bottle.GetChildrenNames().size(), 1);

        auto        model = PXR_NS::UsdModelAPI(bottle);
        std::string assetName;
        model.GetAssetName(&assetName);
        ASSERT_EQ(assetName, "Bottle");

        auto spoon = stage->get().GetPrimAtPath(
            PXR_NS::SdfPath("/Props_grp/MeasuringSpoon"));
        ASSERT_TRUE(spoon);
    }
    {
        // Test opening masked.
        auto pxr_layer =
            PXR_NS::SdfLayer::FindOrOpen("kitchen_set/kitchen_props.usd");

        Amino::MutablePtr<BifrostUsd::Stage> stage;
        BifrostUsd::Layer                    layer{pxr_layer, false};
        Amino::Array<Amino::String>            mask{"/Props_grp/bottle"};
        auto load = BifrostUsd::InitialLoadSet::LoadAll;

        USD::Stage::open_stage_from_layer(layer, mask, load, -1, stage);

        auto bottle =
            stage->get().GetPrimAtPath(PXR_NS::SdfPath("/Props_grp/bottle"));
        ASSERT_TRUE(bottle);
        ASSERT_EQ(bottle.GetChildrenNames().size(), 1);

        auto        model = PXR_NS::UsdModelAPI(bottle);
        std::string assetName;
        model.GetAssetName(&assetName);
        ASSERT_EQ(assetName, "Bottle");

        auto spoon = stage->get().GetPrimAtPath(
            PXR_NS::SdfPath("/Props_grp/MeasuringSpoon"));
        ASSERT_FALSE(spoon);
    }
    {
        // Test opening unloaded.
        auto pxr_layer =
            PXR_NS::SdfLayer::FindOrOpen("kitchen_set/kitchen_props.usd");

        Amino::MutablePtr<BifrostUsd::Stage> stage;
        BifrostUsd::Layer                    layer{pxr_layer, true};
        Amino::Array<Amino::String>            mask;
        auto load = BifrostUsd::InitialLoadSet::LoadNone;

        USD::Stage::open_stage_from_layer(layer, mask, load, -1, stage);

        auto bottle =
            stage->get().GetPrimAtPath(PXR_NS::SdfPath("/Props_grp/bottle"));
        ASSERT_TRUE(bottle);
        ASSERT_EQ(bottle.GetChildrenNames().size(), 0);
    }
    {
        // Test opening and setting edit targe.
        BifrostUsd::Layer root_layer{"root.usd"};
        auto a_layer = Amino::newClassPtr<BifrostUsd::Layer>("a.usd");
        auto b_layer = Amino::newClassPtr<BifrostUsd::Layer>("b.usd");
        auto c_layer = Amino::newClassPtr<BifrostUsd::Layer>("c.usd");
        auto d_layer = Amino::newClassPtr<BifrostUsd::Layer>("d.usd");

        root_layer.insertSubLayer(*d_layer);
        root_layer.insertSubLayer(*c_layer);
        root_layer.insertSubLayer(*b_layer);
        root_layer.insertSubLayer(*a_layer);

        Amino::MutablePtr<BifrostUsd::Stage> stage;
        Amino::Array<Amino::String>            mask;
        auto load = BifrostUsd::InitialLoadSet::LoadAll;

        USD::Stage::open_stage_from_layer(root_layer, mask, load, 0, stage);

        ASSERT_TRUE(*stage);
        ASSERT_TRUE(std::regex_match(
            stage->get().GetEditTarget().GetLayer()->GetIdentifier().c_str(),
            std::regex("anon:.*:a.usd")));

        USD::Stage::open_stage_from_layer(root_layer, mask, load, 1, stage);

        ASSERT_TRUE(*stage);
        ASSERT_TRUE(std::regex_match(
            stage->get().GetEditTarget().GetLayer()->GetIdentifier().c_str(),
            std::regex("anon:.*:b.usd")));

        USD::Stage::open_stage_from_layer(root_layer, mask, load, 2, stage);

        ASSERT_TRUE(*stage);
        ASSERT_TRUE(std::regex_match(
            stage->get().GetEditTarget().GetLayer()->GetIdentifier().c_str(),
            std::regex("anon:.*:c.usd")));

        USD::Stage::open_stage_from_layer(root_layer, mask, load, 3, stage);

        ASSERT_TRUE(*stage);
        ASSERT_TRUE(std::regex_match(
            stage->get().GetEditTarget().GetLayer()->GetIdentifier().c_str(),
            std::regex("anon:.*:d.usd")));
    }
}

TEST(StageNodeDefs, open_stage_from_cache) {
    {
        auto cachedStage = Amino::newMutablePtr<BifrostUsd::Stage>(
            getResourcePath("helloworld.usd").c_str());
        ASSERT_TRUE(*cachedStage);
        auto id = PXR_NS::UsdUtilsStageCache::Get()
                      .Insert(cachedStage->getStagePtr())
                      .ToLongInt();

        auto stage = Amino::newClassPtr<BifrostUsd::Stage>();
        USD::Stage::open_stage_from_cache(id, -1, stage);
        ASSERT_TRUE(*stage);
        auto prim = stage->get().GetPrimAtPath(PXR_NS::SdfPath("/hello/world"));
        ASSERT_TRUE(prim.IsValid());
    }
    {
        // with edit target
        auto root_path =
            getThisTestOutputPath("open_stage_from_cache_root.usda");
        auto a_path = getThisTestOutputPath("open_stage_from_cache_a.usda");
        auto b_path = getThisTestOutputPath("open_stage_from_cache_b.usda");

        auto a_layer = BifrostUsd::Layer{a_path};
        ASSERT_TRUE(a_layer);
        a_layer.setFilePath(a_path);

        auto b_layer = BifrostUsd::Layer{b_path};
        ASSERT_TRUE(b_layer);
        b_layer.setFilePath(b_path);

        // Create the root layer:
        BifrostUsd::Layer root_layer{root_path};
        ASSERT_TRUE(root_layer);

        // Add sublayers to it:
        root_layer.insertSubLayer(b_layer);
        root_layer.insertSubLayer(a_layer);

        // At this point of the test, the root and sublayer files should not yet
        // exist on disk (see the initial_cleanup test phase above):
        ASSERT_FALSE(Bifrost::FileUtils::filePathExists(root_path))
            << "The output root layer file " << root_path.c_str()
            << " must not already exist when this test runs.\n";
        ASSERT_FALSE(Bifrost::FileUtils::filePathExists(a_path))
            << "The output sublayer file " << a_path.c_str()
            << " must not already exist when this test runs.\n";
        ASSERT_FALSE(Bifrost::FileUtils::filePathExists(b_path))
            << "The output sublayer file " << b_path.c_str()
            << " must not already exist when this test runs.\n";

        // Export the root and sublayers to disk:
        ASSERT_TRUE(root_layer.exportToFile(root_path));

        // Generate the usd StageCache:
        auto cachedStage = Amino::newMutablePtr<BifrostUsd::Stage>(root_path);
        ASSERT_TRUE(*cachedStage);
        auto id = PXR_NS::UsdUtilsStageCache::Get()
                      .Insert(cachedStage->getStagePtr())
                      .ToLongInt();

        Amino::Ptr<BifrostUsd::Stage> stage;
        USD::Stage::open_stage_from_cache(id, -1, stage);
        ASSERT_TRUE(stage);
        ASSERT_TRUE(*stage);
        ASSERT_TRUE(std::regex_match(
            stage->get().GetEditTarget().GetLayer()->GetIdentifier().c_str(),
            std::regex("anon:.*:open_stage_from_cache_root.usda")));

        stage.reset();
        USD::Stage::open_stage_from_cache(id, 0, stage);
        ASSERT_TRUE(stage);
        ASSERT_TRUE(*stage);
        ASSERT_TRUE(std::regex_match(
            stage->get().GetEditTarget().GetLayer()->GetIdentifier().c_str(),
            std::regex("anon:.*:open_stage_from_cache_a.usda")));

        stage.reset();
        USD::Stage::open_stage_from_cache(id, 1, stage);
        ASSERT_TRUE(stage);
        ASSERT_TRUE(*stage);
        ASSERT_TRUE(std::regex_match(
            stage->get().GetEditTarget().GetLayer()->GetIdentifier().c_str(),
            std::regex("anon:.*:open_stage_from_cache_b.usda")));
    }
}

TEST(StageNodeDefs, send_stage_to_cache) {
    auto stage = Amino::newClassPtr<BifrostUsd::Stage>(
        getResourcePath("helloworld.usd").c_str());
    ASSERT_TRUE(*stage);
    Amino::long_t id = USD::Stage::send_stage_to_cache(stage);
    ASSERT_TRUE(id > 0);
    auto pxrStage = PXR_NS::UsdUtilsStageCache::Get().Find(
        PXR_NS::UsdStageCache::Id::FromLongInt(static_cast<int>(id)));
    ASSERT_TRUE(pxrStage);
    auto prim = pxrStage->GetPrimAtPath(PXR_NS::SdfPath("/hello/world"));
    ASSERT_TRUE(prim.IsValid());
}

TEST(StageNodeDefs, export_stage_to_string) {
    BifrostUsd::Stage stage{
        getResourcePath("layer_with_sub_layers.usda").c_str()};
    ASSERT_TRUE(stage);

    Amino::String result = "";
    USD::Stage::export_stage_to_string(stage, result);

    const char* flattenedFileContent = R"usda(#usda 1.0
(
    doc = """Generated from Composed Stage of root layer 
"""
)

def Xform "hello"
{
    def Sphere "world"
    {
        int testAttr = 123
    }
}

def Xform "hi"
{
    def Sphere "world"
    {
        double3 xformOp:translate = (1, 1, 1)
        uniform token[] xformOpOrder = ["xformOp:translate"]
    }
}

)usda";
    ASSERT_STREQ(result.c_str(), flattenedFileContent);
}

TEST(StageNodeDefs, export_stage_to_file) {
    BifrostUsd::Stage stage{
        getResourcePath("layer_with_sub_layers.usda").c_str()};
    ASSERT_TRUE(stage);

    auto filepath = getThisTestOutputPath("testExportStage.usda");
    bool success  = USD::Stage::export_stage_to_file(stage, filepath.c_str());
    ASSERT_TRUE(success);

    const char* flattenedFileContent = R"usda(#usda 1.0
(
    doc = """Generated from Composed Stage of root layer 
"""
)

def Xform "hello"
{
    def Sphere "world"
    {
        int testAttr = 123
    }
}

def Xform "hi"
{
    def Sphere "world"
    {
        double3 xformOp:translate = (1, 1, 1)
        uniform token[] xformOpOrder = ["xformOp:translate"]
    }
}

)usda";

    std::ifstream     filestream(filepath.c_str());
    std::stringstream filebuffer;
    filebuffer << filestream.rdbuf();
    ASSERT_STREQ(filebuffer.str().c_str(), flattenedFileContent);
}
