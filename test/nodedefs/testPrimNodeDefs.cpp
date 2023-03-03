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
#include <bifusd/config/CfgWarningMacros.h>
#include <gtest/gtest.h>
#include <nodedefs/usd_pack/usd_layer_nodedefs.h>
#include <nodedefs/usd_pack/usd_prim_nodedefs.h>
#include <nodedefs/usd_pack/usd_stage_nodedefs.h>
#include <utils/test/testUtils.h>

// Note: To silence warnings coming from USD library
#include <bifusd/config/CfgWarningMacros.h>
BIFUSD_WARNING_PUSH
BIFUSD_WARNING_DISABLE_MSC(4003)
#include <pxr/usd/usd/inherits.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <pxr/usd/usd/modelAPI.h>
BIFUSD_WARNING_POP

#include <cstdlib>
#include <string>

using namespace BifrostUsd::TestUtils;

TEST(PrimNodeDefs, get_prim_at_path) {
    auto primPath = pxr::SdfPath("/a");

    // create an return an immutable stage
    Amino::Ptr<BifrostUsd::Stage> stage = [&primPath]() {
        auto out = Amino::newMutablePtr<BifrostUsd::Stage>();
        out->get().DefinePrim(primPath);
        return out;
    }();

    Amino::MutablePtr<BifrostUsd::Prim> prim;
    bool success = USD::Prim::get_prim_at_path(stage, primPath.GetText(), prim);
    ASSERT_TRUE(success);
    ASSERT_TRUE((*prim)->IsValid());
}

TEST(PrimNodeDefs, get_prim_children) {
    auto primPathA  = pxr::SdfPath("/a");
    auto primPathB  = pxr::SdfPath("/a/b");
    auto primPathB2 = pxr::SdfPath("/a/b2");
    auto primPathC  = pxr::SdfPath("/a/b/c");

    // create an return an immutable stage
    Amino::Ptr<BifrostUsd::Stage> stage = [&]() {
        auto out = Amino::newMutablePtr<BifrostUsd::Stage>();
        out->get().DefinePrim(primPathA);
        out->get().DefinePrim(primPathB);
        auto prim = out->get().DefinePrim(primPathB2);
        prim.SetActive(false);
        out->get().DefinePrim(primPathC);
        return out;
    }();

    Amino::MutablePtr<Amino::Array<Amino::Ptr<BifrostUsd::Prim>>> children;

    // Test Children of pseudo root
    USD::Prim::get_prim_children(stage, Amino::String("/"),
                                 BifrostUsd::UsdPrimChildren, children);

    ASSERT_EQ(children->size(), 1);

    // Passing an empty prim_path is a shortcut for the pseudo root "/"
    children.reset();
    USD::Prim::get_prim_children(stage, Amino::String(),
                                 BifrostUsd::UsdPrimChildren, children);

    ASSERT_EQ(children->size(), 1);

    // Test Children
    children.reset();
    USD::Prim::get_prim_children(stage, primPathA.GetText(),
                                 BifrostUsd::UsdPrimChildren, children);

    ASSERT_EQ(children->size(), 1);
    ASSERT_EQ((*children->at(0))->GetPath().GetString(), "/a/b");

    // Test AllChildren
    children.reset();
    USD::Prim::get_prim_children(stage, primPathA.GetText(),
                                 BifrostUsd::UsdPrimAllChildren, children);

    ASSERT_EQ(children->size(), 2);
    ASSERT_EQ((*children->at(0))->GetPath().GetString(), "/a/b");
    ASSERT_EQ((*children->at(1))->GetPath().GetString(), "/a/b2");

    // Test UsdPrimDescendants
    children.reset();
    USD::Prim::get_prim_children(stage, primPathA.GetText(),
                                 BifrostUsd::UsdPrimDescendants, children);

    ASSERT_EQ(children->size(), 2);
    ASSERT_EQ((*children->at(0))->GetPath().GetString(), "/a/b");
    ASSERT_EQ((*children->at(1))->GetPath().GetString(), "/a/b/c");

    // Test UsdPrimAllDescendants
    children.reset();
    USD::Prim::get_prim_children(stage, primPathA.GetText(),
                                 BifrostUsd::UsdPrimAllDescendants, children);

    ASSERT_EQ(children->size(), 3);
    ASSERT_EQ((*children->at(0))->GetPath().GetString(), "/a/b");
    ASSERT_EQ((*children->at(1))->GetPath().GetString(), "/a/b/c");
    ASSERT_EQ((*children->at(2))->GetPath().GetString(), "/a/b2");
}

TEST(PrimNodeDefs, get_prim_path) {
    auto stage_mut = Amino::newMutablePtr<BifrostUsd::Stage>();
    auto primPath  = pxr::SdfPath("/a");
    auto pxr_prim  = stage_mut->get().DefinePrim(primPath);
    Amino::Ptr<BifrostUsd::Stage> stage = std::move(stage_mut);

    BifrostUsd::Prim prim{pxr_prim, stage};
    Amino::String      result;
    USD::Prim::get_prim_path(prim, result);
    ASSERT_EQ(result, primPath.GetText());
}

TEST(PrimNodeDefs, get_prim_type) {
    auto stage_mut = Amino::newMutablePtr<BifrostUsd::Stage>();
    auto pxr_prim =
        stage_mut->get().DefinePrim(pxr::SdfPath("/a"), pxr::TfToken("Scope"));
    Amino::Ptr<BifrostUsd::Stage> stage = std::move(stage_mut);

    BifrostUsd::Prim prim{pxr_prim, stage};
    Amino::String      result;
    USD::Prim::get_prim_type(prim, result);
    ASSERT_EQ(result, "Scope");
}

TEST(PrimNodeDefs, get_all_attribute_names) {
    auto stage_mut = Amino::newMutablePtr<BifrostUsd::Stage>();
    auto pxr_prim =
        stage_mut->get().DefinePrim(pxr::SdfPath("/a"), pxr::TfToken("Scope"));
    pxr_prim.CreateAttribute(pxr::TfToken("attrName"), pxr::SdfValueTypeNames->String, false);

    BifrostUsd::Prim prim{pxr_prim, std::move(stage_mut)};
    Amino::MutablePtr<Amino::Array<Amino::String>> result;
    USD::Prim::get_all_attribute_names(prim, result);
    ASSERT_EQ(result->size(), 3);
    EXPECT_EQ((*result)[0], "attrName");
}

TEST(PrimNodeDefs, get_authored_attribute_names) {
    auto stage_mut = Amino::newMutablePtr<BifrostUsd::Stage>();
    auto pxr_prim  = stage_mut->get().DefinePrim(pxr::SdfPath("/a"),
                                                 pxr::TfToken("Capsule"));
    ASSERT_TRUE(pxr_prim);

    BifrostUsd::Prim prim{pxr_prim, std::move(stage_mut)};
    Amino::MutablePtr<Amino::Array<Amino::String>> result;
    USD::Prim::get_authored_attribute_names(prim, result);
    ASSERT_EQ(result->size(), 0);

    auto attr = pxr_prim.CreateAttribute(pxr::TfToken("radius"),
                                         pxr::SdfValueTypeNames->Float, false);
    attr.Set(3.14);

    USD::Prim::get_authored_attribute_names(prim, result);
    ASSERT_EQ(result->size(), 1);
    EXPECT_EQ((*result)[0], "radius");
}

TEST(PrimNodeDefs, create_prim) {
    Amino::String path = "/A";
    Amino::String type = "Scope";

    BifrostUsd::Stage stage;
    USD::Prim::create_prim(stage, path, type);
    ASSERT_TRUE(stage->GetPrimAtPath(pxr::SdfPath(path.c_str())));
}

TEST(PrimNodeDefs, create_class_prim) {
    BifrostUsd::Stage stage;
    Amino::String       path = "/A";
    USD::Prim::create_class_prim(stage, path);

    auto prim = stage->GetPrimAtPath(pxr::SdfPath(path.c_str()));
    ASSERT_TRUE(prim);
    ASSERT_EQ(prim.GetSpecifier(), pxr::SdfSpecifierClass);
}

TEST(PrimNodeDefs, add_applied_schema) {
    BifrostUsd::Stage stage;
    auto              primPath = pxr::SdfPath("/S");
    auto              prim     = stage->DefinePrim(primPath);

    ASSERT_EQ(prim.GetAppliedSchemas().size(), 0);

    Amino::String appliedSchemaName = "SkelBindingAPI";

    bool success = USD::Prim::add_applied_schema(stage, primPath.GetText(),
                                                 appliedSchemaName);
    EXPECT_TRUE(success);

    prim = stage->GetPrimAtPath(primPath);
    EXPECT_EQ(prim.GetAppliedSchemas().size(), 1);
    EXPECT_EQ(prim.GetAppliedSchemas()[0], pxr::TfToken{"SkelBindingAPI"});
}

TEST(PrimNodeDefs, remove_applied_schema) {
    BifrostUsd::Stage stage;
    auto              primPath          = pxr::SdfPath("/S");
    auto              prim              = stage->DefinePrim(primPath);
    Amino::String     appliedSchemaName = "SkelBindingAPI";
    ASSERT_TRUE(prim.AddAppliedSchema(pxr::TfToken{appliedSchemaName.c_str()}));
    ASSERT_EQ(prim.GetAppliedSchemas().size(), 1);

    bool success = USD::Prim::remove_applied_schema(stage, primPath.GetText(),
                                                    appliedSchemaName);
    EXPECT_TRUE(success);

    prim = stage->GetPrimAtPath(primPath);
    EXPECT_EQ(prim.GetAppliedSchemas().size(), 0);
}

TEST(PrimNodeDefs, add_reference_prim) {
    BifrostUsd::Stage stage;
    auto                primPath = pxr::SdfPath("/a");
    auto                prim     = stage->DefinePrim(primPath);

    BifrostUsd::Layer referenceLayer{getResourcePath("Mushroom1.usd").c_str(),
                                       ""};
    Amino::String       referencePrimPath = "/Mushroom1";
    double              layerOffset       = 0.0;
    double              layerOffsetScale  = 1.0;

    BifrostUsd::UsdListPosition referencePosition =
        BifrostUsd::UsdListPositionFrontOfPrependList;

    ASSERT_FALSE(pxr::UsdGeomMesh(prim));
    bool success = USD::Prim::add_reference_prim(
        stage, primPath.GetText(), referenceLayer, referencePrimPath,
        layerOffset, layerOffsetScale, referencePosition);
    ASSERT_TRUE(success);

    prim = stage->GetPrimAtPath(primPath);
    ASSERT_TRUE(pxr::UsdGeomMesh(prim));
}

TEST(PrimNodeDefs, remove_reference_prim) {
    {
        Amino::String identifier =
            getResourcePath("referenced_tree1.usda").c_str();
        BifrostUsd::Stage stage{identifier.c_str()};

        auto primPath = pxr::SdfPath("/Tree");
        auto prim     = stage->GetPrimAtPath(primPath);
        ASSERT_EQ(prim.GetTypeName(), pxr::TfToken("Mesh"));

        Amino::String referenceIdentifier = "Tree1.usd";
        Amino::String referencePrimPath   = "/Tree1";

        double layerOffset = 0.0;
        bool   clear_all   = false;

        bool success = USD::Prim::remove_reference_prim(
            stage, primPath.GetText(), referenceIdentifier, referencePrimPath,
            layerOffset, clear_all);
        ASSERT_TRUE(success);
        prim = stage->GetPrimAtPath(primPath);
        ASSERT_EQ(prim.GetTypeName(), pxr::TfToken(""));
    }
    {
        BifrostUsd::Stage stage;
        auto                primPath = pxr::SdfPath("/a");
        auto                prim     = stage->DefinePrim(primPath);

        Amino::String       referenceIdentifier = "AnonLayer.usd";
        BifrostUsd::Layer referenceLayer{referenceIdentifier};
        auto                referenceStage =
            Amino::newMutablePtr<BifrostUsd::Stage>(referenceLayer);
        auto          referencePrimPath = pxr::SdfPath("/b");
        Amino::String referencePrimType = "Capsule";
        referenceStage->get().DefinePrim(
            referencePrimPath, pxr::TfToken(referencePrimType.c_str()));

        ASSERT_TRUE(referenceStage->get().GetRootLayer()->GetPrimAtPath(
            referencePrimPath));

        BifrostUsd::UsdListPosition referencePosition =
            BifrostUsd::UsdListPositionFrontOfPrependList;
        double layerOffset      = 0.0;
        double layerOffsetScale = 1.0;

        ASSERT_EQ(prim.GetTypeName(), pxr::TfToken(""));
        bool success = USD::Prim::add_reference_prim(
            stage, primPath.GetText(), *referenceStage->getRootLayer(),
            referencePrimPath.GetText(), layerOffset, layerOffsetScale,
            referencePosition);
        ASSERT_TRUE(success);

        prim = stage->GetPrimAtPath(primPath);
        ASSERT_EQ(prim.GetTypeName(), pxr::TfToken(referencePrimType.c_str()));

        bool clear_all = false;
        success        = USD::Prim::remove_reference_prim(
            stage, primPath.GetText(),
            referenceStage->get().GetRootLayer()->GetIdentifier().c_str(),
            referencePrimPath.GetText(), layerOffset, clear_all);
        ASSERT_TRUE(success);
        prim = stage->GetPrimAtPath(primPath);
        ASSERT_EQ(prim.GetTypeName(), pxr::TfToken(""));
    }
}

TEST(PrimNodeDefs, add_reference_prim_in_variant) {
    BifrostUsd::Stage stage;
    auto                prim = stage->DefinePrim(pxr::SdfPath("/top"));
    auto                vset = prim.GetVariantSets().AddVariantSet("vset");
    vset.AddVariant("no_ref");
    vset.AddVariant("with_ref");

    vset.SetVariantSelection("with_ref");
    stage.last_modified_variant_set_prim = prim.GetPath().GetText();
    stage.last_modified_variant_set_name = "vset";
    stage.last_modified_variant_name     = "with_ref";

    auto primInVariantPath = pxr::SdfPath("/top/a");
    prim                   = stage->DefinePrim(primInVariantPath);

    BifrostUsd::Layer referenceLayer{getResourcePath("Mushroom1.usd").c_str(),
                                       ""};
    Amino::String       referencePrimPath = "/Mushroom1";
    double              layerOffset       = 0.0;
    double              layerOffsetScale  = 1.0;

    BifrostUsd::UsdListPosition referencePosition =
        BifrostUsd::UsdListPositionFrontOfPrependList;

    ASSERT_FALSE(pxr::UsdGeomMesh(prim));
    bool success = USD::Prim::add_reference_prim(
        stage, primInVariantPath.GetText(), referenceLayer, referencePrimPath,
        layerOffset, layerOffsetScale, referencePosition);
    ASSERT_TRUE(success);

    prim = stage->GetPrimAtPath(primInVariantPath);
    ASSERT_TRUE(pxr::UsdGeomMesh(prim));

    auto vsetPrim = stage->GetPrimAtPath(pxr::SdfPath("/top"));
    vset          = vsetPrim.GetVariantSets().GetVariantSet("vset");
    vset.SetVariantSelection("no_ref");

    prim = stage->GetPrimAtPath(primInVariantPath);
    ASSERT_FALSE(pxr::UsdGeomMesh(prim));
}

TEST(PrimNodeDefs, add_payload_prim) {
    BifrostUsd::Stage stage;
    auto                primPath = pxr::SdfPath("/a");
    auto                prim     = stage->DefinePrim(primPath);

    BifrostUsd::Layer payloadLayer{getResourcePath("Mushroom1.usd").c_str(),
                                     ""};
    Amino::String       payloadPrimPath  = "/Mushroom1";
    double              layerOffset      = 0.0;
    double              layerOffsetScale = 1.0;

    BifrostUsd::UsdListPosition payloadPosition =
        BifrostUsd::UsdListPositionFrontOfPrependList;

    ASSERT_FALSE(prim.HasPayload());
    ASSERT_FALSE(pxr::UsdGeomMesh(prim));
    bool success = USD::Prim::add_payload_prim(
        stage, primPath.GetText(), payloadLayer, payloadPrimPath, layerOffset,
        layerOffsetScale, payloadPosition);
    ASSERT_TRUE(success);

    prim = stage->GetPrimAtPath(primPath);
    ASSERT_TRUE(prim.HasPayload());
    ASSERT_TRUE(pxr::UsdGeomMesh(prim));
}

TEST(PrimNodeDefs, add_inherit_prim) {
    BifrostUsd::Stage stage;
    auto                clsPath = pxr::SdfPath("/cls");
    stage->CreateClassPrim(clsPath);

    auto primPath = pxr::SdfPath("/a");
    stage->DefinePrim(primPath);

    bool success = USD::Prim::add_inherit_prim(
        stage, primPath.GetText(), clsPath.GetText(),
        BifrostUsd::UsdListPositionFrontOfPrependList);

    ASSERT_TRUE(success);
    auto prim = stage->GetPrimAtPath(primPath);
    ASSERT_EQ(prim.GetInherits().GetAllDirectInherits().size(), 1);
    ASSERT_EQ(prim.GetInherits().GetAllDirectInherits().at(0), clsPath);
}

TEST(PrimNodeDefs, add_specialize_prim) {
    BifrostUsd::Stage stage;
    auto                clsPath = pxr::SdfPath("/cls");
    stage->CreateClassPrim(clsPath);

    auto primPath = pxr::SdfPath("/a");
    stage->DefinePrim(primPath);
    auto prim = stage->GetPrimAtPath(primPath);
    ASSERT_TRUE(prim);
    ASSERT_FALSE(prim.HasAuthoredSpecializes());

    bool success = USD::Prim::add_specialize_prim(
        stage, primPath.GetText(), clsPath.GetText(),
        BifrostUsd::UsdListPositionFrontOfPrependList);
    ASSERT_TRUE(success);
    ASSERT_TRUE(prim.HasAuthoredSpecializes());
}

TEST(PrimNodeDefs, create_prim_relationship) {
    BifrostUsd::Stage stage;

    auto primPath = pxr::SdfPath("/a");
    auto prim     = stage->DefinePrim(primPath);
    auto attr =
        prim.CreateAttribute(pxr::TfToken("a"), pxr::SdfValueTypeNames->String,
                             /* custom */ false);
    auto targetPath = pxr::SdfPath("/b");
    stage->DefinePrim(targetPath);

    Amino::String relName = "rel";
    bool          custom  = true;
    Amino::String target;

    bool success = USD::Prim::create_prim_relationship(
        stage, primPath.GetText(), relName, custom, targetPath.GetText(),
        BifrostUsd::UsdListPositionFrontOfPrependList);
    ASSERT_TRUE(success);

    prim = stage->GetPrimAtPath(primPath);
    ASSERT_TRUE(prim);
    auto rel = prim.GetRelationship(pxr::TfToken(relName.c_str()));
    ASSERT_TRUE(rel);

    pxr::SdfPathVector targets;
    ASSERT_TRUE(rel.GetTargets(&targets));
    ASSERT_EQ(targets.size(), 1);
    ASSERT_EQ(targets.at(0), targetPath);
}

TEST(PrimNodeDefs, add_relationship_target) {
    BifrostUsd::Stage stage;

    auto primPath = pxr::SdfPath("/a");
    auto prim     = stage->DefinePrim(primPath);
    auto attr =
        prim.CreateAttribute(pxr::TfToken("a"), pxr::SdfValueTypeNames->String,
                             /* custom */ false);
    auto targetPath = pxr::SdfPath("/b");
    stage->DefinePrim(targetPath);

    Amino::String relName = "rel";
    auto rel = prim.CreateRelationship(pxr::TfToken(relName.c_str()), true);
    ASSERT_TRUE(rel);

    bool success = USD::Prim::add_relationship_target(
        stage, primPath.GetText(), relName, targetPath.GetText(),
        BifrostUsd::UsdListPositionFrontOfPrependList);
    ASSERT_TRUE(success);
    pxr::SdfPathVector targets;
    ASSERT_TRUE(rel.GetTargets(&targets));
    ASSERT_EQ(targets.size(), 1);
    ASSERT_EQ(targets.at(0), targetPath);
}

TEST(PrimNodeDefs, remove_relationship_target) {
    BifrostUsd::Stage stage;

    auto primPath = pxr::SdfPath("/a");
    auto prim     = stage->DefinePrim(primPath);
    auto attr =
        prim.CreateAttribute(pxr::TfToken("a"), pxr::SdfValueTypeNames->String,
                             /* custom */ false);
    auto targetPath = pxr::SdfPath("/b");
    stage->DefinePrim(targetPath);

    Amino::String relName = "rel";
    auto rel = prim.CreateRelationship(pxr::TfToken(relName.c_str()), true);
    ASSERT_TRUE(rel);

    bool success = USD::Prim::add_relationship_target(
        stage, primPath.GetText(), relName, targetPath.GetText(),
        BifrostUsd::UsdListPositionFrontOfPrependList);
    ASSERT_TRUE(success);
    pxr::SdfPathVector targets;
    ASSERT_TRUE(rel.GetTargets(&targets));
    ASSERT_EQ(targets.size(), 1);
    ASSERT_EQ(targets.at(0), targetPath);

    success = USD::Prim::remove_relationship_target(
        stage, primPath.GetText(), relName, targetPath.GetText());
    ASSERT_TRUE(success);
    ASSERT_TRUE(rel.GetTargets(&targets));
    ASSERT_EQ(targets.size(), 0);
}

TEST(PrimNodeDefs, set_relationship_targets) {
    BifrostUsd::Stage stage;

    auto primPath = pxr::SdfPath("/a");
    auto prim     = stage->DefinePrim(primPath);
    auto attr =
        prim.CreateAttribute(pxr::TfToken("a"), pxr::SdfValueTypeNames->String,
                             /* custom */ false);
    auto targetPath1 = pxr::SdfPath("/t1");
    stage->DefinePrim(targetPath1);
    auto targetPath2 = pxr::SdfPath("/t2");
    stage->DefinePrim(targetPath2);
    auto targetPath3 = pxr::SdfPath("/t3");
    stage->DefinePrim(targetPath3);

    Amino::String relName = "rel";
    auto rel = prim.CreateRelationship(pxr::TfToken(relName.c_str()), true);
    ASSERT_TRUE(rel);

    auto targets = Amino::Array<Amino::String>{
        targetPath1.GetText(), targetPath2.GetText(), targetPath3.GetText()};
    bool success = USD::Prim::set_relationship_targets(
        stage, primPath.GetText(), relName, targets);
    ASSERT_TRUE(success);
    pxr::SdfPathVector pxrTargets;
    ASSERT_TRUE(rel.GetTargets(&pxrTargets));
    ASSERT_EQ(pxrTargets.size(), 3);
    ASSERT_EQ(pxrTargets.at(0), targetPath1);
    ASSERT_EQ(pxrTargets.at(1), targetPath2);
    ASSERT_EQ(pxrTargets.at(2), targetPath3);
}

TEST(PrimNodeDefs, clear_relationship_targets) {
    BifrostUsd::Stage stage;

    auto primPath = pxr::SdfPath("/a");
    auto prim     = stage->DefinePrim(primPath);
    auto attr =
        prim.CreateAttribute(pxr::TfToken("a"), pxr::SdfValueTypeNames->String,
                             /* custom */ false);
    auto targetPath1 = pxr::SdfPath("/t1");
    stage->DefinePrim(targetPath1);
    auto targetPath2 = pxr::SdfPath("/t2");
    stage->DefinePrim(targetPath2);
    auto targetPath3 = pxr::SdfPath("/t3");
    stage->DefinePrim(targetPath3);

    Amino::String relName = "rel";
    auto rel = prim.CreateRelationship(pxr::TfToken(relName.c_str()), true);
    ASSERT_TRUE(rel);

    auto targets = Amino::Array<Amino::String>{
        targetPath1.GetText(), targetPath2.GetText(), targetPath3.GetText()};
    bool success = USD::Prim::set_relationship_targets(
        stage, primPath.GetText(), relName, targets);
    ASSERT_TRUE(success);
    pxr::SdfPathVector pxrTargets;
    ASSERT_TRUE(rel.GetTargets(&pxrTargets));
    ASSERT_EQ(pxrTargets.size(), 3);

    // Clear targets without removing relationship prim spec
    success = USD::Prim::clear_relationship_targets(stage, primPath.GetText(),
                                                    relName, false);
    ASSERT_TRUE(success);
    ASSERT_FALSE(rel.GetTargets(&pxrTargets));
    rel = prim.GetRelationship(pxr::TfToken(relName.c_str()));
    ASSERT_TRUE(rel);

    // re-add the targets
    success = USD::Prim::set_relationship_targets(stage, primPath.GetText(),
                                                  relName, targets);
    ASSERT_TRUE(success);
    ASSERT_TRUE(rel.GetTargets(&pxrTargets));
    ASSERT_EQ(pxrTargets.size(), 3);

    // Now Clear the targets and remove relationship prim spec
    success = USD::Prim::clear_relationship_targets(stage, primPath.GetText(),
                                                    relName, true);
    ASSERT_TRUE(success);
    ASSERT_FALSE(rel.GetTargets(&pxrTargets));
    rel = prim.GetRelationship(pxr::TfToken(relName.c_str()));
    ASSERT_FALSE(rel);
}

TEST(PrimNodeDefs, get_relationship_targets) {
    BifrostUsd::Stage stage;

    auto primPath = pxr::SdfPath("/a");
    auto prim     = stage->DefinePrim(primPath);
    auto attr =
        prim.CreateAttribute(pxr::TfToken("a"), pxr::SdfValueTypeNames->String,
                             /* custom */ false);
    auto targetPath1 = pxr::SdfPath("/t1");
    stage->DefinePrim(targetPath1);
    auto targetPath2 = pxr::SdfPath("/t2");
    stage->DefinePrim(targetPath2);
    auto targetPath3 = pxr::SdfPath("/t3");
    stage->DefinePrim(targetPath3);

    Amino::String relName = "rel";
    auto rel = prim.CreateRelationship(pxr::TfToken(relName.c_str()), true);
    ASSERT_TRUE(rel);

    auto targets = Amino::Array<Amino::String>{
        targetPath1.GetText(), targetPath2.GetText(), targetPath3.GetText()};
    bool success = USD::Prim::set_relationship_targets(
        stage, primPath.GetText(), relName, targets);
    ASSERT_TRUE(success);

    Amino::MutablePtr<Amino::Array<Amino::String>> outTargets;
    success = USD::Prim::get_relationship_targets(stage, primPath.GetText(),
                                                  relName, outTargets);
    ASSERT_TRUE(success);
    ASSERT_EQ(outTargets->size(), 3);
    ASSERT_STREQ(outTargets->at(0).c_str(), targets[0].c_str());
    ASSERT_STREQ(outTargets->at(1).c_str(), targets[1].c_str());
    ASSERT_STREQ(outTargets->at(2).c_str(), targets[2].c_str());
}

TEST(PrimNodeDefs, get_forwarded_relationship_targets) {
    BifrostUsd::Stage stage;

    auto attrPrimPath = pxr::SdfPath("/a");
    auto attrPrim     = stage->DefinePrim(attrPrimPath);
    auto attr         = attrPrim.CreateAttribute(pxr::TfToken("a"),
                                         pxr::SdfValueTypeNames->String,
                                         /* custom */ false);

    auto          relPrimPath = pxr::SdfPath("/b");
    auto          relPrim     = stage->DefinePrim(relPrimPath);
    Amino::String relName     = "rel";
    auto rel = relPrim.CreateRelationship(pxr::TfToken(relName.c_str()));
    ASSERT_TRUE(rel);
    pxr::SdfPathVector targets;
    ASSERT_FALSE(rel.GetTargets(&targets));
    ASSERT_TRUE(targets.empty());
    Amino::MutablePtr<Amino::Array<Amino::String>> outTargets;
    bool success = USD::Prim::get_forwarded_relationship_targets(
        stage, relPrimPath.GetText(), relName, outTargets);
    ASSERT_FALSE(success);
    ASSERT_EQ(outTargets->size(), 0);

    Amino::String forwardingRelName = "forwardingRel";
    auto          forwardingRel =
        relPrim.CreateRelationship(pxr::TfToken(forwardingRelName.c_str()));
    // Add a target to the previous relationship, GetTargets
    // returns true and gets the targeted relationship. However
    // GetForwardedTargets returns false because the only target is a
    // relationship that has no authored targets
    ASSERT_TRUE(forwardingRel.AddTarget(rel.GetPath()));
    ASSERT_TRUE(forwardingRel.GetTargets(&targets));
    ASSERT_TRUE(targets == pxr::SdfPathVector({rel.GetPath()}));
    success = USD::Prim::get_forwarded_relationship_targets(
        stage, relPrimPath.GetText(), forwardingRelName, outTargets);
    ASSERT_FALSE(success);
    ASSERT_EQ(outTargets->size(), 0);

    ASSERT_TRUE(rel.AddTarget(attrPrimPath));
    ASSERT_TRUE(rel.GetTargets(&targets));
    ASSERT_TRUE(targets == pxr::SdfPathVector({attrPrimPath}));
    success = USD::Prim::get_forwarded_relationship_targets(
        stage, relPrimPath.GetText(), relName, outTargets);
    ASSERT_TRUE(success);
    ASSERT_EQ(outTargets->size(), 1);
    ASSERT_STREQ(outTargets->at(0).c_str(), attrPrimPath.GetText());
    // GetForwardedTargets on the forwarding relationship also returns true
    // because its targeted  relation now has targets.
    success = USD::Prim::get_forwarded_relationship_targets(
        stage, relPrimPath.GetText(), forwardingRelName, outTargets);
    ASSERT_TRUE(success);
    ASSERT_EQ(outTargets->size(), 1);
    ASSERT_STREQ(outTargets->at(0).c_str(), attrPrimPath.GetText());
}

TEST(PrimNodeDefs, create_prim_relationship_in_variant) {
    BifrostUsd::Stage stage;

    auto primPath = pxr::SdfPath("/a");
    auto prim     = stage->DefinePrim(primPath);
    auto attr =
        prim.CreateAttribute(pxr::TfToken("a"), pxr::SdfValueTypeNames->String,
                             /* custom */ false);
    auto targetPath = pxr::SdfPath("/b");
    stage->DefinePrim(targetPath);

    auto vset = prim.GetVariantSets().AddVariantSet("vset");
    vset.AddVariant("no_rel");
    vset.AddVariant("with_rel");

    vset.SetVariantSelection("with_rel");
    stage.last_modified_variant_set_prim = prim.GetPath().GetText();
    stage.last_modified_variant_set_name = "vset";
    stage.last_modified_variant_name     = "with_rel";

    Amino::String relName = "rel";
    bool          custom  = true;
    Amino::String target;

    bool success = USD::Prim::create_prim_relationship(
        stage, primPath.GetText(), relName, custom, targetPath.GetText(),
        BifrostUsd::UsdListPositionFrontOfPrependList);
    ASSERT_TRUE(success);

    prim = stage->GetPrimAtPath(primPath);
    ASSERT_TRUE(prim);
    auto rel = prim.GetRelationship(pxr::TfToken("rel"));
    ASSERT_TRUE(rel);

    pxr::SdfPathVector targets;
    ASSERT_TRUE(rel.GetTargets(&targets));
    ASSERT_EQ(targets.size(), 1);
    ASSERT_EQ(targets.at(0), targetPath);

    vset = prim.GetVariantSets().GetVariantSet("vset");
    vset.SetVariantSelection("no_rel");
    stage->GetRootLayer()->Export(getTestOutputPath("rel.usda").c_str());

    rel = prim.GetRelationship(pxr::TfToken("rel"));
    ASSERT_FALSE(rel);
}

TEST(PrimNodeDefs, set_prim_active) {
    BifrostUsd::Stage stage;
    auto                primPath = pxr::SdfPath("/a");
    auto                prim     = stage->DefinePrim(primPath);
    ASSERT_TRUE(prim.IsActive());

    bool success = USD::Prim::set_prim_active(stage, primPath.GetText(), false);
    ASSERT_TRUE(success);
    prim = stage->DefinePrim(primPath);
    ASSERT_FALSE(prim.IsActive());
}

TEST(PrimNodeDefs, prim_metadata) {
    BifrostUsd::Stage stage{getResourcePath("helloworld.usd")};
    ASSERT_TRUE(stage);
    auto primPath    = Amino::String{"/hello"};
    auto pxrPrimPath = pxr::SdfPath(primPath.c_str());
    auto prim        = stage->GetPrimAtPath(pxrPrimPath);
    // Test String
    {
        auto docVal  = Amino::String{"This is my documentation"};
        bool success = USD::Prim::set_prim_metadata(stage, primPath,
                                                    "documentation", docVal);
        ASSERT_TRUE(success);
        ASSERT_TRUE(stage);
        Amino::String strDefault = "Oups an error!";
        Amino::String strResult;
        success = USD::Prim::get_prim_metadata(stage, primPath, "documentation",
                                               strDefault, strResult);
        ASSERT_TRUE(success);
        ASSERT_STREQ(docVal.c_str(), strResult.c_str());
    }
    // Test bool
    {
        bool hiddenVal = true;
        bool success =
            USD::Prim::set_prim_metadata(stage, primPath, "hidden", hiddenVal);
        ASSERT_TRUE(success);
        ASSERT_TRUE(stage);
        bool boolDefault = false;
        bool boolResult  = false;
        success = USD::Prim::get_prim_metadata(stage, primPath, "hidden",
                                               boolDefault, boolResult);
        ASSERT_TRUE(success);
        ASSERT_EQ(hiddenVal, boolResult);
    }
    // Test in64
    {
        Amino::long_t int64Value = 3223372036854775807;
        bool          success    = USD::Prim::set_prim_metadata(stage, primPath,
                                                    "customData", int64Value);
        ASSERT_TRUE(success);
        ASSERT_TRUE(stage);
        Amino::long_t int64Default = 0;
        Amino::long_t int64Result  = 0;
        success = USD::Prim::get_prim_metadata(stage, primPath, "customData",
                                               int64Default, int64Result);
        ASSERT_TRUE(success);
        ASSERT_EQ(int64Value, int64Result);
    }
    // Test Bifrost::Object
    {
        auto         objValue    = Bifrost::createObject();
        Amino::int_t intValue    = 7;
        auto         subObjValue = Bifrost::createObject();
        subObjValue->setProperty("my_int", intValue);
        Amino::long_t   int64Value = 10333222111;
        Amino::String   strValue("foo");
        Amino::float_t  fltValue  = 749.3f;
        Amino::double_t dblValue  = 1001.0;
        bool            boolValue = true;
        objValue->setProperty("my_subObject", std::move(subObjValue));
        objValue->setProperty("my_int64", int64Value);
        objValue->setProperty("my_string", strValue);
        objValue->setProperty("my_float", fltValue);
        objValue->setProperty("my_double", dblValue);
        objValue->setProperty("my_bool", boolValue);
        bool success = USD::Prim::set_prim_metadata(stage, primPath,
                                                    "customData", *objValue);
        ASSERT_TRUE(success);
        ASSERT_TRUE(stage);
        Amino::Ptr<Bifrost::Object> objDefault = Bifrost::createObject();
        Amino::Ptr<Bifrost::Object> objResult;

        success = USD::Prim::get_prim_metadata(stage, primPath, "customData",
                                               objDefault, objResult);
        ASSERT_TRUE(success);
        ASSERT_TRUE(objResult);
        auto any         = objResult->getProperty("my_subObject");
        auto mySubObject = Amino::any_cast<Amino::Ptr<Bifrost::Object>>(&any);
        ASSERT_TRUE(*mySubObject);
        any        = (*mySubObject)->getProperty("my_int");
        auto myInt = Amino::any_cast<Amino::int_t>(&any);
        ASSERT_TRUE(myInt);
        ASSERT_EQ(*myInt, intValue);
        any          = objResult->getProperty("my_int64");
        auto myInt64 = Amino::any_cast<Amino::long_t>(&any);
        ASSERT_TRUE(myInt64);
        ASSERT_EQ(*myInt64, int64Value);
        any           = objResult->getProperty("my_string");
        auto myString = Amino::any_cast<Amino::String>(&any);
        ASSERT_TRUE(myString);
        ASSERT_STREQ((*myString).c_str(), strValue.c_str());
        any          = objResult->getProperty("my_float");
        auto myFloat = Amino::any_cast<Amino::float_t>(&any);
        ASSERT_TRUE(myFloat);
        ASSERT_FLOAT_EQ(*myFloat, fltValue);
        any           = objResult->getProperty("my_double");
        auto myDouble = Amino::any_cast<Amino::double_t>(&any);
        ASSERT_TRUE(myDouble);
        ASSERT_DOUBLE_EQ(*myDouble, dblValue);
        any         = objResult->getProperty("my_bool");
        auto myBool = Amino::any_cast<Amino::bool_t>(&any);
        ASSERT_TRUE(myBool);
        ASSERT_EQ(*myBool, boolValue);
    }
}

TEST(PrimNodeDefs, int64_metadata_export) {
    BifrostUsd::Stage stage;
    auto                primPath    = Amino::String{"/a"};
    auto                pxrPrimPath = pxr::SdfPath(primPath.c_str());
    auto                prim        = stage->DefinePrim(pxrPrimPath);
    ASSERT_TRUE(prim);
    auto          objValue   = Bifrost::createObject();
    Amino::long_t int64Value = 5223372036854775808;
    ASSERT_TRUE(objValue->setProperty("my_int64", int64Value));
    bool success =
        USD::Prim::set_prim_metadata(stage, primPath, "customData", *objValue);
    ASSERT_TRUE(success);
    ASSERT_TRUE(stage);
    std::string result;
    ASSERT_TRUE(stage->GetRootLayer()->ExportToString(&result));
    // clang-format off
    const char* exportedLayer = R"usda(#usda 1.0

def "a" (
    customData = {
        int64 my_int64 = 5223372036854775808
    }
)
{
}

)usda";
    // clang-format on
    ASSERT_STREQ(result.c_str(), exportedLayer);
}

TEST(PrimNodeDefs, set_prim_asset_info) {
    BifrostUsd::Stage stage;
    auto              primPath = pxr::SdfPath("/abc");
    auto              prim     = stage->DefinePrim(primPath);

    Amino::String asset_identifier{"abc/abc.usd"};
    Amino::String asset_name{"abc"};
    Amino::String asset_version{"v001"};

    USD::Prim::set_prim_asset_info(stage, primPath.GetText(), asset_identifier,
                                   asset_name, asset_version);

    prim                       = stage->GetPrimAtPath(primPath);
    auto              modelAPI = pxr::UsdModelAPI(prim);
    pxr::SdfAssetPath expected_asset_identifier;
    EXPECT_TRUE(modelAPI.GetAssetIdentifier(&expected_asset_identifier));
    EXPECT_EQ(expected_asset_identifier, pxr::SdfAssetPath{"abc/abc.usd"});

    std::string expected_asset_name;
    EXPECT_TRUE(modelAPI.GetAssetName(&expected_asset_name));
    EXPECT_EQ(expected_asset_name, std::string{"abc"});

    std::string expected_asset_version;
    EXPECT_TRUE(modelAPI.GetAssetVersion(&expected_asset_version));
    EXPECT_EQ(expected_asset_version, std::string{"v001"});
}

TEST(PrimNodeDefs, get_prim_asset_info) {
    BifrostUsd::Stage stage;
    auto              primPath = pxr::SdfPath("/abc");
    auto              prim     = stage->DefinePrim(primPath);
    auto              modelAPI = pxr::UsdModelAPI(prim);
    modelAPI.SetAssetIdentifier(pxr::SdfAssetPath("abc/abc.usd"));
    modelAPI.SetAssetName("abc");
    modelAPI.SetAssetVersion("v001");

    Amino::String asset_identifier;
    Amino::String asset_name;
    Amino::String asset_version;

    USD::Prim::get_prim_asset_info(stage, primPath.GetText(), asset_identifier,
                                   asset_name, asset_version);
    ASSERT_EQ(asset_identifier, "abc/abc.usd");
    ASSERT_EQ(asset_name, "abc");
    ASSERT_EQ(asset_version, "v001");

    // test if setting asset version only (passing empty string for other
    // parameters) is not setting asset identifier and name.
    asset_identifier = "";
    asset_name       = "";
    asset_version    = "v002"; // non-empty string should update version!

    USD::Prim::set_prim_asset_info(stage, primPath.GetText(), asset_identifier,
                                   asset_name, asset_version);

    prim     = stage->GetPrimAtPath(primPath);
    modelAPI = pxr::UsdModelAPI(prim);
    pxr::SdfAssetPath expected_asset_identifier;
    EXPECT_TRUE(modelAPI.GetAssetIdentifier(&expected_asset_identifier));
    EXPECT_EQ(expected_asset_identifier, pxr::SdfAssetPath{"abc/abc.usd"});
    std::string expected_asset_name;
    EXPECT_TRUE(modelAPI.GetAssetName(&expected_asset_name));
    EXPECT_EQ(expected_asset_name, std::string{"abc"});

    std::string expected_asset_version;
    EXPECT_TRUE(modelAPI.GetAssetVersion(&expected_asset_version));
    EXPECT_EQ(expected_asset_version, std::string{"v002"});
}
