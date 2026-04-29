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
#include <Amino/Core/String.h>
#include <gtest/gtest.h>
#include <BifrostUsd/VariantSelection.h>

#include <nodedefs/usd_pack/usd_prim_nodedefs.h>
#include <nodedefs/usd_pack/usd_layer_nodedefs.h>
#include <nodedefs/usd_pack/usd_stage_nodedefs.h>
#include <nodedefs/usd_pack/usd_variantset_nodedefs.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <utils/test/testUtils.h>

#include <pxr/usd/usd/editTarget.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/stage.h>

#include <cstdlib>
#include <iostream>
#include <ostream>
#include <string>

using namespace BifrostUsd::TestUtils;

namespace {
auto addVariantSet(BifrostUsd::Stage& stage,
                   const Amino::String& prim_path,
                   const Amino::String& name) {
    auto primPath = PXR_NS::SdfPath(prim_path.c_str());
    auto prim     = stage->DefinePrim(primPath);
    USD::VariantSet::add_variant_set(stage, primPath.GetText(), name);
    return prim;
}
} // namespace

TEST(VariantSetNodeDefs, add_variant_set) {
    BifrostUsd::Stage stage;
    auto                prim_path        = Amino::String("/a");
    auto                variant_set_name = Amino::String("look");

    auto prim = addVariantSet(stage, prim_path, variant_set_name);

    auto vset = prim.GetVariantSet("look");
    ASSERT_TRUE(vset);
}

TEST(VariantSetNodeDefs, add_variant) {
    BifrostUsd::Stage stage;
    auto                prim_path        = Amino::String("/a");
    auto                variant_set_name = Amino::String("look");
    auto prim = addVariantSet(stage, prim_path, variant_set_name);

    auto variant_name          = Amino::String("red");
    bool setVariantSelection = true;
    USD::VariantSet::add_variant(stage, prim_path, variant_set_name,
                                 variant_name, setVariantSelection);

    auto vset = prim.GetVariantSet("look");

    auto expectedVariants = std::vector<std::string>{"red"};
    ASSERT_EQ(vset.GetVariantNames(), expectedVariants);
    ASSERT_EQ(vset.GetVariantSelection(), std::string("red"));

    variant_name = Amino::String("green");
    // Add "green" Variant from VariantSet "look" and Variant "red",
    // so the "green" is only available when "red" is selected.
    USD::VariantSet::add_variant(stage, prim_path, variant_set_name,
                                 variant_name, setVariantSelection);

    expectedVariants = std::vector<std::string>{"green", "red"};
    ASSERT_EQ(vset.GetVariantNames(), expectedVariants);
    ASSERT_EQ(vset.GetVariantSelection(), std::string("red"));

    variant_name          = Amino::String("blue");
    setVariantSelection = false;
    USD::VariantSet::add_variant(stage, prim_path, variant_set_name,
                                 variant_name, setVariantSelection);

    expectedVariants = std::vector<std::string>{"blue", "green", "red"};
    ASSERT_EQ(vset.GetVariantNames(), expectedVariants);
    ASSERT_EQ(vset.GetVariantSelection(), std::string("green"));
}

TEST(VariantSetNodeDefs, set_variant_selection) {
    BifrostUsd::Stage stage;
    auto                prim_path        = Amino::String("/a");
    auto                variant_set_name = Amino::String("look");
    auto prim = addVariantSet(stage, prim_path, variant_set_name);

    auto variant_name          = Amino::String("red");
    bool setVariantSelection = false;
    USD::VariantSet::add_variant(stage, prim_path, variant_set_name,
                                 variant_name, setVariantSelection);

    auto vset = prim.GetVariantSet("look");
    ASSERT_EQ(vset.GetVariantSelection(), std::string());

    USD::VariantSet::set_variant_selection(stage, prim_path, variant_set_name,
                                           variant_name);
    ASSERT_EQ(vset.GetVariantSelection(), std::string("red"));

    USD::VariantSet::clear_variant_selection(stage, prim_path, variant_set_name);
    ASSERT_EQ(vset.GetVariantSelection(), std::string());
}

TEST(VariantSetNodeDefs, get_variant_sets) {
    BifrostUsd::Stage stage;
    auto                prim_path = Amino::String("/a");

    auto names = Amino::MutablePtr<Amino::Array<Amino::String>>();
    USD::VariantSet::get_variant_sets(stage, prim_path, names);
    ASSERT_EQ(names->size(), 0);

    auto variant_set_name = Amino::String("vset");
    addVariantSet(stage, prim_path, variant_set_name);

    names->clear();
    USD::VariantSet::get_variant_sets(stage, prim_path, names);
    ASSERT_EQ(names->size(), 1);
    ASSERT_EQ((*names)[0], variant_set_name);
}

TEST(VariantSetNodeDefs, get_variants) {
    BifrostUsd::Stage stage;
    auto                prim_path        = Amino::String("/a");
    auto                variant_set_name = Amino::String("vset");
    addVariantSet(stage, prim_path, variant_set_name);

    auto names = Amino::MutablePtr<Amino::Array<Amino::String>>();
    USD::VariantSet::get_variants(stage, prim_path, variant_set_name, names);
    ASSERT_EQ(names->size(), 0);

    auto variant_name          = Amino::String("a");
    bool setVariantSelection = false;
    USD::VariantSet::add_variant(stage, prim_path, variant_set_name,
                                 variant_name, setVariantSelection);

    names->clear();
    USD::VariantSet::get_variants(stage, prim_path, variant_set_name, names);
    ASSERT_EQ(names->size(), 1);
    ASSERT_EQ((*names)[0], variant_name);
}

TEST(VariantSetNodeDefs, get_variant_selection) {
    BifrostUsd::Stage stage;
    auto                prim_path        = Amino::String("/a");
    auto                variant_set_name = Amino::String("look");
    auto prim = addVariantSet(stage, prim_path, variant_set_name);

    auto red_variant          = Amino::String("red");
    USD::VariantSet::add_variant(stage, prim_path, variant_set_name,
                                 red_variant, /*setVariantSelection*/ false);

    Amino::String selection;
    USD::VariantSet::get_variant_selection(stage, prim_path, variant_set_name,
                                           selection);
    ASSERT_TRUE(selection.empty());

    USD::VariantSet::set_variant_selection(stage, prim_path, variant_set_name,
                                           red_variant, /*clear*/ true);

    USD::VariantSet::get_variant_selection(stage, prim_path, variant_set_name,
                                           selection);
    ASSERT_EQ(selection, red_variant);

    // Clear the variant selection before adding an other one,
    // otherwise it would create the new variant nested in current one.
    USD::VariantSet::clear_variant_selection(stage, prim_path, variant_set_name);

    auto green_variant = Amino::String("green");
    USD::VariantSet::add_variant(stage, prim_path, variant_set_name,
                                 green_variant, /*setVariantSelection*/ false);

    USD::VariantSet::set_variant_selection(stage, prim_path, variant_set_name,
                                           green_variant, /*clear*/ true);

    USD::VariantSet::get_variant_selection(stage, prim_path, variant_set_name,
                                           selection);
    ASSERT_EQ(selection, green_variant);

    USD::VariantSet::clear_variant_selection(stage, prim_path, variant_set_name);
    USD::VariantSet::get_variant_selection(stage, prim_path, variant_set_name,
                                           selection);
    ASSERT_TRUE(selection.empty());
}

TEST(VariantSetNodeDefs, create_variant_in_variant) {
    /*********************************************/
    /*     Create New Stage with a Prim          */
    /*********************************************/

    auto filePath   = Amino::String("/tmp/create_variant_in_variant.usda");
    auto fileFormat = Amino::String("usda");

    auto layerPtr = Amino::MutablePtr<BifrostUsd::Layer>();
    USD::Layer::create_layer(filePath, fileFormat, layerPtr);
    ASSERT_TRUE(layerPtr->isValid());

    auto mask = Amino::Array<Amino::String>();
    auto load = BifrostUsd::InitialLoadSet::LoadAll;

    auto stagePtr = Amino::MutablePtr<BifrostUsd::Stage>();
    USD::Stage::open_stage_from_layer(*layerPtr, mask, load, -1, stagePtr);

    ASSERT_TRUE(*stagePtr);

    BifrostUsd::Stage& stageRef = *stagePtr;
    auto               primPath = Amino::String("/prim");
    auto               type     = Amino::String("Xform");
    USD::Prim::create_prim(stageRef, primPath, type);

    /*********************************************/
    /*       Add VariantSets and Variants        */
    /*********************************************/

    const auto& variantSelection = stageRef.variantSelection();
    EXPECT_TRUE(variantSelection.empty());

    USD::VariantSet::add_variant_set(stageRef, primPath, "parent");
    EXPECT_TRUE(variantSelection.primPath().empty());
    EXPECT_EQ(variantSelection.stack().size(), 0);

    USD::VariantSet::add_variant(stageRef, primPath, "parent", "without_child",
                                 /*set_variant_selection*/ false);
    USD::VariantSet::add_variant(stageRef, primPath, "parent", "with_child",
                                 /*set_variant_selection*/ false);
    EXPECT_EQ(variantSelection.stack().size(), 0);

    USD::VariantSet::set_variant_selection(stageRef, primPath, "parent", "with_child");
    EXPECT_EQ(variantSelection.primPath(), primPath);
    using PairOfNames = BifrostUsd::VariantSelection::PairOfNames;

    EXPECT_EQ(variantSelection.stack().size(), 1);
    EXPECT_EQ(variantSelection.stack()[0],
              (PairOfNames{"parent", "with_child"}));

    USD::VariantSet::add_variant_set(stageRef, primPath, "child");
    EXPECT_EQ(variantSelection.primPath(), primPath);
    EXPECT_EQ(variantSelection.stack().size(), 1);
    EXPECT_EQ(variantSelection.stack()[0],
              (PairOfNames{"parent", "with_child"}));

    USD::VariantSet::add_variant(stageRef, primPath, "child", "x",
                                 /*set_variant_selection*/ false);
    USD::VariantSet::add_variant(stageRef, primPath, "child", "y",
                                 /*set_variant_selection*/ false);
    EXPECT_EQ(variantSelection.stack().size(), 1);
    EXPECT_EQ(variantSelection.stack()[0],
              (PairOfNames{"parent", "with_child"}));

    // Set the variant selection to "child:y" and create a prim in it.
    USD::VariantSet::set_variant_selection(stageRef, primPath, "child", "y");
    EXPECT_EQ(variantSelection.stack().size(), 2);
    EXPECT_EQ(variantSelection.stack()[0],
              (PairOfNames{"parent", "with_child"}));
    EXPECT_EQ(variantSelection.stack()[1], (PairOfNames{"child", "y"}));

    USD::Prim::create_prim(stageRef, Amino::String("/prim/inside_child_y"),
                           type);

    // Clear the Variant Selection and Create a prim.
    USD::VariantSet::clear_variant_selection(stageRef, primPath, "child");

    USD::Prim::create_prim(stageRef, Amino::String("/prim/not_in_a_variant"),
                           type);

    // Create more than one level of nested VariantSets
    size_t levels = 3;
    for (size_t i = 0; i < levels; i++) {
        std::string geoVariantSet = "VSet_at_level" + std::to_string(i + 1);
        USD::VariantSet::add_variant_set(stageRef, primPath,
                                         geoVariantSet.c_str());
        USD::VariantSet::add_variant(stageRef, primPath, geoVariantSet.c_str(),
                                     "empty", /*set_variant_selection*/ false);

        USD::VariantSet::add_variant(stageRef, primPath, geoVariantSet.c_str(),
                                     "with_geo",
                                     /*set_variant_selection*/ true);

        std::string geoVisVariantSet =
            "SubVSet_at_level" + std::to_string(i + 1);
        USD::VariantSet::add_variant_set(stageRef, primPath,
                                         geoVisVariantSet.c_str());
        USD::VariantSet::add_variant(stageRef, primPath,
                                     geoVisVariantSet.c_str(), "empty",
                                     /*set_variant_selection*/ false);

        USD::VariantSet::add_variant(stageRef, primPath,
                                     geoVisVariantSet.c_str(), "with_geo",
                                     /*set_variant_selection*/ true);

        USD::VariantSet::set_variant_selection(
            stageRef, primPath, geoVariantSet.c_str(), "with_geo");
        USD::VariantSet::set_variant_selection(
            stageRef, primPath, geoVisVariantSet.c_str(), "with_geo");
    }

    // Add a geometry only in deepest with_geo variant
    USD::Prim::create_prim(stageRef, Amino::String{"/prim/inSubVSetAtLevel3"},
                           "Capsule");

    /*********************************************/
    /*         Test Created Variants             */
    /*********************************************/

    auto rootLayerPtr = Amino::Ptr<BifrostUsd::Layer>();
    USD::Layer::get_root_layer(stageRef, rootLayerPtr);
    Amino::String result;
    USD::Layer::export_layer_to_string(*rootLayerPtr,
                                       /*export_sub_layers*/ false, result);
    EXPECT_FALSE(result.empty());
    auto layer = PXR_NS::SdfLayer::CreateAnonymous("from_bifrost.usda");
    EXPECT_TRUE(layer->ImportFromString(result.c_str()));

    auto stage       = PXR_NS::UsdStage::Open(layer);
    auto sdfPrimPath = PXR_NS::SdfPath{primPath.c_str()};
    auto prim        = stage->GetPrimAtPath(sdfPrimPath);
    EXPECT_TRUE(prim);
    auto parentVarSet = prim.GetVariantSet("parent");
    parentVarSet.SetVariantSelection("without_child");
    auto insideChildYPrimPath =
        sdfPrimPath.AppendChild(PXR_NS::TfToken{"inside_child_y"});
    auto childPrim = stage->GetPrimAtPath(insideChildYPrimPath);
    EXPECT_FALSE(childPrim);

    parentVarSet.SetVariantSelection("with_child");
    childPrim = stage->GetPrimAtPath(insideChildYPrimPath);
    EXPECT_TRUE(childPrim);

    parentVarSet.ClearVariantSelection();
    EXPECT_FALSE(stage->GetPrimAtPath(insideChildYPrimPath));
    auto notInAVariantPath =
        sdfPrimPath.AppendChild(PXR_NS::TfToken{"not_in_a_variant"});
    EXPECT_TRUE(stage->GetPrimAtPath(notInAVariantPath));

    // test the deeply nested VariantSets
    auto inSubVSetAtLevel3Path =
        sdfPrimPath.AppendChild(PXR_NS::TfToken{"inSubVSetAtLevel3"});

    // first level
    auto varSetAtLevel1 = prim.GetVariantSet("VSet_at_level1");
    varSetAtLevel1.SetVariantSelection("with_geo");
    EXPECT_TRUE(stage->GetPrimAtPath(inSubVSetAtLevel3Path));
    varSetAtLevel1.SetVariantSelection("empty");
    EXPECT_FALSE(stage->GetPrimAtPath(inSubVSetAtLevel3Path));

    // switch to "with_geo" to inspect next level
    varSetAtLevel1.SetVariantSelection("with_geo");
    auto varSetAtLevel2 = prim.GetVariantSet("VSet_at_level2");
    varSetAtLevel2.SetVariantSelection("with_geo");
    EXPECT_TRUE(stage->GetPrimAtPath(inSubVSetAtLevel3Path));
    varSetAtLevel2.SetVariantSelection("empty");
    EXPECT_FALSE(stage->GetPrimAtPath(inSubVSetAtLevel3Path));

    // switch to "with_geo" to inspect next level
    varSetAtLevel2.SetVariantSelection("with_geo");
    auto varSetAtLevel3 = prim.GetVariantSet("VSet_at_level3");
    varSetAtLevel3.SetVariantSelection("with_geo");
    EXPECT_TRUE(stage->GetPrimAtPath(inSubVSetAtLevel3Path));
    varSetAtLevel3.SetVariantSelection("empty");
    EXPECT_FALSE(stage->GetPrimAtPath(inSubVSetAtLevel3Path));

#if 0
    // Enable if you want to check the content in usdview.
    // /*********************************************/
    // /*               Save Root Layer             */
    // /*********************************************/

    USD::Layer::get_root_layer(stageRef, rootLayerPtr);
    EXPECT_TRUE(USD::Layer::export_layer_to_file(*rootLayerPtr,
                                                 filePath.c_str(), false));
#endif
}

TEST(VariantSetNodeDefs, create_variantset_using_last_modified_prim) {
    /*********************************************/
    /*     Create New Stage with a Prim          */
    /*********************************************/

    auto filePath =
        Amino::String("/tmp/create_variantset_using_last_modified_prim.usda");
    auto fileFormat = Amino::String("usda");

    auto layerPtr = Amino::MutablePtr<BifrostUsd::Layer>();
    USD::Layer::create_layer(filePath, fileFormat, layerPtr);
    ASSERT_TRUE(layerPtr->isValid());

    auto mask = Amino::Array<Amino::String>();
    auto load = BifrostUsd::InitialLoadSet::LoadAll;

    auto stagePtr = Amino::MutablePtr<BifrostUsd::Stage>();
    USD::Stage::open_stage_from_layer(*layerPtr, mask, load, -1, stagePtr);

    ASSERT_TRUE(*stagePtr);

    BifrostUsd::Stage& stageRef = *stagePtr;
    ASSERT_TRUE(stageRef.lastModifiedVariantSet().empty());
    ASSERT_TRUE(stageRef.lastModifiedVariant().empty());
    ASSERT_TRUE(stageRef.variantSelection().stack().empty());

    auto               primPath = Amino::String("/a");
    auto               type     = Amino::String("Xform");
    USD::Prim::create_prim(stageRef, primPath, type);

    Amino::MutablePtr<BifrostUsd::Prim> prim;
    ASSERT_TRUE(USD::Prim::get_prim_at_path(stagePtr.toImmutable(),
                                               primPath.c_str(), prim));

    auto pxrPrimPtr = *prim;
    ASSERT_TRUE(pxrPrimPtr->IsValid());
    ASSERT_FALSE(pxrPrimPtr->HasVariantSets());

    /*********************************************/
    /*       Add VariantSets and Variants        */
    /*********************************************/

    auto emptyPrimPath = Amino::String{""};
    USD::VariantSet::add_variant_set(stageRef, emptyPrimPath, "VSet");
    ASSERT_TRUE(pxrPrimPtr->HasVariantSets());

    USD::VariantSet::add_variant(stageRef, emptyPrimPath, "VSet", "variant_a",
                                 /*set_variant_selection*/ false);

    using NameVector = std::vector<std::string>;
    ASSERT_EQ(pxrPrimPtr->GetVariantSet("VSet").GetVariantNames(),
              NameVector({"variant_a"}));

    USD::VariantSet::add_variant(stageRef, emptyPrimPath, "VSet", "variant_b",
                                 /*set_variant_selection*/ false);

    ASSERT_EQ(pxrPrimPtr->GetVariantSet("VSet").GetVariantNames(),
              NameVector({"variant_a", "variant_b"}));

    USD::VariantSet::set_variant_selection(stageRef, emptyPrimPath, "VSet",
                                           "variant_b");
    ASSERT_EQ(pxrPrimPtr->GetVariantSet("VSet").GetVariantSelection(),
              "variant_b");

    // Clear the Variant Set Selection.
    USD::VariantSet::clear_variant_selection(stageRef, emptyPrimPath, "VSet");
    ASSERT_EQ(pxrPrimPtr->GetVariantSet("VSet").GetVariantSelection(), "");

    // Clear the Variant Set Selection with a non empty string for the variant
    // name. It should still clear the Variant Selection as the clear parameter
    // win.
    USD::VariantSet::set_variant_selection(stageRef, emptyPrimPath, "VSet",
                                           "variant_a");
    ASSERT_EQ(pxrPrimPtr->GetVariantSet("VSet").GetVariantSelection(),
              "variant_a");
    USD::VariantSet::clear_variant_selection(stageRef, emptyPrimPath, "VSet");
    ASSERT_EQ(pxrPrimPtr->GetVariantSet("VSet").GetVariantSelection(), "");
}

TEST(VariantSetNodeDefs, clear_variant_selection) {
    /*********************************************/
    /*     Create New Stage with a Prim          */
    /*********************************************/

    auto filePath =
        Amino::String("/tmp/clear_variant_selection.usda");
    auto fileFormat = Amino::String("usda");

    auto layerPtr = Amino::MutablePtr<BifrostUsd::Layer>();
    USD::Layer::create_layer(filePath, fileFormat, layerPtr);
    ASSERT_TRUE(layerPtr->isValid());

    auto mask = Amino::Array<Amino::String>();
    auto load = BifrostUsd::InitialLoadSet::LoadAll;

    auto stagePtr = Amino::MutablePtr<BifrostUsd::Stage>();
    USD::Stage::open_stage_from_layer(*layerPtr, mask, load, -1, stagePtr);

    ASSERT_TRUE(*stagePtr);

    BifrostUsd::Stage& stageRef = *stagePtr;
    ASSERT_TRUE(stageRef.lastModifiedVariantSet().empty());
    ASSERT_TRUE(stageRef.lastModifiedVariant().empty());
    ASSERT_TRUE(stageRef.variantSelection().stack().empty());

    auto primPath = Amino::String("/a");
    auto type     = Amino::String("Xform");
    USD::Prim::create_prim(stageRef, primPath, type);

    Amino::MutablePtr<BifrostUsd::Prim> prim;
    ASSERT_TRUE(USD::Prim::get_prim_at_path(stagePtr.toImmutable(),
                                            primPath.c_str(), prim));

    auto pxrPrimPtr = *prim;
    ASSERT_TRUE(pxrPrimPtr->IsValid());
    ASSERT_FALSE(pxrPrimPtr->HasVariantSets());

    /*********************************************/
    /*      Add N VariantSets and N Variants     */
    /*********************************************/

    constexpr size_t variantSetCount = 4;
    constexpr size_t variantCount    = 3;

    std::vector<std::string> expectedVariantSetNames;
    std::vector<std::string> expectedVariantNames;
    // Create the Variants without selecting them.
    for (size_t i = 0; i < variantSetCount; i++) {
        auto vsetName = "VSet" + std::to_string(i);
        USD::VariantSet::add_variant_set(stageRef, primPath, vsetName.c_str());
        expectedVariantSetNames.push_back(vsetName);
        for (size_t j = 0; j < variantCount; j++) {
            auto varName = "var" + std::to_string(j);
            USD::VariantSet::add_variant(stageRef, primPath, vsetName.c_str(),
                                         varName.c_str(),
                                         /*set_variant_selection*/ false);
            expectedVariantNames.push_back(varName);
        }
    }
    ASSERT_TRUE(pxrPrimPtr->HasVariantSets());
    ASSERT_EQ(pxrPrimPtr->GetVariantSets().GetNames(), expectedVariantSetNames);
    std::vector<std::string> allVariantNames;
    for (const auto& vsetName : pxrPrimPtr->GetVariantSets().GetNames()) {
        auto vset = pxrPrimPtr->GetVariantSet(vsetName);
        for (const auto& varName : vset.GetVariantNames()) {
            allVariantNames.push_back(varName);
        }
    }
    EXPECT_EQ(allVariantNames, expectedVariantNames);

    // Add a prim in each variant
    size_t index = 0;
    for (const auto& vsetName : expectedVariantSetNames) {
        for (const auto& varName : expectedVariantNames) {
            USD::VariantSet::set_variant_selection(
                stageRef, primPath, vsetName.c_str(), varName.c_str());
            auto primInVarName = "prim" + std::to_string(index++);
            auto primInVarPath = Amino::String("/a/") + primInVarName.c_str();
            USD::Prim::create_prim(stageRef, primInVarPath, type);
            USD::VariantSet::clear_variant_selection(stageRef, primPath,
                                                     vsetName.c_str());
        }
    }

    /*********************************************/
    /*         Test Created Variants             */
    /*********************************************/

    auto rootLayerPtr = Amino::Ptr<BifrostUsd::Layer>();
    USD::Layer::get_root_layer(stageRef, rootLayerPtr);
    Amino::String result;
    USD::Layer::export_layer_to_string(*rootLayerPtr,
                                       /*export_sub_layers*/ false, result);
    EXPECT_FALSE(result.empty());
    auto layer = PXR_NS::SdfLayer::CreateAnonymous("from_bifrost.usda");
    EXPECT_TRUE(layer->ImportFromString(result.c_str()));

    auto stage       = PXR_NS::UsdStage::Open(layer);
    auto sdfPrimPath = PXR_NS::SdfPath{primPath.c_str()};
    auto pxrPrim     = stage->GetPrimAtPath(sdfPrimPath);
    EXPECT_TRUE(pxrPrim);

    // Retrieve the prim created in variants.
    size_t primNameSuffix = 0;
    for (const auto& vsetName : expectedVariantSetNames) {
        auto vset = pxrPrim.GetVariantSet(vsetName);
        for (const auto& varName : expectedVariantNames) {
            vset.SetVariantSelection(varName);
            auto primName = "prim" + std::to_string(primNameSuffix++);
            EXPECT_TRUE(stage->GetPrimAtPath(
                sdfPrimPath.AppendChild(PXR_NS::TfToken{primName.c_str()})));
        }
    }

    // Check that prims are not found when no variant selected.
    for (const auto& vsetName : expectedVariantSetNames) {
        auto vset = pxrPrim.GetVariantSet(vsetName);
        vset.ClearVariantSelection();
    }
    for (size_t i = 0;
         i < expectedVariantSetNames.size() + expectedVariantNames.size();
         i++) {
        auto primName = "prim" + std::to_string(i);
        EXPECT_FALSE(stage->GetPrimAtPath(
            sdfPrimPath.AppendChild(PXR_NS::TfToken{primName.c_str()})));
    }

#if 0
    // Enable if you want to check the content in usdview.
    // /*********************************************/
    // /*               Save Root Layer             */
    // /*********************************************/

    USD::Layer::get_root_layer(stageRef, rootLayerPtr);
    EXPECT_TRUE(USD::Layer::export_layer_to_file(*rootLayerPtr,
                                                 filePath.c_str(), false));
#endif
}
