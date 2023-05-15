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
#include <gtest/gtest.h>
#include <pxr/base/arch/systemInfo.h>
#include <pxr/usd/ar/defaultResolver.h>
#include <utils/test/testUtils.h>
#include <BifrostUsd/Attribute.h>
#include <BifrostUsd/Layer.h>
#include <BifrostUsd/Stage.h>

using namespace BifrostUsd::TestUtils;

#include <cstdlib>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {
void testCopyAndMoveOps(const BifrostUsd::Layer& layer, bool editable) {
    // copy ctor & equality op
    // Note: a new Anonymous SdfLayer is created in the copy if source is
    //       editable, hence they are not equal anymore.
    BifrostUsd::Layer layerCopyCtor{layer};
    if (!editable) {
        // Note: Comparing the root layers is enough, as it compares the root
        //       and all sublayers recursively
        EXPECT_TRUE(layer == layerCopyCtor);
    }

    // assignment op & equality op
    // Note: a new Anonymous SdfLayer is created in the copy if source is
    //       editable, hence they are not equal anymore.
    BifrostUsd::Layer layerAssignOp;
    layerAssignOp.operator=(layer);
    if (!editable) {
        EXPECT_TRUE(layer == layerAssignOp);
    }

    // move ctor & equality op
    // Note: a new Anonymous SdfLayer is created in the copy if source is
    //       editable, hence they are not equal anymore.
    BifrostUsd::Layer layer2{layer};
    BifrostUsd::Layer layerMoveCtor{std::move(layer2)};
    if (!editable) {
        EXPECT_TRUE(layer == layerMoveCtor);
    }

    // move assignment op & equality op
    // Note: a new Anonymous SdfLayer is created in the copy if source is
    //       editable, hence they are not equal anymore.
    BifrostUsd::Layer layer3{layer};
    BifrostUsd::Layer layerMoveAssignOp;
    layerMoveAssignOp.operator=(std::move(layer3));
    if (!editable) {
        EXPECT_TRUE(layer == layerMoveAssignOp);
    }
}
}

TEST(BifrostUsdTests, Layer_ctors) {
    std::vector<std::string> sourceFilenames;
    std::vector<bool> editableArgs = {false, true};

    // ctor with SdfLayer: from a VALID pxr layer
    sourceFilenames = {"helloworld.usd", "layer_with_sub_layers.usda"};
    for (std::string filename : sourceFilenames) {
        for (bool editable : editableArgs) {
            Amino::String path = getResourcePath(filename.c_str());
            pxr::SdfLayerRefPtr pxrLayer = pxr::SdfLayer::FindOrOpen(
                path.c_str());
            EXPECT_NE(nullptr, pxrLayer);

            BifrostUsd::Layer layer1{pxrLayer, editable, path};
            EXPECT_TRUE(layer1); // VALID
            EXPECT_STREQ(layer1.getOriginalFilePath().c_str(),
                path.c_str());
            EXPECT_STREQ(layer1.getFilePath().c_str(),
                        editable ? path.c_str() : "");

            testCopyAndMoveOps(layer1, editable);
        }
    }

    // ctor with SdfLayer: from a NULLPTR pxr layer
    for (bool editable : editableArgs) {
        Amino::String path = getResourcePath("__inexistent_originalPath.usd");
        BifrostUsd::Layer layer1{nullptr, editable, path};
        EXPECT_FALSE(layer1); // INVALID (since SdfLayer provided is nullptr)
        EXPECT_STREQ(layer1.getOriginalFilePath().c_str(), path.c_str());
        EXPECT_STREQ(layer1.getFilePath().c_str(),
                     editable ? path.c_str() : "");

        testCopyAndMoveOps(layer1, editable);

        // Case with an empty supplied originalFilePath...
        BifrostUsd::Layer layer2{nullptr, editable, ""};
        EXPECT_FALSE(layer2); // INVALID (since SdfLayer provided is nullptr)
        EXPECT_STREQ(layer2.getOriginalFilePath().c_str(), "");
        EXPECT_STREQ(layer2.getFilePath().c_str(), "");

        testCopyAndMoveOps(layer2, editable);
    }

    // ctor with strings only: refer to an EXISTENT pxr layer
    sourceFilenames = {
        "helloworld.usd",
        "helloworld", // default ".usd" extension will be added by Layer ctor
        "layer_with_sub_layers.usda"};
    for (std::string filename : sourceFilenames) {
        for (bool editable : editableArgs) {
            Amino::String path = getResourcePath(filename.c_str());
            Amino::String savePath = getTestOutputPath(
                "Layer_ctors_saveFilePath.usd");
            BifrostUsd::Layer layer1{path/*originalPath*/, "my_tag", savePath,
                editable};
            EXPECT_TRUE(layer1); // VALID

            Amino::String pathWithExtension =
                BifrostUsd::Layer::getPathWithValidUsdFileFormat(path);
            EXPECT_STREQ(layer1.getOriginalFilePath().c_str(),
                pathWithExtension.c_str());
            EXPECT_STREQ(layer1.getFilePath().c_str(), savePath.c_str());

            testCopyAndMoveOps(layer1, editable);
        }
    }

    // ctor with strings only: refer to a NON-EXISTENT pxr layer
    for (bool editable : editableArgs) {
        Amino::String path = getResourcePath("__inexistent_originalPath.usd");
        Amino::String savePath = getTestOutputPath(
            "Layer_ctors_saveFilePath.usd");
        BifrostUsd::Layer layer1{path/*originalPath*/, "my_tag", savePath,
            editable};
        EXPECT_FALSE(layer1); // INVALID (since no file at provided path)
        EXPECT_STREQ(layer1.getOriginalFilePath().c_str(), path.c_str());
        EXPECT_STREQ(layer1.getFilePath().c_str(), savePath.c_str());
        testCopyAndMoveOps(layer1, editable);

        BifrostUsd::Layer layer2{"", "my_tag", "", editable};
        EXPECT_FALSE(layer2); // INVALID (since no file at provided path)
        EXPECT_STREQ(layer2.getOriginalFilePath().c_str(), "");
        EXPECT_STREQ(layer2.getFilePath().c_str(), "");
        testCopyAndMoveOps(layer2, editable);
    }
}

TEST(BifrostUsdTests, createLayer) {
    // new empty layer tests
    {
        auto layer = Amino::newMutablePtr<BifrostUsd::Layer>();
        ASSERT_EQ(std::string("bifrost.usd"), layer->get().GetDisplayName());

        layer = Amino::newMutablePtr<BifrostUsd::Layer>("a");
        ASSERT_EQ(std::string("a.usd"), layer->get().GetDisplayName());

        layer = Amino::newMutablePtr<BifrostUsd::Layer>("a.usd");
        ASSERT_EQ(std::string("a.usd"), layer->get().GetDisplayName());

        layer = Amino::newMutablePtr<BifrostUsd::Layer>("a.usda");
        ASSERT_EQ(std::string("a.usda"), layer->get().GetDisplayName());

        layer = Amino::newMutablePtr<BifrostUsd::Layer>("a.mod.usdc");
        ASSERT_EQ(std::string("a.mod.usdc"), layer->get().GetDisplayName());

        layer = Amino::newMutablePtr<BifrostUsd::Layer>("a.mod");
        ASSERT_EQ(std::string("a.mod.usd"), layer->get().GetDisplayName());

        layer = Amino::newMutablePtr<BifrostUsd::Layer>("a.abc");
        ASSERT_EQ(std::string("a.abc.usd"), layer->get().GetDisplayName());

        ASSERT_EQ("", layer->getFilePath());

        const auto expectedPath = getTestOutputPath("createLayer.usd");
        layer->setFilePath(expectedPath);
        ASSERT_EQ(expectedPath, layer->getFilePath());

        ASSERT_TRUE(layer->exportToFile());
        ASSERT_TRUE(Bifrost::FileUtils::filePathExists(expectedPath));
    }

    // new layer from file tests with one sublayer
    {
        auto expectedName = Amino::String("layer_with_sub_layers_v2.usda");
        auto savePath     = getTestOutputPath(expectedName);

        auto layer = Amino::newClassPtr<BifrostUsd::Layer>(
            getResourcePath("layer_with_sub_layers.usda"), "", savePath);

        ASSERT_TRUE(*layer);
        ASSERT_TRUE(layer->get().IsAnonymous());
        ASSERT_EQ(layer->get().GetDisplayName(), expectedName.c_str());
        ASSERT_TRUE(layer->exportToFile());
        ASSERT_TRUE(Bifrost::FileUtils::filePathExists(savePath));

        // check the sublayer
        auto sublayer = layer->getSubLayer(0);
        ASSERT_TRUE(sublayer);
        ASSERT_TRUE(sublayer->IsAnonymous());
        ASSERT_EQ(sublayer->GetDisplayName(), "helloworld.usd");
        ASSERT_TRUE(Bifrost::FileUtils::filePathExists(
            getTestOutputPath("helloworld.usd")));
    }

    // new layer from pxr layer
    {
        auto pxrLayer = pxr::SdfLayer::FindOrOpen(
            getResourcePath("helloworld.usd").c_str());
        auto layer = Amino::newClassPtr<BifrostUsd::Layer>(pxrLayer, false);
        ASSERT_TRUE(*layer);
        ASSERT_FALSE(layer->get().IsAnonymous());
    }

    // new layer from anonymous pxr layer
    {
        const std::vector<std::pair<std::string, std::string>> tests = {
            {"",            "bifrost.usd"},
            {"a",           "a.usd"},
            {"a.",          "a..usd"},
            {".abc",        ".abc.usd"},
            {".usd",        "bifrost.usd"},
            {".usda",       "bifrost.usda"},
            {".usdc",       "bifrost.usdc"},
            {"abc.usd",     "abc.usd"},
            {"abc.usda",    "abc.usda"},
            {"abc.usdc",    "abc.usdc"}
        };
        for(auto& test : tests) {
            auto pxrLayer = pxr::SdfLayer::CreateAnonymous(test.first);
            EXPECT_NE(pxrLayer, nullptr) << "Error creating anonymous SdfLayer with tag=`"
                << test.first << "`";
            if (pxrLayer) {
                // Create an editable Layer from it, which will create a new
                // anonymous layer copy from it with a valid tag (valid filename
                // and USD file extension):
                auto layer = Amino::newClassPtr<BifrostUsd::Layer>(pxrLayer, true);
                EXPECT_TRUE(*layer) << "Error creating Layer from anonymous with tag=`"
                    << test.first << "`";
                if (*layer) {
                    EXPECT_TRUE(layer->get().IsAnonymous());
                    EXPECT_STREQ(layer->get().GetDisplayName().c_str(), test.second.c_str())
                        << "input tag=`" << test.first
                        << "`; actual tag on copied SdfLayer=`" << layer->get().GetDisplayName()
                        << "`; expected tag on copied SdfLayer=`" << test.second << "`";
                }
            }
        }
    }

    // new layer from a read only pxr layer with a sublayer (to check that the
    // sublayer can't be edited if comming from a non-anonymous layer)
    {
        auto stage = pxr::UsdStage::Open(
            getResourcePath("layer_with_sub_layers.usda").c_str());

        auto layer = Amino::newClassPtr<BifrostUsd::Layer>(
            stage->GetRootLayer(), false);
        ASSERT_TRUE(*layer);
        ASSERT_FALSE(layer->get().IsAnonymous());

        // check if sublayers are editable
        auto layerStack = stage->GetLayerStack(false);
        for (size_t i = 1; i < layerStack.size(); ++i) {
            ASSERT_FALSE(layerStack[i]->PermissionToEdit());
        }
    }

    // Open read-only layer that has sublayers
    {
        // Test opening read only SdfLayer.
        auto pxr_layer = pxr::SdfLayer::FindOrOpen(
            getResourcePath("layer_with_sub_layers.usda").c_str());
        ASSERT_TRUE(pxr_layer);
        pxr_layer->SetPermissionToEdit(false);
        ASSERT_FALSE(pxr_layer->PermissionToEdit());
        auto layer = Amino::newClassPtr<BifrostUsd::Layer>(pxr_layer, true);
        ASSERT_TRUE(*layer);
        auto sublayer = layer->getSubLayer(0);
        ASSERT_TRUE(sublayer);
    }

    // Open layer that has sublayers
    {
        // Test opening read only SdfLayer.
        auto pxr_layer = pxr::SdfLayer::FindOrOpen(
            getResourcePath("layer_with_sub_layers.usda").c_str());
        ASSERT_TRUE(pxr_layer);
        ASSERT_TRUE(pxr_layer->PermissionToEdit());
        auto layer = Amino::newClassPtr<BifrostUsd::Layer>(pxr_layer);
        ASSERT_TRUE(*layer);
        auto sublayer = layer->getSubLayer(0);
        ASSERT_TRUE(sublayer);
    }

    // copy constructed from an other BifrostUsd::Layer
    {
        auto expectedSourceLayerExportPath =
            getTestOutputPath("sourceExportPath.usda");

        BifrostUsd::Layer source_layer{getResourcePath("helloworld.usd"), "",
                                         expectedSourceLayerExportPath.c_str()};

        ASSERT_TRUE(source_layer->PermissionToEdit());

        BifrostUsd::Layer new_layer{source_layer};
        ASSERT_TRUE(new_layer);
        ASSERT_TRUE(new_layer->IsAnonymous());

        // pxr layer display names should be the same between source and new
        // layers...
        ASSERT_EQ(new_layer->GetDisplayName(), source_layer->GetDisplayName());
        // ...but not the identifiers
        ASSERT_FALSE(new_layer->GetIdentifier() ==
                     source_layer->GetIdentifier());

        auto expectedNewLayerExportPath =
            getTestOutputPath("duplicateExportPath.usda");
        new_layer.setFilePath(expectedNewLayerExportPath);

        // modify source layer
        {
            auto stage = pxr::UsdStage::Open(source_layer.getLayerPtr());
            ASSERT_TRUE(stage);
            // Modify source layer
            auto prim = stage->GetPrimAtPath(pxr::SdfPath("/hello/world"));
            ASSERT_TRUE(prim);
            prim.GetAttribute(pxr::TfToken("testAttr")).Set(456);
            int value = 0;
            ASSERT_TRUE(
                prim.GetAttribute(pxr::TfToken("testAttr")).Get(&value));
            ASSERT_EQ(value, 456);
        }

        // check that copied layer is not modified
        {
            auto stage = pxr::UsdStage::Open(new_layer.getLayerPtr());
            ASSERT_TRUE(stage);
            auto prim = stage->GetPrimAtPath(pxr::SdfPath("/hello/world"));
            ASSERT_TRUE(prim);
            int value = 0;
            ASSERT_TRUE(
                prim.GetAttribute(pxr::TfToken("testAttr")).Get(&value));
            ASSERT_EQ(value, 123);
        }

        ASSERT_TRUE(source_layer.exportToFile());
        ASSERT_TRUE(
            Bifrost::FileUtils::filePathExists(expectedSourceLayerExportPath));

        ASSERT_TRUE(new_layer.exportToFile());
        ASSERT_TRUE(
            Bifrost::FileUtils::filePathExists(expectedNewLayerExportPath));
    }
}

TEST(BifrostUsdTests, createLayerWithSubLayer) {
    BifrostUsd::Layer layer{"a"};
    auto                aFilePath = getTestOutputPath("a.usd");
    layer.setFilePath(aFilePath);

    BifrostUsd::Layer sublayer{getResourcePath("helloworld.usd"), ""};

    ASSERT_EQ("helloworld.usd", sublayer->GetDisplayName());

    Amino::String const subLayerName{"b.usd"};
    auto bFilePath = getTestOutputPath(subLayerName);
    sublayer.setFilePath(bFilePath);
    ASSERT_TRUE(layer.insertSubLayer(sublayer));
    ASSERT_TRUE(layer.exportToFile());
    ASSERT_TRUE(Bifrost::FileUtils::filePathExists(aFilePath));
    ASSERT_TRUE(Bifrost::FileUtils::filePathExists(bFilePath));

    // Export and check sublayer is present.
    std::string exportString = layer.exportToString().c_str();
    EXPECT_NE(exportString.find(subLayerName.c_str()), std::string::npos)
        << "Exported layer:\n"
        << exportString << "\n does not contain expected sublayer `"
        << subLayerName.c_str() << "`\n";
}

TEST(BifrostUsdTests, getSubLayer) {
    // Open a root SdfLayer with some sub SdfLayers in it:
    const Amino::String rootName = "helloworld.usd";
    const Amino::Array<Amino::String> subNames = {
        "Grass1.usd", "Mushroom1.usd", "Tree1.usd"}; // weakest to strongest
    const int numSubNames = static_cast<int>(subNames.size());
    pxr::SdfLayerRefPtr sdfRootLayer = pxr::SdfLayer::FindOrOpen(rootName.c_str());
    ASSERT_NE(sdfRootLayer, nullptr);
    Amino::String errorMsg;
    addSubLayers(sdfRootLayer, subNames, errorMsg);
    ASSERT_STREQ("", errorMsg.c_str());
    checkSdfSublayerPaths(*sdfRootLayer, subNames, errorMsg);
    ASSERT_STREQ("", errorMsg.c_str());

    // Case 1: root layer has no sublayers
    {
        BifrostUsd::Layer layer{rootName};
        for(int i = -2; i <= 2; ++i) {
            const BifrostUsd::Layer& subLayer = layer.getSubLayer(i);
            EXPECT_FALSE(subLayer.isValid())
                << "\t Case 1: Root has no sublayers - We expect an INVALID layer at index "
                << i << "\n";
        }
    }

    // Case 2: root layer has some sublayers
    {
        BifrostUsd::Layer layer{sdfRootLayer};
        std::ostringstream stream;
        stream << ("\t Case 2: Root has ") << numSubNames << " sublayers - ";
        std::string title = stream.str();

        // Valid indices
        for(int i = 0; i < numSubNames; ++i) {
            const BifrostUsd::Layer& subLayer = layer.getSubLayer(i);
            EXPECT_TRUE(subLayer.isValid()) << title <<
                "We expect a VALID layer at index=" << i << "\n";
            if (subLayer.isValid()) {
                // getSubLayer() uses a strongest to weakest ordering,
                // but subNames has weakest first:
                int nameIndex = (numSubNames - 1) - i;

                // Make sure the subLayer's Id contains the subLayer's filename:
                const std::string layerId = subLayer.get().GetIdentifier();
                EXPECT_NE(layerId.find(subNames[nameIndex].c_str()), std::string::npos) <<
                    title << "SdfLayer at index=" << i << " has identifier=`" <<
                    layerId << "` but we expect it to contain string=`" <<
                    subNames[nameIndex].c_str() << "`\n";
            }
        }

        // Special case for weakest sublayer at index -1:
        {
            const BifrostUsd::Layer& subLayer = layer.getSubLayer(-1);
            EXPECT_TRUE(subLayer.isValid()) << title <<
                "We expect a VALID layer at index=-1\n";
            if (subLayer.isValid()) {
                // Make sure the subLayer's Id contains the subLayer's filename:
                const std::string layerId = subLayer.get().GetIdentifier();
                EXPECT_NE(layerId.find(subNames[0].c_str()), std::string::npos) <<
                    title << "SdfLayer at index=-1 has identifier=`" <<
                    layerId << "` but we expect it to contain string=`" <<
                    subNames[0].c_str() << "`\n";
            }
        }

        // Invalid indices
        auto indices = std::vector<int>{-3, -2, numSubNames, numSubNames + 1};
        for(auto it = indices.begin(); it != indices.end(); ++it) {
            const BifrostUsd::Layer& subLayer = layer.getSubLayer(*it);
            EXPECT_FALSE(subLayer.isValid()) << title <<
                "We expect an INVALID layer at index=" << (*it) << "\n";
        }
    }
}

TEST(BifrostUsdTests, insertSubLayer) {
    // Open a root SdfLayer with some sub SdfLayers in it:
    const Amino::String rootName = "helloworld.usd";
    const Amino::Array<Amino::String> subNames = {
        "Grass1.usd", "Mushroom1.usd", "Tree1.usd"}; // weakest to strongest
    const int numSubNames = static_cast<int>(subNames.size());
    pxr::SdfLayerRefPtr sdfRootLayer = pxr::SdfLayer::FindOrOpen(rootName.c_str());
    ASSERT_NE(sdfRootLayer, nullptr);
    Amino::String errorMsg;
    addSubLayers(sdfRootLayer, subNames, errorMsg);
    ASSERT_STREQ("", errorMsg.c_str());
    checkSdfSublayerPaths(*sdfRootLayer, subNames, errorMsg);
    ASSERT_STREQ("", errorMsg.c_str());

    // Create new Bifrost sublayers to be inserted:
    const Amino::Array<Amino::String> subNames_Insert = {"Tree2.usd"};
    Amino::Array<Amino::Ptr<BifrostUsd::Layer>> subLayers_Insert;
    createBifrostLayers(subNames_Insert, subLayers_Insert, errorMsg);
    ASSERT_STREQ("", errorMsg.c_str());

    // Case 1: root layer has no sublayers
    {
        std::string title("\t Case 1: Root has no sublayers - ");

        // Valid indices (-1 and 0)
        for(int i = -1; i <= 0; ++i) {
            BifrostUsd::Layer layer{rootName};
            EXPECT_TRUE(layer.isValid());
            if (layer.isValid()) {
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(), 0);
                EXPECT_TRUE(layer.insertSubLayer(*subLayers_Insert[0], i)) <<
                    title << "We expect insertSubLayer() to SUCCEED with index=" <<
                    i << "\n";
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(), 1) <<
                    title << "We expect GetNumSubLayerPaths() to return 1 " <<
                    "after inserting a layer at index=" << i << "\n";

                const BifrostUsd::Layer& subLayer = layer.getSubLayer(0);
                EXPECT_TRUE(subLayer.isValid()) << title <<
                    "We expect a VALID layer at index=0 " <<
                    "after inserting a layer at index=" << i << "\n";
                if (subLayer.isValid()) {
                    // Make sure the subLayer's Id contains the subLayer's filename:
                    const std::string layerId = subLayer.get().GetIdentifier();
                    EXPECT_NE(layerId.find(subNames_Insert[0].c_str()), std::string::npos) <<
                        title << "SdfLayer at index=0 has identifier=`" <<
                        layerId << "` but we expect it to contain string=`" <<
                        subNames_Insert[0].c_str() << "`\n";
                }
            }
        }

        // Invalid indices
        auto indices = std::vector<int>{-3, -2, 1, 2};
        for(auto it = indices.begin(); it != indices.end(); ++it) {
            BifrostUsd::Layer layer{rootName};
            EXPECT_TRUE(layer.isValid());
            if (layer.isValid()) {
                EXPECT_FALSE(layer.insertSubLayer(*subLayers_Insert[0], *it)) <<
                    title << "We expect insertSubLayer() to FAIL with INVALID index=" <<
                    (*it) << "\n";
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(), 0) <<
                    title << "We expect GetNumSubLayerPaths() to return 0 " <<
                    "after an attempt to insert a layer at INVALID index=" << (*it) << "\n";
            }
        }

        // Attempt to add an INVALID sublayer
        {
            BifrostUsd::Layer layer{rootName};
            EXPECT_TRUE(layer);
            BifrostUsd::Layer sublayer{BifrostUsd::Layer::Invalid{}};
            EXPECT_FALSE(sublayer);

            if (layer.isValid()) {
                for(int i = -1; i <= 0; ++i) {
                    EXPECT_FALSE(layer.insertSubLayer(sublayer, i)) <<
                        title << "We expect insertSubLayer() to FAIL with index=" <<
                        i << " when sublayer is INVALID\n";
                    EXPECT_EQ(layer.get().GetNumSubLayerPaths(), 0);
                    EXPECT_EQ(layer.getSubLayers().size(), 0);
                }
            }
        }
    }

    // Case 2: root layer has some sublayers
    {
        std::ostringstream stream;
        stream << ("\t Case 2: Root has ") << numSubNames << " sublayers - ";
        std::string title = stream.str();

        // Valid indices (0 to n)
        for(int i = 0; i <= numSubNames; ++i) {
            BifrostUsd::Layer layer{sdfRootLayer};
            EXPECT_TRUE(layer.isValid());
            if (layer.isValid()) {
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(),
                    static_cast<size_t>(numSubNames));
                EXPECT_TRUE(layer.insertSubLayer(*subLayers_Insert[0], i)) <<
                    title << "We expect insertSubLayer() to SUCCEED with index=" <<
                    i << "\n";
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(),
                    static_cast<size_t>(numSubNames + 1)) <<
                    title << "We expect GetNumSubLayerPaths() to return " <<
                    (numSubNames + 1) <<
                    " after inserting a layer at index=" << i << "\n";
            }
            const BifrostUsd::Layer& subLayer = layer.getSubLayer(i);
            EXPECT_TRUE(subLayer.isValid()) << title <<
                "We expect a VALID layer at index=" << i <<
                " after inserting one at this index\n";
            if (subLayer.isValid()) {
                // Make sure the subLayer's Id contains the subLayer's filename:
                const std::string layerId = subLayer.get().GetIdentifier();
                EXPECT_NE(layerId.find(subNames_Insert[0].c_str()), std::string::npos) <<
                    title << "SdfLayer at index=" << i << " has identifier=`" <<
                    layerId << "` but we expect it to contain string=`" <<
                    subNames_Insert[0].c_str() << "`\n";
            }
        }

        // Special case to insert as new weakest sublayer:
        {
            BifrostUsd::Layer layer{sdfRootLayer};
            EXPECT_TRUE(layer);
            if (layer.isValid()) {
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(),
                    static_cast<size_t>(numSubNames));
                EXPECT_TRUE(layer.insertSubLayer(*subLayers_Insert[0], -1)) <<
                    title << "We expect insertSubLayer() to SUCCEED with index=-1\n";
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(),
                    static_cast<size_t>(numSubNames + 1)) <<
                    title << "We expect GetNumSubLayerPaths() to return " <<
                    (numSubNames + 1) <<
                    " after inserting a layer at index=-1\n";
            }
            // Both indices {-1, n} indicate the new weakest sublayer:
            auto indices = std::vector<int>{-1, numSubNames};
            for(auto it = indices.begin(); it != indices.end(); ++it) {
                const BifrostUsd::Layer& subLayer = layer.getSubLayer(*it);
                EXPECT_TRUE(subLayer.isValid()) << title <<
                    "We expect to retrieve the new layer at index=" << (*it) <<
                    " after inserting it using index -1\n";
                if (subLayer.isValid()) {
                    // Make sure the subLayer's Id contains the subLayer's filename:
                    const std::string layerId = subLayer.get().GetIdentifier();
                    EXPECT_NE(layerId.find(subNames_Insert[0].c_str()), std::string::npos) <<
                        title << "SdfLayer at index=" << (*it) << " has identifier=`" <<
                        layerId << "` but we expect it to contain string=`" <<
                        subNames_Insert[0].c_str() << "`\n";
                }
            }
        }

        // Invalid indices {-3, -2, n+1, n+2}
        auto indices = std::vector<int>{-3, -2, numSubNames + 1, numSubNames + 2};
        for(auto it = indices.begin(); it != indices.end(); ++it) {
            BifrostUsd::Layer layer{sdfRootLayer};
            EXPECT_TRUE(layer);
            if (layer.isValid()) {
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(),
                    static_cast<size_t>(numSubNames));
                EXPECT_FALSE(layer.insertSubLayer(*subLayers_Insert[0], *it)) <<
                    title << "We expect insertSubLayer() to FAIL with INVALID index=" <<
                    (*it) << "\n";
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(), static_cast<size_t>(numSubNames)) <<
                    title << "We expect GetNumSubLayerPaths() to return " << numSubNames <<
                    " after an attempt to insert a layer at INVALID index=" << (*it) << "\n";
            }
        }

        // Attempt to add an INVALID sublayer
        {
            BifrostUsd::Layer layer{sdfRootLayer};
            EXPECT_TRUE(layer);
            BifrostUsd::Layer sublayer{BifrostUsd::Layer::Invalid{}};
            EXPECT_FALSE(sublayer);
            if (layer.isValid()) {
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(),
                    static_cast<size_t>(numSubNames));

                for(int i = -1; i <= 2; ++i) {
                    EXPECT_FALSE(layer.insertSubLayer(sublayer, i)) <<
                        title << "We expect insertSubLayer() to FAIL with index=" <<
                        i << " when sublayer is INVALID\n";
                    EXPECT_EQ(layer.get().GetNumSubLayerPaths(),
                        static_cast<size_t>(numSubNames));
                    EXPECT_EQ(layer.getSubLayers().size(),
                        static_cast<size_t>(numSubNames));
                }
            }
        }
    }

    // Case 3: root layer is READONLY
    {
        std::string title("\t Case 3: Root is READONLY - ");

        BifrostUsd::Layer layer{sdfRootLayer, false /*readonly*/};
        EXPECT_TRUE(layer);
        if (layer.isValid()) {
            EXPECT_EQ(layer.get().GetNumSubLayerPaths(),
                static_cast<size_t>(numSubNames));
            EXPECT_EQ(layer.getSubLayers().size(),
                static_cast<size_t>(numSubNames));

            // Valid indices (-1 to 2)
            for(int i = -1; i <= 2; ++i) {
                EXPECT_FALSE(layer.insertSubLayer(*subLayers_Insert[0], i)) <<
                    title << "We expect insertSubLayer() to FAIL with index=" <<
                    i << " when root layer is READONLY\n";
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(),
                    static_cast<size_t>(numSubNames));
                EXPECT_EQ(layer.getSubLayers().size(),
                    static_cast<size_t>(numSubNames));
            }
        }
    }

    // Case 4: root layer has an INVALID PATH
    {
        std::string title("\t Case 4: Root has an INVALID PATH - ");

        BifrostUsd::Layer layer{"invalidPath.usd", ""};
        EXPECT_FALSE(layer.isValid());
        EXPECT_EQ(layer.getSubLayers().size(), 0);

        // Valid indices (-2 to 2)
        for(int i = -2; i <= 2; ++i) {
            EXPECT_FALSE(layer.insertSubLayer(*subLayers_Insert[0], i)) <<
                title << "We expect insertSubLayer() to FAIL with index=" <<
                i << " when root layer has an INVALID PATH\n";
            EXPECT_EQ(layer.getSubLayers().size(), 0);
        }
    }
}

TEST(BifrostUsdTests, replaceSubLayer) {
    // Open a root SdfLayer with some sub SdfLayers in it:
    const Amino::String rootName = "helloworld.usd";
    const Amino::Array<Amino::String> subNames = {
        "Grass1.usd", "Mushroom1.usd", "Tree1.usd"}; // weakest to strongest
    const int numSubNames = static_cast<int>(subNames.size());
    pxr::SdfLayerRefPtr sdfRootLayer = pxr::SdfLayer::FindOrOpen(rootName.c_str());
    ASSERT_NE(sdfRootLayer, nullptr);
    Amino::String errorMsg;
    addSubLayers(sdfRootLayer, subNames, errorMsg);
    ASSERT_STREQ("", errorMsg.c_str());
    checkSdfSublayerPaths(*sdfRootLayer, subNames, errorMsg);
    ASSERT_STREQ("", errorMsg.c_str());

    // Create new Bifrost sublayers to replace an existing one:
    const Amino::Array<Amino::String> subNames_Replace = {"Tree2.usd"};
    Amino::Array<Amino::Ptr<BifrostUsd::Layer>> subLayers_Replace;
    createBifrostLayers(subNames_Replace, subLayers_Replace, errorMsg);
    ASSERT_STREQ("", errorMsg.c_str());

    // Case 1: root layer has no sublayers
    {
        std::string title("\t Case 1: Root has no sublayers - ");

        // Invalid indices (-2 to 2)
        for(int i = -2; i < 3; ++i) {
            BifrostUsd::Layer layer{rootName};
            EXPECT_TRUE(layer);
            if (layer.isValid()) {
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(), 0);
                EXPECT_FALSE(layer.replaceSubLayer(*subLayers_Replace[0], i)) <<
                    title << "We expect replaceSubLayer() to FAIL with INVALID index=" <<
                    i << "\n";
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(), 0) <<
                    title << "We expect GetNumSubLayerPaths() to return 0 " <<
                    "after an attempt to replace a layer at INVALID index=" << i << "\n";
            }
        }
    }

    // Case 2: root layer has some sublayers
    {
        std::ostringstream stream;
        stream << ("\t Case 2: Root has ") << numSubNames << " sublayers - ";
        std::string title = stream.str();

        // Valid indices (0 to n-1)
        for(int i = 0; i < numSubNames; ++i) {
            BifrostUsd::Layer layer{sdfRootLayer};
            EXPECT_TRUE(layer);
            if (layer.isValid()) {
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(),
                    static_cast<size_t>(numSubNames));
                EXPECT_TRUE(layer.replaceSubLayer(*subLayers_Replace[0], i)) <<
                    title << "We expect replaceSubLayer() to SUCCEED with index=" <<
                    i << "\n";
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(),
                    static_cast<size_t>(numSubNames)) <<
                    title << "We expect GetNumSubLayerPaths() to return " <<
                    numSubNames <<
                    " after replacing a layer at index=" << i << "\n";

                // replaceSubLayer() uses a strongest to weakest ordering,
                // but subNames has weakest first:
                int nameIndex = (numSubNames - 1) - i;

                // Check all resulting sublayer paths, making sure the replacing one
                // is where we expect it:
                Amino::Array<Amino::String> subNames_New = subNames;
                subNames_New[nameIndex] = subNames_Replace[0];
                checkSdfSublayerPaths(layer.get(), subNames_New, errorMsg);
                EXPECT_STREQ("", errorMsg.c_str());
            }
        }

        // Special case to replace the weakest sublayer at index -1:
        {
            BifrostUsd::Layer layer{sdfRootLayer};
            EXPECT_TRUE(layer);
            if (layer.isValid()) {
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(),
                    static_cast<size_t>(numSubNames));
                EXPECT_TRUE(layer.replaceSubLayer(*subLayers_Replace[0], -1)) <<
                    title << "We expect replaceSubLayer() to SUCCEED with index=-1\n";
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(),
                    static_cast<size_t>(numSubNames)) <<
                    title << "We expect GetNumSubLayerPaths() to return " <<
                    numSubNames <<
                    " after replacing a layer at index=-1\n";

                // Check all resulting sublayer paths, making sure the replacing one
                // is the weakest:
                Amino::Array<Amino::String> subNames_New = subNames;
                subNames_New[0] = subNames_Replace[0];
                checkSdfSublayerPaths(layer.get(), subNames_New, errorMsg);
                EXPECT_STREQ("", errorMsg.c_str());
            }
        }

        // Invalid indices {-3, -2, n, n+1}
        auto indices = std::vector<int>{-3, -2, numSubNames, numSubNames + 1};
        for(auto it = indices.begin(); it != indices.end(); ++it) {
            BifrostUsd::Layer layer{sdfRootLayer};
            EXPECT_TRUE(layer);
            if (layer.isValid()) {
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(),
                    static_cast<size_t>(numSubNames));
                EXPECT_FALSE(layer.replaceSubLayer(*subLayers_Replace[0], *it)) <<
                    title << "We expect replaceSubLayer() to FAIL with INVALID index=" <<
                    (*it) << "\n";
                EXPECT_EQ(layer.get().GetNumSubLayerPaths(),
                    static_cast<size_t>(numSubNames)) <<
                    title << "We expect GetNumSubLayerPaths() to return " << numSubNames <<
                    " after an attempt to replace a layer at INVALID index=" << (*it) << "\n";
                checkSdfSublayerPaths(layer.get(), subNames, errorMsg);
                EXPECT_STREQ("", errorMsg.c_str());
            }
        }
    }
}

TEST(BifrostUsdTests, getTagWithValidUsdFileFormat) {
    const Amino::Array<std::pair<Amino::String, Amino::String>> tests = {
        {"",                    "bifrost.usd"},
        {".",                   "bifrost.usd"},
        {"..",                  "bifrost.usd"},
        {"./dir/..",            "bifrost.usd"},
        {"./../../",            "bifrost.usd"},
        {".usd",                "bifrost.usd"},
        {".usda",               "bifrost.usda"},
        {".usdc",               "bifrost.usdc"},
        {"abc",                 "abc.usd"},
        {"abc.",                "abc..usd"},
        {".a.b.c.",             ".a.b.c..usd"},
        {"abc.usd",             "abc.usd"},
        {"/dir/abc.usd",        "abc.usd"},
        {"abc.usda",            "abc.usda"},
        {"../dir/abc.usda",     "abc.usda"},
        {"abc.usdc",            "abc.usdc"},
        {"../../dir/abc.usdc",  "abc.usdc"}
    };
    for(auto& test : tests) {
        Amino::String tagIn = Bifrost::FileUtils::makePreferred(test.first);
        Amino::String tagExpected = Bifrost::FileUtils::makePreferred(test.second);
        Amino::String tagOut = BifrostUsd::Layer::getTagWithValidUsdFileFormat(tagIn);
        EXPECT_STREQ(tagOut.c_str(), tagExpected.c_str()) <<
            "in=`" << tagIn.c_str() << "`; out=`" << tagOut.c_str() <<
            "`; expected=`" << tagExpected.c_str() << "`";
    }
}

TEST(BifrostUsdTests, getPathWithValidUsdFileFormat) {
    const Amino::Array<std::pair<Amino::String, Amino::String>> tests = {
        {"",                    "bifrost.usd"},
        {"/",                   "/bifrost.usd"},
        {".",                   "./bifrost.usd"},
        {"./",                  "./bifrost.usd"},
        {"./dir/",              "./dir/bifrost.usd"},
        {"..",                  "../bifrost.usd"},
        {"../",                 "../bifrost.usd"},
        {"./../..",             "./../../bifrost.usd"},
        {"./../../",            "./../../bifrost.usd"},
        {"../dir/",             "../dir/bifrost.usd"},
        {".usd",                "bifrost.usd"},
        {"dir1/dir2/.usd",      "dir1/dir2/bifrost.usd"},
        {".usda",               "bifrost.usda"},
        {".usdc",               "bifrost.usdc"},
        {"../.a.b.c./.usdc",    "../.a.b.c./bifrost.usdc"},
        {"abc",                 "abc.usd"},
        {"../../dir/.a.b.c.",   "../../dir/.a.b.c..usd"},
        {"abc.usd",             "abc.usd"},
        {"/dir/abc.usd",        "/dir/abc.usd"},
        {"abc.usda",            "abc.usda"},
        {"../dir/abc..usda",    "../dir/abc..usda"},
        {"abc.usdc",            "abc.usdc"},
        {"../../dir/.abc.usdc", "../../dir/.abc.usdc"}
    };
    for(auto& test : tests) {
        Amino::String pathIn = Bifrost::FileUtils::makePreferred(test.first);
        Amino::String pathExpected = Bifrost::FileUtils::makePreferred(test.second);
        Amino::String pathOut = BifrostUsd::Layer::getPathWithValidUsdFileFormat(pathIn);
        EXPECT_STREQ(pathOut.c_str(), pathExpected.c_str()) <<
            "in=`" << pathIn.c_str() << "`; out=`" << pathOut.c_str() <<
            "`; expected=`" << pathExpected.c_str() << "`";
    }
}
