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

TEST(GraphExecutorLibraryTests, test_make_executable_no_errors) {
    using namespace BifrostUsd::GraphExecutor;

    // The name of the compound to execute.
    constexpr auto string_remove_suffix_compound_name =
        "Core::String::string_remove_suffix"_asv;

    // Create an executable from the compound.
    StringArray       errors;
    Amino::Executable executable_untyped = makeExecutable(
                        string_remove_suffix_compound_name, errors);
    EXPECT_TRUE(executable_untyped.isValid());
    ASSERT_TRUE(errors.empty())
        << "Unexpected errors while creating executable: "
        << printMessages(errors).c_str();

    using namespace Amino::ExecutableT_Types;

    // Create a typed executable from the untyped one.
    // This will allow us to call it with typed inputs and will return typed
    // outputs.
    auto string_remove_suffix = Amino::ExecutableT{
        std::move(executable_untyped), // untyped executable
        Ports{Port<Amino::String>{"string"},
              Port<Amino::String>{"suffix"}}, // input names, types

        Ports{Port<Amino::String>{"remainder"},
              Port<bool>{"found"}} // output names, types
    };

    // Create an observer to monitor the executions.
    auto observer = Amino::TaskObserver::make<StreamObserver<std::ostream>>(
        std::cout, std::cerr, errors);
    observer->setVerbosityLevel(VerbosityLevel::eErrorsOnly);
    observer->setPrintPrefix("[string_remove_suffix] ");

    std::tuple<Amino::String, bool> result =
        string_remove_suffix("Prefix Word", "Word", observer.getNotifier());
    EXPECT_EQ(result, std::make_tuple("Prefix ", true))
        << "Unexpected outputs: remainder=" << std::get<0>(result).c_str()
        << ", found=" << std::boolalpha << std::get<1>(result)
        << "\nErrors: " << printMessages(errors).c_str();
}

TEST(GraphExecutorLibraryTests, test_reloadLibrary) {

#ifndef PLACEHOLDER_CONFIG_FILE
#error "PLACEHOLDER_CONFIG_FILE must be defined"
#endif
#ifndef EMPTY_CONFIG_FILE
#error "EMPTY_CONFIG_FILE must be defined"
#endif
#ifndef EXTRA_CONFIG_FILE
#error "EXTRA_CONFIG_FILE must be defined"
#endif

    using namespace BifrostUsd::GraphExecutor;
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

    // Check that the "placeholder" and "extra" config files exist:
    EXPECT_TRUE(std::filesystem::exists(PLACEHOLDER_CONFIG_FILE))
        << "Placeholder config file does not exist: " << PLACEHOLDER_CONFIG_FILE;
    EXPECT_TRUE(std::filesystem::path(PLACEHOLDER_CONFIG_FILE).is_absolute())
        << "Placeholder config file is not absolute: " << PLACEHOLDER_CONFIG_FILE;
    EXPECT_TRUE(std::filesystem::exists(EXTRA_CONFIG_FILE))
        << "Extra config file does not exist: " << EXTRA_CONFIG_FILE;
    EXPECT_TRUE(std::filesystem::path(EXTRA_CONFIG_FILE).is_absolute())
        << "Extra config file is not absolute: " << EXTRA_CONFIG_FILE;

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

    // Check that the "empty" config file exists:
    EXPECT_TRUE(std::filesystem::exists(EMPTY_CONFIG_FILE))
        << "Empty config file does not exist: " << EMPTY_CONFIG_FILE;
    EXPECT_TRUE(std::filesystem::path(EMPTY_CONFIG_FILE).is_absolute())
        << "Empty config file is not absolute: " << EMPTY_CONFIG_FILE;

    // Copy the "empty" config file back into the "placeholder" config file.
    // This way, the test can be re-run without needing to manually restore the
    // "placeholder" config file.
    EXPECT_TRUE(std::filesystem::copy_file(
        EMPTY_CONFIG_FILE, PLACEHOLDER_CONFIG_FILE,
        std::filesystem::copy_options::overwrite_existing))
        << "Failed to copy `empty` config file `" << EMPTY_CONFIG_FILE
        << "` to `placeholder` config file `" << PLACEHOLDER_CONFIG_FILE << "`";
}
