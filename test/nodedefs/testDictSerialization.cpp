//-
// Copyright 2022 Autodesk, Inc.
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

#include <nodedefs/usd_pack/usd_prim_nodedefs.h>
#include <nodedefs/usd_pack/usd_stage_nodedefs.h>
#include <BifrostUsd/Stage.h>

#include <Amino/Core/String.h>
#include <Bifrost/Object/Object.h>

TEST(DictionarySerialization, serialize_dictionary_metadata) {
    // This is linked to the following investigation.
    // - Compiling/linking with USD no python
    // - Running against USD with python
    //
    // The issue, some serialization functions are not
    // at the same location in the two USD versions.
    // Found doing dictionary serialization via metadata.
    //
    // This test is for manual testing when switching
    // runtimes in the context of the issue above.
    //
    // In the build system, we do not switch USD runtimes
    // because we build/link/test against one USD.
    // So, it should always work!

    // Create Prim
    BifrostUsd::Stage out_stage;
    Amino::String       path{"/foo"};
    Amino::String       type{"Capsule"};

    USD::Prim::create_prim(out_stage, path, type);

    // Prepare metadata
    int  defFoo{5};
    int  defBar{2};
    auto dict = Bifrost::createObject();
    dict->setProperty("foo", defFoo);
    dict->setProperty("bar", defBar);

    // Set the metadata
    Amino::String key{"customData"};
    bool success = USD::Prim::set_prim_metadata(out_stage, path, key, *dict);
    EXPECT_TRUE(success);

    // Get the metadata
    // Note that Getting is also tested in testPrimNodeDefs.cpp
    Amino::Ptr<Bifrost::Object> objDefault = Bifrost::createObject();
    Amino::Ptr<Bifrost::Object> objResult;

    success = USD::Prim::get_prim_metadata(out_stage, path, "customData",
                                           objDefault, objResult);

    ASSERT_TRUE(success);
    ASSERT_TRUE(objResult);
    auto any    = objResult->getProperty("foo");
    auto resFoo = Amino::any_cast<Amino::int_t>(&any);
    ASSERT_TRUE(resFoo);
    ASSERT_EQ(*resFoo, defFoo);

    any         = objResult->getProperty("bar");
    auto resBar = Amino::any_cast<Amino::int_t>(&any);
    ASSERT_TRUE(resBar);
    ASSERT_EQ(*resBar, defBar);

    // Serialize the metadata and check the output.
    // The code exercised to serialize is not the same as for get_prim_metadata.
    // The code to serialize uses "function tables" in USD.
    // If the code was built against USD with no python, for example then
    // it may not run correctly against a USD with python because the
    // "function tables" changed.
    // This issue is being investigated and it does not hurt to
    // keep the test.
    //
    // Debugging : std::cout << "Metadata is " << result.c_str() << std::endl;
    Amino::String result;
    USD::Stage::export_stage_to_string(out_stage, result);
    // clang-format off
    const char* flattenedFileContent = R"usda(#usda 1.0
(
    doc = """Generated from Composed Stage of root layer 
"""
)

def Capsule "foo" (
    customData = {
        int bar = 2
        int foo = 5
    }
)
{
}

)usda";
// clang-format on
    ASSERT_STREQ(result.c_str(), flattenedFileContent);
}
