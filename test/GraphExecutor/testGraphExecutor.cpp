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
#include <Amino/Core/StringStl.h>
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
#include <map>

using StringArray = Amino::Array<Amino::String>;

/// \brief Alias for graph input arguments.
///
/// An "ordered" map from graph input port names as \c Amino::String to their
/// corresponding values.
/// Using an ordered map allows us to have deterministic order of setInput()
/// calls when looping over the map, which is helpful for testing and debugging.
///
/// Since values are stored as \c Amino::Any, a single map can hold inputs of
/// mixed types (int, float, string, etc.) without requiring separate containers
/// per type.
using GraphInputArgs = std::map<Amino::String, Amino::Any>;

using namespace Amino::StringViewLiterals;
using namespace BifrostUsd::TestUtils;

namespace {
std::string printCollectedMessages(const StringArray& msgs) {
    std::string result = "[BEGIN MESSAGES]\n";
    for (const auto& msg : msgs) {
        result += msg.c_str();
        result += "\n";
    }
    result += "[END MESSAGES]\n";
    return result;
}

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

/// Loop over \p args and call executor->setInput() for each entry.
/// Returns true if and only if all setInput() calls succeed.
/// Any error messages from failed calls are appended to \p messages.
bool setInputs(const BifrostUsd::GraphExecutor::GraphExecutorPtr& executor,
               const GraphInputArgs&                              args,
               StringArray&                                       messages) {
    messages.clear();
    bool success = true;
    for (const auto& [name, value] : args) {
        StringArray portMessages;
        if (!executor->setInput(name, value, portMessages)) {
            for (const auto& msg : portMessages) {
                messages.push_back(msg);
            }
            success = false;
        }
    }
    return success;
}

bool setInputs(const BifrostUsd::GraphExecutor::GraphExecutorPtr& executor,
               const GraphInputArgs&                              args) {
    StringArray messages;
    return setInputs(executor, args, messages);
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

    GraphInputArgs args;
    args["a"] = 1.f;
    args["b"] = 2.f;

    // This graph does not contain errors and it should execute successfully.
    // Case 1: execute it with verbosity==eSilent
    StringArray messages;
    EXPECT_TRUE(setInputs(executor, args));
    EXPECT_TRUE(executor->execute(messages, VerbosityLevel::eSilent));
    EXPECT_TRUE(messages.empty())
        << "execute() with verbosity==eSilent should not collect any "
           "messages.\n"
        << printCollectedMessages(messages).c_str();

    // Case 2: execute it with verbosity==eErrorsOnly:
    EXPECT_TRUE(setInputs(executor, args));
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
    EXPECT_TRUE(setInputs(executor, args));
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
    GraphExecutorPtr executor =
        makeGraphExecutor("Test::Error::no_promotion_exists");
    ASSERT_TRUE(executor);

    // This graph contains an error that should cause execution to fail.
    // Case 1: execute it with verbosity==eSilent
    StringArray    messages;
    EXPECT_FALSE(executor->execute(messages, VerbosityLevel::eSilent));
    EXPECT_TRUE(messages.empty())
        << "execute() with verbosity==eSilent should not report any messages.\n"
        << printCollectedMessages(messages).c_str();

    // Case 2: execute it with verbosity==eErrorsOnly: both "can't execute" and
    // "completed with errors" are present.
    constexpr const char* kCannotExecute = "can't execute";
    constexpr const char* kCompleted = "completed with errors";
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

TEST(GraphExecutorTests, setInput) {
    using namespace BifrostUsd::GraphExecutor;
    StringArray      errors;
    GraphExecutorPtr executor = makeGraphExecutor("Test::my_add", errors);
    EXPECT_TRUE(errors.empty())
        << "Unexpected errors while creating GraphExecutor: \n"
        << printCollectedMessages(errors).c_str();
    ASSERT_TRUE(executor);

    // Executing without any prior setInput() call uses the graph's default
    // values; "Test::my_add" defaults are "a"==0 and "b"==0:
    {
        EXPECT_TRUE(executor->execute(errors, VerbosityLevel::eErrorsOnly));
        EXPECT_TRUE(errors.empty());
        auto closure = executor->extractOutputClosure("result"_asv);
        ASSERT_TRUE(closure);
        EXPECT_EQ(closure.get<float>(), 0.f);
    }

    // A type mismatch is reported; an unknown port name is also an error.
    {
        GraphInputArgs args;
        args["a"]       = 1.5f;  // 1st: start with a valid input
        args["b"]       = false; // 2nd: attempt wrong type, "b" keeps default 0
        args["unknown"] = 20.f;  // 3rd: unknown port, reported as error
        EXPECT_FALSE(setInputs(executor, args, errors));
        EXPECT_FALSE(errors.empty())
            << "setInputs() should have reported errors but none were found.";
        EXPECT_TRUE(findMessage(errors, "type does not match"))
            << "Expected a type mismatch error for \"b\".\n"
            << printCollectedMessages(errors).c_str();
        EXPECT_TRUE(findMessage(errors, "does not exist"))
            << "Expected an unknown port error for \"unknown\".\n"
            << printCollectedMessages(errors).c_str();

        // setInputs() helper keeps on processing all inputs even upon errors.
        // "a" was set to 1.5; "b" failed and keeps its default value 0:
        EXPECT_TRUE(executor->execute(errors, VerbosityLevel::eErrorsOnly));
        EXPECT_TRUE(errors.empty());
        auto closure = executor->extractOutputClosure("result"_asv);
        ASSERT_TRUE(closure);
        EXPECT_EQ(closure.get<float>(), 1.5f);
    }

    // Re-execute the graph without setting the inputs again.
    // Since the ExecutionInputs are consumed by the previous execution and
    // not set again, the execution should succeed with default values again,
    // i.e. "a"==0 and "b"==0:
    {
        EXPECT_TRUE(executor->execute(errors, VerbosityLevel::eErrorsOnly));
        EXPECT_TRUE(errors.empty());
        auto closure = executor->extractOutputClosure("result"_asv);
        ASSERT_TRUE(closure);
        EXPECT_EQ(closure.get<float>(), 0.f);
    }

    // Setting valid values for all ports produces the expected result:
    {
        GraphInputArgs args;
        args["a"] = 1.f;
        args["b"] = 2.f;
        EXPECT_TRUE(setInputs(executor, args, errors));
        EXPECT_TRUE(errors.empty())
            << "Unexpected errors while calling setInputs() with valid args:\n"
            << printCollectedMessages(errors).c_str();
        EXPECT_TRUE(executor->execute(VerbosityLevel::eErrorsOnly));
        auto closure = executor->extractOutputClosure("result"_asv);
        ASSERT_TRUE(closure);
        EXPECT_EQ(closure.get<float>(), 3.f);
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

    GraphInputArgs args;
    args["a"] = 1.f;
    args["b"] = 2.f;
    EXPECT_TRUE(setInputs(executor, args));
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
    EXPECT_TRUE(setInputs(executor, args));
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

    Amino::long_t  expectedCount = 4;
    GraphInputArgs args;
    args["layer"]   = Amino::String{"/path/to/saved/layer.usda"};
    args["type"]    = Amino::String{"Sphere"};
    args["count"]   = expectedCount;
    args["save"]    = false;
    args["up_axis"] = BifrostUsd::UpAxis::Y;

    EXPECT_TRUE(setInputs(executor, args, errors));
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

    GraphInputArgs args;
    unsigned int majorSegments = 20;
    unsigned int minorSegments = 20;
    args["major_segments"] = majorSegments;
    args["minor_segments"] = minorSegments;
    EXPECT_TRUE(setInputs(executor, args, errors));
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

    // Wrap the single output Object into a length-1 array and convert it
    // to a USD Stage:
    auto objectsArrayMutablePtr =
        Amino::newMutablePtr<Amino::Array<Amino::Ptr<Bifrost::Object>>>(1);
    (*objectsArrayMutablePtr)[0] = *outObjectPtr;
    Amino::Ptr<BifrostUsd::Stage> outStagePtr =
        BifrostUsd::DynamicPayload::objects_to_stage(
            objectsArrayMutablePtr.toImmutable());
    ASSERT_TRUE(outStagePtr);
    ASSERT_TRUE(*outStagePtr);

    auto rootPrim = (*outStagePtr)->GetPrimAtPath(SdfPath{"/root"});
    EXPECT_TRUE(rootPrim);
    EXPECT_TRUE(UsdGeomXform{rootPrim});

    // objects_to_stage creates a /root/geo Scope under the root Xform:
    auto geoPrim = (*outStagePtr)->GetPrimAtPath(SdfPath{"/root/geo"});
    EXPECT_TRUE(geoPrim);
    auto geoScope = UsdGeomScope{geoPrim};
    EXPECT_TRUE(geoScope);

    // objects_to_stage creates a /root/geo/mesh Mesh prim under the geo scope.
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

    GraphInputArgs args;
    args["axis_segments"]   = axis_segments;
    args["height_segments"] = height_segments;
    args["cap_segments"]    = cap_segments;
    EXPECT_TRUE(setInputs(executor, args, errors));
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
    // objects_to_stage() with default purpose creates:
    //   /root          - Xform
    //   /root/geo      - Scope
    //   /root/geo/mesh - Mesh (one per input object)
    Amino::Ptr<BifrostUsd::Stage> outStagePtr =
        BifrostUsd::DynamicPayload::objects_to_stage(
            objectsPtr.toImmutable());
    ASSERT_TRUE(outStagePtr);
    ASSERT_TRUE(*outStagePtr);

    auto rootPrim = (*outStagePtr)->GetPrimAtPath(SdfPath{"/root"});
    EXPECT_TRUE(rootPrim);
    EXPECT_TRUE(UsdGeomXform{rootPrim});

    // objects_to_stage creates a /root/geo Scope:
    auto geoPrim = (*outStagePtr)->GetPrimAtPath(SdfPath{"/root/geo"});
    EXPECT_TRUE(geoPrim);
    EXPECT_TRUE(UsdGeomScope{geoPrim});

    // One capsule mesh expected at /root/geo/mesh:
    auto meshPrim = (*outStagePtr)->GetPrimAtPath(SdfPath{"/root/geo/mesh"});
    EXPECT_TRUE(meshPrim);
    auto meshGeom = UsdGeomMesh{meshPrim};
    EXPECT_TRUE(meshGeom);
}

PXR_NAMESPACE_CLOSE_SCOPE
