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

// Amino
#include <Amino/Core/BuiltInTypes.h>

#include <gtest/gtest.h>

using namespace BifrostUsd::DynamicPayload;

/* clang-format off */

//------------------------------------------------------------------------------
// maybeStof  (smoke test only - maybeStod is the primary function being used)
//------------------------------------------------------------------------------

TEST(ParsingToolsTests, maybeStof_Success) {
    EXPECT_EQ(maybeStof("0"),        0.0f);
    EXPECT_EQ(maybeStof("3.14"),     3.14f);
    EXPECT_EQ(maybeStof("-3.14"),    -3.14f);
    EXPECT_EQ(maybeStof("1e3"),      1e3f);
    EXPECT_EQ(maybeStof("1e+37"),    1e+37f);   // near max
    EXPECT_EQ(maybeStof("  3.14  "), 3.14f);    // whitespace
}

TEST(ParsingToolsTests, maybeStof_Failure) {
    EXPECT_FALSE(maybeStof("").has_value());        // empty
    EXPECT_FALSE(maybeStof("1e+39").has_value());   // out of range
    EXPECT_FALSE(maybeStof("3.14f").has_value());   // trailing suffix
    EXPECT_FALSE(maybeStof("inf").has_value());     // infinity rejected
    EXPECT_FALSE(maybeStof("nan").has_value());     // NaN rejected
}

//------------------------------------------------------------------------------
// maybeStod
//------------------------------------------------------------------------------

TEST(ParsingToolsTests, maybeStod_Success) {
    EXPECT_EQ(maybeStod("0"),      0.0);
    EXPECT_EQ(maybeStod("42"),     42.0);
    EXPECT_EQ(maybeStod("-1"),     -1.0);
    EXPECT_EQ(maybeStod("3.14"),   3.14);
    EXPECT_EQ(maybeStod("-3.14"),  -3.14);
    EXPECT_EQ(maybeStod("1e3"),    1e3);
    EXPECT_EQ(maybeStod("1.5e-2"), 1.5e-2);
    EXPECT_EQ(maybeStod("1e+307"), 1e+307); // near max

    // Whitespace handling:
    EXPECT_EQ(maybeStod("  3.14"),   3.14); // leading
    EXPECT_EQ(maybeStod("3.14  "),   3.14); // trailing
    EXPECT_EQ(maybeStod("  3.14  "), 3.14); // both
}

TEST(ParsingToolsTests, maybeStod_Failure_OutOfRange) {
    EXPECT_FALSE(maybeStod("1e+309").has_value());   // just over max double
    EXPECT_FALSE(maybeStod("-1e+309").has_value());
}

TEST(ParsingToolsTests, maybeStod_Failure_IllegalInput) {
    EXPECT_FALSE(maybeStod("").has_value());        // empty
    EXPECT_FALSE(maybeStod(" ").has_value());       // whitespace-only
    EXPECT_FALSE(maybeStod("abc").has_value());     // non-numeric
    EXPECT_FALSE(maybeStod("3.14f").has_value());   // trailing suffix
    EXPECT_FALSE(maybeStod("3,14").has_value());    // wrong decimal separator
    EXPECT_FALSE(maybeStod("1 0").has_value());     // embedded space
    EXPECT_FALSE(maybeStod("inf").has_value());     // infinity rejected
    EXPECT_FALSE(maybeStod("-inf").has_value());
    EXPECT_FALSE(maybeStod("nan").has_value());     // NaN rejected
    EXPECT_FALSE(maybeStod("NaN").has_value());
}

//------------------------------------------------------------------------------
// maybeStoIntegral<Amino::char_t>  (signed char, range [-128, 127])
//------------------------------------------------------------------------------

TEST(ParsingToolsTests, maybeStoIntegral_char_Success) {
    EXPECT_EQ(maybeStoIntegral<Amino::char_t>("0"),    Amino::char_t{0});
    EXPECT_EQ(maybeStoIntegral<Amino::char_t>("42"),   Amino::char_t{42});
    EXPECT_EQ(maybeStoIntegral<Amino::char_t>("-43"),  Amino::char_t{-43});
    EXPECT_EQ(maybeStoIntegral<Amino::char_t>("127"),  Amino::char_t{127});   // max
    EXPECT_EQ(maybeStoIntegral<Amino::char_t>("-128"), Amino::char_t{-128});  // min
}

TEST(ParsingToolsTests, maybeStoIntegral_char_Failure) {
    EXPECT_FALSE(maybeStoIntegral<Amino::char_t>("128").has_value());   // just over max
    EXPECT_FALSE(maybeStoIntegral<Amino::char_t>("-129").has_value());  // just under min
}

//------------------------------------------------------------------------------
// maybeStoIntegral<Amino::uchar_t>  (unsigned char, range [0, 255])
//------------------------------------------------------------------------------

TEST(ParsingToolsTests, maybeStoIntegral_uchar_Success) {
    EXPECT_EQ(maybeStoIntegral<Amino::uchar_t>("0"),   Amino::uchar_t{0});
    EXPECT_EQ(maybeStoIntegral<Amino::uchar_t>("200"), Amino::uchar_t{200});
    EXPECT_EQ(maybeStoIntegral<Amino::uchar_t>("255"), Amino::uchar_t{255});  // max
}

TEST(ParsingToolsTests, maybeStoIntegral_uchar_Failure) {
    EXPECT_FALSE(maybeStoIntegral<Amino::uchar_t>("256").has_value());  // just over max
    EXPECT_FALSE(maybeStoIntegral<Amino::uchar_t>("-1").has_value());   // negative rejected
}

//------------------------------------------------------------------------------
// maybeStoIntegral<Amino::short_t>  (signed short, range [-32768, 32767])
//------------------------------------------------------------------------------

TEST(ParsingToolsTests, maybeStoIntegral_short_Success) {
    EXPECT_EQ(maybeStoIntegral<Amino::short_t>("0"),      Amino::short_t{0});
    EXPECT_EQ(maybeStoIntegral<Amino::short_t>("1000"),   Amino::short_t{1000});   // beyond char range
    EXPECT_EQ(maybeStoIntegral<Amino::short_t>("-1001"),  Amino::short_t{-1001});
    EXPECT_EQ(maybeStoIntegral<Amino::short_t>("32767"),  Amino::short_t{32767});  // max
    EXPECT_EQ(maybeStoIntegral<Amino::short_t>("-32768"), Amino::short_t{-32768}); // min
}

TEST(ParsingToolsTests, maybeStoIntegral_short_Failure) {
    EXPECT_FALSE(maybeStoIntegral<Amino::short_t>("32768").has_value());   // just over max
    EXPECT_FALSE(maybeStoIntegral<Amino::short_t>("-32769").has_value());  // just under min
}

//------------------------------------------------------------------------------
// maybeStoIntegral<Amino::ushort_t>  (unsigned short, range [0, 65535])
//------------------------------------------------------------------------------

TEST(ParsingToolsTests, maybeStoIntegral_ushort_Success) {
    EXPECT_EQ(maybeStoIntegral<Amino::ushort_t>("0"),     Amino::ushort_t{0});
    EXPECT_EQ(maybeStoIntegral<Amino::ushort_t>("1000"),  Amino::ushort_t{1000});  // beyond uchar range
    EXPECT_EQ(maybeStoIntegral<Amino::ushort_t>("50000"), Amino::ushort_t{50000});
    EXPECT_EQ(maybeStoIntegral<Amino::ushort_t>("65535"), Amino::ushort_t{65535}); // max
}

TEST(ParsingToolsTests, maybeStoIntegral_ushort_Failure) {
    EXPECT_FALSE(maybeStoIntegral<Amino::ushort_t>("65536").has_value());  // just over max
    EXPECT_FALSE(maybeStoIntegral<Amino::ushort_t>("-1").has_value());     // negative rejected
}

//------------------------------------------------------------------------------
// maybeStoIntegral<Amino::int_t>  (int, range [-2147483648, 2147483647])
// Canonical type for whitespace and general illegal-input tests.
//------------------------------------------------------------------------------

TEST(ParsingToolsTests, maybeStoIntegral_int_Success) {
    EXPECT_EQ(maybeStoIntegral<Amino::int_t>("0"),           Amino::int_t{0});
    EXPECT_EQ(maybeStoIntegral<Amino::int_t>("1000000"),     Amino::int_t{1000000});        // beyond short range
    EXPECT_EQ(maybeStoIntegral<Amino::int_t>("-1000001"),    Amino::int_t{-1000001});
    EXPECT_EQ(maybeStoIntegral<Amino::int_t>("2147483647"),  Amino::int_t{2147483647});     // max
    EXPECT_EQ(maybeStoIntegral<Amino::int_t>("-2147483648"), Amino::int_t{-2147483647 - 1});// min

    // Whitespace handling (tested here once; behavior is identical for all integral types):
    EXPECT_EQ(maybeStoIntegral<Amino::int_t>("  42"),    Amino::int_t{42}); // leading
    EXPECT_EQ(maybeStoIntegral<Amino::int_t>("42  "),    Amino::int_t{42}); // trailing
    EXPECT_EQ(maybeStoIntegral<Amino::int_t>("  42  "),  Amino::int_t{42}); // both
    EXPECT_EQ(maybeStoIntegral<Amino::int_t>("\t42\n"),  Amino::int_t{42}); // tabs/newlines
}

TEST(ParsingToolsTests, maybeStoIntegral_int_Failure_OutOfRange) {
    EXPECT_FALSE(maybeStoIntegral<Amino::int_t>("2147483648").has_value());   // just over max
    EXPECT_FALSE(maybeStoIntegral<Amino::int_t>("-2147483649").has_value());  // just under min
}

TEST(ParsingToolsTests, maybeStoIntegral_int_Failure_IllegalInput) {
    // Illegal input tests are here once; behavior is the same for all integral types.
    EXPECT_FALSE(maybeStoIntegral<Amino::int_t>("").has_value());    // empty
    EXPECT_FALSE(maybeStoIntegral<Amino::int_t>(" ").has_value());   // whitespace-only
    EXPECT_FALSE(maybeStoIntegral<Amino::int_t>("abc").has_value()); // non-numeric
    EXPECT_FALSE(maybeStoIntegral<Amino::int_t>("+1").has_value());  // leading '+' not accepted
    EXPECT_FALSE(maybeStoIntegral<Amino::int_t>("1x").has_value());  // trailing non-whitespace
    EXPECT_FALSE(maybeStoIntegral<Amino::int_t>("1 0").has_value()); // embedded space
    EXPECT_FALSE(maybeStoIntegral<Amino::int_t>("1.0").has_value()); // decimal point
    EXPECT_FALSE(maybeStoIntegral<Amino::int_t>("0x1").has_value()); // hex prefix
}

//------------------------------------------------------------------------------
// maybeStoIntegral<Amino::uint_t>  (unsigned int, range [0, 4294967295])
//------------------------------------------------------------------------------

TEST(ParsingToolsTests, maybeStoIntegral_uint_Success) {
    EXPECT_EQ(maybeStoIntegral<Amino::uint_t>("0"), Amino::uint_t{0});
    EXPECT_EQ(maybeStoIntegral<Amino::uint_t>("1"), Amino::uint_t{1});

    // Check value is not capped to ushort range:
    EXPECT_EQ(maybeStoIntegral<Amino::uint_t>("1000000"),    Amino::uint_t{1000000});
    EXPECT_EQ(maybeStoIntegral<Amino::uint_t>("3000000000"), Amino::uint_t{3000000000});

    EXPECT_EQ(maybeStoIntegral<Amino::uint_t>("  1000000"),  Amino::uint_t{1000000});
    EXPECT_EQ(maybeStoIntegral<Amino::uint_t>("3000000000"), Amino::uint_t{3000000000}); // beyond int range
    EXPECT_EQ(maybeStoIntegral<Amino::uint_t>("4294967295"), Amino::uint_t{4294967295}); // max
}

TEST(ParsingToolsTests, maybeStoIntegral_uint_Failure) {
    EXPECT_FALSE(maybeStoIntegral<Amino::uint_t>("4294967296").has_value());  // just over max
    EXPECT_FALSE(maybeStoIntegral<Amino::uint_t>("-1").has_value());          // negative rejected
}

//------------------------------------------------------------------------------
// maybeStoIntegral<Amino::long_t>  (signed long long, range [-9223372036854775808, 9223372036854775807])
//------------------------------------------------------------------------------

TEST(ParsingToolsTests, maybeStoIntegral_long_Success) {
    EXPECT_EQ(maybeStoIntegral<Amino::long_t>("0"),                    Amino::long_t{0});
    EXPECT_EQ(maybeStoIntegral<Amino::long_t>("5000000000000000000"),  Amino::long_t{5000000000000000000LL});  // beyond int range
    EXPECT_EQ(maybeStoIntegral<Amino::long_t>("-5000000000000000001"), Amino::long_t{-5000000000000000001LL});
    EXPECT_EQ(maybeStoIntegral<Amino::long_t>("9223372036854775807"),  Amino::long_t{9223372036854775807LL});  // max

    // LLONG_MIN written as -9223372036854775807LL - 1LL to avoid the
    // undefined-behaviour trap of writing the literal -9223372036854775808
    // directly (the compiler parses it as unary minus applied to a value that
    // doesn't fit in long long before the negation):
    EXPECT_EQ(maybeStoIntegral<Amino::long_t>("-9223372036854775808"), Amino::long_t{-9223372036854775807LL - 1LL}); // min
}

TEST(ParsingToolsTests, maybeStoIntegral_long_Failure) {
    EXPECT_FALSE(maybeStoIntegral<Amino::long_t>("9223372036854775808").has_value());   // just over max
    EXPECT_FALSE(maybeStoIntegral<Amino::long_t>("-9223372036854775809").has_value());  // just under min
}

//------------------------------------------------------------------------------
// maybeStoIntegral<Amino::ulong_t>  (unsigned long long, range [0, 18446744073709551615])
//------------------------------------------------------------------------------

TEST(ParsingToolsTests, maybeStoIntegral_ulong_Success) {
    EXPECT_EQ(maybeStoIntegral<Amino::ulong_t>("0"),                    Amino::ulong_t{0});
    EXPECT_EQ(maybeStoIntegral<Amino::ulong_t>("10000000000000000000"), Amino::ulong_t{10000000000000000000ULL}); // beyond long range
    EXPECT_EQ(maybeStoIntegral<Amino::ulong_t>("18446744073709551615"), Amino::ulong_t{18446744073709551615ULL}); // max
}

TEST(ParsingToolsTests, maybeStoIntegral_ulong_Failure) {
    EXPECT_FALSE(maybeStoIntegral<Amino::ulong_t>("18446744073709551616").has_value()); // just over max
    EXPECT_FALSE(maybeStoIntegral<Amino::ulong_t>("-1").has_value());                   // negative rejected
}

/* clang-format on */
