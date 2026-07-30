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
#include <utils/test/testUtils.h>

// Amino
#include <Amino/Core/Array.h>
#include <Amino/Core/Ptr.h>
#include <Amino/Core/String.h>
#include <Amino/Core/StringView.h>
#include <Amino/Executor/ExecutableT.h>
#include <Amino/Executor/ExecutionInputs.h>
#include <Amino/Executor/ExecutionOutputs.h>
#include <Amino/Library/ConstLibrary.h>

// BifrostGraph
#include <BifrostGraph/Executor/Utility.h>

// BifrostUsd::GraphExecutor
#include <BifrostUsd/GraphExecutor/GraphExecutor.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorFactory.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorFactoryInternal.h>
#include <BifrostUsd/GraphExecutor/StreamObserver.h>

#include <filesystem>
#include <ios>
#include <ostream>
#include <tuple>

using StringArray = Amino::Array<Amino::String>;
using namespace Amino::StringViewLiterals;
using namespace BifrostUsd::TestUtils;

#ifndef PLACEHOLDER_CONFIG_FILE
#error "PLACEHOLDER_CONFIG_FILE must be defined"
#endif
#ifndef EMPTY_CONFIG_FILE
#error "EMPTY_CONFIG_FILE must be defined"
#endif
#ifndef EXTRA_CONFIG_FILE
#error "EXTRA_CONFIG_FILE must be defined"
#endif

namespace {
// Reset the placeholder config file on disk to the empty version. Must be
// called before the very first makeExecutable/makeGraphExecutor in the
// process, so library initialization reads a clean placeholder regardless of
// whether a prior process was killed mid-test and left it in its "extra"
// state. Also called at the top of test_reloadLibrary so the placeholder
// dance still works when only that test is selected via --gtest_filter.
void resetPlaceholderToEmpty() {
    ASSERT_TRUE(std::filesystem::copy_file(
        EMPTY_CONFIG_FILE, PLACEHOLDER_CONFIG_FILE,
        std::filesystem::copy_options::overwrite_existing))
        << "Failed to reset placeholder config to empty: "
        << PLACEHOLDER_CONFIG_FILE;
}
} // namespace

TEST(GraphExecutorLibraryTests, test_make_executable_no_errors) {
    using namespace BifrostUsd::GraphExecutor;

    ASSERT_NO_FATAL_FAILURE(resetPlaceholderToEmpty());

    // The name of the compound to execute.
    constexpr auto any_true_compound_name = "Core::Array::any_true_in_array"_asv;

    // Create an executable from the compound.
    StringArray       errors;
    Amino::Executable executable_untyped =
        makeExecutable(any_true_compound_name, errors);
    EXPECT_TRUE(executable_untyped.isValid());
    ASSERT_TRUE(errors.empty())
        << "Unexpected errors while creating executable: "
        << printMessages(errors).c_str();

    using namespace Amino::ExecutableT_Types;

    // Create a typed executable from the untyped one.
    // This will allow us to call it with typed inputs and will return typed
    // outputs.
    auto any_true = Amino::ExecutableT{
        std::move(executable_untyped), // untyped executable
        Ports{Port<Amino::Ptr<Amino::Array<bool>>>{"array"}}, // input names, types
        Ports{Port<bool>{"any_true"}} // output names, types
    };

    // Create an observer to monitor the executions.
    auto observer = Amino::TaskObserver::make<StreamObserver<std::ostream>>(
        std::cout, std::cerr, errors);
    observer->setVerbosityLevel(VerbosityLevel::eErrorsOnly);
    observer->setPrintPrefix("[any_true] ");

    auto inputMutable = Amino::newMutablePtr<Amino::Array<bool>>(3);
    (*inputMutable)[0] = false;
    (*inputMutable)[1] = true;
    (*inputMutable)[2] = false;
    Amino::Ptr<Amino::Array<bool>> input = inputMutable.toImmutable();
    std::tuple<bool> result =
        any_true(input, observer.getNotifier());
    EXPECT_EQ(result, std::make_tuple(true))
        << "Unexpected outputs: any_true=" << std::get<0>(result)
        << "\nErrors: " << printMessages(errors).c_str();
}

TEST(GraphExecutorLibraryTests, test_reloadLibrary) {
    using namespace BifrostUsd::GraphExecutor;

    // Reset the on-disk placeholder before any library access in this test,
    // so the test is correct when run in isolation via --gtest_filter (i.e.,
    // when test_make_executable_no_errors has not run first to do the reset).
    // In the normal in-order run this is just a defensive no-op.
    ASSERT_NO_FATAL_FAILURE(resetPlaceholderToEmpty());

    constexpr auto less_compound_name = "Test::Reload::Library::less"_asv;

    // The compound should not be available before reloading the library
    // with an extra config file:
    StringArray       errors;
    GraphExecutorPtr  executor = makeGraphExecutor(
        less_compound_name, errors);
    EXPECT_FALSE(executor)
        << "makeGraphExecutor() should have failed for non-existent compound.";
    EXPECT_FALSE(errors.empty())
        << "makeGraphExecutor() did not report errors as expected.";

    // Check that the config files exist and are absolute paths:
    EXPECT_TRUE(std::filesystem::exists(PLACEHOLDER_CONFIG_FILE))
        << "Placeholder config file does not exist: " << PLACEHOLDER_CONFIG_FILE;
    EXPECT_TRUE(std::filesystem::path(PLACEHOLDER_CONFIG_FILE).is_absolute())
        << "Placeholder config file is not absolute: " << PLACEHOLDER_CONFIG_FILE;
    EXPECT_TRUE(std::filesystem::exists(EXTRA_CONFIG_FILE))
        << "Extra config file does not exist: " << EXTRA_CONFIG_FILE;
    EXPECT_TRUE(std::filesystem::path(EXTRA_CONFIG_FILE).is_absolute())
        << "Extra config file is not absolute: " << EXTRA_CONFIG_FILE;
    EXPECT_TRUE(std::filesystem::exists(EMPTY_CONFIG_FILE))
        << "Empty config file does not exist: " << EMPTY_CONFIG_FILE;
    EXPECT_TRUE(std::filesystem::path(EMPTY_CONFIG_FILE).is_absolute())
        << "Empty config file is not absolute: " << EMPTY_CONFIG_FILE;

    struct PlaceholderGuard {
        ~PlaceholderGuard() {
            // Cleanup upon exit or early exit: Copy the "empty" config file
            // back into the "placeholder" config file. This way, the test can
            // be re-run without needing to manually restore the "placeholder"
            // config file.
            std::error_code ec;
            std::filesystem::copy_file(
                EMPTY_CONFIG_FILE, PLACEHOLDER_CONFIG_FILE,
                std::filesystem::copy_options::overwrite_existing,
                ec); // best-effort; swallow ec in destructor
        }
    } guard;

    // Copy the "extra" config file into the "placeholder" config file in the
    // build output directory.
    ASSERT_TRUE(std::filesystem::copy_file(
        EXTRA_CONFIG_FILE, PLACEHOLDER_CONFIG_FILE,
        std::filesystem::copy_options::overwrite_existing))
        << "Failed to copy `extra` config file `" << EXTRA_CONFIG_FILE
        << "` to `placeholder` config file `" << PLACEHOLDER_CONFIG_FILE << "`";

    // The "placeholder" config file contains the compound needed by the test.
    // Now reload the library and attempt to create the graph (not need to run):
    reloadLibrary();
    errors.clear();
    executor = makeGraphExecutor(less_compound_name, errors);
    EXPECT_TRUE(executor)
        << "makeGraphExecutor() should have succeeded for existing compound.";
    EXPECT_TRUE(errors.empty())
        << "Unexpected errors while creating GraphExecutor: "
        << printMessages(errors).c_str();
}
