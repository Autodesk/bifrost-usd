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
#include <nodedefs/usd_pack/usd_collection_nodedefs.h>
#include <nodedefs/usd_pack/usd_prim_nodedefs.h>
#include <nodedefs/usd_pack/usd_stage_nodedefs.h>
#include <utils/test/testUtils.h>

BIFUSD_WARNING_PUSH
BIFUSD_WARNING_DISABLE_MSC(4003)
BIFUSD_WARNING_DISABLE_MSC(4267)
#include <pxr/usd/usd/collectionAPI.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
BIFUSD_WARNING_POP

#include <cstdlib>
#include <string>

using namespace BifrostUsd::TestUtils;

TEST(CollectionNodeDefs, create_collection) {
    auto pxrStage = pxr::UsdStage::CreateInMemory();

    auto city   = pxrStage->DefinePrim(pxr::SdfPath{"/city"});
    auto geom   = pxrStage->DefinePrim(pxr::SdfPath{"/city/geom"});
    auto house1 = pxrStage->DefinePrim(pxr::SdfPath{"/city/geom/house1"});
    auto house2 = pxrStage->DefinePrim(pxr::SdfPath{"/city/geom/house2"});

    auto house2_a = pxrStage->DefinePrim(pxr::SdfPath{"/city/geom/house2/a"});
    auto house2_b = pxrStage->DefinePrim(pxr::SdfPath{"/city/geom/house2/b"});

    auto stage =
        BifrostUsd::Stage(pxrStage->GetRootLayer()->GetIdentifier().c_str());
    ASSERT_TRUE(stage);

    const Amino::Array<Amino::String> include_paths = {"/city/geom/house1",
                                                       "/city/geom/house2"};

    const Amino::Array<Amino::String> exclude_paths = {"/city/geom/house2/a"};

    // Test the get_or_create_collection node
    EXPECT_TRUE(USD::Collection::get_or_create_collection(
        stage,
        /*prim_path*/ "/city/geom",
        /*collection_name*/ "houses",
        /*rule*/ BifrostUsd::ExpansionRule::Default, include_paths,
        exclude_paths));

    geom = stage->GetPrimAtPath(pxr::SdfPath{"/city/geom"});
    auto collectionAPI =
        pxr::UsdCollectionAPI::Get(geom, pxr::TfToken{"houses"});
    ASSERT_TRUE(collectionAPI);

    auto               includesRel = collectionAPI.GetIncludesRel();
    pxr::SdfPathVector includesTargetsPaths;
    ASSERT_TRUE(includesRel.GetTargets(&includesTargetsPaths));

    auto expectedIncludesTargetsPaths = pxr::SdfPathVector{
        pxr::SdfPath{"/city/geom/house1"}, pxr::SdfPath{"/city/geom/house2"}};
    ASSERT_EQ(includesTargetsPaths, expectedIncludesTargetsPaths);

    auto               excludesRel = collectionAPI.GetExcludesRel();
    pxr::SdfPathVector excludesTargetsPaths;
    ASSERT_TRUE(excludesRel.GetTargets(&excludesTargetsPaths));

    auto expectedExcludesTargetsPaths =
        pxr::SdfPathVector{pxr::SdfPath{"/city/geom/house2/a"}};
    ASSERT_EQ(excludesTargetsPaths, expectedExcludesTargetsPaths);
}

TEST(CollectionNodeDefs, get_existing_collection_and_add_input_path) {
    auto pxrStage = pxr::UsdStage::CreateInMemory();

    auto city =
        pxrStage->DefinePrim(pxr::SdfPath{"/city"}, pxr::TfToken{"Xform"});
    auto geom   = pxrStage->DefinePrim(pxr::SdfPath{"/city/geom"});
    auto house1 = pxrStage->DefinePrim(pxr::SdfPath{"/city/geom/house1"});
    auto house2 = pxrStage->DefinePrim(pxr::SdfPath{"/city/geom/house2"});

    auto collectionAPI =
        pxr::UsdCollectionAPI::Apply(geom, pxr::TfToken{"houses"});
    ASSERT_TRUE(collectionAPI);

    auto includeRelationship = collectionAPI.CreateIncludesRel();
    ASSERT_TRUE(
        includeRelationship.AddTarget(pxr::SdfPath{"/city/geom/house1"}));

    auto               includesRel = collectionAPI.GetIncludesRel();
    pxr::SdfPathVector includesTargetsPaths;
    ASSERT_TRUE(includesRel.GetTargets(&includesTargetsPaths));
    auto expectedIncludesTargetsPathsBeforeBifrost =
        pxr::SdfPathVector{pxr::SdfPath{"/city/geom/house1"}};
    EXPECT_EQ(includesTargetsPaths, expectedIncludesTargetsPathsBeforeBifrost);

    // Test the get_or_create_collection node
    auto stage =
        BifrostUsd::Stage(pxrStage->GetRootLayer()->GetIdentifier().c_str());
    ASSERT_TRUE(stage);

    const Amino::Array<Amino::String> include_paths{"/city/geom/house2"};
    const Amino::Array<Amino::String> exclude_paths;

    EXPECT_FALSE(USD::Collection::get_or_create_collection(
        stage,
        /*prim_path*/ "TEST INVALID PRIM PATH",
        /*collection_name*/ "houses",
        /*rule*/ BifrostUsd::ExpansionRule::Default, include_paths,
        exclude_paths));

    EXPECT_TRUE(USD::Collection::get_or_create_collection(
        stage,
        /*prim_path*/ "/city/geom",
        /*collection_name*/ "houses",
        /*rule*/ BifrostUsd::ExpansionRule::Default, include_paths,
        exclude_paths));

    geom          = stage->GetPrimAtPath(pxr::SdfPath{"/city/geom"});
    collectionAPI = pxr::UsdCollectionAPI::Get(geom, pxr::TfToken{"houses"});

    ASSERT_TRUE(collectionAPI);

    includesRel = collectionAPI.GetIncludesRel();
    includesTargetsPaths.clear();
    ASSERT_TRUE(includesRel.GetTargets(&includesTargetsPaths));

    auto expectedIncludesTargetsPathsAfterBiforst = pxr::SdfPathVector{
        pxr::SdfPath{"/city/geom/house1"}, pxr::SdfPath{"/city/geom/house2"}};
    ASSERT_EQ(includesTargetsPaths, expectedIncludesTargetsPathsAfterBiforst);
}

TEST(CollectionNodeDefs, create_collection_with_expansion_rule) {
    auto pxrStage = pxr::UsdStage::CreateInMemory();

    auto obj = pxrStage->DefinePrim(pxr::SdfPath{"/obj"});

    // Test the get_or_create_collection node
    auto stage =
        BifrostUsd::Stage(pxrStage->GetRootLayer()->GetIdentifier().c_str());
    ASSERT_TRUE(stage);

    const Amino::Array<Amino::String> include_paths;
    const Amino::Array<Amino::String> exclude_paths;

    EXPECT_TRUE(USD::Collection::get_or_create_collection(
        stage,
        /*prim_path*/ "/obj",
        /*collection_name*/ "foo",
        /*rule*/ BifrostUsd::ExpansionRule::ExplicitOnly, include_paths,
        exclude_paths));

    obj                = stage->GetPrimAtPath(pxr::SdfPath{"/obj"});
    auto collectionAPI = pxr::UsdCollectionAPI::Get(obj, pxr::TfToken{"foo"});

    auto attr = collectionAPI.GetExpansionRuleAttr();
    ASSERT_TRUE(attr);

    pxr::TfToken rule;
    ASSERT_TRUE(attr.Get(&rule));

    EXPECT_EQ(rule, pxr::TfToken{"explicitOnly"});
}

TEST(CollectionNodeDefs, get_all_collection_names) {
    auto pxrStage = pxr::UsdStage::CreateInMemory();

    auto prim = pxrStage->DefinePrim(pxr::SdfPath{"/obj"});

    pxr::UsdCollectionAPI::Apply(prim, pxr::TfToken{"first"});
    pxr::UsdCollectionAPI::Apply(prim, pxr::TfToken{"second"});

    auto stage = Amino::newClassPtr<BifrostUsd::Stage>(
        pxrStage->GetRootLayer()->GetIdentifier().c_str());
    ASSERT_TRUE(stage);

    const auto bif_prim = BifrostUsd::Prim{prim, stage};
    ASSERT_TRUE(bif_prim);

    Amino::MutablePtr<Amino::Array<Amino::String>> paths;
    // Test the get_all_collection_names node
    USD::Collection::get_all_collection_names(bif_prim, paths);
    EXPECT_NE(paths, nullptr);
    EXPECT_EQ(paths->size(), 2);
    EXPECT_EQ(paths->at(0).c_str(), Amino::String{"first"});
    EXPECT_EQ(paths->at(1).c_str(), Amino::String{"second"});
}

TEST(CollectionNodeDefs, get_includes_paths) {
    auto pxrStage = pxr::UsdStage::CreateInMemory();

    pxrStage->DefinePrim(pxr::SdfPath{"/obj1"});
    pxrStage->DefinePrim(pxr::SdfPath{"/obj2"});

    auto prim = pxrStage->DefinePrim(pxr::SdfPath{"/grp"});

    auto collectionAPI =
        pxr::UsdCollectionAPI::Apply(prim, pxr::TfToken{"obj1_and_obj2"});
    auto includeRelationship = collectionAPI.CreateIncludesRel();
    includeRelationship.AddTarget(pxr::SdfPath{"/obj1"});
    includeRelationship.AddTarget(pxr::SdfPath{"/obj2"});

    auto stage = Amino::newClassPtr<BifrostUsd::Stage>(
        pxrStage->GetRootLayer()->GetIdentifier().c_str());
    ASSERT_TRUE(stage);

    const auto bif_prim = BifrostUsd::Prim{prim, stage};
    ASSERT_TRUE(bif_prim);
    Amino::MutablePtr<Amino::Array<Amino::String>> paths;
    // Test the get_includes_paths node
    USD::Collection::get_includes_paths(bif_prim, "obj1_and_obj2", paths);
    EXPECT_NE(paths, nullptr);
    EXPECT_EQ(paths->size(), 2);
    EXPECT_EQ(paths->at(0).c_str(), Amino::String{"/obj1"});
    EXPECT_EQ(paths->at(1).c_str(), Amino::String{"/obj2"});
}

TEST(CollectionNodeDefs, get_excludes_paths) {
    auto pxrStage = pxr::UsdStage::CreateInMemory();

    auto prim = pxrStage->DefinePrim(pxr::SdfPath{"/grp"});

    auto collectionAPI =
        pxr::UsdCollectionAPI::Apply(prim, pxr::TfToken{"exclude_obj"});

    auto excludeRelationship = collectionAPI.CreateExcludesRel();
    excludeRelationship.AddTarget(pxr::SdfPath{"/obj"});

    auto stage = Amino::newClassPtr<BifrostUsd::Stage>(
        pxrStage->GetRootLayer()->GetIdentifier().c_str());
    ASSERT_TRUE(stage);

    const auto bif_prim = BifrostUsd::Prim{prim, stage};
    ASSERT_TRUE(bif_prim);
    Amino::MutablePtr<Amino::Array<Amino::String>> paths;
    // Test the get_excludes_paths node
    USD::Collection::get_excludes_paths(bif_prim, "exclude_obj", paths);
    EXPECT_NE(paths, nullptr);
    EXPECT_EQ(paths->size(), 1);
    EXPECT_EQ(paths->at(0).c_str(), Amino::String{"/obj"});
}
