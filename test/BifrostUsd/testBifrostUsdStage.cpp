//-
// Copyright 2022 Autodesk, Inc.
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

#include <Amino/Core/Ptr.h>
#include <Amino/Core/String.h>
#include <Bifrost/FileUtils/FileUtils.h>

#include <BifrostUsd/Layer.h>
#include <BifrostUsd/Stage.h>
#include <utils/test/testUtils.h>

#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace BifrostUsd::TestUtils;

namespace {
Amino::String getThisTestOutputDir() {
    return Bifrost::FileUtils::filePath(getTestOutputDir(),
                                        "testBifrostUsdStage");
}
// Amino::String getThisTestOutputPath(const Amino::String& filename) {
//     return Bifrost::FileUtils::filePath(getThisTestOutputDir(), filename);
// }

// Helper comparison function between two stages.
// We compare only a subset of Stage's data, the data that should be equal to
// a source Stage when it is copied.
void reasonablyEqual(const BifrostUsd::Stage& lhs,
                     const BifrostUsd::Stage& rhs) {
    EXPECT_EQ(lhs.isValid(), rhs.isValid());
    if (lhs.isValid()) {
        // A valid stage always has a valid root layer
        ASSERT_NE(nullptr, lhs.getRootLayer());
        ASSERT_TRUE(lhs.getRootLayer()->isValid());
        ASSERT_NE(nullptr, rhs.getRootLayer());
        ASSERT_TRUE(rhs.getRootLayer()->isValid());

        // equality of cached EditLayerIndex in BifrostUsd::Layer objects
        EXPECT_EQ(lhs.getEditLayerIndex(), rhs.getEditLayerIndex());

        bool editLhs = lhs.getRootLayer()->get().PermissionToEdit();
        bool editRhs = rhs.getRootLayer()->get().PermissionToEdit();
        EXPECT_EQ(editLhs, editRhs);
        if (editLhs) {
            // When we copy a Stage with an editable root layer,
            // a new Anonymous root layer is created for that Stage.
            // But the EditLayerIndex must still refer to the right
            // layer within the sublayers:
            int index = lhs.getEditLayerIndex();
            if (index == -1) {
                EXPECT_EQ(lhs.getRootLayer()->                 operator->(),
                          lhs.get().GetEditTarget().GetLayer().operator->());
            } else {
                EXPECT_EQ(lhs.getRootLayer()->getSubLayer(index).operator->(),
                          lhs.get().GetEditTarget().GetLayer().  operator->());
            }
            index = rhs.getEditLayerIndex();
            if (index == -1) {
                EXPECT_EQ(rhs.getRootLayer()->                 operator->(),
                          rhs.get().GetEditTarget().GetLayer().operator->());
            } else {
                EXPECT_EQ(rhs.getRootLayer()->getSubLayer(index).operator->(),
                          rhs.get().GetEditTarget().GetLayer().  operator->());
            }
        } else {
            // equality of BifrostUsd root Layer objects
            EXPECT_TRUE(*lhs.getRootLayer() == *rhs.getRootLayer());
            // equality of UsdStage's root layer
            EXPECT_TRUE(lhs.get().GetRootLayer() == rhs.get().GetRootLayer());
            // equality of layer referred by UsdStage's EditTarget
            EXPECT_TRUE(lhs.get().GetEditTarget() == rhs.get().GetEditTarget());
        }

        // all other strings
        EXPECT_STREQ(lhs.last_modified_prim.c_str(),
                     rhs.last_modified_prim.c_str());
        EXPECT_STREQ(lhs.last_modified_variant_set_prim.c_str(),
                     rhs.last_modified_variant_set_prim.c_str());
        EXPECT_STREQ(lhs.last_modified_variant_set_name.c_str(),
                     rhs.last_modified_variant_set_name.c_str());
        EXPECT_STREQ(lhs.last_modified_variant_name.c_str(),
                     rhs.last_modified_variant_name.c_str());
    }
}

void testCopyAndMoveOps(const BifrostUsd::Stage& stage) {
    // copy ctor & equality op
    // Note: a new Anonymous root layer is created in the copy if root layer
    //       in source stage is editable, hence they are not equal anymore.
    BifrostUsd::Stage stageCopyCtor{stage}; // NOLINT(performance-unnecessary-copy-initialization)
    reasonablyEqual(stage, stageCopyCtor);

    // assignment op & equality op
    // Note: a new Anonymous root layer is created in the copy if root layer
    //       in source stage is editable, hence they are not equal anymore.
    BifrostUsd::Stage stageAssignOp;
    stageAssignOp.    operator=(stage);
    reasonablyEqual(stage, stageAssignOp);

    // move ctor & equality op
    // Note: a new Anonymous root layer is created in the copy if root layer
    //       in source stage is editable, hence they are not equal anymore.
    BifrostUsd::Stage stage2{stage};
    BifrostUsd::Stage stageMoveCtor{std::move(stage2)};
    reasonablyEqual(stage, stageMoveCtor);

    // move assignment op & equality op
    // Note: a new Anonymous root layer is created in the copy if root layer
    //       in source stage is editable, hence they are not equal anymore.
    BifrostUsd::Stage stage3{stage};
    BifrostUsd::Stage stageMoveAssignOp;
    stageMoveAssignOp.operator=(std::move(stage3));
    reasonablyEqual(stage, stageMoveAssignOp);
}
} // namespace

TEST(BifrostUsdTests, initial_cleanup) {
    ASSERT_TRUE(Bifrost::FileUtils::removeAll(getThisTestOutputDir()));
}

TEST(BifrostUsdTests, Stage_ctors) {
    const Amino::String               rootName = "helloworld.usd";
    const Amino::Array<Amino::String> subNames = {"Grass1.usd", "Grass2.usd",
                                                  "Mushroom1.usd"};
    const int         numSubNames  = static_cast<int>(subNames.size());
    std::vector<bool> editableArgs = {false, true};
    std::vector<BifrostUsd::InitialLoadSet> loadSetArgs = {
        BifrostUsd::InitialLoadSet::LoadAll,
        BifrostUsd::InitialLoadSet::LoadNone};
    std::vector<PXR_NS::SdfPath> maskPaths = {PXR_NS::SdfPath("/grp1"),
                                           PXR_NS::SdfPath("/grp2")};

    // default ctor
    {
        BifrostUsd::Stage stage{};
        EXPECT_TRUE(stage); // VALID

        testCopyAndMoveOps(stage);
    }

#if 0
    // TODO: BIFROST-8047 - Investigate why UsdStage::HasLocalLayer returns
    //     false on the sublayer during SetEditTarget when
    //     root layer is non-editable !!!
    // Reported error by Pixar USD is:
    //      "Coding Error: in SetEditTarget at line 3551 of
    //      S:\jenkins\workspace\ecg-usd-full-minimal-windows\ecg-usd-build\usd\pxr\usd\usd\stage.cpp
    //      -- Layer @helloworld.usd@ is not in the local
    //      LayerStack rooted at
    //      @d:/repos/bmp2/gitmodules/extra/ecg-bifrost-scatter-pack/gitmodules/bifrost-usd-pack/test/resources/layer_with_sub_layers.usda@"
    std::vector<std::string> sourceFilenames = {"layer_with_sub_layers.usda"};
    for (std::string filename : sourceFilenames) {
        for (bool editable : editableArgs) {
            Amino::String path     = getResourcePath(filename.c_str());
            Amino::String savePath = getThisTestOutputPath("saved_filename.usda");
            BifrostUsd::Layer layer{path /*originalPath*/, "", savePath,
                                    editable};
            EXPECT_TRUE(layer); // VALID
            const int numLayers = static_cast<int>(layer.getSubLayers().size());
            EXPECT_GT(numLayers, 0);

            for (BifrostUsd::InitialLoadSet loadSet : loadSetArgs) {
                for (int index = -2; index < numLayers; ++index) {
                    BifrostUsd::Stage stage{layer, loadSet};
                    EXPECT_TRUE(stage); // VALID
                    if (index >= -1) { // -2 == test with default EditLayer
                        EXPECT_TRUE(stage.setEditLayerIndex(index, false));
                    }

                    testCopyAndMoveOps(stage);
                }
            }
        }
    }
#endif

    // ctor with BifrostUSD::Layer + InitialLoadSet
    for (bool editable : editableArgs) {
        for (BifrostUsd::InitialLoadSet loadSet : loadSetArgs) {
            PXR_NS::SdfLayerRefPtr sdfRootLayer =
                PXR_NS::SdfLayer::FindOrOpen(rootName.c_str());
            ASSERT_NE(sdfRootLayer, nullptr);
            Amino::String errorMsg;
            addSubLayers(sdfRootLayer, subNames, errorMsg);
            ASSERT_STREQ("", errorMsg.c_str());

            BifrostUsd::Layer layer{sdfRootLayer, editable};
            EXPECT_TRUE(layer.isValid());
            EXPECT_EQ(layer.getSubLayers().size(), numSubNames);

            BifrostUsd::Stage stage{layer, loadSet};
            EXPECT_TRUE(stage); // VALID

            for (int index = -2; index < numSubNames; ++index) {
                if (index >= -1) { // -2 == use default EditLayer
                    EXPECT_TRUE(stage.setEditLayerIndex(index, false));
                }

                testCopyAndMoveOps(stage);
            }
        }
    }

    // ctor with BifrostUSD::Layer + PopulationMask + InitialLoadSet
    for (bool editable : editableArgs) {
        for (BifrostUsd::InitialLoadSet loadSet : loadSetArgs) {
            PXR_NS::SdfLayerRefPtr sdfRootLayer =
                PXR_NS::SdfLayer::FindOrOpen(rootName.c_str());
            ASSERT_NE(sdfRootLayer, nullptr);
            Amino::String errorMsg;
            addSubLayers(sdfRootLayer, subNames, errorMsg);
            ASSERT_STREQ("", errorMsg.c_str());

            BifrostUsd::Layer layer{sdfRootLayer, editable};
            EXPECT_TRUE(layer.isValid());
            EXPECT_EQ(layer.getSubLayers().size(), numSubNames);

            BifrostUsd::Stage stage{
                layer, PXR_NS::UsdStagePopulationMask(maskPaths), loadSet};
            EXPECT_TRUE(stage); // VALID

            for (int index = -2; index < numSubNames; ++index) {
                if (index >= -1) { // -2 == use default EditLayer
                    EXPECT_TRUE(stage.setEditLayerIndex(index, false));
                }

                testCopyAndMoveOps(stage);
            }
        }
    }

    // ctor with filepath + InitialLoadSet
    for (BifrostUsd::InitialLoadSet loadSet : loadSetArgs) {
        BifrostUsd::Stage stage{getResourcePath("layer_with_sub_layers.usda"),
                                loadSet};
        EXPECT_TRUE(stage); // VALID
        ASSERT_NE(nullptr, stage.getRootLayer());
        const int numLayers =
            static_cast<int>(stage.getRootLayer()->getSubLayers().size());
        EXPECT_GT(numLayers, 0);

        for (int index = -2; index < numLayers; ++index) {
            if (index >= -1) { // -2 == use default EditLayer
                EXPECT_TRUE(stage.setEditLayerIndex(index, false));
            }

            testCopyAndMoveOps(stage);
        }
    }

    // ctor with filepath + PopulationMask + InitialLoadSet
    for (BifrostUsd::InitialLoadSet loadSet : loadSetArgs) {
        BifrostUsd::Stage stage{getResourcePath("layer_with_sub_layers.usda"),
                                PXR_NS::UsdStagePopulationMask(maskPaths),
                                loadSet};
        EXPECT_TRUE(stage); // VALID
        ASSERT_NE(nullptr, stage.getRootLayer());
        const int numLayers =
            static_cast<int>(stage.getRootLayer()->getSubLayers().size());
        EXPECT_GT(numLayers, 0);

        for (int index = -2; index < numLayers; ++index) {
            if (index >= -1) { // -2 == use default EditLayer
                EXPECT_TRUE(stage.setEditLayerIndex(index, false));
            }

            testCopyAndMoveOps(stage);
        }
    }
}
