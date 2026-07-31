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
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/points.h>

// C++ Standard Library
#include <regex>
#include <string>
#include <vector>

using namespace BifrostUsd::TestUtils;
using namespace DffTestHelpers;
using BifrostUsd::GraphExecutor::VerbosityLevel;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

UniqueTestOutputSubdir g_OutputDir{"testFileFormat_globalsArgs",
                                   true /*autoDelete*/};

// Field keys for the bifrostGlobals dictionary (local copies - the production
// tokens live inside the plugin shared library and are not exported).
constexpr const char* kStartFrameField = "timeline_info_start_frame";
constexpr const char* kEndFrameField   = "timeline_info_end_frame";
constexpr const char* kFpsField        = "time_fps";

// Full attribute names for bifrost:global:* overrides on the Root prim.
constexpr const char* kStartFrameAttr =
    "bifrost:global:timeline_info_start_frame";
constexpr const char* kEndFrameAttr = "bifrost:global:timeline_info_end_frame";
constexpr const char* kFpsAttr      = "bifrost:global:time_fps";

// The move_up_point_simulation compound is a pseudo-simulation feedback loop
// that moves a single point up on the Y-axis by vertical_step units per frame.
//   - At startFrame the point is at (0, 0, 0).
//   - At endFrame   the point is at (0, delta * vertical_step, 0),
//     where delta = endFrame - startFrame.
constexpr const char* kCompound =
    "Test::DynamicFileFormat::move_up_point_simulation";
constexpr const char* kOutputPort   = "point";
constexpr float       kVerticalStep = 1.0f;

// The default fps used by the DFF when no valid time_fps globals field is
// found.
constexpr double kDefaultFps = BifrostUsd::GraphExecutor::defaultFps;

/// Return the first simulated point at \p frame, or (0,0,0) if the prim or
/// attribute is unavailable.
GfVec3f getPointAtFrame(const UsdStageRefPtr& stage, double frame) {
    auto pointsPrim = stage->GetPrimAtPath(SdfPath{"/Root/geo/points"});
    if (!pointsPrim) {
        return GfVec3f{0.f, 0.f, 0.f};
    }
    auto pointsGeom = UsdGeomPoints{pointsPrim};
    if (!pointsGeom) {
        return GfVec3f{0.f, 0.f, 0.f};
    }
    VtVec3fArray points;
    pointsGeom.GetPointsAttr().Get(&points, UsdTimeCode{frame});
    if (points.empty()) {
        return GfVec3f{0.f, 0.f, 0.f};
    }
    return points[0];
}

} // namespace

// ===========================================================================
// GlobalsStartFrame - timeline_info_start_frame field and attribute
// ===========================================================================

/// An invalid type for the start_frame field causes the DFF
/// to report an error at read time and fail to load the payload.
/// The /Root prim is valid but has no children.
TEST(GlobalsStartFrame, invalid_field_type) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "GlobalsStartFrame_invalid_field_type";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{bool{true}}},    // wrong type
            {kEndFrameField,   VtValue{double{3.0}}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{
            R"(start_frame[^\n]*has type[^\n]*bool[^\n]*expected[^\n]*double)"}));

    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);
}

/// When start_frame or end_frame is provided, the other must be provided too.
/// start_frame is provided as a field, but end_frame is missing => error.
TEST(GlobalsStartFrame, valid_field_but_no_end_frame) {
    DffDiagnosticCollector collector;
    constexpr auto bucketKey = "GlobalsStartFrame_valid_field_but_no_end_frame";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{double{1}}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(required[^\n]*end_frame[^\n]*is missing)"}));

    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);
}

/// When start_frame or end_frame is provided, the other must be provided too.
/// start_frame is provided as an attribute, but end_frame is missing => error.
TEST(GlobalsStartFrame, valid_attr_but_no_end_frame) {
    DffDiagnosticCollector collector;
    constexpr auto bucketKey = "GlobalsStartFrame_valid_attr_but_no_end_frame";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // clang-format off
    setLiveAttributeOverrides(stage,
        std::vector<AttributeNameAndValue>{
            {TfToken{kStartFrameAttr},
             {SdfValueTypeNames->Double, VtValue{double{10}}}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(required[^\n]*end_frame[^\n]*is missing)"}));

    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);
}

/// end_frame must be greater or equal to start_frame.
TEST(GlobalsStartFrame, start_frame_greater_than_end_frame) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey =
        "GlobalsStartFrame_start_frame_greater_than_end_frame";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{double{1}}},
            {kEndFrameField, VtValue{double{0}}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{
            R"(start_frame[^\n]*must be smaller or equal[^\n]*end_frame)"}));

    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);
}

/// A valid start_frame field controls where the simulation
/// starts. At startFrame the point is at the origin; at endFrame it is at
/// (0, delta*step, 0).
TEST(GlobalsStartFrame, valid_field_value) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "GlobalsStartFrame_valid_field_value";

    constexpr double startFrame = 2.0;
    constexpr double endFrame   = 4.0;
    constexpr float  delta      = static_cast<float>(endFrame - startFrame);

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set verbosity so that warnings and errors are captured.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsAndWarnings);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{startFrame}},
            {kEndFrameField,   VtValue{endFrame}},
            {kFpsField,        VtValue{double{1.0}}},
        }
    );
    setLiveInputsField(stage, VtDictionary{
            {"vertical_step", VtValue{kVerticalStep}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));

    EXPECT_EQ(getPointAtFrame(stage, startFrame), GfVec3f(0.f, 0.f, 0.f));
    EXPECT_EQ(getPointAtFrame(stage, endFrame),
              GfVec3f(0.f, delta * kVerticalStep, 0.f));
}

/// start_frame is supplied only as an attribute - there is no
/// corresponding field entry. end_frame is provided via the fields.
/// The attribute value is consumed directly; it is not an override,
/// the field being absent.
TEST(GlobalsStartFrame, attribute_only_no_field) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "GlobalsStartFrame_attr_only_no_field";

    constexpr double startFrame = 1.0;
    constexpr double endFrame   = 3.0;
    constexpr float  delta      = static_cast<float>(endFrame - startFrame);

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set verbosity so that warnings and errors are captured.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsAndWarnings);

    // Dict carries only end_frame; start_frame will come later from attribute.
    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kEndFrameField, VtValue{endFrame}},
            {kFpsField,      VtValue{double{1.0}}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(required[^\n]*start_frame[^\n]*is missing)"}));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    collector.clear();

    // clang-format off
    setLiveAttributeOverrides(stage,
        std::vector<AttributeNameAndValue>{
            {TfToken{kStartFrameAttr},
             {SdfValueTypeNames->Double, VtValue{startFrame}}},
        }
    );
    setLiveInputsField(stage, VtDictionary{
            {"vertical_step", VtValue{kVerticalStep}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));

    EXPECT_EQ(getPointAtFrame(stage, startFrame), GfVec3f(0.f, 0.f, 0.f));
    EXPECT_EQ(getPointAtFrame(stage, endFrame),
              GfVec3f(0.f, delta * kVerticalStep, 0.f));
}

/// An attribute override for start_frame whose type differs
/// from the field type is rejected with an error.
TEST(GlobalsStartFrame, attribute_type_mismatch_override_rejected) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "GlobalsStartFrame_attr_type_mismatch";

    constexpr double fieldStartFrame = 2.0; // field value
    constexpr double endFrame        = 5.0;

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set verbosity so that warnings and errors are captured.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsAndWarnings);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{fieldStartFrame}}, // correct type
            {kEndFrameField,   VtValue{endFrame}},
            {kFpsField,        VtValue{double{1.0}}},
        }
    );
    // Override with type mismatch: must be rejected with an error.
    setLiveAttributeOverrides(stage,
        std::vector<AttributeNameAndValue>{
            {TfToken{kStartFrameAttr},
             {SdfValueTypeNames->Int, VtValue{int{4}}}},
        }
    );
    setLiveInputsField(stage, VtDictionary{
            {"vertical_step", VtValue{kVerticalStep}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{
            R"(override for[^\n]*start_frame[^\n]*but the field declared type is[^\n]*double)",
            std::regex_constants::icase}));

    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);
}

/// An attribute override for start_frame whose type matches the
/// field replaces the field value. The simulation starts at the override value,
/// not the field value.
TEST(GlobalsStartFrame, attribute_override_succeeds) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "GlobalsStartFrame_override_succeeds";

    constexpr double fieldStartFrame = 0.0; // field value - will be overridden
    constexpr double attrStartFrame  = 1.0; // override value
    constexpr double endFrame        = 3.0;
    constexpr float  delta = static_cast<float>(endFrame - attrStartFrame);

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set verbosity so that warnings and errors are captured.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsAndWarnings);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{fieldStartFrame}},
            {kEndFrameField,   VtValue{endFrame}},
            {kFpsField,        VtValue{double{1.0}}},
        }
    );
    // Override with matching type (double) - must succeed.
    setLiveAttributeOverrides(stage,
        std::vector<AttributeNameAndValue>{
            {TfToken{kStartFrameAttr},
             {SdfValueTypeNames->Double, VtValue{attrStartFrame}}},
        }
    );
    setLiveInputsField(stage, VtDictionary{
            {"vertical_step", VtValue{kVerticalStep}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));

    // Simulation starts at attrStartFrame, not the overridden field value.
    EXPECT_EQ(getPointAtFrame(stage, attrStartFrame), GfVec3f(0.f, 0.f, 0.f));
    EXPECT_EQ(getPointAtFrame(stage, endFrame),
              GfVec3f(0.f, delta * kVerticalStep, 0.f));
}

// ===========================================================================
// GlobalsEndFrame - timeline_info_end_frame field and attribute
// ===========================================================================

/// An invalid type for the end_frame field causes the DFF to report
/// an error at read time and fail to load the payload.
/// The /Root prim is valid but has no children.
TEST(GlobalsEndFrame, invalid_field_type) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "GlobalsEndFrame_invalid_field_type";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{double{1.0}}},
            {kEndFrameField,   VtValue{"10"}}, // wrong type
        }
    );
    setLiveInputsField(stage, VtDictionary{
            {"vertical_step", VtValue{kVerticalStep}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{
            R"(end_frame[^\n]*has type[^\n]*string[^\n]*expected[^\n]*double)"}));

    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);
}

/// When start_frame or end_frame is provided, the other must be provided too.
/// end_frame is provided as a field, but start_frame is missing => error.
TEST(GlobalsEndFrame, valid_field_but_no_start_frame) {
    DffDiagnosticCollector collector;
    constexpr auto bucketKey = "GlobalsEndFrame_valid_field_but_no_start_frame";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kEndFrameField, VtValue{double{1}}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(required[^\n]*start_frame[^\n]*is missing)"}));

    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);
}

/// When start_frame or end_frame is provided, the other must be provided too.
/// end_frame is provided as an attribute, but start_frame is missing => error.
TEST(GlobalsEndFrame, valid_attr_but_no_start_frame) {
    DffDiagnosticCollector collector;
    constexpr auto bucketKey = "GlobalsEndFrame_valid_attr_but_no_start_frame";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // clang-format off
    setLiveAttributeOverrides(stage,
        std::vector<AttributeNameAndValue>{
            {TfToken{kEndFrameAttr},
             {SdfValueTypeNames->Double, VtValue{double{10}}}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(required[^\n]*start_frame[^\n]*is missing)"}));

    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);
}

/// A valid double end_frame field controls where the simulation ends.
TEST(GlobalsEndFrame, valid_field_value) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "GlobalsEndFrame_valid_field_value";

    constexpr double startFrame = 1.0;
    constexpr double endFrame   = 4.0;
    constexpr float  delta      = static_cast<float>(endFrame - startFrame);

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set verbosity so that warnings and errors are captured.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsAndWarnings);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{startFrame}},
            {kEndFrameField,   VtValue{endFrame}},
            {kFpsField,        VtValue{double{1.0}}},
        }
    );
    setLiveInputsField(stage, VtDictionary{
            {"vertical_step", VtValue{kVerticalStep}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));

    EXPECT_EQ(getPointAtFrame(stage, startFrame), GfVec3f(0.f, 0.f, 0.f));
    EXPECT_EQ(getPointAtFrame(stage, endFrame),
              GfVec3f(0.f, delta * kVerticalStep, 0.f));
}

/// end_frame is supplied only as an attribute - there is no
/// corresponding field entry. start_frame is provided via the fields.
/// The attribute value is consumed directly; it is not an override,
/// the field being absent.
TEST(GlobalsEndFrame, attribute_only_no_field) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "GlobalsEndFrame_attr_only_no_field";

    constexpr double startFrame = 1.0;
    constexpr double endFrame   = 4.0;
    constexpr float  delta      = static_cast<float>(endFrame - startFrame);

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set verbosity so that warnings and errors are captured.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsAndWarnings);

    // Dict carries only start_frame; end_frame will come later from attribute.
    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{startFrame}},
            {kFpsField,        VtValue{double{1.0}}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(required[^\n]*end_frame[^\n]*is missing)"}));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    collector.clear();

    // clang-format off
    setLiveAttributeOverrides(stage,
        std::vector<AttributeNameAndValue>{
            {TfToken{kEndFrameAttr},
             {SdfValueTypeNames->Double, VtValue{endFrame}}},
        }
    );
    setLiveInputsField(stage, VtDictionary{
            {"vertical_step", VtValue{kVerticalStep}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));

    EXPECT_EQ(getPointAtFrame(stage, startFrame), GfVec3f(0.f, 0.f, 0.f));
    EXPECT_EQ(getPointAtFrame(stage, endFrame),
              GfVec3f(0.f, delta * kVerticalStep, 0.f));
}

/// An attribute override for end_frame whose type differs
/// from the field type is rejected with an error.
TEST(GlobalsEndFrame, attribute_type_mismatch_override_rejected) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "GlobalsEndFrame_attr_type_mismatch";

    constexpr double startFrame    = 1.0;
    constexpr double fieldEndFrame = 4.0; // field value

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set verbosity so that warnings and errors are captured.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsAndWarnings);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{startFrame}},
            {kEndFrameField,   VtValue{fieldEndFrame}}, // correct type
            {kFpsField,        VtValue{double{1.0}}},
        }
    );
    // Override with type mismatch: must be rejected with an error.
    setLiveAttributeOverrides(stage,
        std::vector<AttributeNameAndValue>{
            {TfToken{kEndFrameAttr},
             {SdfValueTypeNames->Int, VtValue{int{20}}}},
        }
    );
    setLiveInputsField(stage, VtDictionary{
            {"vertical_step", VtValue{kVerticalStep}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{
            R"(override for[^\n]*end_frame[^\n]*but the field declared type is[^\n]*double)",
            std::regex_constants::icase}));

    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);
}

/// An attribute override for end_frame whose type matches the
/// field replaces the field value. The simulation ends at the override value,
/// not the field value.
TEST(GlobalsEndFrame, attribute_override_succeeds) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "GlobalsEndFrame_override_succeeds";

    constexpr double startFrame    = 1.0;
    constexpr double fieldEndFrame = 3.0; // field value - will be overridden
    constexpr double attrEndFrame  = 4.0; // override value
    constexpr float  delta = static_cast<float>(attrEndFrame - startFrame);

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set verbosity so that warnings and errors are captured.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsAndWarnings);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{startFrame}},
            {kEndFrameField,   VtValue{fieldEndFrame}},
            {kFpsField,        VtValue{double{1.0}}},
        }
    );
    // Override with matching type (double) - must succeed.
    setLiveAttributeOverrides(stage,
        std::vector<AttributeNameAndValue>{
            {TfToken{kEndFrameAttr},
             {SdfValueTypeNames->Double, VtValue{attrEndFrame}}},
        }
    );
    setLiveInputsField(stage, VtDictionary{
            {"vertical_step", VtValue{kVerticalStep}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));

    // Simulation ends at attrEndFrame, not the overridden field value.
    EXPECT_EQ(getPointAtFrame(stage, startFrame), GfVec3f(0.f, 0.f, 0.f));
    EXPECT_EQ(getPointAtFrame(stage, attrEndFrame),
              GfVec3f(0.f, delta * kVerticalStep, 0.f));
}

// ===========================================================================
// GlobalsFps - time_fps field and attribute
// ===========================================================================
//
// The move_up_point_simulation compound multiplies vertical_step by
// time.frame_length (= 1/fps) each frame, so fps directly controls the Y
// displacement per frame. At fps=1 the increment equals vertical_step; at
// fps=2 it is halved.

/// An invalid type for the fps field causes the DFF to report an error at
/// read time. The /Root prim is valid but has no children.
TEST(GlobalsFps, invalid_field_type) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "GlobalsFps_invalid_field_type";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{double{1.0}}},
            {kEndFrameField,   VtValue{double{3.0}}},
            {kFpsField,        VtValue{int{60}}}, // wrong type
        }
    );
    setLiveInputsField(stage, VtDictionary{
            {"vertical_step", VtValue{kVerticalStep}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(fps[^\n]*has type[^\n]*int[^\n]*expected[^\n]*double)"}));

    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);
}

/// fps must be greater than 0.
TEST(GlobalsFps, positive_value_required) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "GlobalsFps_positive_value_required";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{double{1}}},
            {kEndFrameField,   VtValue{double{2}}},
            {kFpsField,        VtValue{double{0}}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasErrorRegex(collector, bucketKey,
                                 std::regex{R"(fps[^\n]*invalid value)"}));

    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);
}

/// No fps is specified, default fps is autmatically used.
/// No errors are emitted and the graph executes normally.
TEST(GlobalsFps, no_fps_specified_default_is_used) {
    DffDiagnosticCollector collector;
    constexpr auto bucketKey = "GlobalsFps_no_fps_specified_default_is_used";

    constexpr double startFrame = 1.0;
    constexpr double endFrame   = 3.0;
    constexpr float  delta      = static_cast<float>(endFrame - startFrame);

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set verbosity so that warnings and errors are captured.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsAndWarnings);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{startFrame}},
            {kEndFrameField,   VtValue{endFrame}},
        }
    );
    setLiveInputsField(stage, VtDictionary{
            {"vertical_step", VtValue{kVerticalStep}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));

    EXPECT_EQ(getPointAtFrame(stage, startFrame), GfVec3f(0.f, 0.f, 0.f));
    const GfVec3f endPoint = getPointAtFrame(stage, endFrame);
    EXPECT_EQ(endPoint[0], 0.f);
    EXPECT_NEAR(endPoint[1],
                delta * kVerticalStep / static_cast<float>(kDefaultFps), 1e-5f);
    EXPECT_EQ(endPoint[2], 0.f);
}

/// A valid fps field is accepted. No errors are emitted and the graph
/// executes normally.
TEST(GlobalsFps, valid_field_value) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "GlobalsFps_valid_field_value";

    constexpr double startFrame = 1.0;
    constexpr double endFrame   = 3.0;
    constexpr float  delta      = static_cast<float>(endFrame - startFrame);
    constexpr double fps        = 10;

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set verbosity so that warnings and errors are captured.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsAndWarnings);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{startFrame}},
            {kEndFrameField,   VtValue{endFrame}},
            {kFpsField,        VtValue{fps}},
        }
    );
    setLiveInputsField(stage, VtDictionary{
            {"vertical_step", VtValue{kVerticalStep}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));

    EXPECT_EQ(getPointAtFrame(stage, startFrame), GfVec3f(0.f, 0.f, 0.f));
    const GfVec3f endPoint = getPointAtFrame(stage, endFrame);
    EXPECT_EQ(endPoint[0], 0.f);
    EXPECT_NEAR(endPoint[1], delta * kVerticalStep / static_cast<float>(fps),
                1e-5f);
    EXPECT_EQ(endPoint[2], 0.f);
}

/// fps is supplied only as an attribute - there is no corresponding
/// field entry. start_frame and end_frame are provided via the fields.
/// The fps attribute value is consumed directly; it is not an override,
/// the field being absent.
TEST(GlobalsFps, attribute_only_no_field) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "GlobalsFps_attr_only_no_field";

    constexpr double startFrame = 1.0;
    constexpr double endFrame   = 3.0;
    constexpr float  delta      = static_cast<float>(endFrame - startFrame);
    constexpr double fps        = 5;

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set verbosity so that warnings and errors are captured.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsAndWarnings);

    // Globals dict has start/end only - fps comes from the attribute.
    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{startFrame}},
            {kEndFrameField,   VtValue{endFrame}},
        }
    );
    setLiveAttributeOverrides(stage,
        std::vector<AttributeNameAndValue>{
            {TfToken{kFpsAttr}, {SdfValueTypeNames->Double, VtValue{fps}}},
        }
    );
    setLiveInputsField(stage, VtDictionary{
            {"vertical_step", VtValue{kVerticalStep}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));

    EXPECT_EQ(getPointAtFrame(stage, startFrame), GfVec3f(0.f, 0.f, 0.f));
    const GfVec3f endPoint = getPointAtFrame(stage, endFrame);
    EXPECT_EQ(endPoint[0], 0.f);
    EXPECT_NEAR(endPoint[1], delta * kVerticalStep / static_cast<float>(fps),
                1e-5f);
    EXPECT_EQ(endPoint[2], 0.f);
}

/// An attribute override for fps whose type differs from the field type is
/// rejected with an error.
TEST(GlobalsFps, attribute_type_mismatch_override_rejected) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "GlobalsFps_attr_type_mismatch";

    constexpr double startFrame = 1.0;
    constexpr double endFrame   = 3.0;
    constexpr double fieldFps   = 20.0; // field value

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set verbosity so that warnings and errors are captured.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsAndWarnings);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{startFrame}},
            {kEndFrameField,   VtValue{endFrame}},
            {kFpsField,        VtValue{fieldFps}}, // double - correct type
        }
    );
    // Override with type mismatch: must be rejected with an error.
    setLiveAttributeOverrides(stage,
        std::vector<AttributeNameAndValue>{
            {TfToken{kFpsAttr}, {SdfValueTypeNames->Bool, VtValue{bool{true}}}},
        }
    );
    setLiveInputsField(stage, VtDictionary{
            {"vertical_step", VtValue{kVerticalStep}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{
            R"(override for[^\n]*fps[^\n]*but the field declared type is[^\n]*double)",
            std::regex_constants::icase}));

    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim.IsValid());
    EXPECT_EQ(rootPrim.GetChildrenNames().size(), 0u);
}

/// An attribute override for fps whose type matches the field
/// replaces the field value. The simulation uses the override fps value,
/// not the field value.
TEST(GlobalsFps, attribute_override_succeeds) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey = "GlobalsFps_override_succeeds";

    constexpr double startFrame = 1.0;
    constexpr double endFrame   = 3.0;
    constexpr float  delta      = static_cast<float>(endFrame - startFrame);
    constexpr double fieldFps   = 5.0;  // field value - will be overridden
    constexpr double attrFps    = 10.0; // override value

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set verbosity so that warnings and errors are captured.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsAndWarnings);

    // clang-format off
    setLiveGlobalsField(stage, VtDictionary{
            {kStartFrameField, VtValue{startFrame}},
            {kEndFrameField,   VtValue{endFrame}},
            {kFpsField,        VtValue{fieldFps}},
        }
    );
    // Override with matching type (double) - must succeed.
    setLiveAttributeOverrides(stage,
        std::vector<AttributeNameAndValue>{
            {TfToken{kFpsAttr}, {SdfValueTypeNames->Double, VtValue{attrFps}}},
        }
    );
    setLiveInputsField(stage, VtDictionary{
            {"vertical_step", VtValue{kVerticalStep}},
        }
    );
    // clang-format on

    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));

    // Simulation uses the attrFps, not the overridden field value.
    EXPECT_EQ(getPointAtFrame(stage, startFrame), GfVec3f(0.f, 0.f, 0.f));
    const GfVec3f endPoint = getPointAtFrame(stage, endFrame);
    EXPECT_EQ(endPoint[0], 0.f);
    EXPECT_NEAR(endPoint[1],
                delta * kVerticalStep / static_cast<float>(attrFps), 1e-5f);
    EXPECT_EQ(endPoint[2], 0.f);
}

PXR_NAMESPACE_CLOSE_SCOPE
