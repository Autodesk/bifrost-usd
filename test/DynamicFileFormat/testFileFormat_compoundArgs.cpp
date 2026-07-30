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

// DFF Headers
#include "dffDiagnosticsRuntime.h"

// DFF test helpers
#include "dffTestDiagnostics.h"
#include "dffTestLayerHelpers.h"

// Bifrost USD
#include <utils/test/testUtils.h>

// Open USD
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>

// C++ Standard Library
#include <string>

using namespace BifrostUsd::TestUtils;
using namespace DffTestHelpers;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

UniqueTestOutputSubdir g_OutputDir{"testFileFormat_compoundArgs",
                                   true /*autoDelete*/};

constexpr const char* kCompoundNameAttr = "bifrost:compound:name";

void setDiagnosticsBucketAttr(const UsdPrim&     rootPrim,
                              const std::string& bucketKey) {
    auto attr = rootPrim.CreateAttribute(TfToken{kDffDiagnosticsBucketAttr.data()},
                                         SdfValueTypeNames->String,
                                         /*custom=*/false);
    ASSERT_TRUE(attr) << "Failed to create \"" << kDffDiagnosticsBucketAttr
                      << "\" attribute";
    attr.Set(bucketKey);
}

} // namespace

// ---------------------------------------------------------------------------
// CompoundField tests — bifrostCompound { string name = "..." }
// ---------------------------------------------------------------------------

// The compoundName argument is mandatory.
TEST(CompoundField, no_compound_name_at_all) {
    DffDiagnosticCollector collector;
    const char*            bucketKey = "no_compound_name_at_all";
    auto layerPath = createRootLayerWithDff(bucketKey, g_OutputDir);

    auto stage = UsdStage::Open(layerPath.c_str());
    ASSERT_TRUE(stage);
    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    EXPECT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);

    EXPECT_TRUE(
        dffHasError(collector, bucketKey,
                    "required \"bifrost:compound:name\" argument is missing"));
}

/// A known valid compound with no output port configured. The stage opens
/// successfully and the /Root prim is valid, but the DFF plugin reports an
/// error because no bifrostOutputs and no terminal port were specified.
TEST(CompoundField, valid_compound_no_output) {
    DffDiagnosticCollector collector;
    const char*            bucketKey = "valid_compound_no_output";
    auto layerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, "Modeling::Primitive::create_mesh_cube",
        "" /*no output will be reported as an error*/);

    auto stage = UsdStage::Open(layerPath.c_str());
    ASSERT_TRUE(stage);
    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    EXPECT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);

    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{
            R"([^\n]*produced no output[^\n]*specify an output port[^\n]*or enable a terminal port)",
            std::regex::icase}));
}

// The bifrost:compound:name attribute can supply the compound name even when
// the bifrostCompound field carries no name. The bifrostOutputs field must
// still identify which output port to expose ("mesh" here).
TEST(CompoundField, attribute_but_no_field) {
    const char* bucketKey = "attribute_but_no_field";
    auto        layerPath = createRootLayerWithDff(bucketKey, g_OutputDir);
    auto        stage     = UsdStage::Open(layerPath.c_str());
    ASSERT_TRUE(stage);

    // Set outputs field. This triggers a recompose that still yields no
    // children because no bifrost:compound:name has been set yet.
    setLiveOutputsField(stage, {"mesh"});
    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    EXPECT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);

    // Now set the compound name via the attribute, not the field.
    DffDiagnosticCollector collector;
    const char* compoundName = "Test::DynamicFileFormat::create_mesh_cube";
    setLiveAttributeOverrides(stage,
                                std::vector<AttributeNameAndValue>{
                                    {TfToken{kCompoundNameAttr},
                                    {SdfValueTypeNames->String,
                                    VtValue{std::string{compoundName}}}},
                                });
    EXPECT_GT(rootPrim.GetChildrenNames().size(), 0u);
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
}

/// When the bifrostCompound field names a non-existent compound the payload
/// fails to load but the stage itself still opens. The /Root prim has no
/// children from the graph.
TEST(CompoundField, not_found_compound) {
    DffDiagnosticCollector collector;
    const char*            bucketKey     = "not_found_compound";
    auto                   rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, "Not::Found", "ignored");
    auto stage = UsdStage::Open(rootLayerPath.c_str());

    // The Stage opens successfully; only the payload fails to load, leaving
    // the /Root prim with no children from the graph output:
    ASSERT_TRUE(stage);
    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    EXPECT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);

    EXPECT_TRUE(dffHasError(collector, bucketKey, "Not::Found"));
    EXPECT_TRUE(dffHasError(
        collector, bucketKey,
        "Failed to create a GraphExecutor for graph \"Not::Found\"."));
}

/// An empty compound name is invalid. DFF Diagnostics must report the empty
/// name error and the stage's /Root prim has no children.
TEST(CompoundField, empty_compound_name_emits_error) {
    DffDiagnosticCollector collector;
    const char*            bucketKey = "empty_compound_name_emits_error";
    auto                   layerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, "" /*empty*/, "ignored");

    auto stage = UsdStage::Open(layerPath.c_str());

    ASSERT_TRUE(stage);
    EXPECT_EQ(stage->GetPrimAtPath(SdfPath{"/Root"}).GetChildrenNames().size(),
              0u);

    // The plugin logs that the compound name attribute is empty.
    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"([^\n]*bifrost:compound:name[^\n]*empty value)"}));
}

/// Reading the bifrostCompound field directly from a USDA text file (not
/// programmatically created). Verifies that the text parser accepts the field
/// syntax and the plugin picks up the compound name correctly.
TEST(CompoundField, usda_text_file_valid_compound) {
    // animated_mesh_deformed.usd is an existing resource with a valid compound
    // field. The test verifies the stage opens and /Root/geo/mesh is a valid
    // mesh.
    auto resourcePath =
        BifrostUsd::TestUtils::getResourcePath("animated_mesh_deformed.usd");

    auto stage = UsdStage::Open(resourcePath.c_str());
    ASSERT_TRUE(stage)
        << "Failed to open animated_mesh_deformed.usd from resources";

    auto meshPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"});
    EXPECT_TRUE(meshPrim)
        << "Failed to find /Root/geo/mesh prim in animated_mesh_deformed.usd";
    auto mesh = UsdGeomMesh{meshPrim};
    EXPECT_TRUE(mesh) << "/Root/geo/mesh is expected to be a UsdGeomMesh in "
                         "animated_mesh_deformed.usd";
}

// ---------------------------------------------------------------------------
// CompoundAttr tests — string bifrost:compound:name = "..." attribute override
// ---------------------------------------------------------------------------

/// When the attribute override names the same compound as the field, behaviour
/// is identical to having only the field. The stage opens, /Root is valid.
TEST(CompoundAttr, attribute_same_as_field_no_change) {
    DffDiagnosticCollector collector;
    const char*            bucketKey = "attribute_same_as_field_no_change";

    const char* compound = "Test::DynamicFileFormat::create_mesh_cube";
    auto layerPath = createRootLayerWithDefaultDffFields(bucketKey, g_OutputDir,
                                                         compound, "mesh");

    // Re-open the saved layer, add the attribute, and save again.
    auto layer = SdfLayer::FindOrOpen(layerPath.c_str());
    ASSERT_TRUE(layer);
    setOfflineCompoundNameAttr(layer, compound);

    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    auto stage = UsdStage::Open(layerPath.c_str());
    ASSERT_TRUE(stage);
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
}

/// When the attribute override names a different valid compound, that compound
/// is used instead of the one in the field. The stage opens successfully with
/// the override compound's output as children of /Root.
TEST(CompoundAttr, attribute_overrides_field_with_valid_compound) {
    DffDiagnosticCollector collector;
    const char*            bucketKey = "attr_overrides_field_valid";

    const char* compound1 = "Test::DynamicFileFormat::create_mesh_cube";
    auto        layerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, compound1, "mesh");

    auto layer = SdfLayer::FindOrOpen(layerPath.c_str());
    ASSERT_TRUE(layer);
    EXPECT_TRUE(dffGetErrorMessages(collector, bucketKey).empty())
        << "Unexpected errors when running 1st graph";

    auto stage = UsdStage::Open(layerPath.c_str());
    ASSERT_TRUE(stage);
    auto meshPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"});
    EXPECT_TRUE(meshPrim) << "Failed to find /Root/geo/mesh prim from "
                          << compound1 << " graph";
    auto mesh = UsdGeomMesh{meshPrim};
    ASSERT_TRUE(mesh);
    VtValue val1;
    mesh.GetPointsAttr().Get(&val1);
    EXPECT_GT(val1.GetArraySize(), 0u) << "1st mesh should have some points";

    // Override compoundName with another known compound.
    // Graph should evaluate and this second mesh should have a different number
    // of points than the first mesh.
    const char* compound2 = "Test::DynamicFileFormat::create_mesh_sphere";
    setOfflineCompoundNameAttr(layer, compound2);
    EXPECT_TRUE(dffGetErrorMessages(collector, bucketKey).empty())
        << "Unexpected errors when running 2nd graph";

    meshPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"});
    EXPECT_TRUE(meshPrim) << "Failed to find /Root/geo/mesh prim from "
                          << compound2 << " graph";
    mesh = UsdGeomMesh{meshPrim};
    ASSERT_TRUE(mesh);
    VtValue val2;
    mesh.GetPointsAttr().Get(&val2);
    EXPECT_TRUE(val2.GetArraySize() > 0u &&
                val2.GetArraySize() != val1.GetArraySize())
        << "2nd mesh should have a different number of points";
}

/// When the attribute overrides the field with an unknown compound name the
/// plugin uses the override name, which fails to load.
TEST(CompoundAttr, attribute_overrides_field_with_unknown_compound) {
    DffDiagnosticCollector collector;
    const char*            bucketKey = "attr_overrides_field_unknown";

    const char* compound1 = "Test::DynamicFileFormat::create_mesh_cube";
    auto        layerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, compound1, "ignored");

    auto layer = SdfLayer::FindOrOpen(layerPath.c_str());
    ASSERT_TRUE(layer);
    const char* compound2 = "Override::Does::Not::Exist";
    setOfflineCompoundNameAttr(layer, compound2);

    auto stage = UsdStage::Open(layerPath.c_str());
    ASSERT_TRUE(stage);
    EXPECT_EQ(stage->GetPrimAtPath(SdfPath{"/Root"}).GetChildrenNames().size(),
              0u);

    // The error message must reference the *override* name, not the field name.
    EXPECT_TRUE(dffHasError(collector, bucketKey,
                            "Failed to create a GraphExecutor for graph "
                            "\"Override::Does::Not::Exist\"."));
}

/// When the attribute provides an empty string override, the plugin treats the
/// compound name as empty/invalid. Plugin reports the error and /Root has
/// no children.
TEST(CompoundAttr, attribute_empty_string_override_emits_error) {
    DffDiagnosticCollector collector;
    const char*            bucketKey = "attr_empty_override";

    const char* compound1 = "Test::DynamicFileFormat::create_mesh_cube";
    auto        layerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, compound1, "mesh");

    auto stage = UsdStage::Open(layerPath.c_str());
    ASSERT_TRUE(stage);
    EXPECT_GT(stage->GetPrimAtPath(SdfPath{"/Root"}).GetChildrenNames().size(),
              0u);

    auto layer = SdfLayer::FindOrOpen(layerPath.c_str());
    ASSERT_TRUE(layer);
    setOfflineCompoundNameAttr(layer, "" /*empty*/);

    stage = UsdStage::Open(layerPath.c_str());
    ASSERT_TRUE(stage);
    EXPECT_EQ(stage->GetPrimAtPath(SdfPath{"/Root"}).GetChildrenNames().size(),
              0u);
    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"([^\n]*bifrost:compound:name[^\n]*empty value)"}));
}

/// Attribute override is authorable from a USDA text file. Verify the text
/// parser accepts `string bifrost:compound:name = "..."` on the Root prim.
TEST(CompoundAttr, usda_text_file_attribute_override_unknown_compound) {
    DffDiagnosticCollector collector;
    const char*            bucketKey = "attr_text_override_unknown";
    auto                   resourcePath =
        BifrostUsd::TestUtils::getResourcePath("animated_mesh_deformed.usd");

    // Create an override layer that is the stage root, with the override
    // resource file as a sublayer. Attributes authored on this override layer
    // are saved to disk before each Reload(), so the DFF plugin reads the
    // bucket key and compound name override from disk during recomposition,
    // exactly as a user-authored USDA stronger-opinion layer would behave at
    // runtime.
    auto overridePath =
        g_OutputDir.getPath_abs("attr_text_override_unknown.usda");
    auto fileFormat    = SdfFileFormat::FindByExtension(".usda");
    auto overrideLayer = SdfLayer::New(fileFormat, overridePath.c_str());
    overrideLayer->InsertSubLayerPath(resourcePath.c_str());
    overrideLayer->Save();

    // Open the stage from the override layer as root. USD's default edit
    // target is the root layer, so CreateAttribute calls below write into
    // overrideLayer, not into the read-only resource file.
    auto stage = UsdStage::Open(overridePath.c_str());
    ASSERT_TRUE(stage);
    EXPECT_EQ(stage->GetRootLayer(), overrideLayer)
        << "The override layer should have been opened as the stage root";
    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim.IsValid());
    EXPECT_GT(rootPrim.GetChildrenNames().size(), 0u)
        << "Resource sublayer should have been composed in, and the original "
           "valid compound should have run and produced children under /Root";

    // Register the bucket key on the override layer and save so it survives
    // Reload() (which re-reads all disk-backed layers). No compound override
    // yet: the original valid compound should run cleanly.
    setDiagnosticsBucketAttr(rootPrim, bucketKey);
    overrideLayer->Save();
    stage->Reload();

    // The original valid compound is still in effect (no compound name override
    // yet), so /Root should still have children and no errors or warnings.
    EXPECT_GT(rootPrim.GetChildrenNames().size(), 0u)
        << "No compound override yet, so the original valid compound should "
           "still produce children after registering the bucket key";
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));

    // Author the compound name override. attr.Set() writes to overrideLayer
    // and immediately fires USD change notifications, which cause the stage
    // to recompose synchronously. The DFF plugin runs with the new compound
    // name before control returns from Set().
    constexpr auto unknownCompound = "Attr::Override::Unknown";
    auto attr = rootPrim.CreateAttribute(TfToken{"bifrost:compound:name"},
                                         SdfValueTypeNames->String,
                                         /*custom=*/false);
    ASSERT_TRUE(attr);
    attr.Set(std::string{unknownCompound});

    // The stage opens and /Root is still a valid prim, but the override
    // compound does not exist so the DFF payload fails to load: /Root has no
    // children and the collector holds the expected error.
    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        // Failed to create a GraphExecutor for graph "Attr::Override::Unknown"
        std::regex{
            R"(Failed to create[^\n]*graph[^\n]*Attr::Override::Unknown[^\n]*)"}));
    auto finalRootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    EXPECT_TRUE(finalRootPrim.IsValid());
    EXPECT_EQ(finalRootPrim.GetChildrenNames().size(), 0u);
}

PXR_NAMESPACE_CLOSE_SCOPE
