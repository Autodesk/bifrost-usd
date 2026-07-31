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

// DFF test helpers
#include "dffTestDiagnostics.h"
#include "dffTestLayerHelpers.h"

// Bifrost USD
#include <utils/test/testUtils.h>

// Open USD
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>

// C++ Standard Library
#include <regex>
#include <string>
#include <vector>

using namespace BifrostUsd::TestUtils;
using namespace DffTestHelpers;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

UniqueTestOutputSubdir g_OutputDir{"testFileFormat_outputsArgs",
                                   true /*autoDelete*/};

// Attribute prefix for Bifrost Outputs overrides on the Root prim.
constexpr const char* kOutputsAttrPrefix = "bifrost:out:";

} // namespace

TEST(Outputs, invalid_port_name_field) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "Outputs_invalid_port_name_field";

    // Create a layer with no initial bifrostOutputs (empty array).
    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, "Test::DynamicFileFormat::read_mesh_from_file");
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set a valid input so the graph could otherwise run.
    const std::string cubeFilePath = {
        BifrostUsd::TestUtils::getResourcePath("simple_cube.usda").c_str()};
    setLiveInputsField(stage, VtDictionary{{"file", VtValue{cubeFilePath}}});

    // Now set an outputs field containing unknown and empty port names.
    setLiveOutputsField(stage, {"unknown1", "meshes", "", "unknown2"});

    // Composition should fail because "unknown1" and "unknown2" are not valid,
    // and empty port names are not allowed. The valid "meshes" entry is ignored
    // due to all-or-nothing semantics.
    EXPECT_TRUE(
        dffHasErrorRegex(collector, bucketKey,
                         std::regex{R"(unknown output port[^\n]*unknown1)"}));
    EXPECT_TRUE(
        dffHasErrorRegex(collector, bucketKey,
                         std::regex{R"(unknown output port[^\n]*unknown2)"}));
    EXPECT_TRUE(dffHasError(collector, bucketKey,
                            "contains an empty output port name"));
    EXPECT_FALSE(dffHasError(collector, bucketKey, "meshes"));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo/cube"}).IsValid());
}

TEST(Outputs, valid_field_value) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "Outputs_valid_field_value";

    // Create a layer with "meshes" as the output port field.
    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, "Test::DynamicFileFormat::read_mesh_from_file",
        "meshes");
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    const std::string cubeFilePath = {
        BifrostUsd::TestUtils::getResourcePath("simple_cube.usda").c_str()};
    setLiveInputsField(stage, VtDictionary{{"file", VtValue{cubeFilePath}}});

    // Graph execution succeeds and produces a mesh child.
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    auto cubePrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/cube"});
    ASSERT_TRUE(cubePrim);
    auto cubeMesh = UsdGeomMesh{cubePrim};
    ASSERT_TRUE(cubeMesh);
    EXPECT_EQ(cubeMesh.GetFaceCount(), 6);
}

TEST(Outputs, attribute_only_no_field) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "Outputs_attribute_only_no_field";

    // Create a layer with an empty bifrostOutputs field (no field entries).
    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, "Test::DynamicFileFormat::read_mesh_from_file");
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    const std::string cubeFilePath = {
        BifrostUsd::TestUtils::getResourcePath("simple_cube.usda").c_str()};
    setLiveInputsField(stage, VtDictionary{{"file", VtValue{cubeFilePath}}});

    // Without any outputs field entries, the graph should fail.
    EXPECT_TRUE(dffHasError(collector, bucketKey,
                            "produced no output: specify an output port"));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo/cube"}).IsValid());
    collector.clear();

    // Specify the output port solely via a "bifrost:out:meshes" attribute,
    // with no corresponding bifrostOutputs field entry.
    const std::string attrName = std::string{kOutputsAttrPrefix} + "meshes";
    setLiveAttributeOverrides(
        stage, std::vector<AttributeNameAndValue>{
                   {TfToken{attrName},
                    {SdfValueTypeNames->Token, VtValue{TfToken{"meshes"}}}}});

    // Now graph execution succeeds and produces a mesh child.
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    auto cubePrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/cube"});
    ASSERT_TRUE(cubePrim);
    auto cubeMesh = UsdGeomMesh{cubePrim};
    ASSERT_TRUE(cubeMesh);
    EXPECT_EQ(cubeMesh.GetFaceCount(), 6);
}

TEST(Outputs, attribute_override_succeeds) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "Outputs_attribute_override_succeeds";

    // Create a layer with "meshes" as the output port via the field.
    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, "Test::DynamicFileFormat::read_mesh_from_file",
        "meshes");
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    const std::string cubeFilePath = {
        BifrostUsd::TestUtils::getResourcePath("simple_cube.usda").c_str()};
    setLiveInputsField(stage, VtDictionary{{"file", VtValue{cubeFilePath}}});

    // Initial execution via field succeeds.
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    auto cubePrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/cube"});
    ASSERT_TRUE(cubePrim);
    EXPECT_EQ(UsdGeomMesh{cubePrim}.GetFaceCount(), 6);
    collector.clear();

    // Add a "bifrost:out:meshes" attribute: triggers all-or-nothing override.
    // The attribute-confirmed port list replaces the field-derived list.
    // Since both resolve to "meshes", the graph still executes successfully.
    const std::string attrName = std::string{kOutputsAttrPrefix} + "meshes";
    setLiveAttributeOverrides(
        stage, std::vector<AttributeNameAndValue>{
                   {TfToken{attrName},
                    {SdfValueTypeNames->Token, VtValue{TfToken{"meshes"}}}}});

    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    cubePrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/cube"});
    ASSERT_TRUE(cubePrim);
    EXPECT_EQ(UsdGeomMesh{cubePrim}.GetFaceCount(), 6);
}

TEST(Outputs, invalid_port_name_attribute) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "Outputs_invalid_port_name_attribute";

    // Create a layer with "meshes" as the output port via the field.
    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, "Test::DynamicFileFormat::read_mesh_from_file",
        "meshes");
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    const std::string cubeFilePath = {
        BifrostUsd::TestUtils::getResourcePath("simple_cube.usda").c_str()};
    setLiveInputsField(stage, VtDictionary{{"file", VtValue{cubeFilePath}}});

    // Author both a valid and an unknown "bifrost:out:*" attribute.
    // PcpDynamicFileFormatContext cannot enumerate prim attributes — only
    // port names that appear in getOutputPortNames() are probed for attribute
    // overrides. Attributes whose port name is not a known output port are
    // silently ignored (no error is emitted for them).
    setLiveAttributeOverrides(
        stage,
        std::vector<AttributeNameAndValue>{
            {TfToken{std::string{kOutputsAttrPrefix} + "meshes"},
             {SdfValueTypeNames->Token, VtValue{TfToken{"meshes"}}}},
            {TfToken{std::string{kOutputsAttrPrefix} + "unknown_port"},
             {SdfValueTypeNames->Token, VtValue{TfToken{"unknown_port"}}}}});

    // The unknown attribute is silently ignored. The valid "bifrost:out:meshes"
    // attribute triggers the all-or-nothing override. Graph succeeds.
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    auto cubePrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/cube"});
    ASSERT_TRUE(cubePrim);
    EXPECT_EQ(UsdGeomMesh{cubePrim}.GetFaceCount(), 6);
}

PXR_NAMESPACE_CLOSE_SCOPE
