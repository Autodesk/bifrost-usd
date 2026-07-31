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
#include <BifrostUsd/GraphExecutor/GraphExecutorFactory.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorTypes.h>
#include <utils/test/testUtils.h>

// Open USD
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

// C++ Standard Library
#include <filesystem>
#include <regex>
#include <vector>

#ifndef PLACEHOLDER_CONFIG_FILE
#error "PLACEHOLDER_CONFIG_FILE must be defined"
#endif
#ifndef EMPTY_CONFIG_FILE
#error "EMPTY_CONFIG_FILE must be defined"
#endif
#ifndef EXTRA_CONFIG_FILE
#error "EXTRA_CONFIG_FILE must be defined"
#endif

using namespace BifrostUsd::TestUtils;
using namespace DffTestHelpers;
using BifrostUsd::GraphExecutor::VerbosityLevel;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

UniqueTestOutputSubdir g_OutputDir{"testFileFormat_reloadLibrary",
                                   true /*autoDelete*/};

// These tests use the Test::Reload::Library::less compound from the
// extra test compounds config file.
// Test::Reload::Library::less takes two int inputs and outputs a bool.
// The bool output triggers "did not produce a valid ... Expecting a Stage"
// in Read(), which proves that composition was reached.
constexpr const char* kReloadCompound = "Test::Reload::Library::less";
constexpr const char* kReloadOutput   = "output";

/// RAII guard that restores the placeholder config file to the empty variant
/// on destruction, ensuring cleanup even on early test exit or process kill.
struct PlaceholderConfigGuard {
    PlaceholderConfigGuard() = default;
    PlaceholderConfigGuard& operator=(PlaceholderConfigGuard&&) noexcept =
        default;
    PlaceholderConfigGuard(const PlaceholderConfigGuard&)            = delete;
    PlaceholderConfigGuard& operator=(const PlaceholderConfigGuard&) = delete;
    ~PlaceholderConfigGuard() {
        std::error_code ec;
        std::filesystem::copy_file(
            EMPTY_CONFIG_FILE, PLACEHOLDER_CONFIG_FILE,
            std::filesystem::copy_options::overwrite_existing,
            ec); // best-effort; swallow ec in destructor
    }
};

/// Shared initial phases for tests that exercise the reloadLibrary feature via
/// either the bifrostSettings field or a prim attribute override.
///
/// This function uses \c ASSERT_* macros; call it via
/// \c ASSERT_NO_FATAL_FAILURE() to propagate fatal failures to the test body.
///
/// \p out_guard is move-assigned *before* the extra-config copy so its
/// destructor restores the placeholder even if a later assertion fails.
/// The caller must keep \p out_guard alive for the duration of the test.
void testSetupWithReloadLibraryField(const char*             bucketKey,
                                     DffDiagnosticCollector& collector,
                                     UsdStageRefPtr&         out_stage,
                                     PlaceholderConfigGuard& out_guard) {
    // Verify that the three config files exist and have absolute paths.
    ASSERT_TRUE(std::filesystem::exists(PLACEHOLDER_CONFIG_FILE))
        << "Placeholder config file does not exist: "
        << PLACEHOLDER_CONFIG_FILE;
    ASSERT_TRUE(std::filesystem::path(PLACEHOLDER_CONFIG_FILE).is_absolute())
        << "Placeholder config file is not absolute: "
        << PLACEHOLDER_CONFIG_FILE;
    ASSERT_TRUE(std::filesystem::exists(EMPTY_CONFIG_FILE))
        << "Empty config file does not exist: " << EMPTY_CONFIG_FILE;
    ASSERT_TRUE(std::filesystem::path(EMPTY_CONFIG_FILE).is_absolute())
        << "Empty config file is not absolute: " << EMPTY_CONFIG_FILE;
    ASSERT_TRUE(std::filesystem::exists(EXTRA_CONFIG_FILE))
        << "Extra config file does not exist: " << EXTRA_CONFIG_FILE;
    ASSERT_TRUE(std::filesystem::path(EXTRA_CONFIG_FILE).is_absolute())
        << "Extra config file is not absolute: " << EXTRA_CONFIG_FILE;

    // Reset on-disk placeholder and in-memory library so this test is
    // independent of whatever state a prior test may have left behind.
    // reloadLibrary() flushes the ExecutableRegistry cache; without it a
    // prior test that loaded the compound would leave it in memory even
    // after the placeholder is restored to the empty config.
    ASSERT_TRUE(std::filesystem::copy_file(
        EMPTY_CONFIG_FILE, PLACEHOLDER_CONFIG_FILE,
        std::filesystem::copy_options::overwrite_existing))
        << "Failed to reset placeholder config to empty: "
        << PLACEHOLDER_CONFIG_FILE;
    BifrostUsd::GraphExecutor::reloadLibrary();

    // Phase 1: reloadLibrary=false and verbosityLevel=eErrorsAndWarnings.
    // The placeholder config is empty -> the library does not know about the
    // compound. Composition fails; no /Root/geo child is produced.
    auto rootLayerPath = createRootLayerWithDefaultDffFields(
        bucketKey, g_OutputDir, kReloadCompound, kReloadOutput);
    out_stage = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(out_stage);
    EXPECT_TRUE(dffHasError(collector, bucketKey,
                            "Failed to create a GraphExecutor for graph"));
    EXPECT_FALSE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(did not produce a valid[^\n]*Expecting a Stage)"}));
    EXPECT_FALSE(out_stage->GetPrimAtPath(SdfPath{"/Root/geo"}).IsValid());
    collector.clear();

    // Activate the guard before the extra-config copy so the placeholder is
    // restored even if the copy or Phase 2 fails.
    out_guard = PlaceholderConfigGuard{};

    // Copy the extra config file over the placeholder. The compound is now
    // on disk, but the in-memory library has not been reloaded yet.
    ASSERT_TRUE(std::filesystem::copy_file(
        EXTRA_CONFIG_FILE, PLACEHOLDER_CONFIG_FILE,
        std::filesystem::copy_options::overwrite_existing))
        << "Failed to copy `extra` config file `" << EXTRA_CONFIG_FILE
        << "` to `placeholder` config file `" << PLACEHOLDER_CONFIG_FILE << "`";

    // Phase 2: reloadLibrary=false (unchanged). Trigger recomposition by
    // upgrading the verbosity level to eAllMessages. The in-memory library has
    // not been reloaded, so the compound is still unknown and composition
    // fails.
    setLiveVerbosityLevelField(out_stage, VerbosityLevel::eAllMessages);
    EXPECT_TRUE(dffHasError(collector, bucketKey,
                            "Failed to create a GraphExecutor for graph"));
    EXPECT_FALSE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(did not produce a valid[^\n]*Expecting a Stage)"}));
    EXPECT_FALSE(out_stage->GetPrimAtPath(SdfPath{"/Root/geo"}).IsValid());
    collector.clear();

    // Provide the required inputs for 'less' before Phase 3. This
    // triggers a recomposition (reloadLibrary still false, compound still
    // unknown), so we clear errors afterwards. The inputs will be in place
    // when Phase 3's reload recomposition calls Read().
    VtDictionary inputsDict;
    inputsDict["first"]  = VtValue{int{0}};
    inputsDict["second"] = VtValue{int{1}};
    setLiveInputsField(out_stage, inputsDict);
    collector.clear();
}

} // namespace

// ===========================================================================
// Test that setting the field reloadLibrary=true triggers a library reload
// during composition so that newly-added compounds become discoverable.
// ===========================================================================
TEST(SettingsReloadLibrary, field_only) {
    DffDiagnosticCollector collector;
    constexpr auto         bucketKey =
        "SettingsReloadLibrary_reload_during_composition";

    // Initial test setup: empty placeholder config, set compound to
    // "Test::Reload::Library::less" (not found), reloadLibrary=false,
    // and verbosityLevel=eAllMessages.
    PlaceholderConfigGuard guard;
    UsdStageRefPtr         stage;
    ASSERT_NO_FATAL_FAILURE(
        testSetupWithReloadLibraryField(bucketKey, collector, stage, guard));

    // Then set field reloadLibrary=true. The DFF plugin calls reloadLibrary()
    // during ComposeFieldsForFileFormatArguments, picks up the updated
    // placeholder config, and successfully creates a GraphExecutor for the
    // compound. Read() is then invoked and fails with "did not produce a
    // valid ... Expecting a Stage" because the compound outputs a bool, not
    // a Stage -- proving that composition was reached.
    setLiveReloadLibraryField(stage, true);
    EXPECT_FALSE(dffHasError(collector, bucketKey,
                             "Failed to create a GraphExecutor for graph"));
    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(did not produce a valid[^\n]*Expecting a Stage)"}));
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo"}).IsValid());
    collector.clear();
}

// ===========================================================================
// Test that setting the reloadLibrary attribute overrides the field and
// triggers a library reload during composition when the final composed value
// (field + attribute override) is true.
// ===========================================================================
TEST(SettingsReloadLibrary, attribute_override) {
    DffDiagnosticCollector collector;
    constexpr auto bucketKey = "SettingsReloadLibrary_attribute_override";

    // Initial test setup: empty placeholder config, set compound to
    // "Test::Reload::Library::less" (not found), reloadLibrary=false,
    // and verbosityLevel=eAllMessages.
    PlaceholderConfigGuard guard;
    UsdStageRefPtr         stage;
    ASSERT_NO_FATAL_FAILURE(
        testSetupWithReloadLibraryField(bucketKey, collector, stage, guard));

    // Then set an attribute override reloadLibrary=true.
    // The DFF plugin must use the final composed value (field + attribute
    // override) when deciding whether to call reloadLibrary().
    setLiveAttributeOverrides(stage,
                              std::vector<AttributeNameAndValue>{
                                  {TfToken{"bifrost:setting:reloadLibrary"},
                                   {SdfValueTypeNames->Bool, VtValue{true}}},
                              });
    EXPECT_FALSE(dffHasError(collector, bucketKey,
                             "Failed to create a GraphExecutor for graph"));
    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(did not produce a valid[^\n]*Expecting a Stage)"}));
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo"}).IsValid());
    collector.clear();
}

// ===========================================================================
// Test that setting the reloadLibrary attribute alone (no field counterpart)
// is sufficient to trigger library reload.
// ===========================================================================
TEST(SettingsReloadLibrary, attribute_only_no_field) {
    DffDiagnosticCollector collector;
    constexpr auto bucketKey = "SettingsReloadLibrary_attribute_only_no_field";

    // Reset state: empty placeholder on disk + reload in-memory library.
    // reloadLibrary() flushes the ExecutableRegistry cache; without it a
    // prior test that loaded the compound would leave it in memory even
    // after the placeholder is restored to the empty config.
    ASSERT_TRUE(std::filesystem::copy_file(
        EMPTY_CONFIG_FILE, PLACEHOLDER_CONFIG_FILE,
        std::filesystem::copy_options::overwrite_existing))
        << "Failed to reset placeholder config to empty: "
        << PLACEHOLDER_CONFIG_FILE;
    BifrostUsd::GraphExecutor::reloadLibrary();

    // Create a minimal root layer (no fields, no bifrostSettings, no
    // reloadLibrary field).
    auto rootLayerPath = createRootLayerWithDff(bucketKey, g_OutputDir);
    auto stage         = UsdStage::Open(rootLayerPath.c_str());
    ASSERT_TRUE(stage);

    // Add the compound name and outputs fields. Each modification triggers a
    // recomposition: the stage initially opens with no fields at all, the first
    // SetField recomposes with the compound name but no outputs yet, and the
    // second SetField recomposes with both fields set but the compound is still
    // unknown to the in-memory library. All resulting errors accumulate in the
    // collector and are verified below.
    stage->GetRootLayer()->SetFieldDictValueByKey(
        SdfPath{"/DynamicFileFormatField"}, TfToken{"bifrostCompound"},
        TfToken{"name"}, TfToken{kReloadCompound});
    stage->GetRootLayer()->SetField(
        SdfPath{"/DynamicFileFormatField"}, TfToken{"bifrostOutputs"},
        VtValue{VtArray<TfToken>{TfToken{kReloadOutput}}});

    // The library does not know about the compound since the placeholder config
    // is empty.
    EXPECT_TRUE(dffHasError(collector, bucketKey,
                            "Failed to create a GraphExecutor for graph"));
    EXPECT_FALSE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(did not produce a valid[^\n]*Expecting a Stage)"}));
    collector.clear();

    PlaceholderConfigGuard guard;

    // Copy extra config to placeholder so the compound is available on disk,
    // but the in-memory library has not been reloaded yet.
    ASSERT_TRUE(std::filesystem::copy_file(
        EXTRA_CONFIG_FILE, PLACEHOLDER_CONFIG_FILE,
        std::filesystem::copy_options::overwrite_existing))
        << "Failed to copy `extra` config file `" << EXTRA_CONFIG_FILE
        << "` to `placeholder` config file `" << PLACEHOLDER_CONFIG_FILE << "`";

    // Provide inputs for compound.
    VtDictionary inputsDict;
    inputsDict["first"]  = VtValue{int{0}};
    inputsDict["second"] = VtValue{int{1}};
    setLiveInputsField(stage, inputsDict);
    collector.clear();

    // Test 1: author reloadLibrary=false via attribute (no field counterpart).
    // Even though the extra config is on disk, no reload happens because
    // reloadLibrary is false. Composition fails: compound is still unknown.
    setLiveAttributeOverrides(stage,
                              std::vector<AttributeNameAndValue>{
                                  {TfToken{"bifrost:setting:reloadLibrary"},
                                   {SdfValueTypeNames->Bool, VtValue{false}}},
                              });
    EXPECT_TRUE(dffHasError(collector, bucketKey,
                            "Failed to create a GraphExecutor for graph"));
    EXPECT_FALSE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(did not produce a valid[^\n]*Expecting a Stage)"}));
    collector.clear();

    // Test 2: change the reloadLibrary attribute to true. Composition picks up
    // the attribute as the final composed value and calls reloadLibrary().
    // The compound is then found and Read() fails with
    // "did not produce a valid ... Expecting a Stage" because the compound
    // outputs a bool, not a Stage -- proving that composition succeeded.
    setLiveAttributeOverrides(stage,
                              std::vector<AttributeNameAndValue>{
                                  {TfToken{"bifrost:setting:reloadLibrary"},
                                   {SdfValueTypeNames->Bool, VtValue{true}}},
                              });
    EXPECT_FALSE(dffHasError(collector, bucketKey,
                             "Failed to create a GraphExecutor for graph"));
    EXPECT_TRUE(dffHasErrorRegex(
        collector, bucketKey,
        std::regex{R"(did not produce a valid[^\n]*Expecting a Stage)"}));
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath{"/Root/geo"}).IsValid());
    collector.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE
