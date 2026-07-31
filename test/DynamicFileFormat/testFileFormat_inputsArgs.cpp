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
#include <BifrostUsd/GraphExecutor/GraphExecutor.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorTypes.h>
#include <utils/test/testUtils.h>

// Open USD
#include <pxr/base/gf/vec3f.h>
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
using BifrostUsd::GraphExecutor::VerbosityLevel;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

UniqueTestOutputSubdir g_OutputDir{"testFileFormat_inputsArgs",
                                   true /*autoDelete*/};

// Attribute prefix for Bifrost Inputs overrides on the Root prim.
constexpr const char* kInputsAttrPrefix = "bifrost:in:";

} // namespace

TEST(Inputs, invalid_port_name_field) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "Inputs_invalid_port_name_field";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, "Test::DynamicFileFormat::read_mesh_from_file",
        "meshes");
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Test with an invalid input port names as fields.
    setLiveInputsField(stage,
                       VtDictionary{{"a_unknown", VtValue{bool{true}}},
                                    {"b_unknown", VtValue{"something"}},
                                    {"file", VtValue{"filename.usda"}},
                                    {"c_unknown", VtValue{int{10}}}});

    // Composition should fail.
    // Graph execution should fail because of the invalid input port fields.
    // Consequence: /Root is present but carries no children.
    EXPECT_TRUE(
        dffHasErrorRegex(collector, bucketKey,
                         std::regex{R"(unknown input port[^\n]*a_unknown)"}));
    EXPECT_TRUE(
        dffHasErrorRegex(collector, bucketKey,
                         std::regex{R"(unknown input port[^\n]*b_unknown)"}));
    EXPECT_TRUE(
        dffHasErrorRegex(collector, bucketKey,
                         std::regex{R"(unknown input port[^\n]*c_unknown)"}));
    EXPECT_FALSE(
        dffHasError(collector, bucketKey,
                    "file")); // No error about "file" port, which is valid.
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
}

TEST(Inputs, invalid_field_type) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "Inputs_invalid_field_type";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, "Test::DynamicFileFormat::read_mesh_from_file",
        "meshes");
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Test with an invalid input field type (int instead of string).
    setLiveInputsField(stage, VtDictionary{{"file", VtValue{int{10101}}}});

    // Both composition and inputs arguments retrieval in Read() should succeed.
    // Graph execution should fail because of the invalid field type.
    // Consequence: /Root is present but carries no children.
    EXPECT_TRUE(
        dffHasErrorRegex(collector, bucketKey,
                         std::regex{R"(\"file\"[^\n]*port type does not match)",
                                    std::regex_constants::icase}));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
}

TEST(Inputs, valid_field_value) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "Inputs_valid_field_value";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, "Test::DynamicFileFormat::read_mesh_from_file",
        "meshes");
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Test with a valid input field type
    const std::string cubeFilePath = {
        BifrostUsd::TestUtils::getResourcePath("simple_cube.usda").c_str()};
    setLiveInputsField(
        stage,
        VtDictionary{{"file", VtValue{cubeFilePath}}});

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

TEST(Inputs, invalid_port_name_attribute) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "Inputs_invalid_port_name_attribute";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, "Test::DynamicFileFormat::read_mesh_from_file",
        "meshes");
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // First set a valid input field
    const std::string cubeFilePath = {
        BifrostUsd::TestUtils::getResourcePath("simple_cube.usda").c_str()};
    setLiveInputsField(stage, VtDictionary{{"file", VtValue{cubeFilePath}}});

    // Test with input attributes where some port names are invalid.
    // PcpDynamicFileFormatContext can only query attributes by name; it cannot
    // enumerate all attributes on the prim. The DFF therefore only probes
    // "bifrost:in:<portName>" attributes for port names it already knows from
    // the graph (getInputPortNames()). Attributes with unknown port names are
    // silently ignored — no error is emitted for them.
    setLiveAttributeOverrides(
        stage, std::vector<AttributeNameAndValue>{
                   {TfToken{std::string{kInputsAttrPrefix} + "a_unknown"},
                    {SdfValueTypeNames->String, VtValue{"something"}}},
                   {TfToken{std::string{kInputsAttrPrefix} + "b_unknown"},
                    {SdfValueTypeNames->Int, VtValue{10}}},
                   {TfToken{std::string{kInputsAttrPrefix} + "file"},
                    {SdfValueTypeNames->String, VtValue{cubeFilePath}}},
                   {TfToken{std::string{kInputsAttrPrefix} + "c_unknown"},
                    {SdfValueTypeNames->Bool, VtValue{true}}}});

    // Unknown port name attributes are silently ignored. The valid
    // "bifrost:in:file" attribute override is applied. Graph execution
    // succeeds.
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    auto cubePrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/cube"});
    ASSERT_TRUE(cubePrim);
    auto cubeMesh = UsdGeomMesh{cubePrim};
    ASSERT_TRUE(cubeMesh);
    EXPECT_EQ(cubeMesh.GetFaceCount(), 6);
}

TEST(Inputs, attribute_only_no_field) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "Inputs_attribute_only_no_field";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, "Test::DynamicFileFormat::read_mesh_from_file",
        "meshes");
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Test with a valid input attribute, no corresponding field being present.
    const std::string cubeFilePath = {
        BifrostUsd::TestUtils::getResourcePath("simple_cube.usda").c_str()};
    const std::string attrName = std::string{kInputsAttrPrefix} + "file";
    setLiveAttributeOverrides(
        stage, std::vector<AttributeNameAndValue>{
                   {TfToken{attrName},
                    {SdfValueTypeNames->String, VtValue{cubeFilePath}}}});

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

TEST(Inputs, attribute_type_mismatch_override_rejected) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "Inputs_attr_type_mismatch_rejected";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, "Test::DynamicFileFormat::read_mesh_from_file",
        "meshes");
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set a valid input field first
    const std::string cubeFilePath = {
        BifrostUsd::TestUtils::getResourcePath("simple_cube.usda").c_str()};
    setLiveInputsField(stage, VtDictionary{{"file", VtValue{cubeFilePath}}});
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    auto cubePrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/cube"});
    ASSERT_TRUE(cubePrim);
    auto cubeMesh = UsdGeomMesh{cubePrim};
    ASSERT_TRUE(cubeMesh);
    EXPECT_EQ(cubeMesh.GetFaceCount(), 6);
    collector.clear();

    // Then override the input field with an input attribute with different type
    const std::string attrName = std::string{kInputsAttrPrefix} + "file";
    setLiveAttributeOverrides(
        stage, std::vector<AttributeNameAndValue>{
                   {TfToken{attrName},
                    {SdfValueTypeNames->Bool, VtValue{bool{true}}}}});

    // Graph fails and produces no child under /Root.
    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{
            R"(override for[^\n]*bifrost:in:file[^\n]*but the field declared type is[^\n]*string)",
            std::regex_constants::icase}));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);
}

TEST(Inputs, attribute_override_succeeds) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "Inputs_attribute_override_succeeds";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, "Test::DynamicFileFormat::read_mesh_from_file",
        "meshes");
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set a valid input field first
    const std::string cubeFilePath = {
        BifrostUsd::TestUtils::getResourcePath("simple_cube.usda").c_str()};
    setLiveInputsField(
        stage,
        VtDictionary{{"file", VtValue{cubeFilePath}}});
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    auto cubePrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/cube"});
    ASSERT_TRUE(cubePrim);
    auto cubeMesh = UsdGeomMesh{cubePrim};
    ASSERT_TRUE(cubeMesh);
    EXPECT_EQ(cubeMesh.GetFaceCount(), 6);
    collector.clear();

    // Then override the input field with a valid input attribute.
    const std::string capsuleFilePath = {
        BifrostUsd::TestUtils::getResourcePath("simple_capsule.usda").c_str()};
    const std::string attrName = std::string{kInputsAttrPrefix} + "file";
    setLiveAttributeOverrides(
        stage, std::vector<AttributeNameAndValue>{
                   {TfToken{attrName},
                    {SdfValueTypeNames->String, VtValue{capsuleFilePath}}}});

    // Graph execution succeeds and produces a mesh child.
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    auto capsulePrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/capsule"});
    ASSERT_TRUE(capsulePrim);
    auto capsuleMesh = UsdGeomMesh{capsulePrim};
    ASSERT_TRUE(capsuleMesh);
    EXPECT_EQ(capsuleMesh.GetFaceCount(), 5);
}

PXR_NAMESPACE_CLOSE_SCOPE
