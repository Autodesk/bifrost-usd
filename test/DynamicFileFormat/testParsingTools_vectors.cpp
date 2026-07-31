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

// Source files from src/DynamicFileFormat:
#include "parsingTools.h"

#include <gtest/gtest.h>

using namespace BifrostUsd::DynamicPayload;

/* clang-format off */

//------------------------------------------------------------------------------
// maybeGetStringVec2
//------------------------------------------------------------------------------

TEST(ParsingToolsTests, maybeGetStringVec2_Success) {
    // Basic form - no space after comma
    StringVec2Opt r = maybeGetStringVec2("(1,2)");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ((*r)[0], "1");
    EXPECT_EQ((*r)[1], "2");

    // Typical "(x, y)" form - space after comma is part of the second component
    r = maybeGetStringVec2("(3.14, 2.71)");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ((*r)[0], "3.14");
    EXPECT_EQ((*r)[1], " 2.71");

    // Signed values
    r = maybeGetStringVec2("(-1, -2)");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ((*r)[0], "-1");
    EXPECT_EQ((*r)[1], " -2");

    // Whitespace around whole expression is trimmed, but preserved inside components
    r = maybeGetStringVec2(" \t( 1 , 2 )  \t");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ((*r)[0], " 1 ");
    EXPECT_EQ((*r)[1], " 2 ");
}

TEST(ParsingToolsTests, maybeGetStringVec2_Failure) {
    EXPECT_FALSE(maybeGetStringVec2("").has_value());           // empty
    EXPECT_FALSE(maybeGetStringVec2("1, 2").has_value());       // missing both parens
    EXPECT_FALSE(maybeGetStringVec2("(1, 2").has_value());      // missing closing paren
    EXPECT_FALSE(maybeGetStringVec2("1, 2)").has_value());      // missing opening paren
    EXPECT_FALSE(maybeGetStringVec2("(1)").has_value());        // only one component
    EXPECT_FALSE(maybeGetStringVec2("(1, 2, 3)").has_value());  // three components
    EXPECT_FALSE(maybeGetStringVec2("(,)").has_value());        // empty components
    EXPECT_FALSE(maybeGetStringVec2("(1, 2)x").has_value());    // trailing non-whitespace
    EXPECT_FALSE(maybeGetStringVec2("x(1, 2)").has_value());    // leading non-whitespace
}

//------------------------------------------------------------------------------
// maybeGetStringVec3 / maybeGetStringVec4
// (structural failure cases are already covered by the vec2 tests above)
//------------------------------------------------------------------------------

TEST(ParsingToolsTests, maybeGetStringVec3_Success) {
    StringVec3Opt r = maybeGetStringVec3("(1, 2, 3)");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ((*r)[0], "1");
    EXPECT_EQ((*r)[1], " 2");
    EXPECT_EQ((*r)[2], " 3");
}

TEST(ParsingToolsTests, maybeGetStringVec3_Failure) {
    EXPECT_FALSE(maybeGetStringVec3("(1, 2)").has_value());     // too few components
    EXPECT_FALSE(maybeGetStringVec3("(1, 2, 3, 4)").has_value()); // too many components
}

TEST(ParsingToolsTests, maybeGetStringVec4_Success) {
    StringVec4Opt r = maybeGetStringVec4("(  1, 2, 3 ,4 )");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ((*r)[0], "  1");
    EXPECT_EQ((*r)[1], " 2");
    EXPECT_EQ((*r)[2], " 3 ");
    EXPECT_EQ((*r)[3], "4 ");
}

TEST(ParsingToolsTests, maybeGetStringVec4_Failure) {
    EXPECT_FALSE(maybeGetStringVec4("(1, 2, 3)").has_value());       // too few components
    EXPECT_FALSE(maybeGetStringVec4("(1, 2, 3, 4, 5)").has_value()); // too many components
}

//------------------------------------------------------------------------------
// Integer vector parsers
// (maybeGetStringVec* and maybeStoIntegral<int> are both already tested)
//------------------------------------------------------------------------------

TEST(ParsingToolsTests, maybeGetInt2FromString_Success) {
    std::optional<Bifrost::Math::int2> r = maybeGetInt2FromString("(10, -20)");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->x, 10);
    EXPECT_EQ(r->y, -20);
}

TEST(ParsingToolsTests, maybeGetInt2FromString_Failure) {
    EXPECT_FALSE(maybeGetInt2FromString("(abc, 1)").has_value()); // invalid component
    EXPECT_FALSE(maybeGetInt2FromString("(1.5, 2)").has_value()); // float not accepted
}

TEST(ParsingToolsTests, maybeGetInt3FromString_Success) {
    std::optional<Bifrost::Math::int3> r = maybeGetInt3FromString("(1, 2, 3)");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->x, 1);
    EXPECT_EQ(r->y, 2);
    EXPECT_EQ(r->z, 3);
}

TEST(ParsingToolsTests, maybeGetInt3FromString_Failure) {
    EXPECT_FALSE(maybeGetInt3FromString("(abc, 2, 3)").has_value());
}

TEST(ParsingToolsTests, maybeGetInt4FromString_Success) {
    std::optional<Bifrost::Math::int4> r = maybeGetInt4FromString("(1, 2, 3, 4)");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->x, 1);
    EXPECT_EQ(r->y, 2);
    EXPECT_EQ(r->z, 3);
    EXPECT_EQ(r->w, 4);
}

TEST(ParsingToolsTests, maybeGetInt4FromString_Failure) {
    EXPECT_FALSE(maybeGetInt4FromString("(1, 2, 3, abc)").has_value());
}

//------------------------------------------------------------------------------
// Float vector parsers
// (maybeGetStringVec* and maybeStof are both already tested)
//------------------------------------------------------------------------------

TEST(ParsingToolsTests, maybeGetFloat2FromString_Success) {
    std::optional<Bifrost::Math::float2> r = maybeGetFloat2FromString("(1.5, -2.5)");
    ASSERT_TRUE(r.has_value());
    EXPECT_FLOAT_EQ(r->x, 1.5f);
    EXPECT_FLOAT_EQ(r->y, -2.5f);
}

TEST(ParsingToolsTests, maybeGetFloat2FromString_Failure) {
    EXPECT_FALSE(maybeGetFloat2FromString("(inf, 1.0)").has_value()); // inf rejected
    EXPECT_FALSE(maybeGetFloat2FromString("(abc, 1.0)").has_value());
}

TEST(ParsingToolsTests, maybeGetFloat3FromString_Success) {
    std::optional<Bifrost::Math::float3> r = maybeGetFloat3FromString("(1.0, 2.0, 3.0)");
    ASSERT_TRUE(r.has_value());
    EXPECT_FLOAT_EQ(r->x, 1.0f);
    EXPECT_FLOAT_EQ(r->y, 2.0f);
    EXPECT_FLOAT_EQ(r->z, 3.0f);
}

TEST(ParsingToolsTests, maybeGetFloat3FromString_Failure) {
    EXPECT_FALSE(maybeGetFloat3FromString("(1.0, nan, 3.0)").has_value()); // nan rejected
}

TEST(ParsingToolsTests, maybeGetFloat4FromString_Success) {
    std::optional<Bifrost::Math::float4> r = maybeGetFloat4FromString("(1.0, 2.0, 3.0, 4.0)");
    ASSERT_TRUE(r.has_value());
    EXPECT_FLOAT_EQ(r->x, 1.0f);
    EXPECT_FLOAT_EQ(r->y, 2.0f);
    EXPECT_FLOAT_EQ(r->z, 3.0f);
    EXPECT_FLOAT_EQ(r->w, 4.0f);
}

TEST(ParsingToolsTests, maybeGetFloat4FromString_Failure) {
    EXPECT_FALSE(maybeGetFloat4FromString("(1.0, 2.0, 3.0, abc)").has_value());
}

//------------------------------------------------------------------------------
// Double vector parsers
// (maybeGetStringVec* and maybeStod are both already tested)
//------------------------------------------------------------------------------

TEST(ParsingToolsTests, maybeGetDouble2FromString_Success) {
    std::optional<Bifrost::Math::double2> r = maybeGetDouble2FromString("(1.5, -2.5)");
    ASSERT_TRUE(r.has_value());
    EXPECT_DOUBLE_EQ(r->x, 1.5);
    EXPECT_DOUBLE_EQ(r->y, -2.5);
}

TEST(ParsingToolsTests, maybeGetDouble2FromString_Failure) {
    EXPECT_FALSE(maybeGetDouble2FromString("(inf, 1.0)").has_value()); // inf rejected
    EXPECT_FALSE(maybeGetDouble2FromString("(abc, 1.0)").has_value());
}

TEST(ParsingToolsTests, maybeGetDouble3FromString_Success) {
    std::optional<Bifrost::Math::double3> r = maybeGetDouble3FromString("(1.0, 2.0, 3.0)");
    ASSERT_TRUE(r.has_value());
    EXPECT_DOUBLE_EQ(r->x, 1.0);
    EXPECT_DOUBLE_EQ(r->y, 2.0);
    EXPECT_DOUBLE_EQ(r->z, 3.0);
}

TEST(ParsingToolsTests, maybeGetDouble3FromString_Failure) {
    EXPECT_FALSE(maybeGetDouble3FromString("(1.0, nan, 3.0)").has_value()); // nan rejected
}

TEST(ParsingToolsTests, maybeGetDouble4FromString_Success) {
    std::optional<Bifrost::Math::double4> r = maybeGetDouble4FromString("(1.0, 2.0, 3.0, 4.0)");
    ASSERT_TRUE(r.has_value());
    EXPECT_DOUBLE_EQ(r->x, 1.0);
    EXPECT_DOUBLE_EQ(r->y, 2.0);
    EXPECT_DOUBLE_EQ(r->z, 3.0);
    EXPECT_DOUBLE_EQ(r->w, 4.0);
}

TEST(ParsingToolsTests, maybeGetDouble4FromString_Failure) {
    EXPECT_FALSE(maybeGetDouble4FromString("(1.0, 2.0, 3.0, abc)").has_value());
}

/* clang-format on */
