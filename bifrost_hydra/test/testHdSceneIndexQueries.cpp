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

// Bifrost USD
#include <utils/test/testUtils.h>

// Pixar USD
#include <pxr/imaging/hd/meshSchema.h>
#include <pxr/imaging/hd/primvarsSchema.h>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace BifrostHdTest;

// Not really tests for Bifrost Hydra. Lets call them "utility tests" to easily
// check the content of an Hydra Scene Index.

TEST_F(TestSceneIndexPrim, usd_geometries_queries) {
    std::string stageFilePath =
        BifrostUsd::TestUtils::getResourcePath("usd_geometries.usda").c_str();

    ASSERT_TRUE(openStage(stageFilePath));
    ASSERT_TRUE(render());
    {
        auto path = SdfPath{"/capsule"};
        ExportAsString(getHdPrim(path), "/tmp/hdCapsule.txt");
    }

    {
        auto path = SdfPath{"/instancer"};
        ExportAsString(getHdPrim(path), "/tmp/hdInstancer.txt");

        auto childrenPaths = sceneIndex->GetChildPrimPaths(path);
        EXPECT_EQ(childrenPaths.size(), 1);

        ExportAsString(getHdPrim(childrenPaths[0]),
                       "/tmp/hdInstancerFirstChild.txt");
    }

    {
        auto path = SdfPath{"/cylinder"};
        ExportAsString(getHdPrim(path), "/tmp/hdCylinder.txt");

        auto childrenPaths = sceneIndex->GetChildPrimPaths(path);
        EXPECT_EQ(childrenPaths.size(), 1);

        ExportAsString(getHdPrim(childrenPaths[0]),
                       "/tmp/hdCylinderFirstChild.txt");
    }

    {
        auto path = SdfPath{"/linear_curves"};
        ExportAsString(getHdPrim(path), "/tmp/hdLinearCurves.txt");
    }
}

TEST_F(TestSceneIndexPrim, create_colored_mesh_cube_query) {
    std::string stageFilePath = BifrostUsd::TestUtils::getResourcePath(
                                    "create_colored_mesh_cube.usda")
                                    .c_str();

    ASSERT_TRUE(openStage(stageFilePath));
    ASSERT_TRUE(render());
    {
        auto path = SdfPath{"/Asset/BifrostGraph"};
        ExportAsString(getHdPrim(path), "/tmp/hdCreateColoredMeshCubeProc.txt");

        auto childrenPaths = sceneIndex->GetChildPrimPaths(path);
        ASSERT_EQ(childrenPaths.size(), 1);

        ExportAsString(getHdPrim(childrenPaths[0]),
                       "/tmp/hdCreateColoredMeshCubeProcChild.txt");
    }
}

TEST_F(TestSceneIndexPrim, create_strands_query) {
    std::string stageFilePath = BifrostUsd::TestUtils::getResourcePath(
                                    "create_strands_test1.usda")
                                    .c_str();

    ASSERT_TRUE(openStage(stageFilePath));
    ASSERT_TRUE(render());
    {
        auto path = SdfPath{"/Asset/BifrostGraph"};
        ExportAsString(getHdPrim(path), "/tmp/hdCreateStrandsProc.txt");

        auto childrenPaths = sceneIndex->GetChildPrimPaths(path);
        ASSERT_EQ(childrenPaths.size(), 1);

        ExportAsString(getHdPrim(childrenPaths[0]),
                       "/tmp/hdCreateStrandsProcChild.txt");
    }
}

#if 0
// TODO(laforgg): Implement Instancing. Currenyly it is broken.
TEST_F(TestSceneIndexPrim, create_instances_query) {
    std::string stageFilePath =
        BifrostUsd::TestUtils::getResourcePath("create_simple_instances_test.usda")
            .c_str();

    ASSERT_TRUE(openStage(stageFilePath));
    ASSERT_TRUE(render());
    {
        auto path = SdfPath{"/Asset/BifrostGraph"};
        ExportAsString(getHdPrim(path), "/tmp/hdCreateInstancesProc.txt");

        auto childrenPaths = sceneIndex->GetChildPrimPaths(path);
        EXPECT_EQ(childrenPaths.size(), 1);

        ExportAsString(getHdPrim(childrenPaths[0]),
                       "/tmp/hdCreateInstancesProc_instancerChild.txt");

        childrenPaths = sceneIndex->GetChildPrimPaths(childrenPaths[0]);
        EXPECT_EQ(childrenPaths.size(), 1);

        ExportAsString(getHdPrim(childrenPaths[0]),
                       "/tmp/hdCreateInstancesProc_protoChild.txt");
    }
}
#endif
