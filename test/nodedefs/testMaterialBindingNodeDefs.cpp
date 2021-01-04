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
#include <bifusd/config/CfgWarningMacros.h>
#include <gtest/gtest.h>
#include <nodedefs/usd_pack/usd_layer_nodedefs.h>
#include <nodedefs/usd_pack/usd_material_binding_nodedefs.h>
#include <nodedefs/usd_pack/usd_stage_nodedefs.h>
#include <utils/test/testUtils.h>

// Note: To silence warnings coming from USD library
// see BIFROST-6414
#include <bifusd/config/CfgWarningMacros.h>

BIFUSD_WARNING_PUSH

BIFUSD_WARNING_DISABLE_MSC(4003)
BIFUSD_WARNING_DISABLE_MSC(4244)
BIFUSD_WARNING_DISABLE_MSC(4267)
BIFUSD_WARNING_DISABLE_MSC(4305)
BIFUSD_WARNING_DISABLE_MSC(4800)

#include <pxr/usd/usd/collectionAPI.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>

BIFUSD_WARNING_POP

#include <cstdlib>
#include <string>

using namespace BifrostUsd::TestUtils;

namespace {

// Create a stage with two geos, a material and a collection of
// those geos.
auto create_pxr_stage_with_collection() {
    auto stage = pxr::UsdStage::CreateInMemory();

    auto city = stage->DefinePrim(pxr::SdfPath{"/city"}, pxr::TfToken{"Xform"});
    auto geom = stage->DefinePrim(pxr::SdfPath{"/city/geom"});
    auto house1    = stage->DefinePrim(pxr::SdfPath{"/city/geom/house1"},
                                       pxr::TfToken{"Capsule"});
    auto house2    = stage->DefinePrim(pxr::SdfPath{"/city/geom/house2"},
                                       pxr::TfToken{"Sphere"});
    auto materials = stage->DefinePrim(pxr::SdfPath{"/city/materials"});

    auto material = pxr::UsdShadeMaterial::Define(
        stage, pxr::SdfPath{"/city/materials/red_mat"});
    auto shader = pxr::UsdShadeShader::Define(
        stage, pxr::SdfPath{"/city/materials/red_mat/surface"});
    shader.CreateIdAttr(pxr::VtValue{pxr::TfToken{"UsdPreviewSurface"}});

    material.CreateSurfaceOutput().ConnectToSource(shader.ConnectableAPI(),
                                                   pxr::TfToken{"surface"});
    shader
        .CreateInput(pxr::TfToken{"diffuseColor"},
                     pxr::SdfValueTypeNames->Color3f)
        .Set(pxr::VtValue{pxr::GfVec3f{1.f, 0.f, 0.f}});

    auto collectionAPI =
        pxr::UsdCollectionAPI::Apply(geom, pxr::TfToken{"houses"});

    auto includeRelationship = collectionAPI.CreateIncludesRel();
    includeRelationship.AddTarget(pxr::SdfPath{"/city/geom/house1"});
    includeRelationship.AddTarget(pxr::SdfPath{"/city/geom/house2"});

    return stage;
}

} // end unnamed namespace

TEST(MaterialBindingNodeDefs, bind_material) {
    auto pxrStage = pxr::UsdStage::CreateInMemory();

    auto gp  = pxrStage->OverridePrim(pxr::SdfPath{"/gp"});
    auto mat = pxr::UsdShadeMaterial::Define(pxrStage, pxr::SdfPath{"/mat"});

    auto invalid_stage = BifrostUsd::Stage(BifrostUsd::Stage::Invalid());
    EXPECT_FALSE(USD::Shading::bind_material(
        invalid_stage,
        /*prim_path*/ "/gp",
        /*material_path*/ "/mat",
        BifrostUsd::MaterialBindingStrength::StrongerThanDescendants,
        /*material_purpose*/ BifrostUsd::MaterialPurpose::All,
        /*collection_prim_path*/ "",
        /*collection_name*/ "", /*binding_name*/ ""));

    auto stage =
        BifrostUsd::Stage(pxrStage->GetRootLayer()->GetIdentifier().c_str());
    ASSERT_TRUE(stage);

    EXPECT_FALSE(USD::Shading::bind_material(
        stage,
        /*prim_path*/ "I_DONT_EXIST",
        /*material_path*/ "/mat",
        BifrostUsd::MaterialBindingStrength::StrongerThanDescendants,
        /*material_purpose*/ BifrostUsd::MaterialPurpose::All,
        /*collection_prim_path*/ "",
        /*collection_name*/ "", /*binding_name*/ ""));

    EXPECT_FALSE(USD::Shading::bind_material(
        stage,
        /*prim_path*/ "/gp",
        /*material_path*/ "I_DONT_EXIST",
        BifrostUsd::MaterialBindingStrength::StrongerThanDescendants,
        /*material_purpose*/ BifrostUsd::MaterialPurpose::All,
        /*collection_prim_path*/ "",
        /*collection_name*/ "", /*binding_name*/ ""));

    EXPECT_TRUE(USD::Shading::bind_material(
        stage,
        /*prim_path*/ "/gp",
        /*material_path*/ "/mat",
        BifrostUsd::MaterialBindingStrength::StrongerThanDescendants,
        /*material_purpose*/ BifrostUsd::MaterialPurpose::All,
        /*collection_prim_path*/ "",
        /*collection_name*/ "", /*binding_name*/ ""));

    gp                      = stage->GetPrimAtPath(pxr::SdfPath{"/gp"});
    auto materialBindingAPI = pxr::UsdShadeMaterialBindingAPI::Apply(gp);

    ASSERT_EQ(materialBindingAPI.GetDirectBinding().GetMaterialPath(),
              mat.GetPath());
}

TEST(MaterialBindingNodeDefs, bind_collection_from_same_prim) {
    auto pxrStage = create_pxr_stage_with_collection();

    auto invalid_stage = BifrostUsd::Stage(BifrostUsd::Stage::Invalid());
    EXPECT_FALSE(USD::Shading::bind_material(
        invalid_stage,
        /*prim_path*/ "/city/geom",
        /*material_path*/ "/city/materials/red_mat",
        BifrostUsd::MaterialBindingStrength::StrongerThanDescendants,
        /*material_purpose*/ BifrostUsd::MaterialPurpose::All,
        /*collection_prim_path*/ "",
        /*collection_name*/ "houses",
        /*binding_name*/ ""));

    auto stage =
        BifrostUsd::Stage(pxrStage->GetRootLayer()->GetIdentifier().c_str());
    ASSERT_TRUE(stage);

    // Test the bind_material node with an invalid usd prim path
    EXPECT_FALSE(USD::Shading::bind_material(
        stage,
        /*prim_path*/ "/I_DONT_EXIST",
        /*material_path*/ "/city/materials/red_mat",
        BifrostUsd::MaterialBindingStrength::StrongerThanDescendants,
        /*material_purpose*/ BifrostUsd::MaterialPurpose::All,
        /*collection_prim_path*/ "",
        /*collection_name*/ "houses",
        /*binding_name*/ ""));

    // Test the bind_material node with an invalid usd material path
    EXPECT_FALSE(USD::Shading::bind_material(
        stage,
        /*prim_path*/ "/city/geom",
        /*material_path*/ "/I_DONT_EXIST",
        BifrostUsd::MaterialBindingStrength::StrongerThanDescendants,
        /*material_purpose*/ BifrostUsd::MaterialPurpose::All,
        /*collection_prim_path*/ "",
        /*collection_name*/ "houses",
        /*binding_name*/ ""));

    // Test the bind_material node with the collection
    EXPECT_TRUE(USD::Shading::bind_material(
        stage,
        /*prim_path*/ "/city/geom",
        /*material_path*/ "/city/materials/red_mat",
        BifrostUsd::MaterialBindingStrength::StrongerThanDescendants,
        /*material_purpose*/ BifrostUsd::MaterialPurpose::All,
        /*collection_prim_path*/ "",
        /*collection_name*/ "houses",
        /*binding_name*/ ""));

    // get the geom prim from the stage authored by Bifrost
    auto geom = stage->GetPrimAtPath(pxr::SdfPath{"/city/geom"});

    auto materialBindingAPI = pxr::UsdShadeMaterialBindingAPI::Apply(geom);
    ASSERT_TRUE(materialBindingAPI);

    auto collectionBindingRels = materialBindingAPI.GetCollectionBindingRels();
    ASSERT_EQ(collectionBindingRels.size(), 1);

    pxr::SdfPathVector targets;
    ASSERT_TRUE(collectionBindingRels[0].GetTargets(&targets));
    ASSERT_EQ(targets.size(), 2);

    ASSERT_EQ(targets[0], pxr::SdfPath{"/city/geom.collection:houses"});
    ASSERT_EQ(targets[1], pxr::SdfPath{"/city/materials/red_mat"});   
}
TEST(MaterialBindingNodeDefs, bind_collection_from_different_prim) {
    auto pxrStage = create_pxr_stage_with_collection();

    auto stage =
        BifrostUsd::Stage(pxrStage->GetRootLayer()->GetIdentifier().c_str());
    ASSERT_TRUE(stage);

    // Test the bind_material node with the collection
    EXPECT_TRUE(USD::Shading::bind_material(
        stage,
        /*prim_path*/ "/city",
        /*material_path*/ "/city/materials/red_mat",
        BifrostUsd::MaterialBindingStrength::StrongerThanDescendants,
        /*material_purpose*/ BifrostUsd::MaterialPurpose::All,
        /*collection_prim_path*/ "/city/geom",
        /*collection_name*/ "houses",
        /*binding_name*/ ""));

    // get the geom prim from the stage authored by Bifrost
    auto city = stage->GetPrimAtPath(pxr::SdfPath{"/city"});

    auto materialBindingAPI = pxr::UsdShadeMaterialBindingAPI::Apply(city);
    ASSERT_TRUE(materialBindingAPI);

    auto collectionBindingRels = materialBindingAPI.GetCollectionBindingRels();
    ASSERT_EQ(collectionBindingRels.size(), 1);

    pxr::SdfPathVector targets;
    ASSERT_TRUE(collectionBindingRels[0].GetTargets(&targets));
    ASSERT_EQ(targets.size(), 2);

    ASSERT_EQ(targets[0], pxr::SdfPath{"/city/geom.collection:houses"});
    ASSERT_EQ(targets[1], pxr::SdfPath{"/city/materials/red_mat"});
}

TEST(MaterialBindingNodeDefs, unbind_direct_binding) {
    auto pxrStage = pxr::UsdStage::CreateInMemory();

    auto gp  = pxrStage->OverridePrim(pxr::SdfPath{"/gp"});
    auto mat = pxr::UsdShadeMaterial::Define(pxrStage, pxr::SdfPath{"/mat"});

    auto materialBindingAPI = pxr::UsdShadeMaterialBindingAPI::Apply(gp);
    materialBindingAPI.Bind(mat);

    auto binding = materialBindingAPI.GetDirectBinding();
    ASSERT_TRUE(binding.GetMaterial());


    auto invalid_stage = BifrostUsd::Stage(BifrostUsd::Stage::Invalid());
    EXPECT_FALSE(USD::Shading::unbind_material(
        invalid_stage, "/gp",
        /*material_purpose*/ BifrostUsd::MaterialPurpose::All,
        /*binding_name*/ ""));

    auto stage =
        BifrostUsd::Stage(pxrStage->GetRootLayer()->GetIdentifier().c_str());
    ASSERT_TRUE(stage);

    EXPECT_FALSE(USD::Shading::unbind_material(
        stage, "I_DONT_EXIST",
        /*material_purpose*/ BifrostUsd::MaterialPurpose::All,
        /*binding_name*/ ""));


    EXPECT_TRUE(USD::Shading::unbind_material(
        stage, "/gp",
        /*material_purpose*/ BifrostUsd::MaterialPurpose::All,
        /*binding_name*/ ""));

    gp                 = stage->GetPrimAtPath(pxr::SdfPath{"/gp"});
    materialBindingAPI = pxr::UsdShadeMaterialBindingAPI::Apply(gp);

    binding = materialBindingAPI.GetDirectBinding();
    ASSERT_FALSE(binding.GetMaterial());
}

TEST(MaterialBindingNodeDefs, unbind_collection_binding) {
    auto pxrStage = create_pxr_stage_with_collection();

    auto geom     = pxrStage->GetPrimAtPath(pxr::SdfPath{"/city/geom"});
    auto material = pxr::UsdShadeMaterial::Get(
        pxrStage, pxr::SdfPath{"/city/materials/red_mat"});

    auto materialBindingAPI = pxr::UsdShadeMaterialBindingAPI::Apply(geom);

    auto collection = pxr::UsdCollectionAPI::Get(geom, pxr::TfToken{"houses"});

    materialBindingAPI.Bind(collection, material);

    auto collectionBindingVector = materialBindingAPI.GetCollectionBindings();
    ASSERT_EQ(collectionBindingVector.size(), 1);

    auto invalid_stage = BifrostUsd::Stage(BifrostUsd::Stage::Invalid());
    EXPECT_FALSE(USD::Shading::unbind_material(
        invalid_stage, "/city/geom",
        /*material_purpose*/ BifrostUsd::MaterialPurpose::All,
        /*binding_name*/ "houses"));

    auto stage =
        BifrostUsd::Stage(pxrStage->GetRootLayer()->GetIdentifier().c_str());
    ASSERT_TRUE(stage);

    EXPECT_FALSE(USD::Shading::unbind_material(
        stage, "I_DONT_EXIST",
        /*material_purpose*/ BifrostUsd::MaterialPurpose::All,
        /*binding_name*/ "houses"));

    EXPECT_TRUE(USD::Shading::unbind_material(
        stage, "/city/geom",
        /*material_purpose*/ BifrostUsd::MaterialPurpose::All,
        /*binding_name*/ "houses"));

    geom                    = stage->GetPrimAtPath(pxr::SdfPath{"/city/geom"});
    materialBindingAPI      = pxr::UsdShadeMaterialBindingAPI::Apply(geom);
    collectionBindingVector = materialBindingAPI.GetCollectionBindings();
    ASSERT_EQ(collectionBindingVector.size(), 0);
}

TEST(MaterialBindingNodeDefs, get_material_path) {
    // We setup a hierarchy in order to test direct biding on ancestor and child
    // This is a translation of the python unitest case
    // TestUsdShadeMaterialBinding.test_DirectBindingAncestorChild available in
    // pxr/usd/usdShade/testenv/testUsdShadeMaterialBinding.py of Pixar USD
    // repository.
    auto pxrStage = pxr::UsdStage::CreateInMemory();

    auto gp     = pxrStage->OverridePrim(pxr::SdfPath{"/gp"});
    auto parent = pxrStage->OverridePrim(pxr::SdfPath{"/gp/parent"});
    auto child  = pxrStage->OverridePrim(pxr::SdfPath{"/gp/parent/child"});

    auto mat1 = pxr::UsdShadeMaterial::Define(pxrStage, pxr::SdfPath{"/mat1"});
    auto mat2 = pxr::UsdShadeMaterial::Define(pxrStage, pxr::SdfPath{"/mat2"});

    auto gpBindingAPI     = pxr::UsdShadeMaterialBindingAPI::Apply(gp);
    auto parentBindingAPI = pxr::UsdShadeMaterialBindingAPI::Apply(parent);
    auto childBindingAPI  = pxr::UsdShadeMaterialBindingAPI::Apply(child);

    // First, binding different materials to the three prims
    // gp, parent and child and verify proper inheritance along
    // namespace.
    parentBindingAPI.Bind(mat1);

    auto stage = Amino::newClassPtr<BifrostUsd::Stage>(
        pxrStage->GetRootLayer()->GetIdentifier().c_str());
    ASSERT_TRUE(stage);

    const auto bif_gp = BifrostUsd::Prim{gp, stage};
    ASSERT_TRUE(bif_gp);

    const auto bif_parent = BifrostUsd::Prim{parent, stage};
    ASSERT_TRUE(bif_parent);

    const auto bif_child = BifrostUsd::Prim{child, stage};
    ASSERT_TRUE(bif_child);

    // testing binding on the three prims
    BifrostUsd::MaterialPurpose material_purpose{
        BifrostUsd::MaterialPurpose::All};
    Amino::String material_path{""};
    EXPECT_FALSE(USD::Shading::get_material_path(
        bif_gp, material_purpose,
        /*compute_bound_material*/ false, material_path));
    EXPECT_EQ(material_path, Amino::String{""});

    material_path = "";
    EXPECT_TRUE(USD::Shading::get_material_path(
        bif_parent, material_purpose,
        /*compute_bound_material*/ false, material_path));
    EXPECT_EQ(material_path, Amino::String{"/mat1"});

    material_path = "";
    EXPECT_FALSE(USD::Shading::get_material_path(
        bif_child, material_purpose,
        /*compute_bound_material*/ false, material_path));
    EXPECT_EQ(material_path, Amino::String{""});

    material_path = "";
    EXPECT_TRUE(USD::Shading::get_material_path(bif_child, material_purpose,
                                                /*compute_bound_material*/ true,
                                                material_path));
    EXPECT_EQ(material_path, Amino::String{"/mat1"});

    gpBindingAPI.Bind(mat2);

    material_path = "";
    EXPECT_TRUE(USD::Shading::get_material_path(
        bif_gp, material_purpose,
        /*compute_bound_material*/ false, material_path));
    EXPECT_EQ(material_path, Amino::String{"/mat2"});
    material_path = "";
    EXPECT_TRUE(USD::Shading::get_material_path(
        bif_parent, material_purpose,
        /*compute_bound_material*/ false, material_path));
    EXPECT_EQ(material_path, Amino::String{"/mat1"});
    material_path = "";
    EXPECT_FALSE(USD::Shading::get_material_path(
        bif_child, material_purpose,
        /*compute_bound_material*/ false, material_path));
    EXPECT_EQ(material_path, Amino::String{""});
    material_path = "";
    EXPECT_TRUE(USD::Shading::get_material_path(bif_child, material_purpose,
                                                /*compute_bound_material*/ true,
                                                material_path));
    EXPECT_EQ(material_path, Amino::String{"/mat1"});

    parentBindingAPI.UnbindAllBindings();

    material_path = "";
    EXPECT_TRUE(USD::Shading::get_material_path(
        bif_gp, material_purpose,
        /*compute_bound_material*/ false, material_path));
    EXPECT_EQ(material_path, Amino::String{"/mat2"});
    material_path = "";
    EXPECT_FALSE(USD::Shading::get_material_path(
        bif_parent, material_purpose,
        /*compute_bound_material*/ false, material_path));
    EXPECT_EQ(material_path, Amino::String{""});
    material_path = "";
    EXPECT_FALSE(USD::Shading::get_material_path(
        bif_child, material_purpose,
        /*compute_bound_material*/ false, material_path));
    EXPECT_EQ(material_path, Amino::String{""});
    material_path = "";
    EXPECT_TRUE(USD::Shading::get_material_path(bif_child, material_purpose,
                                                /*compute_bound_material*/ true,
                                                material_path));
    EXPECT_EQ(material_path, Amino::String{"/mat2"});

    childBindingAPI.Bind(mat1);

    material_path = "";
    EXPECT_TRUE(USD::Shading::get_material_path(
        bif_child, material_purpose,
        /*compute_bound_material*/ false, material_path));
    EXPECT_EQ(material_path, Amino::String{"/mat1"});
    material_path = "";
    EXPECT_TRUE(USD::Shading::get_material_path(bif_child, material_purpose,
                                                /*compute_bound_material*/ true,
                                                material_path));
    EXPECT_EQ(material_path, Amino::String{"/mat1"});
}
