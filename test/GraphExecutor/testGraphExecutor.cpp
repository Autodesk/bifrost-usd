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
#include <Amino/Core/Any.h>
#include <Amino/Core/BuiltInTypes.h>
#include <Amino/Core/Ptr.h>
#include <Amino/Core/Span.h>
#include <Amino/Core/String.h>
#include <Amino/Core/StringView.h>
#include <Amino/Executor/ExecutableT.h>
#include <Amino/Executor/ExecutionInputs.h>
#include <Amino/Executor/ExecutionOutputs.h>
#include <Amino/Library/ConstLibrary.h>
#include <Amino/RTTI/TypeRegistry.h>

// Bifrost
#include <Bifrost/Geometry/GeoProperty.h>
#include <Bifrost/Geometry/Primitives.h>
#include <Bifrost/Object/Object.h>

// Bifrost USD
#include <BifrostUsd/GraphExecutor/GraphExecutor.h>
#include <BifrostUsd/GraphExecutor/GraphExecutorFactory.h>
#include <BifrostUsd/UsdTranslator/ObjectToStage.h>

#include <BifrostUsd/Stage.h>
#include <nodedefs/usd_pack/usd_utils.h>

// Open USD
#include <pxr/usd/kind/registry.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/xform.h>

// C++ Standard Library
#include <algorithm>
#include <cctype>
#include <regex>
#include <string>

using StringArray = Amino::Array<Amino::String>;
using namespace Amino::StringViewLiterals;
using namespace BifrostUsd::TestUtils;

namespace {
auto printCollectedMessages = [](const StringArray& msgs) {
    std::string result = "[BEGIN MESSAGES]\n";
    for (const auto& msg : msgs) {
        result += msg.c_str();
        result += "\n";
    }
    result += "[END MESSAGES]\n";
    return result;
};

bool findMessage(const StringArray& msgs, const std::string& substring) {
    auto toLower = [](const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return result;
    };
    const std::string lowerSubstring = toLower(substring);
    for (const auto& msg : msgs) {
        if (toLower(msg.c_str()).find(lowerSubstring) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool findMessageRegex(const StringArray& msgs,
                              const std::string& pattern) {
    const std::regex re{pattern};
    for (const auto& msg : msgs) {
        if (std::regex_search(msg.c_str(), re)) {
            return true;
        }
    }
    return false;
}
} // namespace

PXR_NAMESPACE_OPEN_SCOPE

TEST(GraphExecutorTests, error_non_existent_compound) {
    using namespace BifrostUsd::GraphExecutor;
    StringArray      errors;
    GraphExecutorPtr executor =
        makeGraphExecutor("non_existent_compound", errors);
    EXPECT_FALSE(executor)
        << "makeGraphExecutor() should have failed for non-existent compound.";
    EXPECT_FALSE(errors.empty())
        << "makeGraphExecutor() did not report errors as expected.";
}

TEST(GraphExecutorTests, verbosity_levels_on_success_case) {
    using namespace BifrostUsd::GraphExecutor;
    StringArray      errors;
    GraphExecutorPtr executor = makeGraphExecutor("Test::my_add", errors);
    EXPECT_TRUE(errors.empty())
        << "Unexpected errors while creating GraphExecutor: \n"
        << printCollectedMessages(errors).c_str();
    ASSERT_TRUE(executor);

    GraphArgs args;
    args["a"] = 1.f;
    args["b"] = 2.f;

    // This graph does not contain errors and it should execute successfully.
    // Case 1: execute it with verbosity==eSilent
    StringArray messages;
    EXPECT_TRUE(executor->setGraphInputs(args));
    EXPECT_TRUE(executor->execute(messages, VerbosityLevel::eSilent));
    EXPECT_TRUE(messages.empty())
        << "execute() with verbosity==eSilent should not collect any "
           "messages.\n"
        << printCollectedMessages(messages).c_str();

    // Case 2: execute it with verbosity==eErrorsOnly:
    EXPECT_TRUE(executor->setGraphInputs(args));
    EXPECT_TRUE(executor->execute(messages, VerbosityLevel::eErrorsOnly));
    EXPECT_TRUE(messages.empty())
        << "execute() with verbosity==eErrorsOnly should not collect any "
           "messages for a successful execution.\n"
        << printCollectedMessages(messages).c_str();

    // Case 3: execute it with verbosity==eAllMessages: both "started", some
    // progress messages, and "completed successfully" are present.
    constexpr const char* kStarted = "started";
    constexpr const char* kProgressPattern =
        R"(-[^\n]+[\d]/[\d])"; // e.g. "- Title 1/3"
    constexpr const char* kCompleted = "completed successfully";
    EXPECT_TRUE(executor->setGraphInputs(args));
    EXPECT_TRUE(executor->execute(messages, VerbosityLevel::eAllMessages));
    EXPECT_TRUE(findMessage(messages, kStarted))
        << "execute() with verbosity==eAllMessages should collect message `"
        << kStarted << "`.\n"
        << printCollectedMessages(messages).c_str();
    EXPECT_TRUE(findMessageRegex(messages, kProgressPattern))
        << "execute() with verbosity==eAllMessages should collect progress "
           "messages matching \""
        << kProgressPattern << "\".\n"
        << printCollectedMessages(messages).c_str();
    EXPECT_TRUE(findMessage(messages, kCompleted))
        << "execute() with verbosity==eAllMessages should collect message `"
        << kCompleted << "`.\n"
        << printCollectedMessages(messages).c_str();
}

TEST(GraphExecutorTests, verbosity_levels_on_failure_case) {
    using namespace BifrostUsd::GraphExecutor;
    StringArray      errors;
    GraphExecutorPtr executor =
        makeGraphExecutor("Test::Error::no_promotion_exists", errors);
    EXPECT_TRUE(errors.empty())
        << "Unexpected errors while creating GraphExecutor: \n"
        << printCollectedMessages(errors).c_str();
    ASSERT_TRUE(executor);

    // This graph contains an error that should cause execution to fail.
    // Case 1: execute it with verbosity==eSilent
    GraphArgs   emptyArgs;
    StringArray messages;
    EXPECT_TRUE(executor->setGraphInputs(emptyArgs));
    EXPECT_FALSE(executor->execute(messages, VerbosityLevel::eSilent));
    EXPECT_TRUE(messages.empty())
        << "execute() with verbosity==eSilent should not report any messages.\n"
        << printCollectedMessages(messages).c_str();

    // Case 2: execute it with verbosity==eErrorsOnly: both "can't execute" and
    // "completed with errors" are present.
    constexpr const char* kCannotExecute = "can't execute";
    constexpr const char* kCompleted = "completed with errors";
    EXPECT_TRUE(executor->setGraphInputs(emptyArgs));
    EXPECT_FALSE(executor->execute(messages, VerbosityLevel::eErrorsOnly));
    EXPECT_TRUE(findMessage(messages, kCannotExecute))
        << "execute() with verbosity==eErrorsOnly should collect message `"
        << kCannotExecute << "`.\n"
        << printCollectedMessages(messages).c_str();
    EXPECT_TRUE(findMessage(messages, kCompleted))
        << "execute() with verbosity==eErrorsOnly should collect message `"
        << kCompleted << "`.\n"
        << printCollectedMessages(messages).c_str();

    // Case 3: execute it with verbosity==eAllMessages: both "can't execute" and
    // "completed with errors" are present.
    EXPECT_TRUE(executor->setGraphInputs(emptyArgs));
    EXPECT_FALSE(executor->execute(messages, VerbosityLevel::eAllMessages));
    EXPECT_TRUE(findMessage(messages, kCannotExecute))
        << "execute() with verbosity==eAllMessages should collect message `"
        << kCannotExecute << "`.\n"
        << printCollectedMessages(messages).c_str();
    EXPECT_TRUE(findMessage(messages, kCompleted))
        << "execute() with verbosity==eAllMessages should collect message `"
        << kCompleted << "`.\n"
        << printCollectedMessages(messages).c_str();
}

TEST(GraphExecutorTests, setGraphInputs) {
    using namespace BifrostUsd::GraphExecutor;
    StringArray      errors;
    GraphExecutorPtr executor = makeGraphExecutor("Test::my_add", errors);
    EXPECT_TRUE(errors.empty())
        << "Unexpected errors while creating GraphExecutor: \n"
        << printCollectedMessages(errors).c_str();
    ASSERT_TRUE(executor);

    // Executing the graph without setting the inputs should fail:
    EXPECT_FALSE(executor->execute(VerbosityLevel::eSilent));

    // Create the ExecutionInputs by calling setGraphInputs(), but with an
    // empty input map. This should succeed, and the default input values
    // defined in the graph "a"==0 and "b"==0 will be used:
    {
        GraphArgs emptyArgs;
        EXPECT_TRUE(executor->setGraphInputs(emptyArgs, errors));
        EXPECT_TRUE(errors.empty()) << "Unexpected errors while calling "
                                       "setGraphInputs() with emptyArgs: \n"
                                    << printCollectedMessages(errors).c_str();
        EXPECT_TRUE(executor->execute(VerbosityLevel::eErrorsOnly));

        auto closure = executor->extractOutputClosure("result"_asv);
        ASSERT_TRUE(closure);
        auto const& outAny = closure.getAny();
        EXPECT_TRUE(outAny.has_value());
        float const result = closure.get<float>();
        EXPECT_EQ(result, 0.f);
    }

    // Re-execute the graph without setting the inputs again.
    // Since the ExecutionInputs are consumed by the previous execution and
    // not set again, the execution should fail:
    EXPECT_FALSE(executor->execute(VerbosityLevel::eErrorsOnly));

    // Attempt to set a mix of correct inputs, and incorrect inputs that
    // have wrong name and wrong type. Correct inputs should be accepted
    // and incorrect inputs should be ignored.
    {
        GraphArgs args;
        args["a"] = 1.5f;
        args["b"] = false; // wrong type: reported but default "b"==0 is used
        args["unknown"] = 20.f;  // wrong name: ignored
        EXPECT_TRUE(executor->setGraphInputs(args, errors));
        const char* search = "type mismatch";
        EXPECT_FALSE(errors.empty())
            << "setGraphInputs() should have reported a `"
            << search << "` error but no errors were reported.";
        EXPECT_TRUE(findMessage(errors, search))
            << "setGraphInputs() did not report a message containing `"
            << search << "` as expected. \n"
            << printCollectedMessages(errors).c_str();
        EXPECT_TRUE(executor->execute(VerbosityLevel::eErrorsOnly));

        auto closure = executor->extractOutputClosure("result"_asv);
        ASSERT_TRUE(closure);
        auto const& outAny = closure.getAny();
        EXPECT_TRUE(outAny.has_value());
        float const result = closure.get<float>();
        EXPECT_EQ(result, 1.5f);
    }

    // Now set the "a" and "b" inputs and execute again:
    {
        GraphArgs args;
        args["a"] = 1.f;
        args["b"] = 2.f;
        EXPECT_TRUE(executor->setGraphInputs(args, errors));
        EXPECT_TRUE(errors.empty()) << "Unexpected errors while calling "
                                       "setGraphInputs() with valid args: \n"
                                    << printCollectedMessages(errors).c_str();
        EXPECT_TRUE(executor->execute(VerbosityLevel::eErrorsOnly));

        auto closure = executor->extractOutputClosure("result"_asv);
        ASSERT_TRUE(closure);
        auto const& outAny = closure.getAny();
        EXPECT_TRUE(outAny.has_value());
        float const result = closure.get<float>();
        EXPECT_EQ(result, 3.f);
    }
}

TEST(GraphExecutorTests, extractOutputClosure) {
    using namespace BifrostUsd::GraphExecutor;
    StringArray      errors;
    GraphExecutorPtr executor = makeGraphExecutor("Test::my_add", errors);
    EXPECT_TRUE(errors.empty())
        << "Unexpected errors while creating GraphExecutor: \n"
        << printCollectedMessages(errors).c_str();
    ASSERT_TRUE(executor);

    GraphArgs args;
    args["a"] = 1.f;
    args["b"] = 2.f;
    EXPECT_TRUE(executor->setGraphInputs(args));
    EXPECT_TRUE(executor->execute(VerbosityLevel::eErrorsOnly));
    {
        // 1st call to extractOutputClosure() return a valid closure:
        auto closure = executor->extractOutputClosure("result"_asv);
        ASSERT_TRUE(closure);
        auto const& outAny = closure.getAny();
        EXPECT_TRUE(outAny.has_value());
        float const result = closure.get<float>();
        EXPECT_EQ(result, 3.f);

        // A 2nd call to extractOutputClosure() before another execution must
        // return an invalid closure:
        closure = executor->extractOutputClosure("result"_asv);
        EXPECT_FALSE(closure.isValid());
    }

    // Re-execute the graph:
    EXPECT_TRUE(executor->setGraphInputs(args));
    EXPECT_TRUE(executor->execute(VerbosityLevel::eErrorsOnly));
    {
        // 1st call to extractOutputClosure() return a valid closure:
        auto closure = executor->extractOutputClosure("result"_asv);
        ASSERT_TRUE(closure);
        auto const& outAny = closure.getAny();
        EXPECT_TRUE(outAny.has_value());
        float const result = closure.get<float>();
        EXPECT_EQ(result, 3.f);

        // The 2nd call to extractOutputClosure() returns invalid closure again:
        closure = executor->extractOutputClosure("result"_asv);
        EXPECT_FALSE(closure.isValid());
    }
}

TEST(GraphExecutorTests, test_stage_output) {
    using namespace BifrostUsd::GraphExecutor;
    StringArray      errors;
    GraphExecutorPtr executor =
        makeGraphExecutor("Test::USD::Stage::pill_generator", errors);
    EXPECT_TRUE(errors.empty())
        << "Unexpected errors while creating GraphExecutor: \n"
        << printCollectedMessages(errors).c_str();
    ASSERT_TRUE(executor);

    Amino::long_t expectedCount = 4;
    GraphArgs     args;
    args["layer"]   = Amino::String{"/path/to/saved/layer.usda"};
    args["type"]    = Amino::String{"Sphere"};
    args["count"]   = expectedCount;
    args["save"]    = false;
    args["up_axis"] = BifrostUsd::UpAxis::Y;

    EXPECT_TRUE(executor->setGraphInputs(args, errors));
    EXPECT_TRUE(errors.empty())
        << "Unexpected errors while setting graph inputs: \n"
        << printCollectedMessages(errors).c_str();
    EXPECT_TRUE(executor->execute(errors, VerbosityLevel::eErrorsOnly));
    EXPECT_TRUE(errors.empty())
        << "Unexpected errors while executing the graph: \n"
        << printCollectedMessages(errors).c_str();

    // Retrieve the output stage:
    auto closure = executor->extractOutputClosure("out_stage"_asv);
    ASSERT_TRUE(closure);
    auto const& outAny = closure.getAny();
    EXPECT_TRUE(outAny.has_value());
    auto const& outStagePtr =
        Amino::any_cast<Amino::Ptr<BifrostUsd::Stage>>(&outAny);
    ASSERT_TRUE(outStagePtr);
    auto const& outStage = *outStagePtr;

    using namespace USDUtils;

    // The pill_generator graph creates a stage containing <expectedCount>
    // `pill<n>` primitives, each is a Xform with a single `shape` child
    // primitive in it of type `Sphere`.

    // Check the total number of root children:
    auto rootPrim = USDUtils::get_prim_at_path("/", *outStage);
    ASSERT_TRUE(rootPrim.IsValid());
    auto          rootChildren = rootPrim.GetChildren();
    Amino::long_t primCnt =
        std::distance(rootChildren.begin(), rootChildren.end());
    EXPECT_EQ(primCnt, expectedCount);

    // Each root `pill<n>` primitive should be an Xform with a single `shape`
    // child primitive of type `Sphere`:
    Amino::long_t checkedCnt = 0;
    for (const auto& pillPrim : rootChildren) {
        const std::string pillPath = pillPrim.GetPath().GetString();
        EXPECT_EQ(pillPrim.GetTypeName().GetString(), "Xform")
            << "Unexpected prim type at: " << pillPath;

        const std::string shapePath = pillPath + "/shape";
        auto              shapePrim =
            USDUtils::get_prim_at_path(shapePath.c_str(), *outStage);
        EXPECT_TRUE(shapePrim.IsValid()) << "Prim not found at: " << shapePath;
        if (shapePrim.IsValid()) {
            EXPECT_EQ(shapePrim.GetTypeName().GetString(), "Sphere")
                << "Unexpected prim type at: " << shapePath;
        }
        ++checkedCnt;
    }
    EXPECT_EQ(checkedCnt, expectedCount);
}

TEST(GraphExecutorTests, test_object_output) {
    using namespace BifrostUsd::GraphExecutor;
    StringArray      errors;
    GraphExecutorPtr executor =
        makeGraphExecutor("Modeling::Primitive::create_mesh_torus", errors);
    EXPECT_TRUE(errors.empty())
        << "Unexpected errors while creating GraphExecutor: \n"
        << printCollectedMessages(errors).c_str();
    ASSERT_TRUE(executor);
    EXPECT_FALSE(executor->hasTerminal());

    GraphArgs args;
    unsigned int majorSegments = 20;
    unsigned int minorSegments = 20;
    args["major_segments"] = majorSegments;
    args["minor_segments"] = minorSegments;
    EXPECT_TRUE(executor->setGraphInputs(args, errors));
    EXPECT_TRUE(errors.empty())
        << "Unexpected errors while setting graph inputs: \n"
        << printCollectedMessages(errors).c_str();
    EXPECT_TRUE(executor->execute(errors, VerbosityLevel::eErrorsOnly));
    EXPECT_TRUE(errors.empty())
        << "Unexpected errors while executing the graph: \n"
        << printCollectedMessages(errors).c_str();

    // Retrieve the output Bifrost Object:
    auto closure = executor->extractOutputClosure("torus_mesh"_asv);
    ASSERT_TRUE(closure);
    auto const& outAny = closure.getAny();
    EXPECT_TRUE(outAny.has_value());
    const Amino::Ptr<Bifrost::Object>* outObjectPtr =
        Amino::any_cast<Amino::Ptr<Bifrost::Object>>(&outAny);
    ASSERT_TRUE(outObjectPtr);

    // Convert the output Bifrost Object to a USD Stage:
    Amino::Ptr<BifrostUsd::Stage> outStagePtr =
        BifrostUsd::DynamicPayload::object_to_stage(*outObjectPtr);
    ASSERT_TRUE(outStagePtr);
    ASSERT_TRUE(*outStagePtr);

    auto rootPrim = (*outStagePtr)->GetPrimAtPath(SdfPath{"/root"});
    EXPECT_TRUE(rootPrim);
    EXPECT_TRUE(UsdGeomXform{rootPrim});

    // object_to_stage creates a /root/geo Scope under the root Xform:
    auto geoPrim = (*outStagePtr)->GetPrimAtPath(SdfPath{"/root/geo"});
    EXPECT_TRUE(geoPrim);
    auto geoScope = UsdGeomScope{geoPrim};
    EXPECT_TRUE(geoScope);

    // object_to_stage creates a /root/geo/mesh Mesh prim under the geo scope.
    // Result mesh is expected to have <majorSegments>*<minorSegments> faces:
    auto meshPrim = (*outStagePtr)->GetPrimAtPath(SdfPath{"/root/geo/mesh"});
    EXPECT_TRUE(meshPrim);
    auto meshGeom = UsdGeomMesh{meshPrim};
    ASSERT_TRUE(meshGeom);
    EXPECT_EQ(meshGeom.GetFaceCount(), majorSegments * minorSegments);
}

TEST(GraphExecutorTests, test_array_of_objects_output) {
    using namespace BifrostUsd::GraphExecutor;
    StringArray       errors;
    GraphExecutorPtr  executor = makeGraphExecutor(
        "Test::GraphExecutor::create_mesh_capsule_with_terminal", errors);
    EXPECT_TRUE(errors.empty())
        << "Unexpected errors while creating GraphExecutor: \n"
        << printCollectedMessages(errors).c_str();
    ASSERT_TRUE(executor);

    // This graph is expected to have a 'final' terminal output:
    EXPECT_TRUE(executor->hasTerminal());
    EXPECT_TRUE(executor->hasTerminalPort(TerminalType::eFinal));

    const int axis_segments   = 8;
    const int height_segments = 5;
    const int cap_segments    = 4;

    GraphArgs args;
    args["axis_segments"]   = axis_segments;
    args["height_segments"] = height_segments;
    args["cap_segments"]    = cap_segments;
    EXPECT_TRUE(executor->setGraphInputs(args, errors));
    EXPECT_TRUE(errors.empty())
        << "Unexpected errors while setting graph inputs: \n"
        << printCollectedMessages(errors).c_str();
    EXPECT_TRUE(executor->execute(errors, VerbosityLevel::eErrorsOnly));
    EXPECT_TRUE(errors.empty())
        << "Unexpected errors while executing the graph: \n"
        << printCollectedMessages(errors).c_str();

    // Retrieve the output from the 'final' terminal:
    Amino::TerminalOutput finalOutput =
        executor->extractTerminalOutput(TerminalType::eFinal);
    EXPECT_TRUE(finalOutput.isValid());

    // Once we extracted the TerminalOutput, it should no longer be available
    // for extraction again:
    {
        Amino::TerminalOutput finalOutput2 =
            executor->extractTerminalOutput(TerminalType::eFinal);
        EXPECT_FALSE(finalOutput2.isValid());
    }

    // Flatten the TerminalOutput to access the array of objects:
    auto flattened = finalOutput.getFlattened();
    ASSERT_TRUE(flattened);

    using T      = Amino::Ptr<Amino::Array<Amino::Ptr<Bifrost::Object>>>;
    using ArrayT = Amino::Ptr<Amino::Array<T>>;
    ASSERT_EQ(Amino::getTypeId<ArrayT>(), flattened.getTypeId());
    auto const& outArrayOfObjectsPtr = flattened.get<ArrayT>();
    EXPECT_TRUE(outArrayOfObjectsPtr);

    Amino::MutablePtr<Amino::Array<Amino::Ptr<Bifrost::Object>>> objectsPtr;
    {
        // Count the total number of objects across all arrays in the output:
        size_t totalCount = 0;
        for (const auto& arrayPtr : *outArrayOfObjectsPtr) {
            totalCount += arrayPtr->size();
        }
        EXPECT_EQ(totalCount, 1u); // only one Object is expected by this test
        
        // Preallocate a flat array and retrieve all Bifrost Objects into it for
        // conversion to USD Stage:
        objectsPtr =
            Amino::newMutablePtr<Amino::Array<Amino::Ptr<Bifrost::Object>>>(
                totalCount);
        size_t idx = 0;
        for (const auto& arrayPtr : *outArrayOfObjectsPtr) {
            for (const auto& objPtr : *arrayPtr) {
                (*objectsPtr)[idx++] = objPtr;
            }
        }
    }

    // Convert the array of Bifrost Objects to a USD Stage:
    // array_of_objects_to_stage() with default purpose creates:
    //   /root           - Xform
    //   /root/geo       - Scope
    //   /root/geo/mesh1 - Mesh (1-indexed, one per input object)
    Amino::Ptr<BifrostUsd::Stage> outStagePtr =
        BifrostUsd::DynamicPayload::array_of_objects_to_stage(
            objectsPtr.toImmutable());
    ASSERT_TRUE(outStagePtr);
    ASSERT_TRUE(*outStagePtr);

    auto rootPrim = (*outStagePtr)->GetPrimAtPath(SdfPath{"/root"});
    EXPECT_TRUE(rootPrim);
    EXPECT_TRUE(UsdGeomXform{rootPrim});

    // array_of_objects_to_stage creates a /root/geo Scope:
    auto geoPrim = (*outStagePtr)->GetPrimAtPath(SdfPath{"/root/geo"});
    EXPECT_TRUE(geoPrim);
    EXPECT_TRUE(UsdGeomScope{geoPrim});

    // One capsule mesh expected at /root/geo/mesh1:
    auto meshPrim = (*outStagePtr)->GetPrimAtPath(SdfPath{"/root/geo/mesh1"});
    EXPECT_TRUE(meshPrim);
    auto meshGeom = UsdGeomMesh{meshPrim};
    EXPECT_TRUE(meshGeom);
}

PXR_NAMESPACE_CLOSE_SCOPE
