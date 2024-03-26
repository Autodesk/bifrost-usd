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

#include <gtest/gtest.h>

#include "testUtils/TestSceneIndexPrim.h"

// Bifrost
#include <Bifrost/Object/Object.h>

// Bifrost USD
#include <utils/test/testUtils.h>

// Bifrost Hydra
#include <BifrostHydra/Engine/Container.h>
#include <BifrostHydra/Engine/Engine.h>
#include <BifrostHydra/Engine/Parameters.h>
#include <BifrostHydra/Engine/Runtime.h>
#include <BifrostHydra/Engine/Workspace.h>
#include <BifrostHydra/Translators/GeometryFn.h>
#include <BifrostHydra/Translators/Instances.h>
#include <BifrostHydra/Translators/Mesh.h>

// Pixar USD
#include <pxr/imaging/hd/meshSchema.h>
#include <pxr/imaging/hd/primvarsSchema.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>


PXR_NAMESPACE_USING_DIRECTIVE

using namespace BifrostHdTest;

TEST_F(TestSceneIndexPrim, runtime) {
    auto& workspace = BifrostHd::Workspace::getInstance();
    ASSERT_TRUE(workspace.isValid());

    auto& runtime = BifrostHd::Runtime::getInstance();

    // test if our configs are loaded.
    auto check_config_is_loaded = [&workspace](const Amino::String& message) {
        auto const& messages = workspace.getMessages();
        EXPECT_NE(messages.end(), std::find(messages.begin(), messages.end(), message));
    };

    check_config_is_loaded(
        "[Library] Bifrost: Loading library: bifrostHdTypeTranslation, "
        "from: Autodesk.");
    check_config_is_loaded(
        "[Library] Bifrost: Loading library: test_bifrost_hd_graph, "
        "from: Autodesk.");

    Amino::Type           floatType;
    const Amino::Library& lib          = runtime.getAmLibrary();
    auto                  builtInTypes = lib.getBuiltInNamespace().getTypes();
    ASSERT_TRUE(builtInTypes.findByName("float", floatType));

    auto           nodeDefs = lib.getNodeDefs();
    Amino::NodeDef simpleAddNodeDef;
    EXPECT_TRUE(
        nodeDefs.findByName("Hydra::Testing::simple_add", simpleAddNodeDef));
}

TEST_F(TestSceneIndexPrim, parameters) {
    // open stage
    std::string stageFilePath =
        BifrostUsd::TestUtils::getResourcePath("create_mesh_cube.usda").c_str();

    ASSERT_TRUE(openStage(stageFilePath));
    ASSERT_TRUE(render());

    auto graphPrim = getHdPrim(SdfPath{"/Asset/BifrostGraph"});
    EXPECT_EQ(graphPrim.primType, TfToken{"resolvedHydraGenerativeProcedural"});

    // ----------------------------------------------------------------------

    BifrostHd::Parameters hdParams;
    hdParams.setInputs(graphPrim);
    EXPECT_EQ(hdParams.compoundName(),
              std::string{"Modeling::Primitive::create_mesh_cube"});

    auto inputs = hdParams.inputs();
    EXPECT_EQ(inputs.size(), 3);

    auto check_float_input = [&inputs](const std::string& name,
                                       float              expectedValue) {
        EXPECT_TRUE(inputs.find(name) != inputs.end());
        EXPECT_TRUE(inputs[name].IsHolding<float>());
        EXPECT_EQ(inputs[name].UncheckedGet<float>(), expectedValue);
    };

    check_float_input("length", 1.f);
    check_float_input("width", 1.f);
    check_float_input("height", 1.f);

    auto output = hdParams.output();
    EXPECT_EQ(output.first, "cube_mesh");
    // Since the graph did not execute, we should get an empty object
    const auto& objectArray = output.second;
    EXPECT_TRUE(objectArray.empty());
}

TEST_F(TestSceneIndexPrim, create_mesh_cube) {
    // open stage
    std::string stageFilePath =
        BifrostUsd::TestUtils::getResourcePath("create_mesh_cube.usda")
            .c_str();

    ASSERT_TRUE(openStage(stageFilePath));
    ASSERT_TRUE(render());

    auto graphPrim = getHdPrim(SdfPath{"/Asset/BifrostGraph"});
    EXPECT_EQ(graphPrim.primType, TfToken{"resolvedHydraGenerativeProcedural"});

    BifrostHd::Engine engine;
    engine.setInputs(graphPrim);
    ASSERT_EQ(engine.execute(), Amino::Job::State::kSuccess);
    const auto& output = engine.output();
    EXPECT_EQ(output.first, "cube_mesh");

    const auto& objectArray = output.second;
    EXPECT_FALSE(objectArray.empty());

    auto& outputGeo = objectArray[0];

    // Tests on the Bifrost object
    EXPECT_EQ(BifrostHd::GetGeoType(*outputGeo), BifrostHdGeoTypes::Mesh);

    const auto vtPoints = BifrostHd::GetPoints(*outputGeo);
    EXPECT_EQ(vtPoints.size(), 8);

    const auto vtDisplayColor =
        BifrostHd::GetDisplayColor(*outputGeo);
    EXPECT_TRUE(vtDisplayColor.empty());

    // Test on BifrostTranslators::Mesh
    BifrostHd::Mesh mesh(*outputGeo);

    const auto& children = mesh.getChildren();
    const auto  it       = children.find(SdfPath{"mesh0"});
    EXPECT_TRUE(it != children.end());

    auto hdPrim = it->second;
    EXPECT_EQ(hdPrim.primType, HdPrimTypeTokens->mesh);

    // Tests on the Hydra scene index prim
    auto hdMeshPrim = BifrostHd::CreateHdSceneIndexMesh(*outputGeo);
    TestHdSceneIndexMesh(hdMeshPrim, /*hasDisplayColor*/false);
}

TEST_F(TestSceneIndexPrim, create_mesh_array) {
    // open stage
    std::string stageFilePath =
        BifrostUsd::TestUtils::getResourcePath("create_mesh_array.usda")
            .c_str();

    ASSERT_TRUE(openStage(stageFilePath));
    ASSERT_TRUE(render());

    auto graphPrim = getHdPrim(SdfPath{"/Asset/BifrostGraph"});
    EXPECT_EQ(graphPrim.primType, TfToken{"resolvedHydraGenerativeProcedural"});

    BifrostHd::Engine engine;
    engine.setInputs(graphPrim);
    ASSERT_EQ(engine.execute(), Amino::Job::State::kSuccess);
    const auto& output = engine.output();
    EXPECT_EQ(output.first, "meshes");

    const auto& objectArray = output.second;
    EXPECT_EQ(objectArray.size(), 3);
}

TEST_F(TestSceneIndexPrim, create_colored_mesh_cube) {
    // open stage
    std::string stageFilePath =
        BifrostUsd::TestUtils::getResourcePath("create_colored_mesh_cube.usda")
            .c_str();

    ASSERT_TRUE(openStage(stageFilePath));
    ASSERT_TRUE(render());

    auto graphPrim = getHdPrim(SdfPath{"/Asset/BifrostGraph"});
    EXPECT_EQ(graphPrim.primType, TfToken{"resolvedHydraGenerativeProcedural"});

    BifrostHd::Engine engine;
    engine.setInputs(graphPrim);
    ASSERT_EQ(engine.execute(), Amino::Job::State::kSuccess);
    const auto& output = engine.output();
    EXPECT_EQ(output.first, "mesh");
    
    const auto& objectArray = output.second;
    EXPECT_EQ(objectArray.size(), 1);

    const auto& object = objectArray[0];

    // Tests on the Bifrost object
    EXPECT_EQ(BifrostHd::GetGeoType(*object), BifrostHdGeoTypes::Mesh);

    const auto vtPoints = BifrostHd::GetPoints(*object);
    EXPECT_EQ(vtPoints.size(), 8);

    const auto vtDisplayColor =
        BifrostHd::GetDisplayColor(*object);
    EXPECT_EQ(vtDisplayColor.size(), 8);

    // Test on BiforstTranslators::Mesh
    BifrostHd::Mesh mesh(*object);

    const auto& children = mesh.getChildren();
    const auto  it       = children.find(SdfPath{"mesh0"});
    EXPECT_TRUE(it != children.end());

    auto hdPrim = it->second;
    EXPECT_EQ(hdPrim.primType, HdPrimTypeTokens->mesh);

    // Tests on the Hydra scene index prim
    auto hdMeshPrim = BifrostHd::CreateHdSceneIndexMesh(*object);
    TestHdSceneIndexMesh(hdMeshPrim);
}

// TODO(laforgg): Implement Instancing. Currenyly it is broken.
#if 0
TEST_F(TestSceneIndexPrim, create_simple_instances_test) {
    // open stage
    std::string stageFilePath = BifrostUsd::TestUtils::getResourcePath(
                                    "create_simple_instances_test.usda")
                                    .c_str();

    ASSERT_TRUE(openStage(stageFilePath));
    ASSERT_TRUE(render());
    auto graphPrim = getHdPrim(SdfPath{"/Asset/BifrostGraph"});
    EXPECT_EQ(graphPrim.primType, TfToken{"resolvedHydraGenerativeProcedural"});
    auto childrenPaths =
        sceneIndex->GetChildPrimPaths(SdfPath{"/Asset/BifrostGraph"});
    EXPECT_EQ(childrenPaths.size(), 1);
    EXPECT_EQ(childrenPaths[0], SdfPath{"/Asset/BifrostGraph/instancer"});

    auto protoPaths =
        sceneIndex->GetChildPrimPaths(SdfPath{"/Asset/BifrostGraph/instancer"});
    EXPECT_EQ(protoPaths.size(), 1);
    EXPECT_EQ(protoPaths[0],
              SdfPath{"/Asset/BifrostGraph/instancer/prototypes"});

    // ----------------------------------------------------------------------

    BifrostHd::Engine engine;
    engine.setInputs(graphPrim);
    ASSERT_EQ(engine.execute(), Amino::Job::State::kSuccess);

    const auto& output = engine.output();
    EXPECT_EQ(output.first, "instances");

    const auto& objectArray = output.second;
    EXPECT_EQ(objectArray.size(), 1);
    const auto& object = objectArray[0];


    // Tests on the Bifrost object
    EXPECT_EQ(BifrostHd::GetGeoType(*object), BifrostHdGeoTypes::Instances);

    const auto vtPoints = BifrostHd::GetPoints(*object);
    EXPECT_EQ(vtPoints.size(), 2);

    const auto vtPointIDs = BifrostHd::GetPointIDs(*object);
    EXPECT_EQ(vtPointIDs.size(), 0);

    auto instanceShape = BifrostHd::GetInstanceShape(*object);
    EXPECT_TRUE(instanceShape);

    const auto vtPointInstanceIDs =
        BifrostHd::GetPointInstanceIDs(*object);
    EXPECT_EQ(vtPointInstanceIDs.size(), 2);

    auto shapeID = vtPointInstanceIDs[0];
    EXPECT_EQ(shapeID, 0);

    auto shape = BifrostHd::GetShapeFromId(*instanceShape, shapeID);
    EXPECT_TRUE(shape);

    auto renderGeometry = BifrostHd::GetRenderGeometry(*shape);
    EXPECT_TRUE(renderGeometry);

    auto vtProtoPoints = BifrostHd::GetPoints(*renderGeometry);
    EXPECT_EQ(vtProtoPoints.size(), 8);

    // Tests on the Hydra scene index prim
    auto hdInstancer =
        BifrostHd::CreateHdSceneIndexExplicitInstancer(*object);
    EXPECT_EQ(hdInstancer.primType, HdPrimTypeTokens->instancer);
}
#endif

TEST_F(TestSceneIndexPrim, create_strands_test1) {
    // open stage
    std::string stageFilePath =
        BifrostUsd::TestUtils::getResourcePath("create_strands_test1.usda")
            .c_str();

    ASSERT_TRUE(openStage(stageFilePath));
    ASSERT_TRUE(render());

    auto graphPrim = getHdPrim(SdfPath{"/Asset/BifrostGraph"});
    EXPECT_EQ(graphPrim.primType, TfToken{"resolvedHydraGenerativeProcedural"});

    BifrostHd::Engine engine;
    engine.setInputs(graphPrim);
    ASSERT_EQ(engine.execute(), Amino::Job::State::kSuccess);
    const auto& output = engine.output();
    EXPECT_EQ(output.first, "strands");

    const auto& objectArray = output.second;
    EXPECT_EQ(objectArray.size(), 1);
    const auto& object = objectArray[0];


    // Tests on the Bifrost object
    EXPECT_EQ(BifrostHd::GetGeoType(*object), BifrostHdGeoTypes::Strands);

    const auto vtPoints = BifrostHd::GetPoints(*object);
    EXPECT_EQ(vtPoints.size(), 18);

    const auto vtDisplayColor =
        BifrostHd::GetDisplayColor(*object);
    EXPECT_EQ(vtDisplayColor.size(), 18);

    const auto vtWidths = BifrostHd::GetWidth(*object);
    EXPECT_EQ(vtWidths.size(), 18);

    // Tests on the Hydra scene index prim
    auto hdBasisCurvesPrim =
        BifrostHd::CreateHdSceneIndexBasisCurves(*object);
    TestHdSceneIndexBasisCurves(hdBasisCurvesPrim);
}

TEST_F(TestSceneIndexPrim, create_mesh_plane_with_animated_pt) {
    // open stage
    std::string stageFilePath = BifrostUsd::TestUtils::getResourcePath(
                                    "create_mesh_plane_with_animated_pt.usda")
                                    .c_str();

    ASSERT_TRUE(openStage(stageFilePath));
    ASSERT_TRUE(render());

    auto graphPrim = getHdPrim(SdfPath{"/Asset/BifrostGraph"});
    EXPECT_EQ(graphPrim.primType, TfToken{"resolvedHydraGenerativeProcedural"});

    // The compound called "create_mesh_plane_with_animated_pt" is moving the
    // first point of a square along the Y axis by adding the frame value of the
    // "time" node to it.
    {
        // Execute at frame 0
        BifrostHd::Engine engine;
        engine.setInputs(graphPrim);
        ASSERT_EQ(engine.execute(), Amino::Job::State::kSuccess);
        const auto& objectArray = engine.output().second;
        EXPECT_EQ(objectArray.size(), 1);
        const auto& object = objectArray[0];
        const auto vtPoints =
            BifrostHd::GetPoints(*object);
        EXPECT_FLOAT_EQ(vtPoints[0][1], 0.50) << "Error for frame 0";
    }

    {
        // Execute at frame 24
        BifrostHd::Engine engine;
        engine.setInputs(graphPrim);
        ASSERT_EQ(engine.execute(24.0), Amino::Job::State::kSuccess);
        const auto& objectArray = engine.output().second;
        EXPECT_EQ(objectArray.size(), 1);
        const auto& object = objectArray[0];        
        const auto vtPoints =
            BifrostHd::GetPoints(*object);
        EXPECT_FLOAT_EQ(vtPoints[0][1], 24.5) << "Error for frame 24";
    }
}

TEST_F(TestSceneIndexPrim, re_render) {
    // open stage
    std::string stageFilePath =
        BifrostUsd::TestUtils::getResourcePath("create_strands_test1.usda")
            .c_str();

    auto stage = openStage(stageFilePath);
    ASSERT_TRUE(stage);
    ASSERT_TRUE(render());
    auto primPath = SdfPath{"/Asset/BifrostGraph"};
    BifrostHd::Engine engine;
    engine.setInputs(getHdPrim(primPath));

    {
        ASSERT_EQ(engine.execute(), Amino::Job::State::kSuccess);
        const auto& output = engine.output();
        const auto& objectArray = output.second;
        EXPECT_EQ(objectArray.size(), 1);
        const auto& object = objectArray[0];
        const auto vtPoints = BifrostHd::GetPoints(*object);
        EXPECT_EQ(vtPoints.size(), 18);

        auto pt = PXR_NS::GfVec3f{0.5f, 1.0f, 0.5f};
        EXPECT_EQ(vtPoints[17], pt);        
    }

    {
        auto prim = stage->GetPrimAtPath(primPath);
        EXPECT_TRUE(prim);
        auto primvarAPI = PXR_NS::UsdGeomPrimvarsAPI(prim);
        auto primvar = primvarAPI.GetPrimvar(PXR_NS::TfToken{"strands_length"});
        EXPECT_TRUE(primvar);

        float value;
        primvar.Get(&value);
        EXPECT_EQ(value, 1.f);

        primvar.Set(2.f);
        reRender();

        // BifrostHd::Engine engine{getHdPrim(primPath)};
        engine.setInputs(getHdPrim(primPath));
        ASSERT_EQ(engine.execute(), Amino::Job::State::kSuccess);
        const auto& output = engine.output();
        const auto& objectArray = output.second;
        EXPECT_EQ(objectArray.size(), 1);
        const auto& object = objectArray[0];
        const auto vtPoints = BifrostHd::GetPoints(*object);
        EXPECT_EQ(vtPoints.size(), 18);

        auto pt = PXR_NS::GfVec3f{0.5f, 2.0f, 0.5f};
        EXPECT_EQ(vtPoints[17], pt);        
    }

    EXPECT_EQ(observer.GetEvents().size(), 4);

    // for (const auto &e : observer.GetEvents()) {
    //     switch(e.eventType) {
    //     case RecordingSceneIndexObserver::EventType_PrimAdded:
    //         printf("EventType_PrimAdded: %s\n", e.primPath.GetText());
    //         break;
    //     case RecordingSceneIndexObserver::EventType_PrimRemoved:
    //         printf("EventType_PrimRemoved: %s\n", e.primPath.GetText());
    //         break;
    //     case RecordingSceneIndexObserver::EventType_PrimDirtied:
    //         printf("EventType_PrimDirtied: %s\n", e.primPath.GetText());
    //         break;
    //     }
    // }
}

TEST_F(TestSceneIndexPrim, create_strands_from_input_mesh) {
    // open stage
    std::string stageFilePath =
        BifrostUsd::TestUtils::getResourcePath("create_strands_from_input_mesh.usda")
            .c_str();

    auto stage = openStage(stageFilePath);
    ASSERT_TRUE(stage);
    ASSERT_TRUE(render());
}
