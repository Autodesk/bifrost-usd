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
#include <nodedefs/usd_pack/usd_attribute_nodedefs.h>
#include <nodedefs/usd_pack/usd_geom_nodedefs.h>
#include <nodedefs/usd_pack/usd_layer_nodedefs.h>
#include <nodedefs/usd_pack/usd_prim_nodedefs.h>
#include <nodedefs/usd_pack/usd_stage_nodedefs.h>
#include <utils/test/testUtils.h>

BIFUSD_WARNING_PUSH
BIFUSD_WARNING_DISABLE_MSC(4003)
#include <pxr/usd/usdGeom/boundable.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

BIFUSD_WARNING_POP

#include <cstdlib>
#include <string>

using namespace BifrostUsd::TestUtils;

TEST(GeomNodeDefs, get_xform_vectors) {
    auto kitchenPropsFilePath = getResourcePath(
        Amino::Array<Amino::String>{"kitchen_set", "kitchen_props.usd"});
    auto pxrStage = PXR_NS::UsdStage::Open(kitchenPropsFilePath.c_str());
    auto pxrPrim =
        pxrStage->GetPrimAtPath(PXR_NS::SdfPath("/Props_grp/MeasuringSpoon"));
    ASSERT_TRUE(pxrPrim);
    auto layer =
        Amino::newClassPtr<BifrostUsd::Layer>(pxrStage->GetRootLayer());
    auto loadall = BifrostUsd::InitialLoadSet::LoadAll;
    auto stage   = Amino::newClassPtr<BifrostUsd::Stage>(*layer, loadall);
    BifrostUsd::Prim prim{pxrPrim, stage};
    float              frame = 0;

    Bifrost::Math::float3         translation, rotation, scale, pivot;
    Bifrost::Math::rotation_order rotation_order;

    bool success = USD::Prim::get_usd_geom_xform_vectors(
        prim, frame, translation, rotation, scale, pivot, rotation_order);
    ASSERT_TRUE(success);

    ASSERT_EQ(translation.x, 0.f);
    ASSERT_EQ(translation.y, -10.f);
    ASSERT_EQ(translation.z, 0.f);

    ASSERT_EQ(rotation.x, 180.f);
    ASSERT_EQ(rotation.y, 3.0688782f);
    ASSERT_EQ(rotation.z, 180.f);

    ASSERT_EQ(scale.x, 1.f);
    ASSERT_EQ(scale.y, 1.f);
    ASSERT_EQ(scale.z, 1.f);

    ASSERT_EQ(pivot.x, 0.f);
    ASSERT_EQ(pivot.y, 0.f);
    ASSERT_EQ(pivot.z, 0.f);

    ASSERT_EQ(rotation_order, Bifrost::Math::rotation_order::XYZ);
}

TEST(GeomNodeDefs, usd_boundable) {
    auto bottleFilePath = getResourcePath(Amino::Array<Amino::String>{
        "kitchen_set", "assets", "Bottle", "Bottle.usd"});
    auto pxrStage       = PXR_NS::UsdStage::Open(bottleFilePath.c_str());
    auto pxrPrim = pxrStage->GetPrimAtPath(PXR_NS::SdfPath("/Bottle/Geom/Bottle"));
    ASSERT_TRUE(pxrPrim);
    auto layer =
        Amino::newClassPtr<BifrostUsd::Layer>(pxrStage->GetRootLayer());
    auto loadall = BifrostUsd::InitialLoadSet::LoadAll;
    auto stage   = Amino::newClassPtr<BifrostUsd::Stage>(*layer, loadall);
    BifrostUsd::Prim                                     prim{pxrPrim, stage};
    bool                                                   local = true;
    float                                                  frame = 0;
    Amino::MutablePtr<Amino::Array<Bifrost::Math::float3>> extent;

    bool success =
        USD::Prim::compute_usdgeom_extent(prim, frame, local, extent);
    ASSERT_TRUE(success);
    ASSERT_TRUE(extent);
    ASSERT_EQ(2, extent->size());

    // compute boundable using pxr API to compare with our result
    auto              boundable = PXR_NS::UsdGeomBoundable(pxrPrim);
    PXR_NS::VtVec3fArray pxr_extent;
    ASSERT_TRUE(PXR_NS::UsdGeomBoundable::ComputeExtentFromPlugins(
        boundable, PXR_NS::UsdTimeCode::Default(), &pxr_extent));

    ASSERT_EQ((*extent)[0].x, pxr_extent[0][0]);
    ASSERT_EQ((*extent)[0].y, pxr_extent[0][1]);
    ASSERT_EQ((*extent)[0].z, pxr_extent[0][2]);

    ASSERT_EQ((*extent)[1].x, pxr_extent[1][0]);
    ASSERT_EQ((*extent)[1].y, pxr_extent[1][1]);
    ASSERT_EQ((*extent)[1].z, pxr_extent[1][2]);
}

TEST(GeomNodeDefs, translate_prim) {
    BifrostUsd::Stage stage{getResourcePath("Tree2.usd"),
                              BifrostUsd::InitialLoadSet::LoadAll};
    ASSERT_TRUE(stage);

    Bifrost::Math::double3 position{2.0, 0.0, 0.0};

    bool success =
        USD::Prim::translate_prim(stage, "/Tree2", position, false, 1.f);
    ASSERT_TRUE(success);

    ASSERT_TRUE(stage);

    auto prim = stage->GetPrimAtPath(PXR_NS::SdfPath("/Tree2"));
    ASSERT_TRUE(prim);

    auto xform_api = PXR_NS::UsdGeomXformCommonAPI(prim);
    ASSERT_TRUE(xform_api);

    PXR_NS::GfVec3d                              translation;
    PXR_NS::GfVec3f                              rotation;
    PXR_NS::GfVec3f                              scale;
    PXR_NS::GfVec3f                              pivot;
    PXR_NS::UsdGeomXformCommonAPI::RotationOrder rotOrder;
    PXR_NS::UsdTimeCode                          time;

    ASSERT_TRUE(xform_api.GetXformVectors(&translation, &rotation, &scale,
                                          &pivot, &rotOrder, time));

    ASSERT_EQ(translation[0], position.x);
}

TEST(GeomNodeDefs, get_usd_point_positions) {
    auto          stage = Amino::newMutablePtr<BifrostUsd::Stage>();
    Amino::String stagePath{"/OneFace"};
    Amino::String stageType{"Mesh"};
    float         dummyFrame{0.0f};

    // This test will exercise get_usd_point_positions
    // that also exercises UsdGeomXformCache that uses
    // USD vt/hashmap.h that uses different code paths
    // for GCC and Clang... It can crash and this test
    // will help catch the issue

    //
    // Create prim
    USD::Prim::create_prim(*stage, stagePath, stageType);
    Amino::Array<Bifrost::Math::float3> faceVertices{{-5.0f, -5.0f, 0.0f},
                                                     {5.0f, -5.0f, 0.0f},
                                                     {5.0f, 5.0f, 0.0f},
                                                     {-5.0f, 5.0f, 0.0f}};

    Amino::String attrPoints{"points"};
    USD::Attribute::set_prim_attribute(*stage, stagePath, attrPoints,
                                       faceVertices, false /*no frame usage*/,
                                       dummyFrame);

    Amino::String     attrFVC{"faceVertexCounts"};
    Amino::Array<int> faceVertexCounts{static_cast<int>(faceVertices.size())};
    USD::Attribute::set_prim_attribute(*stage, stagePath, attrFVC,
                                       faceVertexCounts,
                                       false /*no frame usage*/, dummyFrame);

    Amino::String     attrFVI{"faceVertexIndices"};
    Amino::Array<int> faceVertexIndices{0, 1, 2, 3};
    USD::Attribute::set_prim_attribute(*stage, stagePath, attrFVI,
                                       faceVertexIndices,
                                       false /*no frame usage*/, dummyFrame);

    Amino::MutablePtr<BifrostUsd::Prim> stagePrim;
    auto                                  stageConst = stage.toImmutable();
    USD::Prim::get_prim_at_path(stageConst, stagePath, stagePrim);

    Amino::MutablePtr<Amino::Array<Bifrost::Math::float3>> pointPositions;
    USD::Prim::get_usd_geom_points(*stagePrim, false /*local space*/,
                                   dummyFrame, pointPositions);

    ASSERT_TRUE(pointPositions != nullptr);
    ASSERT_EQ(pointPositions->size(), faceVertices.size());

    for (size_t i = 0; i < faceVertices.size(); ++i) {
        auto const& v = faceVertices[i];
        auto const& p = (*pointPositions)[i];
        ASSERT_FLOAT_EQ(v.x, p.x);
        ASSERT_FLOAT_EQ(v.y, p.y);
        ASSERT_FLOAT_EQ(v.z, p.z);
    }
}

TEST(GeomNodeDefs, usd_point_instancer) {}

TEST(GeomNodeDefs, usd_volume) {}
