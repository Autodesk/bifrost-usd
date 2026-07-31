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
// maybeStob
//------------------------------------------------------------------------------

TEST(ParsingToolsTests, maybeStob_True) {
    // Exact matches:
    EXPECT_EQ(maybeStob("1"),     true);
    EXPECT_EQ(maybeStob("true"),  true);
    EXPECT_EQ(maybeStob("yes"),   true);

    // Case-insensitive:
    EXPECT_EQ(maybeStob("True"),  true);
    EXPECT_EQ(maybeStob("TRUE"),  true);
    EXPECT_EQ(maybeStob("tRuE"),  true);
    EXPECT_EQ(maybeStob("Yes"),   true);
    EXPECT_EQ(maybeStob("YES"),   true);

    // Whitespace trimming:
    EXPECT_EQ(maybeStob("  1"),         true);   // leading
    EXPECT_EQ(maybeStob("1  "),         true);   // trailing
    EXPECT_EQ(maybeStob("  1  "),       true);   // both
    EXPECT_EQ(maybeStob(" \t true  "),  true);
    EXPECT_EQ(maybeStob("  TRUE \t "),  true);
    EXPECT_EQ(maybeStob("  yes  "),     true);
    EXPECT_EQ(maybeStob("\t1\n"),       true);   // other whitespace characters
}

TEST(ParsingToolsTests, maybeStob_False) {
    // Exact matches:
    EXPECT_EQ(maybeStob("0"),      false);
    EXPECT_EQ(maybeStob("false"),  false);
    EXPECT_EQ(maybeStob("no"),     false);

    // Case-insensitive:
    EXPECT_EQ(maybeStob("False"),  false);
    EXPECT_EQ(maybeStob("FALSE"),  false);
    EXPECT_EQ(maybeStob("fAlSe"),  false);
    EXPECT_EQ(maybeStob("No"),     false);
    EXPECT_EQ(maybeStob("NO"),     false);

    // Whitespace trimming:
    EXPECT_EQ(maybeStob("  0"),            false); // leading
    EXPECT_EQ(maybeStob("0  "),            false); // trailing
    EXPECT_EQ(maybeStob("  0  "),          false); // both
    EXPECT_EQ(maybeStob("  false \n\t "),  false);
    EXPECT_EQ(maybeStob("\t  FALSE  "),    false);
    EXPECT_EQ(maybeStob("  no  "),         false);
    EXPECT_EQ(maybeStob("\t0\n"),          false); // other whitespace characters
}

TEST(ParsingToolsTests, maybeStob_Failure_Empty) {
    EXPECT_FALSE(maybeStob("").has_value());      // empty string
    EXPECT_FALSE(maybeStob(" ").has_value());     // whitespace-only
    EXPECT_FALSE(maybeStob("\t\n").has_value());  // whitespace-only (other chars)
}

TEST(ParsingToolsTests, maybeStob_Failure_IllegalInput) {
    // Wrong integer values:
    EXPECT_FALSE(maybeStob("2").has_value());
    EXPECT_FALSE(maybeStob("-1").has_value());
    EXPECT_FALSE(maybeStob("10").has_value());

    // Floating-point representations:
    EXPECT_FALSE(maybeStob("1.0").has_value());
    EXPECT_FALSE(maybeStob("0.0").has_value());

    // Alternate boolean-like words not supported:
    EXPECT_FALSE(maybeStob("on").has_value());
    EXPECT_FALSE(maybeStob("off").has_value());
    EXPECT_FALSE(maybeStob("y").has_value());
    EXPECT_FALSE(maybeStob("n").has_value());
    EXPECT_FALSE(maybeStob("t").has_value());
    EXPECT_FALSE(maybeStob("f").has_value());

    // Embedded whitespace (not mere leading/trailing):
    EXPECT_FALSE(maybeStob("t rue").has_value());
    EXPECT_FALSE(maybeStob("fals e").has_value());
    EXPECT_FALSE(maybeStob("1 0").has_value());

    // Trailing garbage after a valid token:
    EXPECT_FALSE(maybeStob("true!").has_value());
    EXPECT_FALSE(maybeStob("0x0").has_value());
    EXPECT_FALSE(maybeStob("1a").has_value());
}
