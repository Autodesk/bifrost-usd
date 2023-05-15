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

#include <nodedefs/usd_pack/usd_layer_nodedefs.h>
#include <nodedefs/usd_pack/usd_prim_nodedefs.h>
#include <nodedefs/usd_pack/usd_stage_nodedefs.h>
#include <nodedefs/usd_pack/usd_utils.h>
#include <utils/test/testUtils.h>

#include <gtest/gtest.h>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace BifrostUsd::TestUtils;

TEST(LayerNodeDefs, get_root_layer) {
    auto stage = Amino::newClassPtr<BifrostUsd::Stage>(
        getResourcePath("helloworld.usd").c_str());
    ASSERT_TRUE(*stage);
    Amino::Ptr<BifrostUsd::Layer> layer =
        Amino::newClassPtr<BifrostUsd::Layer>();
    USD::Layer::get_root_layer(*stage, layer);
    ASSERT_TRUE(*layer);
    ASSERT_TRUE(&*stage->get().GetRootLayer() == &layer->get());

    stage = Amino::newClassPtr<BifrostUsd::Stage>("badPath");
    ASSERT_FALSE(*stage);
    layer = Amino::newClassPtr<BifrostUsd::Layer>("badPath", "");
    ASSERT_FALSE(*layer);
    USD::Layer::get_root_layer(*stage, layer);
    ASSERT_FALSE(*layer);
}

TEST(LayerNodeDefs, get_layer) {
    // Open a root SdfLayer with some sub SdfLayers in it:
    const Amino::String rootName = "helloworld.usd";
    const Amino::Array<Amino::String> subNames = {
        "Grass1.usd", "Mushroom1.usd", "Tree1.usd"}; // weakest to strongest
    const size_t numSubNames = subNames.size();
    pxr::SdfLayerRefPtr sdfRootLayer = pxr::SdfLayer::FindOrOpen(rootName.c_str());
    ASSERT_NE(sdfRootLayer, nullptr);
    Amino::String errorMsg;
    addSubLayers(sdfRootLayer, subNames, errorMsg);
    ASSERT_STREQ("", errorMsg.c_str());

    auto edits = std::vector<bool>{true,false};
    for(auto itEdit = edits.begin(); itEdit != edits.end(); ++itEdit) {
        // Initialize our Bifrost Layer and Stage types:
        Amino::Ptr<BifrostUsd::Layer> layer = Amino::newClassPtr<BifrostUsd::Layer>(
            sdfRootLayer, *itEdit /*editable*/);
        ASSERT_TRUE(layer);
        ASSERT_TRUE(layer->isValid());
        Amino::Ptr<BifrostUsd::Stage> stage = Amino::newClassPtr<BifrostUsd::Stage>(
            *layer);
        ASSERT_TRUE(stage);
        ASSERT_TRUE(stage->isValid());

        // Check the root layer's Id at index -1
        {
            Amino::Ptr<BifrostUsd::Layer> root;
            USD::Layer::get_layer(*stage, -1, root);
            ASSERT_TRUE(root);
            ASSERT_TRUE(root->isValid());

            Amino::String id;
            USD::Layer::get_layer_identifier(*root, id);
            ASSERT_LT(id.find(rootName), id.length());
        }

        // Check each existing sublayer using get_layer()
        int i = 0;
        for(auto itName = subNames.begin(); itName != subNames.end(); ++itName, ++i) {
            // get_layer() returns the sublayers from weakest to strongest,
            // same order as the filenames in vector subNames.
            Amino::Ptr<BifrostUsd::Layer> sublayer;
            USD::Layer::get_layer(*stage, i, sublayer);
            ASSERT_TRUE(sublayer);
            ASSERT_TRUE(sublayer->isValid());

            // Check that the sublayer's Id contains the expected filename
            Amino::String id;
            USD::Layer::get_layer_identifier(*sublayer, id);
            ASSERT_LT(id.find(*itName), id.length());
        }

        // Check invalid layer indices:
        Amino::Ptr<BifrostUsd::Layer> badLayer;
        USD::Layer::get_layer(*stage, -3, badLayer);
        ASSERT_TRUE(badLayer);
        ASSERT_FALSE(badLayer->isValid());

        USD::Layer::get_layer(*stage, -2, badLayer);
        ASSERT_TRUE(badLayer);
        ASSERT_FALSE(badLayer->isValid());

        USD::Layer::get_layer(*stage, static_cast<int>(numSubNames), badLayer);
        ASSERT_TRUE(badLayer);
        ASSERT_FALSE(badLayer->isValid());

        USD::Layer::get_layer(*stage, static_cast<int>(numSubNames+1), badLayer);
        ASSERT_TRUE(badLayer);
        ASSERT_FALSE(badLayer->isValid());
    }
}

TEST(LayerNodeDefs, create_layer) {
    const Amino::String file = getResourcePath("namedoesntmatter.usd").c_str();
    Amino::MutablePtr<BifrostUsd::Layer> layer;
    USD::Layer::create_layer(file, layer);
    ASSERT_TRUE(layer);
    ASSERT_TRUE(*layer);
}

TEST(LayerNodeDefs, open_layer) {
    {
        Amino::MutablePtr<BifrostUsd::Layer> layer;
        USD::Layer::open_layer(getResourcePath("helloworld.usd").c_str(), "",
                               /*read_only*/ false, layer);
        ASSERT_TRUE(layer);
        ASSERT_TRUE(*layer);
        ASSERT_TRUE(layer->get().GetPrimAtPath(pxr::SdfPath("hello")));

        layer.reset();
        USD::Layer::open_layer("", "", false, layer);
        ASSERT_TRUE(layer);
        ASSERT_TRUE(*layer);
    }
    {
        Amino::MutablePtr<BifrostUsd::Layer> layer;
        USD::Layer::open_layer(getResourcePath("helloworld.usd").c_str(), "",
                               /*read_only*/ true, layer);
        ASSERT_TRUE(layer);
        ASSERT_TRUE(*layer);
        ASSERT_TRUE(layer->get().GetPrimAtPath(pxr::SdfPath("hello")));
        ASSERT_FALSE(layer->get().PermissionToEdit());
    }
    {
        Amino::MutablePtr<BifrostUsd::Layer> layer;
        USD::Layer::open_layer("invalidlayer.usda", "",
                               /*read_only*/ true, layer);
        ASSERT_FALSE(layer->isValid());
    }
}

TEST(LayerNodeDefs, duplicate_layer) {
    Amino::MutablePtr<BifrostUsd::Layer> sourceLayer;
    auto                                   sourceSaveFilepath =
        getTestOutputPath("testDuplicateLayer_source_output.usda");
    USD::Layer::open_layer(getResourcePath("helloworld.usd").c_str(),
                           sourceSaveFilepath.c_str(),
                           /*read_only*/ false, sourceLayer);
    ASSERT_TRUE(sourceLayer);
    ASSERT_TRUE(*sourceLayer);

    Amino::MutablePtr<BifrostUsd::Layer> newLayer;
    auto saveFilepath = getTestOutputPath("testDuplicateLayer_output.usda");

    USD::Layer::duplicate_layer(*sourceLayer, saveFilepath.c_str(), newLayer);
    ASSERT_TRUE(newLayer);
    ASSERT_TRUE(*newLayer);
    ASSERT_TRUE(newLayer->getFilePath().c_str() == saveFilepath);
    ASSERT_TRUE(newLayer->exportToFile());
}

TEST(LayerNodeDefs, get_layer_file_path) {
    auto                filePath = getResourcePath("helloworld.usd");
    BifrostUsd::Layer layer{filePath.c_str(), ""};
    layer.setFilePath(filePath);
    Amino::String outFilePath = "";
    USD::Layer::get_layer_file_path(layer, outFilePath);
    ASSERT_TRUE(filePath == outFilePath.c_str());
}

TEST(LayerNodeDefs, get_layer_identifier) {
    auto layer = BifrostUsd::Layer("my_layer.usd");

    Amino::String identifier = "";
    USD::Layer::get_layer_identifier(layer, identifier);
    // Check that the Id contains at least the layer filename
    ASSERT_LT(identifier.find("my_layer.usd"), identifier.length());
}

TEST(LayerNodeDefs, export_layer_to_string) {
    const char* helloworldContent = R"usda(#usda 1.0

def Xform "hello"
{
    def Sphere "world"
    {
        int testAttr = 123
    }
}

)usda";
    auto        layer             = Amino::newClassPtr<BifrostUsd::Layer>(
        getResourcePath("helloworld.usd").c_str(), "", "", false);
    Amino::String result = "";
    USD::Layer::export_layer_to_string(*layer, false, result);
    ASSERT_STREQ(result.c_str(), helloworldContent);
    const char* layerWithSublayerContent = R"usda(#usda 1.0
(
    subLayers = [
        @helloworld.usd@
    ]
)

def Xform "hi"
{
    def Sphere "world"
    {
        double3 xformOp:translate = (1, 1, 1)
        uniform token[] xformOpOrder = ["xformOp:translate"]
    }
}

)usda";
    layer = Amino::newClassPtr<BifrostUsd::Layer>(
        getResourcePath("layer_with_sub_layers.usda").c_str(), "", "", false);
    result = "";
    USD::Layer::export_layer_to_string(*layer, true, result);
    ASSERT_STREQ(result.c_str(), layerWithSublayerContent);
}

TEST(LayerNodeDefs, export_layer_to_file) {
    const char* helloworldContent = R"usda(#usda 1.0

def Xform "hello"
{
    def Sphere "world"
    {
        int testAttr = 123
    }
}

)usda";
    auto        layer             = Amino::newClassPtr<BifrostUsd::Layer>(
        getResourcePath("helloworld.usd").c_str(), "", "", true);
    auto filepath = getTestOutputPath("testLayerExport.usda");
    bool success =
        USD::Layer::export_layer_to_file(*layer, filepath.c_str(), false);
    ASSERT_TRUE(success);
    std::ifstream     layerstream(filepath.c_str());
    std::stringstream layerbuffer;
    layerbuffer << layerstream.rdbuf();
    ASSERT_STREQ(layerbuffer.str().c_str(), helloworldContent);

    const char* layerWitHSublayerContent = R"usda(#usda 1.0
(
    subLayers = [
        @helloworld.usd@
    ]
)

def Xform "hi"
{
    def Sphere "world"
    {
        double3 xformOp:translate = (1, 1, 1)
        uniform token[] xformOpOrder = ["xformOp:translate"]
    }
}

)usda";
    layer = Amino::newClassPtr<BifrostUsd::Layer>(
        getResourcePath("layer_with_sub_layers.usda").c_str(), "", "", true);
    filepath = getTestOutputPath("testSublayerExport.usda");
    success  = USD::Layer::export_layer_to_file(*layer, filepath.c_str(), true);
    ASSERT_TRUE(success);
    std::ifstream     layerWitHSublayerStream(filepath.c_str());
    std::stringstream layerWitHSublayerBuffer;
    layerWitHSublayerBuffer << layerWitHSublayerStream.rdbuf();
    ASSERT_STREQ(layerWitHSublayerBuffer.str().c_str(), layerWitHSublayerContent);
}

TEST(LayerNodeDefs, get_sublayer_paths) {
    const Amino::String rootName = "helloworld.usd";
    const Amino::Array<Amino::String> subNames = {
        "Grass1.usd", "Grass2.usd", "Mushroom1.usd"}; // weakest to strongest
    const int numSubNames = static_cast<int>(subNames.size());

    // Case 1: root layer has no sublayers
    auto edits = std::vector<bool>{true,false};
    for(auto itEdit = edits.begin(); itEdit != edits.end(); ++itEdit) {
        // Open an SdfLayer with no sublayer in it:
        pxr::SdfLayerRefPtr sdfRootLayer = pxr::SdfLayer::FindOrOpen(rootName.c_str());
        EXPECT_NE(sdfRootLayer, nullptr);
        if(!sdfRootLayer)
            continue;

        // Create a Bifrost root layer and stage from the SdfLayer:
        BifrostUsd::Layer layer{sdfRootLayer, *itEdit /*editable?*/};
        EXPECT_TRUE(layer.isValid());
        if(!layer)
            continue;
        EXPECT_EQ(layer.getSubLayers().size(), 0);
        BifrostUsd::Stage stage{layer};
        EXPECT_TRUE(stage.isValid());
        if (!stage)
            continue;

        // Get the SublayerPaths
        Amino::MutablePtr<Amino::Array<Amino::String>> paths;
        USD::Layer::get_sublayer_paths(stage, paths);

        std::ostringstream stream;
        stream << "Case " << (itEdit == edits.begin() ? 1 : 2)
            << ": No sublayers & root layer is "
            << (*itEdit ? "EDITABLE" : "READONLY") << " - ";
        std::string title = stream.str();

        EXPECT_NE(paths, nullptr)
            << title << "get_sublayer_paths() must not return a nullptr\n";
        if(!paths)
            continue;
        EXPECT_EQ(paths->size(), 0)
            << title << "get_sublayer_paths() must return an empty array\n";
    }

    // Case 2: root layer has multiple sublayers
    for(auto itEdit = edits.begin(); itEdit != edits.end(); ++itEdit) {
        // Open an SdfLayer with some sublayers in it:
        pxr::SdfLayerRefPtr sdfRootLayer = pxr::SdfLayer::FindOrOpen(rootName.c_str());
        EXPECT_NE(sdfRootLayer, nullptr);
        if(!sdfRootLayer)
            continue;
        Amino::String errorMsg;
        bool success = addSubLayers(sdfRootLayer, subNames, errorMsg);
        EXPECT_STREQ("", errorMsg.c_str());
        if(!success)
            continue;
        success = checkSdfSublayerPaths(*sdfRootLayer, subNames, errorMsg);
        EXPECT_STREQ("", errorMsg.c_str());
        if(!success)
            continue;

        // Create a Bifrost root layer and stage from the SdfLayer:
        BifrostUsd::Layer layer{sdfRootLayer, *itEdit /*editable?*/};
        EXPECT_TRUE(layer.isValid());
        if(!layer)
            continue;
        EXPECT_EQ(layer.getSubLayers().size(), numSubNames);
        BifrostUsd::Stage stage{layer};
        EXPECT_TRUE(stage.isValid());
        if (!stage)
            continue;

        // Get the SublayerPaths
        Amino::MutablePtr<Amino::Array<Amino::String>> paths;
        USD::Layer::get_sublayer_paths(stage, paths);

        std::ostringstream stream;
        stream << "Case " << (itEdit == edits.begin() ? 3 : 4)
            << ": with " << numSubNames << " sublayers & root layer is "
            << (*itEdit ? "EDITABLE" : "READONLY") << " - ";
        std::string title = stream.str();

        EXPECT_NE(paths, nullptr)
            << title << "get_sublayer_paths() must not return a nullptr\n";
        if(!paths)
            continue;
        EXPECT_EQ(paths->size(), numSubNames)
            << title << "get_sublayer_paths() must return an array with "
            << numSubNames << " elements\n";

        // Check that each SublayerPath contains at least the sublayer
        // filename and that Paths are listed from WEAKEST to STRONGEST:
        for(int i = 0; i < numSubNames && i < static_cast<int>(paths->size()); ++i) {
            const Amino::String path = paths->at(i);
            auto pos = path.find(subNames[i]);
            EXPECT_LT(pos, path.length())
                << title << "get_sublayer_paths() returned a path at index=" << i
                << " with value=`" << path.c_str()
                << "` but we expect it to contain string=`"
                << subNames[i].c_str() << "`\n";
        }
    }
}

TEST(LayerNodeDefs, add_sublayer) {
    {
        std::string title = "Case 1: Root is EDITABLE - ";

        BifrostUsd::Layer layer{getResourcePath("helloworld.usd").c_str(), ""};
        EXPECT_TRUE(layer);
        BifrostUsd::Layer sublayer1{getResourcePath("Mushroom1.usd").c_str(), ""};
        EXPECT_TRUE(sublayer1);
        BifrostUsd::Layer sublayer2{getResourcePath("Tree1.usd").c_str(), ""};
        EXPECT_TRUE(sublayer2);

        // Add first sublayer
        EXPECT_EQ(layer.getSubLayers().size(), 0);
        USD::Layer::add_sublayer(sublayer1, layer);
        EXPECT_TRUE(layer)
            << title << "containing layer has been invalidated\n";
        EXPECT_EQ(layer.getSubLayers().size(), 1)
            << title << "sublayers count should increase\n";

        // Add 2nd sublayer, will be the STRONGEST
        USD::Layer::add_sublayer(sublayer2, layer);
        EXPECT_TRUE(layer)
            << title << "containing layer has been invalidated\n";
        EXPECT_EQ(layer.getSubLayers().size(), 2)
            << title << "sublayers count should increase\n";

        // Check sublayerPaths, from WEAKEST to STRONGEST
        const Amino::Array<Amino::String> subNames = {
            "Mushroom1.usd", "Tree1.usd"}; // weakest to strongest
        if (layer) {
            Amino::String errorMsg;
            checkSdfSublayerPaths(layer.get(), subNames, errorMsg);
            EXPECT_STREQ("", errorMsg.c_str());
        }
    }
    {
        std::string title = "Case 2: Root is READONLY - ";

        BifrostUsd::Layer layer{getResourcePath("helloworld.usd").c_str(),
            "", "", false /*readonly*/};
        EXPECT_TRUE(layer);
        BifrostUsd::Layer sublayer{getResourcePath("Mushroom1.usd").c_str(), ""};
        EXPECT_TRUE(sublayer);

        EXPECT_EQ(layer.getSubLayers().size(), 0);
        USD::Layer::add_sublayer(sublayer, layer);
        EXPECT_EQ(layer.getSubLayers().size(), 0)
            << title << "sublayers count should not change\n";
    }
    {
        std::string title = "Case 3: Root has an INVALID PATH - ";

        BifrostUsd::Layer layer{"invalidPath.usd", ""};
        EXPECT_FALSE(layer.isValid());
        EXPECT_EQ(layer.getSubLayers().size(), 0);

        BifrostUsd::Layer sublayer{getResourcePath("Mushroom1.usd").c_str(),
            ""};
        EXPECT_TRUE(sublayer);

        USD::Layer::add_sublayer(sublayer, layer);
        EXPECT_EQ(layer.getSubLayers().size(), 0)
            << title << "sublayers count should not change\n";
    }
    {
        std::string title = "Case 4: Attempt to add an INVALID sublayer - ";

        BifrostUsd::Layer layer{getResourcePath("helloworld.usd").c_str(), ""};
        EXPECT_TRUE(layer);
        EXPECT_EQ(layer.getSubLayers().size(), 0);

        BifrostUsd::Layer sublayer{BifrostUsd::Layer::Invalid{}};
        EXPECT_FALSE(sublayer);

        USD::Layer::add_sublayer(sublayer, layer);
        EXPECT_EQ(layer.getSubLayers().size(), 0)
            << title << "sublayers count should not change\n";
    }
}

TEST(LayerNodeDefs, add_sublayer_array) {
    // Open a root SdfLayer with some sub SdfLayers in it:
    const Amino::String rootName = "helloworld.usd";
    const Amino::Array<Amino::String> subNames_Start = {
        "Grass1.usd", "Grass2.usd", "Mushroom1.usd"}; // weakest to strongest
    const size_t numSubNames_Start = subNames_Start.size();
    pxr::SdfLayerRefPtr sdfRootLayer = pxr::SdfLayer::FindOrOpen(rootName.c_str());
    ASSERT_NE(sdfRootLayer, nullptr);
    Amino::String errorMsg;
    addSubLayers(sdfRootLayer, subNames_Start, errorMsg);
    ASSERT_STREQ("", errorMsg.c_str());
    checkSdfSublayerPaths(*sdfRootLayer, subNames_Start, errorMsg);
    ASSERT_STREQ("", errorMsg.c_str());

    // Create new Bifrost sublayers to be added,
    // listed from weakest to strongest:
    const Amino::Array<Amino::String> subNames_Add = {
        "Mushroom2.usd", "Tree1.usd", "Tree2.usd"};
    Amino::Array<Amino::Ptr<BifrostUsd::Layer>> subLayers_Add;
    createBifrostLayers(subNames_Add, subLayers_Add, errorMsg);
    ASSERT_STREQ("", errorMsg.c_str());

    {
        // Case 1: add three sublayers
        std::string title = "Case 1: Add three valid sublayers - ";

        // Create a Bifrost root layer from the SdfLayer:
        BifrostUsd::Layer layer{sdfRootLayer, true /*editable*/};
        EXPECT_TRUE(layer.isValid());
        if (layer.isValid()) {
            EXPECT_EQ(layer.getSubLayers().size(), numSubNames_Start);
            EXPECT_EQ(layer->GetSubLayerPaths().size(), numSubNames_Start);

            // Add the array of sublayers. These are expected to be appended to
            // the current sublayers, and these new sublayers become the STRONGEST
            USD::Layer::add_sublayer(subLayers_Add, layer);

            // Check the Paths of all sublayers
            Amino::Array<Amino::String> subNames_All(subNames_Start);
            subNames_All += subNames_Add;
            checkSdfSublayerPaths(layer.get(), subNames_All, errorMsg);
            EXPECT_STREQ("", errorMsg.c_str())
                << title << "unexpected sublayers count or identifiers\n";
        }
    }

    {
        // Case 2: add a mix of valid and invalid sublayers
        std::string title = "Case 2: Attempt to add valid and invalid sublayers - ";

        // Create a Bifrost root layer from the SdfLayer:
        BifrostUsd::Layer layer{sdfRootLayer, true /*editable*/};
        EXPECT_TRUE(layer.isValid());
        if (layer.isValid()) {
            EXPECT_EQ(layer.getSubLayers().size(), numSubNames_Start);
            EXPECT_EQ(layer->GetSubLayerPaths().size(), numSubNames_Start);

            // Insert nullptr and an invalid layer in the array.
            // These should be ignored by add_sublayer().
            auto subLayers_Mixed = subLayers_Add;
            subLayers_Mixed.insert(subLayers_Mixed.begin() + 1, nullptr);
            EXPECT_EQ(subLayers_Mixed.size(), subLayers_Add.size() + 1);
            subLayers_Mixed.insert(subLayers_Mixed.end() - 1,
                Amino::newClassPtr<BifrostUsd::Layer>(
                    BifrostUsd::Layer::Invalid{}));
            EXPECT_EQ(subLayers_Mixed.size(), subLayers_Add.size() + 2);

            // Add the array of sublayers. These are expected to be appended to
            // the current sublayers, and these new sublayers become the STRONGEST,
            // and invalid array entries should be ignored:
            USD::Layer::add_sublayer(subLayers_Mixed, layer);

            // Check the Paths of all sublayers
            Amino::Array<Amino::String> subNames_All(subNames_Start);
            subNames_All += subNames_Add;
            checkSdfSublayerPaths(layer.get(), subNames_All, errorMsg);
            EXPECT_STREQ("", errorMsg.c_str())
                << title << "unexpected sublayers count or identifiers\n";
        }
    }

    {
        // Case 3: try to add an empty array of sublayers
        std::string title = "Case 3: Attempt to add an empty array of sublayers - ";

        Amino::Array<Amino::Ptr<BifrostUsd::Layer>> noLayers;

        // Create a Bifrost root layer from the SdfLayer:
        BifrostUsd::Layer layer{sdfRootLayer, true /*editable*/};
        EXPECT_TRUE(layer.isValid());
        if (layer.isValid()) {
            EXPECT_EQ(layer.getSubLayers().size(), numSubNames_Start);
            EXPECT_EQ(layer->GetSubLayerPaths().size(), numSubNames_Start);

            // Add an empty array
            USD::Layer::add_sublayer(noLayers, layer);

            // Check the Paths of all sublayers
            checkSdfSublayerPaths(layer.get(), subNames_Start, errorMsg);
            EXPECT_STREQ("", errorMsg.c_str())
                << title << "sublayers count and identifiers should not change\n";
        }
    }

    {
        // Case 4: root layer is readonly
        std::string title = "Case 4: Root layer is READONLY - ";

        // Create a Bifrost root layer from the SdfLayer:
        BifrostUsd::Layer layer{sdfRootLayer, false /*readonly*/};
        EXPECT_TRUE(layer.isValid());
        if (layer.isValid()) {
            EXPECT_EQ(layer.getSubLayers().size(), numSubNames_Start);
            EXPECT_EQ(layer->GetSubLayerPaths().size(), numSubNames_Start);

            // Try to add the array of sublayers. None should be added
            USD::Layer::add_sublayer(subLayers_Add, layer);

            // Check the Paths of all sublayers
            checkSdfSublayerPaths(layer.get(), subNames_Start, errorMsg);
            EXPECT_STREQ("", errorMsg.c_str())
                << title << "sublayers count and identifiers should not change\n";
        }
    }
}

TEST(LayerNodeDefs, export_to_file_with_edit_layer) {
    std::vector<bool> relPathArgs = {false, true};
    std::string temp;
    for(bool useRelPath : relPathArgs) {
        // Create the sublayer
        temp = "export_to_file_with_edit_layer_SUBLAYER_";
        temp += (useRelPath ? "Rel" : "Abs");
        temp += "Path.usda";
        auto editTargetSubLayerPath = getTestOutputPath(temp.c_str());
        {
            BifrostUsd::Layer exportLayer;
            bool success = USD::Layer::export_layer_to_file(
                exportLayer, editTargetSubLayerPath.c_str(),
                false /*absolute path*/);
            ASSERT_TRUE(success);
        }
        BifrostUsd::Layer editTargetSublayer{editTargetSubLayerPath.c_str(), ""};
        editTargetSublayer.setFilePath(editTargetSubLayerPath.c_str());
        ASSERT_TRUE(editTargetSublayer);

        // Create the root layer and add the sublayer
        BifrostUsd::Layer rootLayer{getResourcePath("helloworld.usd").c_str(),
                                    ""};
        ASSERT_TRUE(rootLayer);
        USD::Layer::add_sublayer(editTargetSublayer, rootLayer);
        ASSERT_TRUE(rootLayer);

        // Create a stage and set the edit target
        BifrostUsd::Stage stage{rootLayer};
        ASSERT_TRUE(stage);
        USD::Stage::set_edit_layer(stage, 0);
        ASSERT_TRUE(stage);

        // Create a new prim on the edit target Layer
        auto primPath = pxr::SdfPath("/obj");
        auto newprim  = stage->DefinePrim(primPath);
        ASSERT_TRUE(newprim.IsValid());

        // Export the root layer and its sublayer
        temp = "export_to_file_with_edit_layer_ROOT_";
        temp += (useRelPath ? "Rel" : "Abs");
        temp += "Path.usda";
        bool success = USD::Layer::export_layer_to_file(
            *stage.getRootLayer(),
            getTestOutputPath(temp.c_str()).c_str(), useRelPath);
        EXPECT_TRUE(success);

        // Create a new stage by re-importing the sublayer and make sure
        // it contains the newly added primitive in it:
        auto sublayerStage =
            Amino::newClassPtr<BifrostUsd::Stage>(editTargetSubLayerPath.c_str());
        auto sublayerprim = sublayerStage->get().GetPrimAtPath(primPath);
        EXPECT_TRUE(sublayerprim.IsValid());
    }
}

TEST(LayerNodeDefs, export_layer_to_file_multicases) {
    // All different locations to be tested for the sublayer directory:
    const std::vector<std::string> sublayerDirs = {
        "", "./", "../", "../../", "../../dir1/", "../dir2/",
        "../newdir1/", "newdir2/", "./newdir3/"
    };
    // This is the common test directory where all exported files should be
    // written. Whether we test RELATIVE or ABSOLUTE options, the files
    // will all be saved below this directory level.
    // So we expect that RELATIVE paths to sublayers MUST NOT contain this
    // directory name, but ABSOLUTE paths to sublayers MUST contain it:
    const std::string commonTestDir{"export_layer_to_file_multicases"};

    for(bool relativeFilePath : std::vector<bool>{false, true}) {
        // Build either a relative or absolute path for the root layer's
        // directory to use in this test:
        const std::string absTestDir = Bifrost::FileUtils::makePreferred(
            getTestOutputDir()).c_str();
        std::string rootDir = Bifrost::FileUtils::filePath(
            relativeFilePath ? "." : absTestDir.c_str(),
            commonTestDir.c_str()).c_str();
        rootDir += relativeFilePath ? "/relFilePathCases/" : "/absFilePathCases/";
        rootDir += "dir1/dir2/"; // subdirs required so we can use ../.. in some tests

        size_t caseId = 0;
        for(std::string sublayerDir : sublayerDirs) {
            // When we use ABSOLUTE FilePaths in root and sublayer, then
            // we can export layers using either RELATIVE or ABSOLUTE paths.
            // When we use RELATIVE FilePaths in root and sublayer, then
            // we can only export layers using RELATIVE paths.
            std::vector<bool> exportRelativePathArgs{true};
            if (!relativeFilePath)
                exportRelativePathArgs.push_back(false);

            for(bool exportRelativePath : exportRelativePathArgs) {
                auto suffix = [caseId, relativeFilePath, exportRelativePath]() -> std::string {
                    std::string letter =
                        relativeFilePath ? "a" : (exportRelativePath ? "b" : "c");
                    return std::string("_Case") + std::to_string(caseId) + letter
                        + "_export" + (exportRelativePath ? "Rel" : "Abs");
                };

                const std::string rootFilename = std::string("ROOT")
                    + suffix() + ".usda";
                const std::string rootFilePath = Bifrost::FileUtils::filePath(
                    rootDir.c_str(), rootFilename.c_str()).c_str();

                const std::string subLayerFilename = std::string("SUBLAYER")
                    + suffix() + ".usda";
                const std::string subFilePath = Bifrost::FileUtils::filePath(
                    (rootDir + sublayerDir).c_str(),
                    subLayerFilename.c_str()).c_str();

                // Create root layer and a stage
                Amino::MutablePtr<BifrostUsd::Stage> ptrStage;
                {
                    // Create root layer
                    BifrostUsd::Layer rootLayer{rootFilename.c_str()};
                    rootLayer.setFilePath(rootFilePath.c_str());

                    // Create and add a sublayer
                    BifrostUsd::Layer sublayer{subLayerFilename.c_str()};
                    sublayer.setFilePath(subFilePath.c_str());

                    EXPECT_TRUE(rootLayer.insertSubLayer(sublayer))
                        << "insertSubLayer() failed for"
                        << subLayerFilename << "\n";

                    // Create a stage
                    ptrStage = Amino::newMutablePtr<BifrostUsd::Stage>(rootLayer);
                    ASSERT_TRUE(ptrStage && ptrStage->isValid());
                }

                // Set the edit layer target to the sublayer
                USD::Stage::set_edit_layer(*ptrStage, 0);
                EXPECT_TRUE(ptrStage->isValid());

                // Create a new prim on current edit layer target
                const Amino::String primPath("/a_space_capsule");
                const Amino::String primType{"Capsule"};
                USD::Prim::create_prim(*ptrStage, primPath, primType);
                {
                    auto ptrStageConst = ptrStage.toImmutable();
                    Amino::MutablePtr<BifrostUsd::Prim> ptrPrim;
                    EXPECT_TRUE(USD::Prim::get_prim_at_path(ptrStageConst,
                        primPath, ptrPrim));
                    EXPECT_TRUE(ptrPrim && *ptrPrim);
                    ptrStage = ptrStageConst.toMutable();
                }

                // Export root and sublayer to files and make sure layer files
                // are present on disk:
                {
                    Amino::Ptr<BifrostUsd::Layer> ptrRootLayer;
                    USD::Layer::get_root_layer(*ptrStage, ptrRootLayer);
                    ASSERT_TRUE(ptrRootLayer && ptrRootLayer->isValid());
                    bool success = USD::Layer::export_layer_to_file(
                        *ptrRootLayer, "", exportRelativePath);
                    EXPECT_TRUE(success)
                        << "export_layer_to_file failed for root layer "
                        << rootFilename << "\n";
                    EXPECT_TRUE(Bifrost::FileUtils::filePathExists(rootFilePath.c_str()))
                        << "filePathExists() returned false for root layer at path:\n"
                        << rootFilePath << "\n";
                    EXPECT_TRUE(Bifrost::FileUtils::filePathExists(subFilePath.c_str()))
                        << "filePathExists() returned false for subLayer at path:\n"
                        << subFilePath << "\n";
                    if (success) {
                        // Read file content of root layer and check that sublayer ids
                        // that it contains are either relative or absolute as expected:
                        std::ifstream     layerStream(rootFilePath.c_str());
                        std::stringstream layerBuffer;
                        layerBuffer << layerStream.rdbuf();
                        const std::string layerContent = layerBuffer.str();
                        bool foundDir = (layerContent.find(commonTestDir) < layerContent.length());
                        if (exportRelativePath) {
                            EXPECT_FALSE(foundDir)
                                << "Exported root layer at:\n"
                                << rootFilePath
                                << "\n contains ABSOLUTE path(s) to sublayer(s)\n";
                        } else {
                            EXPECT_TRUE(foundDir)
                                << "Exported root layer at:\n"
                                << rootFilePath
                                << "\n does not contain ABSOLUTE path(s) to sublayer(s)\n";
                        }
                    }
                    if (success) {
                        // Read file content of sublayer and check that it
                        // defines the expected primitive:
                        std::ifstream     layerStream(subFilePath.c_str());
                        std::stringstream layerBuffer;
                        layerBuffer << layerStream.rdbuf();
                        const std::string layerContent = layerBuffer.str();
                        bool foundPrim = (layerContent.find(primType.c_str()) < layerContent.length());
                        EXPECT_TRUE(foundPrim)
                            << "Exported sublayer at:\n"
                            << subFilePath
                            << "\n does not define a primitive of type '"
                            << primType.c_str() << "'\n";
                    }
                }

                // Open the exported root layer from disk
                {
                    Amino::MutablePtr<BifrostUsd::Layer> ptrRootLayer;
                    USD::Layer::open_layer(rootFilePath.c_str(), "",
                        true/*read_only*/, ptrRootLayer);
                    EXPECT_TRUE(ptrRootLayer && ptrRootLayer->isValid());
                    if (ptrRootLayer && ptrRootLayer->isValid()) {
                        const Amino::Array<BifrostUsd::Layer>& expSublayers =
                            ptrRootLayer->getSubLayers();
                        EXPECT_GT(expSublayers.size(), 0)
                            << "Exported root layer at:\n"
                            << rootFilePath
                            << "\n has no sublayers\n";
                        if (expSublayers.size() > 0) {
                            const BifrostUsd::Layer& expSublayer = expSublayers.at(0);
                            EXPECT_TRUE(expSublayer.isValid());
                            if (expSublayer.isValid()) {
                                Amino::String id;
                                USD::Layer::get_layer_identifier(expSublayer, id);
                                EXPECT_LT(id.find(subLayerFilename.c_str()), id.length())
                                    << "Exported root layer at:\n"
                                    << rootFilePath
                                    << "\n does not contain expected sublayer `"
                                    << subLayerFilename << "`\n";
                            }
                        }
                    }
                }

                // Create a new stage by re-importing the sublayer and make
                // sure it contains the expected primitive:
                {
                    Amino::Ptr<BifrostUsd::Stage> ptrStage2 =
                        Amino::newClassPtr<BifrostUsd::Stage>(subFilePath.c_str());
                    EXPECT_TRUE(ptrStage2 && ptrStage2->isValid())
                        << "Unable to create a new stage from exported sublayer at:\n"
                        << subFilePath
                        << "\n";
                    if (ptrStage2 && ptrStage2->isValid()) {
                        Amino::MutablePtr<BifrostUsd::Prim> ptrPrim;
                        EXPECT_TRUE(USD::Prim::get_prim_at_path(ptrStage2, primPath, ptrPrim));
                        EXPECT_TRUE(ptrPrim && *ptrPrim)
                            << "New stage from exported sublayer at:\n"
                            << rootFilePath
                            << "\n does not contain expected primitive at path `"
                            << primPath.c_str() << "`\n";
                    }
                }
            }
            caseId++;
        }
    }
}

TEST(LayerNodeDefs, replace_layer) {
    // Open a root SdfLayer with some sub SdfLayers in it:
    const Amino::String rootName = "helloworld.usd";
    const Amino::Array<Amino::String> subNames = {
        "Grass1.usd", "Grass2.usd", "Mushroom1.usd"}; // weakest to strongest
    const int numSubNames = static_cast<int>(subNames.size());
    pxr::SdfLayerRefPtr sdfRootLayer = pxr::SdfLayer::FindOrOpen(rootName.c_str());
    ASSERT_NE(sdfRootLayer, nullptr);
    Amino::String errorMsg;
    addSubLayers(sdfRootLayer, subNames, errorMsg);
    ASSERT_STREQ("", errorMsg.c_str());
    checkSdfSublayerPaths(*sdfRootLayer, subNames, errorMsg);
    ASSERT_STREQ("", errorMsg.c_str());

    // Create new layer to replace an existing one
    const Amino::String newLayerName = "Tree1.usd";
    BifrostUsd::Layer newLayer{getResourcePath(newLayerName).c_str(), ""};
    ASSERT_TRUE(newLayer.isValid());

    // Case 1: replace any sublayer, from 0 to N-1, 0 being the WEAKEST
    //         and N-1 being the STRONGEST in Pixar sublayers
    {
        std::string title = "Case 1: Root is EDITABLE - ";

        for (int i = 0; i < numSubNames; ++i) {
            for (int editIndex = -1; editIndex < numSubNames; ++editIndex) {
                // Create a Bifrost root layer and stage from the SdfLayer:
                BifrostUsd::Layer layer{sdfRootLayer, true /*editable*/};
                EXPECT_TRUE(layer.isValid());
                EXPECT_EQ(layer.getSubLayers().size(), numSubNames);
                BifrostUsd::Stage stage{layer};
                EXPECT_TRUE(stage.isValid());
                if (layer && stage) {
                    // USD nodedefs use a WEAKEST to STRONGEST index ordering,
                    // while BifrostUsd classes use the opposite ordering (like
                    // Pixar layer stack):
                    int reversedEditIndex =
                        (editIndex == -1 ? -1
                                         : USDUtils::reversedSublayerIndex(
                                               editIndex, numSubNames));

                    // Set the EditLayerIndex to a given layer (root or
                    // sublayer), possibly the sublayer that will be replaced:
                    USD::Stage::set_edit_layer(stage, editIndex);
                    EXPECT_EQ(stage.getEditLayerIndex(), reversedEditIndex);

                    // Replace the sublayer
                    EXPECT_TRUE(USD::Layer::replace_layer(stage, i, newLayer))
                        << title << "unexpected FAILURE when replacing layer `"
                        << subNames[i].c_str() << "` by `"
                        << newLayerName.c_str() << "` at array index=" << i
                        << "\n";

                    // Check that actual stage's EditTarget is the right one:
                    EXPECT_EQ(stage.getEditLayerIndex(), reversedEditIndex);
                    const pxr::SdfLayerHandle& sdfEditLayer =
                        stage.get().GetEditTarget().GetLayer();
                    EXPECT_TRUE(sdfEditLayer)
                        << title
                        << "UsdStage's EditTarget is INVALID after replacing "
                           "layer at index="
                        << i << " while current EditTargetIndex=" << editIndex
                        << "\n";
                    if (sdfEditLayer) {
                        if (i == editIndex) {
                            EXPECT_NE(sdfEditLayer->GetIdentifier().find(
                                          newLayerName.c_str()),
                                      std::string::npos)
                                << title << "replacing layer at index=" << i
                                << " that is the current EditTargetIndex did "
                                   "not update the actual stage's EditTarget "
                                   "(expected EditTarget layer's name=`"
                                << newLayerName.c_str()
                                << "`; actual layer's ID=`"
                                << sdfEditLayer->GetIdentifier() << "`)\n";
                        } else {
                            std::string editLayerName =
                                (editIndex == -1 ? rootName.c_str()
                                                 : subNames[editIndex].c_str());
                            EXPECT_NE(sdfEditLayer->GetIdentifier().find(
                                          editLayerName),
                                      std::string::npos)
                                << title << "replacing layer at index=" << i
                                << " while current EditTargetIndex="
                                << editIndex
                                << " has changed the actual stage's EditTarget "
                                   "(expected EditTarget layer's name=`"
                                << editLayerName << "`; actual layer's ID=`"
                                << sdfEditLayer->GetIdentifier() << "`)\n";
                        }
                    }

                    // Check the Paths of all sublayers
                    Amino::Array<Amino::String> subNames_New(subNames);
                    subNames_New[i] = newLayerName;
                    checkSdfSublayerPaths(stage.getRootLayer()->get(),
                                          subNames_New, errorMsg);
                    EXPECT_STREQ("", errorMsg.c_str())
                        << title
                        << "unexpected sublayers count or identifiers after "
                           "replacing layer `"
                        << subNames[i].c_str() << "` at array index=" << i
                        << "\n";
                }
            }
        }
    }

    // Case 2: root layer is READONLY
    {
        std::string title = "Case 2: Root is READONLY - ";

        // Create a Bifrost root layer and stage from the SdfLayer:
        BifrostUsd::Layer layer{sdfRootLayer, false /*readonly*/};
        EXPECT_TRUE(layer.isValid());
        EXPECT_EQ(layer.getSubLayers().size(), numSubNames);
        BifrostUsd::Stage stage{layer};
        EXPECT_TRUE(stage.isValid());

        if (layer && stage) {
            for(int i = 0; i < numSubNames; ++i) {
                for (int editIndex = -1; editIndex < numSubNames; ++editIndex) {
                    // USD nodedefs use a WEAKEST to STRONGEST index ordering,
                    // while BifrostUsd classes use the opposite ordering (like
                    // Pixar layer stack):
                    int reversedEditIndex =
                        (editIndex == -1 ? -1
                                         : USDUtils::reversedSublayerIndex(
                                               editIndex, numSubNames));

                    // Set the EditLayerIndex to a given layer (root or
                    // sublayer), possibly the sublayer we will attempt to
                    // replace:
                    USD::Stage::set_edit_layer(stage, editIndex);
                    EXPECT_EQ(stage.getEditLayerIndex(), reversedEditIndex);

                    // Try to replace a sublayer, it should not be replaced
                    EXPECT_FALSE(USD::Layer::replace_layer(stage, i, newLayer))
                        << title
                        << "unexpected SUCCESS when attempting to replace "
                           "layer `"
                        << subNames[i].c_str() << "` at array index=" << i
                        << "\n";

                    // Check that actual stage's EditTarget is unchanged:
                    EXPECT_EQ(stage.getEditLayerIndex(), reversedEditIndex);
                    const pxr::SdfLayerHandle& sdfEditLayer =
                        stage.get().GetEditTarget().GetLayer();
                    EXPECT_TRUE(sdfEditLayer)
                        << title
                        << "UsdStage's EditTarget is INVALID after attempting "
                           "to replace layer at index="
                        << i << " while current EditTargetIndex=" << editIndex
                        << "\n";
                    if (sdfEditLayer) {
                        std::string editLayerName =
                            (editIndex == -1 ? rootName.c_str()
                                             : subNames[editIndex].c_str());
                        EXPECT_NE(
                            sdfEditLayer->GetIdentifier().find(editLayerName),
                            std::string::npos)
                            << title
                            << "attempting to replace layer at index=" << i
                            << " while current EditTargetIndex=" << editIndex
                            << " has changed the actual stage's EditTarget "
                               "(expected EditTarget layer's name=`"
                            << editLayerName << "`; actual layer's ID=`"
                            << sdfEditLayer->GetIdentifier() << "`)\n";
                    }

                    // Check that Paths of all sublayers are not changed
                    checkSdfSublayerPaths(stage.getRootLayer()->get(), subNames,
                                          errorMsg);
                    EXPECT_STREQ("", errorMsg.c_str())
                        << title
                        << "unexpected sublayers count or identifiers after "
                           "attempt to replace layer `"
                        << subNames[i].c_str() << "` at array index=" << i
                        << "\n";
                }
            }
        }
    }

    // Case 3: INVALID sublayer indices
    {
        std::string title = "Case 3: INVALID indices - ";

        // Create a Bifrost root layer and stage from the SdfLayer:
        BifrostUsd::Layer layer{sdfRootLayer, true /*editable*/};
        EXPECT_TRUE(layer.isValid());
        EXPECT_EQ(layer.getSubLayers().size(), numSubNames);
        BifrostUsd::Stage stage{layer};
        EXPECT_TRUE(stage.isValid());

        if (layer && stage) {
            auto indices = std::vector<int>{-2, -1, numSubNames, numSubNames + 1};
            for(auto it = indices.begin(); it != indices.end(); ++it) {
                EXPECT_FALSE(USD::Layer::replace_layer(stage, (*it), newLayer))
                    << title << "unexpected SUCCESS when attempting to replace a layer "
                    << "at INVALID array index=" << (*it) << "\n";

                // Check that Paths of all sublayers are not changed
                checkSdfSublayerPaths(stage.getRootLayer()->get(), subNames, errorMsg);
                EXPECT_STREQ("", errorMsg.c_str())
                    << title
                    << "unexpected sublayers count or identifiers after attempt to replace a layer "
                    << "at INVALID array index=" << (*it) << "\n";
            }
        }
    }
}
