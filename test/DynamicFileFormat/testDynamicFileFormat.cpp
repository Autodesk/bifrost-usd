//-
// Copyright 2026 Autodesk, Inc.
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

// Amino
#include <Amino/Core/BuiltInTypes.h>
#include <Amino/Core/StringView.h>

// Bifrost USD
#include <pxr/base/vt/types.h>
#include <utils/test/testUtils.h>

// Open USD
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/kind/registry.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/reference.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/payloads.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/points.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/xform.h>

// C++ Standard Library
#include <regex>
#include <string>

using namespace BifrostUsd::TestUtils;

namespace {
UniqueTestOutputSubdir g_OutputDir{"testDynamicFileFormat",
                                   true /*autoDelete*/};
} // namespace

PXR_NAMESPACE_OPEN_SCOPE

namespace {
Amino::String createRootLayerWithDefaultFields(Amino::StringView layerName,
                                               Amino::StringView compoundName,
                                               Amino::StringView outputName) {
    auto exportPath =
        g_OutputDir.getPath_abs(Amino::String{layerName} + ".usd");

    // Dynamic Payload works only with usda file format.
    // TODO(laforgg): Investigate why.
    auto           fileFormat = SdfFileFormat::FindByExtension(".usda");
    SdfLayerRefPtr layer      = SdfLayer::New(fileFormat, exportPath.c_str());

    auto rootPrimSpec = SdfPrimSpec::New(layer, "Root", SdfSpecifierDef);

    auto payload     = SdfPayload("anon:autodesk:bifrost.bifrostDynamicFile");
    auto payloadList = rootPrimSpec->GetPayloadList();
    payloadList.Append(payload);

    auto dynFffPrimSpec =
        SdfPrimSpec::New(layer, "DynamicFileFormatField", SdfSpecifierDef);

    auto internalReference =
        SdfReference(std::string(), dynFffPrimSpec->GetPath());

    auto referenceList = rootPrimSpec->GetReferenceList();
    referenceList.Append(internalReference);

    layer->SetField(dynFffPrimSpec->GetPath(),
                    TfToken{"BifrostGraph_CompoundName"},
                    TfToken{compoundName.data()});

    layer->SetField(dynFffPrimSpec->GetPath(),
                    TfToken{"BifrostGraph_OutputName"},
                    TfToken{outputName.data()});

    layer->SetField(dynFffPrimSpec->GetPath(),
                    TfToken{"BifrostGraph_ReloadLibrary"}, false);

    layer->Save();

    return exportPath;
}

[[maybe_unused]] void setVerbosityLevelField(UsdStageRefPtr&    stage,
                                             const std::string& level) {
    stage->GetRootLayer()->SetFieldDictValueByKey(
        SdfPath{"/DynamicFileFormatField"}, TfToken{"BifrostGraph_Options"},
        TfToken("verbosity_level"), level);
}

void setGlobalsField(UsdStageRefPtr& stage, const VtDictionary& paramsDict) {
    stage->GetRootLayer()->SetField(SdfPath{"/DynamicFileFormatField"},
                                    TfToken{"BifrostGraph_Globals"},
                                    paramsDict);
}

void setParamsField(UsdStageRefPtr& stage, const VtDictionary& paramsDict) {
    stage->GetRootLayer()->SetField(SdfPath{"/DynamicFileFormatField"},
                                    TfToken{"BifrostGraph_Params"}, paramsDict);
}

using ValueTypeNameAndValue = std::pair<SdfValueTypeName, VtValue>;
using AttributeName         = TfToken;
using AttributeNameAndValue = std::pair<AttributeName, ValueTypeNameAndValue>;

void setAttributeOverrides(
    UsdStageRefPtr&                           stage,
    const std::vector<AttributeNameAndValue>& attributeVector) {
    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});

    for (const auto& [attrName, attrTypeAndValue] : attributeVector) {
        auto attr = rootPrim.GetAttribute(attrName);
        if (!attr) {
            attr = rootPrim.CreateAttribute(attrName, attrTypeAndValue.first,
                                            /*custom*/ false);
        }
        attr.Set(attrTypeAndValue.second);
    }
}

bool hasSameTranslation(const UsdPrim& prim, const GfVec3f& expected) {
    auto        xform         = UsdGeomXform{prim};
    auto        translateOp   = xform.GetTranslateOp();
    const auto& translateAttr = translateOp.GetAttr();
    GfVec3f     translation;
    translateAttr.Get(&translation);
    if (translation == expected) {
        return true;
    }
    return false;
}

} // namespace

TEST(BifrostDynamicFileFormatTests, not_found_compound) {
    auto rootLayerPath = createRootLayerWithDefaultFields(
        "test_not_found_compound", "Not::Found", "nope");

    // Capture stderr before opening the stage so we can assert on the error
    // messages produced when the compound is not found.
    testing::internal::CaptureStderr();

    auto stage = UsdStage::Open(rootLayerPath.c_str());

    std::string capturedErr = testing::internal::GetCapturedStderr();

    // The Stage opens successfully; only the payload fails to load, leaving
    // the /Root prim with no children from the graph output:
    ASSERT_TRUE(stage);
    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    EXPECT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);

    // Verify the expected error messages were emitted to stderr:
    EXPECT_NE(capturedErr.find("'Not::Found' not found"), std::string::npos);
    EXPECT_NE(capturedErr.find(
                  "Failed to create a GraphExecutor for graph 'Not::Found'"),
              std::string::npos);
}

TEST(BifrostDynamicFileFormatTests, core_string_string_to_array_compound) {
    auto rootLayerPath = createRootLayerWithDefaultFields(
        "test_core_string_string_to_array_compound",
        "Core::String::string_to_array", "string_array");

    // Capture stderr before opening the stage so we can assert on the error
    // messages produced when the compound is not found.
    testing::internal::CaptureStderr();

    auto stage = UsdStage::Open(rootLayerPath.c_str());

    std::string capturedErr = testing::internal::GetCapturedStderr();

    // The Stage opens successfully; only the payload fails to load, leaving
    // the /Root prim with no children from the graph output:
    ASSERT_TRUE(stage);
    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    EXPECT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);

    // Verify stderr contains a message matching:
    //   "The graph ... did not produce a valid ... output"
    std::regex pattern{
        R"(The graph[^\n]* did not produce a valid[^\n]* output)"};
    EXPECT_TRUE(std::regex_search(capturedErr, pattern))
        << "Expected stderr to contain: "
           "The graph... did not produce a valid... output\n"
        << "Actual stderr:\n"
        << capturedErr;
}

TEST(BifrostDynamicFileFormatTests, create_helix_of_prims_compound) {
    auto rootLayerPath = createRootLayerWithDefaultFields(
        "test_create_helix_of_prims_compound",
        "Test::DynamicFileFormat::create_helix_of_prims", "stage");

    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Test with default fields only
    {
        auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
        ASSERT_TRUE(rootPrim);
        EXPECT_TRUE(hasSameTranslation(rootPrim, GfVec3f{0.0f, 0.0f, 0.0f}));

        // The graph creates a helix of 20 shapes with increasing translation.
        // The output of the compound is a stage, hence the DynamicFileFormat
        // plugin does not need to apply any translation to the root prim and
        // will return the stage as is, with a "shapes" primitive in it:
        auto shapesPrim = stage->GetPrimAtPath(SdfPath{"/Root/shapes"});
        ASSERT_TRUE(shapesPrim);

        // We check the translation of the last shape to make sure the graph was
        // executed and translated correctly.
        EXPECT_EQ(shapesPrim.GetChildrenNames().size(), 20);
        auto shapePrim = stage->GetPrimAtPath(SdfPath{"/Root/shapes/shape_19"});
        ASSERT_TRUE(shapePrim);
        EXPECT_EQ(std::string{shapePrim.GetTypeName().GetText()},
                  std::string{"Capsule"});
        EXPECT_TRUE(hasSameTranslation(
            shapePrim, GfVec3f{11.938052f, 1.2135255f, -0.88167775f}));
    }

    // Test with some params fields
    {
        // clang-format off
        setParamsField(stage, VtDictionary{
                {"name", VtValue{std::string{"newShape"}}},
                {"count", VtValue{int{3}}},
                {"prim_type", VtValue{TfToken{"Cylinder"}}},
                {"length", VtValue{float{2}}},
                {"translation", VtValue{GfVec3f{1.0f, 2.0f, 3.0f}}}
            }
        );
        // clang-format on

        auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
        ASSERT_TRUE(rootPrim);
        EXPECT_TRUE(hasSameTranslation(rootPrim, GfVec3f{1.0f, 2.0f, 3.0f}));

        auto shapesPrim = stage->GetPrimAtPath(SdfPath{"/Root/shapes"});
        ASSERT_TRUE(shapesPrim);
        EXPECT_EQ(shapesPrim.GetChildrenNames().size(), 3);
        auto newShapePrim =
            stage->GetPrimAtPath(SdfPath{"/Root/shapes/newShape_2"});
        ASSERT_TRUE(newShapePrim);
        EXPECT_EQ(std::string{newShapePrim.GetTypeName().GetText()},
                  std::string{"Cylinder"});
        EXPECT_TRUE(hasSameTranslation(
            newShapePrim, GfVec3f{16.755161f, -0.7499994f, -1.2990384f}));
    }

    // Test with some attribute overrides
    {
        // clang-format off
        setAttributeOverrides(stage,
            std::vector<AttributeNameAndValue> {
                {TfToken{"name"}, {SdfValueTypeNames->String, VtValue{std::string{"ball"}}}},
                {TfToken{"count"}, {SdfValueTypeNames->Int, VtValue{int{6}}}},
                {TfToken{"prim_type"}, {SdfValueTypeNames->Token, VtValue{TfToken{"Sphere"}}}},
                {TfToken{"length"}, {SdfValueTypeNames->Float, VtValue{float{10}}}},
                {TfToken{"translation"}, {SdfValueTypeNames->Vector3f, VtValue{GfVec3f{4.0f, 5.0f, 6.0f}}}},
            }
        );
        // clang-format on

        auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
        ASSERT_TRUE(rootPrim);
        ASSERT_TRUE(hasSameTranslation(rootPrim, GfVec3f{4.0f, 5.0f, 6.0f}));

        auto shapesPrim = stage->GetPrimAtPath(SdfPath{"/Root/shapes"});
        ASSERT_TRUE(shapesPrim);
        EXPECT_EQ(shapesPrim.GetChildrenNames().size(), 6);
        auto ballPrim = stage->GetPrimAtPath(SdfPath{"/Root/shapes/ball_2"});
        ASSERT_TRUE(ballPrim);
        EXPECT_EQ(std::string{ballPrim.GetTypeName().GetText()},
                  std::string{"Sphere"});
        EXPECT_TRUE(hasSameTranslation(
            ballPrim, GfVec3f{41.8879f, -0.75000095f, -1.2990376f}));
    }
}

TEST(BifrostDynamicFileFormatTests, create_mesh_torus_compound) {
    auto rootLayerPath = createRootLayerWithDefaultFields(
        "test_create_mesh_torus_compound",
        "Modeling::Primitive::create_mesh_torus", "torus_mesh");

    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Test with some params fields
    {
        // clang-format off
        setParamsField(stage, VtDictionary{
                {"major_radius", VtValue{float{1}}},
                {"minor_radius", VtValue{float{0.5}}},
                {"major_segments", VtValue{static_cast<unsigned>(30)}},
                {"minor_segments", VtValue{static_cast<unsigned>(30)}}
            }
        );
        // clang-format on

        // The output of the compound is a single mesh Object, hence the
        // DynamicFileFormat plugin applies the 'object_to_stage' translation
        // compound to it. The default 'purpose' produces a 'geo' scope
        // primitive in root primitive.
        auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
        EXPECT_TRUE(rootPrim);
        auto geoPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo"});
        EXPECT_TRUE(geoPrim);

        // Check geo primitive is a scope:
        TfToken kind;
        EXPECT_TRUE(geoPrim.GetKind(&kind));
        EXPECT_EQ(kind, KindTokens->group);
        auto geoScope = UsdGeomScope(geoPrim);
        EXPECT_TRUE(geoScope);

        // Check the mesh primitive created under the geo scope:
        auto meshPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"});
        EXPECT_TRUE(meshPrim);
        auto mesh = UsdGeomMesh{meshPrim};
        ASSERT_TRUE(mesh);
        EXPECT_EQ(mesh.GetFaceCount(), 900);
    }

    // Test with some attribute overrides
    {
        // clang-format off
        setAttributeOverrides(stage,
            std::vector<AttributeNameAndValue> {
                {TfToken{"major_radius"}, {SdfValueTypeNames->Float, VtValue{float{1.2f}}}},
                {TfToken{"minor_radius"}, {SdfValueTypeNames->Float, VtValue{float{0.6f}}}},
                {TfToken{"major_segments"}, {SdfValueTypeNames->UInt, VtValue{static_cast<unsigned>(45)}}},
                {TfToken{"minor_segments"}, {SdfValueTypeNames->UInt, VtValue{static_cast<unsigned>(45)}}},
            }
        );
        // clang-format on

        auto meshPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"});
        EXPECT_TRUE(meshPrim);
        auto mesh = UsdGeomMesh{meshPrim};
        ASSERT_TRUE(mesh);
        EXPECT_EQ(mesh.GetFaceCount(), 2025);
    }
}

TEST(BifrostDynamicFileFormatTests, create_torus_and_cylinder_compound) {
    auto rootLayerPath = createRootLayerWithDefaultFields(
        "test_create_torus_and_cylinder_compound",
        "Test::DynamicFileFormat::create_torus_and_cylinder",
        "torus_and_cylinder");

    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Test with default fields only
    {
        // The output of the compound is multiple mesh Objects, hence the
        // DynamicFileFormat plugin applies the 'array_of_objects_to_stage'
        // translation compound to it. The default 'purpose' produces a 'geo'
        // scope primitive in root primitive.
        auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
        EXPECT_TRUE(rootPrim);
        auto geoPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo"});
        EXPECT_TRUE(geoPrim);

        auto mesh1Prim = stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh1"});
        EXPECT_TRUE(mesh1Prim);
        auto mesh1 = UsdGeomMesh{mesh1Prim};
        ASSERT_TRUE(mesh1);
        EXPECT_EQ(mesh1.GetFaceCount(), 400);

        auto mesh2Prim = stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh2"});
        EXPECT_TRUE(mesh2Prim);
        auto mesh2 = UsdGeomMesh{mesh2Prim};
        ASSERT_TRUE(mesh2);
        EXPECT_EQ(mesh2.GetFaceCount(), 100);
    }

    // Test with some params fields
    {
        // Set the input 'set_prim_paths' of compound
        // 'create_torus_and_cylinder' to true, forcing that compound to set the
        // prim_paths in the output Objects. After that, the
        // 'array_of_objects_to_stage' translation compound should detect that
        // the prim_paths are already set and should not change them.
        // clang-format off
        setParamsField(stage, VtDictionary{
                {"set_prim_paths", VtValue{bool{true}}}
            }
        );
        // clang-format on

        // Check prim_paths have not been changed by 'array_of_objects_to_stage'
        // compound.
        auto torusPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/torus"});
        EXPECT_TRUE(torusPrim);
        auto cylinderPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/cylinder"});
        EXPECT_TRUE(cylinderPrim);
    }

    // Test with some attribute overrides
    {
        // Set also an attribute override for the 'set_prim_paths' to false, to
        // check that attribute overrides have more priority than input compound
        // params fields.
        // clang-format off
        setAttributeOverrides(stage,
            std::vector<AttributeNameAndValue> {
                {TfToken{"set_prim_paths"}, {SdfValueTypeNames->Bool, VtValue{bool{false}}}},
            }
        );
        // clang-format on

        // Since the 'set_prim_paths' is set to true in params fields, but
        // overridden to false by the attribute override, no prim_paths should
        // be set by the 'create_torus_and_cylinder' compound, and the
        // 'array_of_objects_to_stage' translation compound should set the
        // prim_paths to the default 'mesh1' and 'mesh2'.
        auto mesh1Prim = stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh1"});
        EXPECT_TRUE(mesh1Prim);
        auto mesh2Prim = stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh2"});
        EXPECT_TRUE(mesh2Prim);
    }
}

TEST(BifrostDynamicFileFormatTests, boolean_fracture_usd_meshes_compound) {
    auto rootLayerPath = createRootLayerWithDefaultFields(
        "boolean_fracture_usd_meshes_compound",
        "Test::DynamicFileFormat::boolean_fracture_usd_meshes", "shards");

    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Test with some params fields
    {
        // clang-format off
        setParamsField(stage, VtDictionary{
                {"file", VtValue{std::string{""}}},
                {"generate_slicer_planes", VtValue{bool{true}}},
                {"slicer_plane_count", VtValue{GfVec3i{0, 2, 0}}},
                {"random_seed", VtValue{Amino::long_t{0}}},
                {"orientation_spread", VtValue{float{0.5f}}},
                {"expand_amount", VtValue{float{1.5f}}},
                {"apply_displacement", VtValue{bool{false}}},
                {"display_planes", VtValue{bool{false}}}
            }
        );
        // clang-format on

        auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
        EXPECT_TRUE(rootPrim);

        // Check output from graph's regular output:
        auto geoPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo"});
        ASSERT_TRUE(geoPrim);
        EXPECT_EQ(geoPrim.GetChildrenNames().size(), 4u);
        auto mesh1Prim = stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh1"});
        EXPECT_TRUE(mesh1Prim);
        auto mesh1 = UsdGeomMesh{mesh1Prim};
        ASSERT_TRUE(mesh1);
        EXPECT_EQ(mesh1.GetFaceCount(), 528);
    }

    // Test with some attribute overrides
    {
        // clang-format off
        setAttributeOverrides(stage,
            std::vector<AttributeNameAndValue> {
                {TfToken{"file"}, {SdfValueTypeNames->Asset, VtValue{SdfAssetPath{"polygonal_mesh_cube.usd"}}}}
            }
        );
        // clang-format on

        auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
        EXPECT_TRUE(rootPrim);

        // Check output from graph's regular output:
        auto geoPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo"});
        ASSERT_TRUE(geoPrim);
        EXPECT_EQ(geoPrim.GetChildrenNames().size(), 1u);
        auto cubePrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/cube"});
        EXPECT_TRUE(cubePrim);
        auto cube = UsdGeomMesh{cubePrim};
        ASSERT_TRUE(cube);
        EXPECT_EQ(cube.GetFaceCount(), 2);
    }
}

TEST(BifrostDynamicFileFormatTests,
     create_mesh_capsule_with_terminal_compound) {
    auto rootLayerPath = createRootLayerWithDefaultFields(
        "create_mesh_capsule_with_terminal_compound",
        "Test::DynamicFileFormat::create_mesh_capsule_with_terminal",
        ""); // no regular output name, so only terminal outputs are present

    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);
    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    EXPECT_TRUE(rootPrim);

    // No regular output is requested from the graph.
    // The "geo" prim should be absent.
    auto geoPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo"});
    EXPECT_FALSE(geoPrim);

    // The diagnostic terminal output is enabled, so geo_guide should contain a
    // BBox geom.
    auto guidePrim = stage->GetPrimAtPath(SdfPath{"/Root/geo_guide"});
    EXPECT_TRUE(guidePrim);
    auto guideMeshPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo_guide/mesh1"});
    EXPECT_TRUE(guideMeshPrim);
    auto guideMesh = UsdGeomMesh{guideMeshPrim};
    ASSERT_TRUE(guideMesh);
    EXPECT_EQ(guideMesh.GetFaceCount(), 6);

    // The proxy terminal output is enabled, so geo_proxy should contain a
    // low-res geom.
    auto proxyPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo_proxy"});
    EXPECT_TRUE(proxyPrim);
    auto proxyMeshPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo_proxy/mesh1"});
    EXPECT_TRUE(proxyMeshPrim);
    auto proxyMesh = UsdGeomMesh{proxyMeshPrim};
    ASSERT_TRUE(proxyMesh);
    EXPECT_EQ(proxyMesh.GetFaceCount(), 80);

    // The final terminal output is enabled, so geo_render should contain a
    // full-res geom.
    auto finalPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo_render"});
    EXPECT_TRUE(finalPrim);
    auto finalMeshPrim =
        stage->GetPrimAtPath(SdfPath{"/Root/geo_render/mesh1"});
    EXPECT_TRUE(finalMeshPrim);
    auto finalMesh = UsdGeomMesh{finalMeshPrim};
    ASSERT_TRUE(finalMesh);
    EXPECT_EQ(finalMesh.GetFaceCount(), 1320);
}

TEST(BifrostDynamicFileFormatTests, graphs_scatter_valley_forest_compound) {
    auto rootLayerPath = createRootLayerWithDefaultFields(
        "graphs_scatter_valley_forest_compound",
        "Graphs::Scatter::valley_forest", "out_points");

    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);
    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    EXPECT_TRUE(rootPrim);

    {
        auto geoPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo"});
        EXPECT_TRUE(geoPrim);
        auto instancer =
            UsdGeomPointInstancer::Get(stage, SdfPath{"/Root/geo/instancer"});
        ASSERT_TRUE(instancer);

        // Check total number of instances:
        // PERFORMANCE NOTE: we get the attribute value as a type-erased VtValue
        // + call GetArraySize() on it -> no array allocation, no element copy.
        size_t  expectedInstanceCount = 11380;
        VtValue val;
        instancer.GetProtoIndicesAttr().Get(&val);
        EXPECT_EQ(val.GetArraySize(), expectedInstanceCount);
        instancer.GetPositionsAttr().Get(&val);
        EXPECT_EQ(val.GetArraySize(), expectedInstanceCount);

        // Check the prototypes being used for instances:
        SdfPathVector   protoPaths;
        UsdRelationship protosRel = instancer.GetPrototypesRel();
        ASSERT_TRUE(protosRel);
        EXPECT_TRUE(protosRel.GetForwardedTargets(&protoPaths));
        size_t expectedProtoCount = 10u;
        ASSERT_EQ(protoPaths.size(), expectedProtoCount);
        auto firstProtoPath = SdfPath{"/Root/geo/instancer/prototypes/obj_0"};
        auto lastProtoPath  = SdfPath{"/Root/geo/instancer/prototypes/obj_9"};
        EXPECT_EQ(protoPaths[0], firstProtoPath);
        EXPECT_EQ(protoPaths[expectedProtoCount - 1], lastProtoPath);

        const std::array<std::pair<SdfPath, size_t>, 2> protoChecks{{
            {firstProtoPath, 2136},
            {lastProtoPath, 2136},
        }};
        for (const auto& [protoPath, expectedFaceCount] : protoChecks) {
            TfToken purpose;
            auto    protoScope = UsdGeomScope::Get(stage, protoPath);
            EXPECT_TRUE(protoScope);

            auto protoRenderMesh = UsdGeomMesh::Get(
                stage, protoPath.AppendChild(TfToken{"_render"}));
            ASSERT_TRUE(protoRenderMesh);
            EXPECT_EQ(protoRenderMesh.GetFaceCount(), expectedFaceCount);
            EXPECT_TRUE(protoRenderMesh.GetPurposeAttr().Get(&purpose));
            EXPECT_EQ(purpose, UsdGeomTokens->render);

            auto protoProxyMesh = UsdGeomMesh::Get(
                stage, protoPath.AppendChild(TfToken{"_proxy"}));
            ASSERT_TRUE(protoProxyMesh);
            EXPECT_TRUE(protoProxyMesh.GetPurposeAttr().Get(&purpose));
            EXPECT_EQ(purpose, UsdGeomTokens->proxy);
        }
    }
    {
        // Check that geo_render scope contains a mesh used for "ground" render
        auto renderScope = stage->GetPrimAtPath(SdfPath{"/Root/geo_render"});
        EXPECT_TRUE(renderScope);
        auto renderMeshPrim =
            stage->GetPrimAtPath(SdfPath{"/Root/geo_render/mesh1"});
        EXPECT_TRUE(renderMeshPrim);
        auto renderMesh = UsdGeomMesh{renderMeshPrim};
        ASSERT_TRUE(renderMesh);
        VtValue val;
        renderMesh.GetPointsAttr().Get(&val);
        EXPECT_EQ(val.GetArraySize(), 40401u);
        renderMesh.GetDisplayColorAttr().Get(&val);
        EXPECT_EQ(val.GetArraySize(), 40401u);
    }
}

TEST(BifrostDynamicFileFormatTests, animated_mesh_deformed) {
    std::string stageRootFilePath =
        BifrostUsd::TestUtils::getResourcePath("animated_mesh_deformed.usd")
            .c_str();

    auto stage = UsdStage::Open(stageRootFilePath);
    ASSERT_TRUE(stage);

    // Retrieve the Bifrost timeline start/end frame range:
    auto rootPrim       = stage->GetPrimAtPath(SdfPath{"/Root"});
    auto startFrameAttr = rootPrim.GetAttribute(
        TfToken{"BifrostGraph_Globals:timeline_info_start_frame"});
    ASSERT_TRUE(startFrameAttr);
    double startFrame;
    startFrameAttr.Get<double>(&startFrame);
    long startFrame_long = static_cast<long>(startFrame);
    auto endFrameAttr = rootPrim.GetAttribute(
        TfToken{"BifrostGraph_Globals:timeline_info_end_frame"});
    ASSERT_TRUE(endFrameAttr);
    double endFrame;
    endFrameAttr.Get<double>(&endFrame);
    long endFrame_long = static_cast<long>(endFrame);
    long animatedFrames = endFrame_long - startFrame_long + 1;
    ASSERT_GT(animatedFrames, 0);

    // Retrieve the OpenUSD start/end time code range:
    double startTimeCode = stage->GetStartTimeCode();
    double endTimeCode   = stage->GetEndTimeCode();

    // The compound being executed uses the timeline settings only to generate
    // a number of frames, not an animation at exact time codes.
    // So we first check that OpenUSD time code range to be larger than Bifrost
    // frame range:
    double timeCodeRange = endTimeCode - startTimeCode;
    double frameRange    = endFrame - startFrame;
    ASSERT_GT(timeCodeRange, frameRange)
        << "This test requires the OpenUSD time code range to be larger than "
           "the Bifrost frame range. "
        << "OpenUSD time code range: [" << startTimeCode << ", " << endTimeCode
        << "], "
        << "Bifrost frame range: [" << startFrame << ", " << endFrame << "]";

    auto geoPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo"});
    EXPECT_TRUE(geoPrim);
    auto meshPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"});
    EXPECT_TRUE(meshPrim);
    auto mesh = UsdGeomMesh{meshPrim};
    ASSERT_TRUE(mesh);

    // Then check that the mesh gets deformed over the whole Bifrost frame
    // range, but it then remains constant over remaining time code range of
    // OpenUSD (for which no animation was created by Bifrost).
    auto                pointsAttr = mesh.GetPointsAttr();
    std::vector<double> timeSamples;
    pointsAttr.GetTimeSamples(&timeSamples);
    ASSERT_EQ(timeSamples.size(), animatedFrames);

    // 1. Get position of the first point at timeCode==0:
    VtVec3fArray pointArray;
    pointsAttr.Get(&pointArray, UsdTimeCode{0.0});
    size_t pointCount = pointArray.size();
    ASSERT_GT(pointCount, 0);
    GfVec3f currentPosition, previousPosition = pointArray[0];

    // 2. Check that the first point moves at each time samples for the duration
    //  of the Bifrost frame range, with the animation remapped to start at t=0:
    for (long timeCode = 1; timeCode < animatedFrames; ++timeCode) {
        pointsAttr.Get(&pointArray, UsdTimeCode{static_cast<double>(timeCode)});
        ASSERT_EQ(pointArray.size(), pointCount)
            << "PointsAttr does not have the expected number of points at "
               "timeCode "
            << timeCode;
        currentPosition = pointArray[0];
        EXPECT_NE(previousPosition, currentPosition)
            << "Point did not move at timeCode " << timeCode << " (pos[t"
            << (timeCode - 1) << "]=" << previousPosition << ", pos[t"
            << timeCode << "]=" << currentPosition << ")";
        previousPosition = currentPosition;
    }

    // 3. Check that first point stops moving for remaining OpenUSD time samples
    for (long timeCode = animatedFrames;
         timeCode <= static_cast<long>(endTimeCode); ++timeCode) {
        pointsAttr.Get(&pointArray, UsdTimeCode{static_cast<double>(timeCode)});
        ASSERT_EQ(pointArray.size(), pointCount)
            << "PointsAttr does not have the expected number of points at "
               "timeCode "
            << timeCode;
        currentPosition = pointArray[0];
        EXPECT_EQ(previousPosition, currentPosition)
            << "Point moved at timeCode " << timeCode << " (pos[t" << timeCode
            << "]=" << currentPosition << ")";
    }
}

TEST(BifrostDynamicFileFormatTests, move_up_point_simulation_compound) {
    auto rootLayerPath = createRootLayerWithDefaultFields(
        "move_up_point_simulation_compound",
        "Test::DynamicFileFormat::move_up_point_simulation", "point");

    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    double startFrame = 1;
    stage->SetStartTimeCode(startFrame);
    EXPECT_EQ(stage->GetStartTimeCode(), startFrame);
    double endFrame = 2;
    stage->SetEndTimeCode(endFrame);
    EXPECT_EQ(stage->GetEndTimeCode(), endFrame);

    // This graph runs a pseudo-simulation feedback loop that moves a point up
    // on Y-axis by <verticalStep> units per frame, so we expect the point to be
    // at (0,0,0) at the first frame, and at (0,<verticalStep>,0) at the second
    // frame.
    constexpr float verticalStep = 1.f;
    {
        // clang-fomat off
        setGlobalsField(stage, VtDictionary{
                {"timeline_info_start_frame", VtValue{startFrame}},
                {"timeline_info_end_frame", VtValue{endFrame}},
            }
        );
        setParamsField(stage, VtDictionary{
                 {"vertical_step", VtValue{verticalStep}},
            }
        );
        // clang-format on

        auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
        EXPECT_TRUE(rootPrim);
        auto geoPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo"});
        EXPECT_TRUE(geoPrim);
        auto pointsPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/points"});
        EXPECT_TRUE(pointsPrim);

        auto pointsGeom = UsdGeomPoints{pointsPrim};
        ASSERT_TRUE(pointsGeom);
        auto pointsAttr = pointsGeom.GetPointsAttr();
        ASSERT_TRUE(pointsAttr);
        EXPECT_TRUE(pointsAttr.HasValue());

        // Test first frame
        VtVec3fArray points;
        EXPECT_TRUE(pointsAttr.Get(&points, UsdTimeCode{startFrame}));
        ASSERT_GT(points.size(), 0);
        auto expectedFirstPoint = GfVec3f{0.f, 0.f, 0.f};
        EXPECT_EQ(points[0], expectedFirstPoint);

        // Test last frame
        EXPECT_TRUE(pointsAttr.Get(&points, UsdTimeCode{endFrame}));
        ASSERT_GT(points.size(), 0);
        auto expectedLastPoint = GfVec3f{0.f, verticalStep, 0.f};
        EXPECT_EQ(points[0], expectedLastPoint);
    }
}

TEST(BifrostDynamicFileFormatTests, spectral_wave_simulation_compound) {
    auto rootLayerPath = createRootLayerWithDefaultFields(
        "spectral_wave_simulation_compound",
        "Test::DynamicFileFormat::spectral_wave_simulation",
        "geometry_with_material");

    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    double startFrame = 1;
    stage->SetStartTimeCode(startFrame);
    double endFrame = 5;
    stage->SetEndTimeCode(endFrame);
    double fps = 60;
    stage->SetFramesPerSecond(fps);

    // Test with some params fields
    {
        // clang-fomat off
        setGlobalsField(stage, VtDictionary{
                {"timeline_info_start_frame", VtValue{startFrame}},
                {"timeline_info_end_frame", VtValue{endFrame}},
                {"time_fps", VtValue{fps}},
            }
        );
        setParamsField(stage, VtDictionary{
                 {"length", VtValue{float{2}}},
                 {"width", VtValue{float{2}}},
                 {"segments", VtValue{int{2}}},
                 {"spectral_wave_space_scale", VtValue{float{1}}},
                 {"spectral_wave_speed_scale", VtValue{float{1}}},
                 {"spectral_wave_resolution", VtValue{int{256}}},
                 {"spectral_wave_ocean_depth", VtValue{float{10}}},
                 {"spectral_wave_height", VtValue{float{2}}},
            }
        );
        // clang-format on

        auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
        EXPECT_TRUE(rootPrim);
        auto geoPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo"});
        EXPECT_TRUE(geoPrim);
        auto meshPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/surface"});
        EXPECT_TRUE(meshPrim);
        
        auto meshGeom = UsdGeomMesh{meshPrim};
        ASSERT_TRUE(meshGeom);
        auto pointsAttr = meshGeom.GetPointsAttr();
        ASSERT_TRUE(pointsAttr);
        EXPECT_TRUE(pointsAttr.HasValue());

        // Compare the first point's position at first frame and last frame:
        VtVec3fArray points;
        EXPECT_TRUE(pointsAttr.Get(&points, UsdTimeCode{startFrame}));
        EXPECT_EQ(points.size(), 9);
        GfVec3f firstPosition = points[0];
        EXPECT_TRUE(pointsAttr.Get(&points, UsdTimeCode{endFrame}));
        EXPECT_EQ(points.size(), 9);
        GfVec3f lastPosition = points[0];
        EXPECT_NE(firstPosition, lastPosition);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
