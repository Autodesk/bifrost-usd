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
#include <BifrostUsd/GraphExecutor/GraphExecutorTypes.h>
#include <utils/test/testUtils.h>

// Open USD
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

// C++ Standard Library
#include <regex>
#include <string>
#include <vector>

using namespace BifrostUsd::TestUtils;
using namespace DffTestHelpers;
using BifrostUsd::GraphExecutor::VerbosityLevel;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

UniqueTestOutputSubdir g_OutputDir{"testFileFormat_settingsArgs",
                                   true /*autoDelete*/};

// A simple static-mesh compound whose "mesh" output port produces an Object.
// This is used to exercise the Object-output path in Read(), which emits a
// status message ("has produced an Object") at eAllMessages verbosity.
constexpr const char* kCompound   = "Test::DynamicFileFormat::create_mesh_cube";
constexpr const char* kOutputPort = "mesh";

// Full attribute names for settings tested in this file.
constexpr const char* kVerbosityAttr    = "bifrost:setting:verbosityLevel";
constexpr const char* kCompoundNameAttr = "bifrost:compound:name";

} // namespace

// ===========================================================================
// SettingsVerbosityLevel - verbosityLevel field and attribute
// ===========================================================================

/// An invalid type for the verbosityLevel field always produces an error,
/// regardless of the currently active verbosity level. This is by design:
/// getVerbosityLevelArg() hardcodes eErrorsAndWarnings for its own error paths
/// so that users always receive feedback when the verbosity setting itself is
/// misconfigured - even if they had previously set the level to "Silent".
///
/// By contrast, errors about OTHER fields (e.g. a compound-name attribute type
/// mismatch) ARE suppressed by eSilent, which the test also verifies.
TEST(SettingsVerbosityLevel, invalid_field_type) {
    DffDiagnosticCollector collector;
    constexpr auto bucketKey = "SettingsVerbosityLevel_invalid_field_type";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    collector.clear();

    // Phase 1 - switch to "Silent". (Almost) no diagnostics at all from this
    // point on until verbosity is raised again.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eSilent);
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    EXPECT_TRUE(dffHasNoStatus(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
    collector.clear();

    // Phase 2 - install an invalid type for an override for the compound name
    // (int instead of string).
    // The DFF detects the type mismatch but does not emit an error due to
    // current silent verbosity.
    // Composition fails because the compound name cannot be applied,
    // leaving /Root with no children.
    setLiveAttributeOverrides(stage,
                              std::vector<AttributeNameAndValue>{
                                  {TfToken{kCompoundNameAttr},
                                   {SdfValueTypeNames->Int, VtValue{int{42}}}},
                              });
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
    collector.clear();

    // Phase 3 - install an invalid verbosityLevel type (long instead of
    // string). getVerbosityLevelArg() hardcodes eErrorsAndWarnings for all its
    // error paths (both type mismatch and invalid value), so this error is
    // ALWAYS reported regardless of the preceding composition's eSilent
    // verbosity.
    // Note: int64_t is used here (not native long) because int64_t is in the
    // encoder's SupportedTypes and maps to Amino type name "long". Native long
    // is not in SupportedTypes and would be rejected with a different error.
    stage->GetRootLayer()->SetFieldDictValueByKey(
        SdfPath{"/DynamicFileFormatField"}, TfToken{"bifrostSettings"},
        TfToken{"verbosityLevel"}, int64_t{10});
    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{
            R"(verbosityLevel[^\n]*has type[^\n]*long[^\n]*expected[^\n]*string)"}));
    EXPECT_FALSE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(attribute override for \"bifrost:compound:name\")",
                   std::regex_constants::icase}));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));

    // Read() cannot decode the verbosity field. The DFF plugin is strict
    // regarding the verbosity setting: if there is an error when reading it
    // (whether it is the field or the attribute), it aborts its processing to
    // ensure user gets the feeback they expected.
    // Consequence: /Root is present but carries no children.
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
    collector.clear();

    // Phase 4 - restore a valid verbosity ("ErrorsOnly"). The field has a valid
    // string type again so no type-mismatch error is produced.
    // But the override for the compoundName is still invalid (int, not string),
    // and now that verbosity is no longer Silent, the DFF reports this error.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsOnly);
    EXPECT_FALSE(dffHasError(collector, bucketKey, "verbosityLevel"));
    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(attribute override for \"bifrost:compound:name\")",
                   std::regex_constants::icase}));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
    collector.clear();

    // Phase 5 - restore a valid compoundName override and install the invalid
    // verbosityLevel using a bad type once more. The error is reported
    // again, confirming the behavior is consistent across compositions.
    // Note: setLiveAttributeOverrides() reuses an existing attribute without
    //       changing its type, so the old string attribute must be removed
    //       first to let a new typed one be created.
    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim.IsValid());
    rootPrim.RemoveProperty(TfToken{kCompoundNameAttr});
    setLiveAttributeOverrides(
        stage, std::vector<AttributeNameAndValue>{
                   {TfToken{kCompoundNameAttr},
                    {SdfValueTypeNames->String, VtValue{kCompound}}},
               });
    stage->GetRootLayer()->SetFieldDictValueByKey(
        SdfPath{"/DynamicFileFormatField"}, TfToken{"bifrostSettings"},
        TfToken{"verbosityLevel"}, bool{true});
    EXPECT_FALSE(dffHasError(collector, bucketKey, kCompoundNameAttr));
    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{
            R"(verbosityLevel[^\n]*has type[^\n]*bool[^\n]*expected[^\n]*string)"}));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
}

/// An unrecognised verbosity level string causes the DFF to report two errors:
/// one naming the invalid value and one listing all accepted values. Both
/// errors are always emitted regardless of current verbosity, so users are
/// always informed about bad values.
TEST(SettingsVerbosityLevel, invalid_field_value) {
    DffDiagnosticCollector collector;
    constexpr auto bucketKey = "SettingsVerbosityLevel_invalid_field_value";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);
    setLiveVerbosityLevelField(stage, VerbosityLevel::eSilent);
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
    collector.clear();

    // Install an unrecognised string value. The type is correct but value is
    // rejected, so DFF reports two errors even if current verbosity is silent:
    //   1. an "invalid value" message naming "unknown"
    //   2. a "Valid verbosity level values are:" message listing all accepted
    //   names
    stage->GetRootLayer()->SetFieldDictValueByKey(
        SdfPath{"/DynamicFileFormatField"}, TfToken{"bifrostSettings"},
        TfToken{"verbosityLevel"}, std::string{"unknown"});
    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(verbosityLevel[^\n]*invalid value[^\n]*unknown)"}));
    EXPECT_TRUE(
        dffHasError(collector, bucketKey, "Valid verbosity level values are"));

    // Read() cannot decode the verbosity argument: /Root is present but carries
    // no children.
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
}

/// verbosityLevel is supplied only via the field (no attribute, no compound).
/// Verifies that the verbosityLevel field value controls whether the compound
/// name missing error is reported, proving the field is applied before compound
/// resolution.
TEST(SettingsVerbosityLevel, field_only_early_fail) {
    DffDiagnosticCollector collector;
    constexpr auto bucketKey = "SettingsVerbosityLevel_field_only_early_fail";

    // Minimal root layer: DFF payload with no compound name and no verbosity.
    // At default verbosity, the initial Open() must report the compound name
    // is missing; clear it before testing specific verbosity levels.
    auto rootLayerPath = createRootLayerWithDff(bucketKey, g_OutputDir);
    auto stage         = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);
    EXPECT_FALSE(dffHasNoError(collector, bucketKey)); // compound name missing
    collector.clear();

    // Phase 1 - set verbosity field to Silent. Recomposition fires but the
    // compound name missing error must not be reported.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eSilent);
    EXPECT_TRUE(dffHasNoError(collector, bucketKey)); // error not reported
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    collector.clear();

    // Phase 2 - raise verbosity to eErrorsOnly. The error is now visible.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsOnly);
    EXPECT_TRUE(
        dffHasErrorRegex(collector, bucketKey,
                         std::regex{R"(bifrost:compound:name[^\n]*missing)",
                                    std::regex_constants::icase}));
}

/// The verbosityLevel field controls what diagnostics are emitted. Setting the
/// field to "ErrorsAndWarnings" makes warnings visible; changing to
/// "ErrorsOnly" suppresses them while still emitting errors.
///
/// The warning used as a probe is a compound-name attribute type mismatch: the
/// bifrostCompound.name field holds a string, but a bifrost:compound:name
/// attribute of type int is authored on /Root. Changing the verbosity level
/// from "ErrorsAndWarnings" to "ErrorsOnly" suppresses this warning.
TEST(SettingsVerbosityLevel, valid_field_value) {
    DffDiagnosticCollector collector;
    constexpr auto bucketKey = "SettingsVerbosityLevel_valid_field_value";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());

    // Phase 1 - "ErrorsAndWarnings": a type-mismatch error must appear.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsAndWarnings);
    // Trigger a type-mismatch error: compound name field is "string", but the
    // attribute override below uses type "int". The error is fired and the
    // graph produces no mesh.
    setLiveAttributeOverrides(stage,
                              std::vector<AttributeNameAndValue>{
                                  {TfToken{kCompoundNameAttr},
                                   {SdfValueTypeNames->Int, VtValue{int{42}}}},
                              });
    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{
            R"(override for[^\n]*compound:name[^\n]*has type[^\n]*int[^\n]*field declared type)",
            std::regex_constants::icase}));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
    collector.clear();

    // Phase 2 - "Silent": the same error must no longer appear.
    // Changing the verbosity triggers a recomposition; the compound-name
    // attribute type mismatch fires again but is now filtered.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eSilent);
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
}

/// verbosityLevel is supplied only as an attribute - the bifrostSettings field
/// carries no verbosityLevel entry, so the system falls back to the default
/// verbosity during field composition.
///
/// The status message "has produced an Object" is emitted inside Read() at
/// eAllMessages verbosity whenever an Object-typed graph output is converted
/// to a USD stage. It is invisible at the default eErrorsOnly level but
/// visible once the attribute sets "AllMessages".
TEST(SettingsVerbosityLevel, attribute_only_no_field) {
    DffDiagnosticCollector collector;
    constexpr auto bucketKey = "SettingsVerbosityLevel_attr_only_no_field";

    // Create a bare DFF layer: DFF payload wired up, but no compound name, no
    // outputs, and - critically - no verbosityLevel in the bifrostSettings
    // field dict.
    auto rootLayerPath = createRootLayerWithDff(bucketKey, g_OutputDir);
    auto stage         = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);
    collector.clear(); // Discard initial "missing compound name" error.

    // Provide compound name and output port via live field edits.
    stage->GetRootLayer()->SetFieldDictValueByKey(
        SdfPath{"/DynamicFileFormatField"}, TfToken{"bifrostCompound"},
        TfToken{"name"}, TfToken{kCompound});
    setLiveOutputsField(stage, {kOutputPort});
    // No verbosity field is present, so graph executes at default verbosity:
    // no status messages expected.
    EXPECT_TRUE(dffHasNoStatus(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
    collector.clear();

    // Set the verbosity attribute to "AllMessages" - no corresponding field.
    // Read() will use this value, making status messages visible.
    setLiveAttributeOverrides(
        stage,
        std::vector<AttributeNameAndValue>{
            {TfToken{kVerbosityAttr},
             {SdfValueTypeNames->String, VtValue{std::string{"AllMessages"}}}},
        });
    EXPECT_TRUE(dffHasStatus(collector, bucketKey, "has produced an Object"));
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
    collector.clear();

    // Attempt a verbosity attribute with wrong type again.
    // Note: setLiveAttributeOverrides() reuses an existing attribute without
    //       changing its type, so the old string attribute must be removed
    //       first to let a new typed one be created.
    //       Otherwise attr.Set(int) silently fails and the stage keeps the
    //       old typed value.
    auto rootPrim = stage->GetPrimAtPath(SdfPath{"/Root"});
    ASSERT_TRUE(rootPrim.IsValid());
    rootPrim.RemoveProperty(TfToken{kVerbosityAttr});
    setLiveAttributeOverrides(stage,
                              std::vector<AttributeNameAndValue>{
                                  {TfToken{kVerbosityAttr},
                                   {SdfValueTypeNames->Int, VtValue{int{42}}}},
                              });
    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{
            R"([^\n]*verbosityLevel[^\n]*has type[^\n]*int[^\n]*expected[^\n]*string)",
            std::regex_constants::icase}));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
    collector.clear();

    // Attempt to install an unrecognised string value.
    // The type is correct but value is rejected.
    stage->GetRootLayer()->SetFieldDictValueByKey(
        SdfPath{"/DynamicFileFormatField"}, TfToken{"bifrostSettings"},
        TfToken{"verbosityLevel"}, std::string{"unknown"});
    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(verbosityLevel[^\n]*invalid value[^\n]*unknown)"}));
    EXPECT_TRUE(
        dffHasError(collector, bucketKey, "Valid verbosity level values are"));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
}

/// verbosityLevel is supplied only as an attribute (no field, no compound).
/// Verifies that the verbosityLevel attribute value controls whether the
/// compound name missing error is reported, proving the attribute is applied
/// before compound resolution.
TEST(SettingsVerbosityLevel, attribute_only_early_fail) {
    DffDiagnosticCollector collector;
    constexpr auto bucketKey = "SettingsVerbosityLevel_attr_only_early_fail";

    // Minimal root layer: DFF payload with no compound name and no verbosity.
    // At default verbosity, the initial Open() must report the compound name
    // is missing; clear it before testing specific verbosity levels.
    auto rootLayerPath = createRootLayerWithDff(bucketKey, g_OutputDir);
    auto stage         = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);
    EXPECT_FALSE(dffHasNoError(collector, bucketKey)); // compound name missing
    collector.clear();

    // Phase 1 - set verbosity attribute to Silent. Recomposition fires but the
    // compound name missing error must not be reported.
    setLiveAttributeOverrides(
        stage,
        std::vector<AttributeNameAndValue>{
            {TfToken{kVerbosityAttr},
             {SdfValueTypeNames->String, VtValue{std::string{"Silent"}}}},
        });
    EXPECT_TRUE(dffHasNoError(collector, bucketKey)); // error not reported
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    collector.clear();

    // Phase 2 - raise verbosity to ErrorsAndWarnings via attribute.
    // The error is now visible.
    setLiveAttributeOverrides(stage,
                              std::vector<AttributeNameAndValue>{
                                  {TfToken{kVerbosityAttr},
                                   {SdfValueTypeNames->String,
                                    VtValue{std::string{"ErrorsAndWarnings"}}}},
                              });
    EXPECT_TRUE(
        dffHasErrorRegex(collector, bucketKey,
                         std::regex{R"(bifrost:compound:name[^\n]*missing)",
                                    std::regex_constants::icase}));
}

/// An attribute override for verbosityLevel whose type differs from the field
/// type (string) is always reported with an error, regardless of the current
/// field-derived verbosity. This ensures users always receive feedback, even
/// when the field is Silent or absent.
TEST(SettingsVerbosityLevel, attribute_type_mismatch_override_rejected) {
    DffDiagnosticCollector collector;
    constexpr auto bucketKey = "SettingsVerbosityLevel_attr_type_mismatch";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Set field verbosity to Silent — the strictest filter possible.
    // The attribute type-mismatch warning must still appear because
    // applyAttrOverrideToFileFormatArgument is called with a hardcoded
    // eErrorsAndWarnings level for the verbosityLevel attribute.
    setLiveVerbosityLevelField(stage, VerbosityLevel::eSilent);
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
    collector.clear();

    // Attempt an attribute override with wrong type (int instead of string).
    // The warning is emitted even though the field is Silent.
    // The field value (eSilent) is kept because types do not match.
    setLiveAttributeOverrides(stage,
                              std::vector<AttributeNameAndValue>{
                                  {TfToken{kVerbosityAttr},
                                   {SdfValueTypeNames->Int, VtValue{int{42}}}},
                              });
    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{
            R"(override for[^\n]*verbosityLevel[^\n]*has type[^\n]*int[^\n]*field declared type[^\n]*string)",
            std::regex_constants::icase}));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root"}).IsValid());
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
}

/// An attribute override for verbosityLevel whose type matches the field type
/// (string) replaces the field value. Upgrading from "ErrorsAndWarnings" to
/// "AllMessages" via the attribute makes status messages from Read() visible.
TEST(SettingsVerbosityLevel, attribute_override_succeeds) {
    DffDiagnosticCollector collector;
    constexpr auto bucketKey = "SettingsVerbosityLevel_override_succeeds";

    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kCompound, kOutputPort);
    auto stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);
    setLiveVerbosityLevelField(stage, VerbosityLevel::eErrorsAndWarnings);

    // Field verbosity is "ErrorsAndWarnings": status messages are not emitted.
    EXPECT_TRUE(dffGetStatusMessages(collector, bucketKey).empty());
    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    EXPECT_TRUE(dffHasNoStatus(collector, bucketKey));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
    collector.clear();

    // Override to "AllMessages" via attribute (string type matches field type).
    // Read() now uses AllMessages: the "has produced an Object" status message
    // is emitted when the Object output of create_mesh_cube is converted to a
    // USD stage.
    setLiveAttributeOverrides(
        stage,
        std::vector<AttributeNameAndValue>{
            {TfToken{kVerbosityAttr},
             {SdfValueTypeNames->String, VtValue{std::string{"AllMessages"}}}},
        });

    EXPECT_TRUE(dffHasNoError(collector, bucketKey));
    EXPECT_TRUE(dffHasNoWarning(collector, bucketKey));
    EXPECT_TRUE(dffHasStatus(collector, bucketKey, "has produced an Object"));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath{"/Root/geo/mesh"}).IsValid());
}

PXR_NAMESPACE_CLOSE_SCOPE
