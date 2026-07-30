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
#include "dffTypeNames.h"
#include "stringToAny.h"

// Amino/Bifrost Files:
#include <Amino/Core/BuiltInTypes.h>
#include <Amino/Core/String.h>
#include <Bifrost/Math/Types.h>

#include <gtest/gtest.h>
#include <string>
#include <string_view>

using namespace BifrostUsd::DynamicPayload;

//------------------------------------------------------------------------------
// CoverageTest - every type name in DffTypeNames::kAllTypeNames must be
// recognized.
//
// This test iterates the authoritative list in dffTypeNames.h and verifies
// that stringToAny() never returns kFailure_UnsupportedType for any of them.
// It acts as a compile-time + run-time guard: if a new type is added to
// kAllTypeNames but the corresponding branch is omitted from stringToAny(),
// this test fails with a clear diagnostic.
//------------------------------------------------------------------------------

TEST(StringToAnyTests, TypeNamesCoverageTest) {
    for (std::string_view typeName : DffTypeNames::kAllTypeNames) {
        // The string value "0" is deliberately invalid for many types - the
        // intent is only to verify the type NAME is recognized, i.e. the status
        // is not kFailure_UnsupportedType. Whether the conversion itself
        // succeeds or fails with kFailure_InvalidInput is irrelevant here.
        auto [value, status] = stringToAny(std::string{typeName}, "0");
        EXPECT_NE(status, ConversionStatus::kFailure_UnsupportedType)
            << "stringToAny() does not recognise type name: " << typeName;
    }
}

//------------------------------------------------------------------------------
// ConversionStatus - unsupported type
//------------------------------------------------------------------------------

TEST(StringToAnyTests, UnknownTypeName) {
    auto [value, status] = stringToAny("MyUnknownType", "42");
    EXPECT_EQ(status, ConversionStatus::kFailure_UnsupportedType);
    EXPECT_FALSE(value.has_value());
}

TEST(StringToAnyTests, EmptyTypeName) {
    auto [value, status] = stringToAny("", "42");
    EXPECT_EQ(status, ConversionStatus::kFailure_UnsupportedType);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// bool
// Accepted tokens (case-insensitive, leading/trailing whitespace stripped):
//   true  : "1", "true", "yes"
//   false : "0", "false", "no"
// Any other string returns kFailure_InvalidInput.
//------------------------------------------------------------------------------

TEST(StringToAnyTests, bool_true_Literal) {
    auto [value, status] = stringToAny("bool", "true");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_TRUE(Amino::any_cast<bool>(value));
}

TEST(StringToAnyTests, bool_true_One) {
    auto [value, status] = stringToAny("bool", "1");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_TRUE(Amino::any_cast<bool>(value));
}

TEST(StringToAnyTests, bool_true_Yes) {
    auto [value, status] = stringToAny("bool", "yes");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_TRUE(Amino::any_cast<bool>(value));
}

TEST(StringToAnyTests, bool_false_Literal) {
    auto [value, status] = stringToAny("bool", "false");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_FALSE(Amino::any_cast<bool>(value));
}

TEST(StringToAnyTests, bool_false_Zero) {
    auto [value, status] = stringToAny("bool", "0");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_FALSE(Amino::any_cast<bool>(value));
}

TEST(StringToAnyTests, bool_false_No) {
    auto [value, status] = stringToAny("bool", "no");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_FALSE(Amino::any_cast<bool>(value));
}

TEST(StringToAnyTests, bool_Failure_InvalidInput) {
    auto [value, status] = stringToAny("bool", "maybe");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// char  (signed, range [-128, 127])
//------------------------------------------------------------------------------

TEST(StringToAnyTests, char_Success) {
    auto [value, status] = stringToAny("char", "42");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(Amino::any_cast<Amino::char_t>(value), Amino::char_t{42});
}

TEST(StringToAnyTests, char_Success_Negative) {
    auto [value, status] = stringToAny("char", "-128");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(Amino::any_cast<Amino::char_t>(value), Amino::char_t{-128});
}

TEST(StringToAnyTests, char_Failure_InvalidInput) {
    auto [value, status] = stringToAny("char", "abc");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

TEST(StringToAnyTests, char_Failure_OutOfRange) {
    auto [value, status] = stringToAny("char", "128");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// uchar  (unsigned char, range [0, 255])
//------------------------------------------------------------------------------

TEST(StringToAnyTests, uchar_Success) {
    auto [value, status] = stringToAny("uchar", "200");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(Amino::any_cast<Amino::uchar_t>(value), Amino::uchar_t{200});
}

TEST(StringToAnyTests, uchar_Failure_Negative) {
    auto [value, status] = stringToAny("uchar", "-1");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// short  (signed, range [-32768, 32767])
//------------------------------------------------------------------------------

TEST(StringToAnyTests, short_Success) {
    auto [value, status] = stringToAny("short", "1000");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(Amino::any_cast<Amino::short_t>(value), Amino::short_t{1000});
}

TEST(StringToAnyTests, short_Failure_OutOfRange) {
    auto [value, status] = stringToAny("short", "32768");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// ushort  (unsigned short, range [0, 65535])
//------------------------------------------------------------------------------

TEST(StringToAnyTests, ushort_Success) {
    auto [value, status] = stringToAny("ushort", "50000");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(Amino::any_cast<Amino::ushort_t>(value), Amino::ushort_t{50000});
}

TEST(StringToAnyTests, ushort_Failure_OutOfRange) {
    auto [value, status] = stringToAny("ushort", "65536");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// int  (range [-2147483648, 2147483647])
//------------------------------------------------------------------------------

TEST(StringToAnyTests, int_Success) {
    auto [value, status] = stringToAny("int", "42");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(Amino::any_cast<Amino::int_t>(value), Amino::int_t{42});
}

TEST(StringToAnyTests, int_Success_Negative) {
    auto [value, status] = stringToAny("int", "-2147483648");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(Amino::any_cast<Amino::int_t>(value), Amino::int_t{-2147483647 - 1});
}

TEST(StringToAnyTests, int_Failure_InvalidInput) {
    auto [value, status] = stringToAny("int", "not_a_number");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

TEST(StringToAnyTests, int_Failure_EmptyString) {
    auto [value, status] = stringToAny("int", "");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

TEST(StringToAnyTests, int_Failure_OutOfRange) {
    auto [value, status] = stringToAny("int", "2147483648");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// uint  (range [0, 4294967295])
//------------------------------------------------------------------------------

TEST(StringToAnyTests, uint_Success) {
    auto [value, status] = stringToAny("uint", "3000000000");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(Amino::any_cast<Amino::uint_t>(value), Amino::uint_t{3000000000});
}

TEST(StringToAnyTests, uint_Failure_Negative) {
    auto [value, status] = stringToAny("uint", "-1");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// long  (range [-9223372036854775808, 9223372036854775807])
//------------------------------------------------------------------------------

TEST(StringToAnyTests, long_Success) {
    auto [value, status] = stringToAny("long", "9223372036854775807");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(Amino::any_cast<Amino::long_t>(value), Amino::long_t{9223372036854775807LL});
}

TEST(StringToAnyTests, long_Failure_OutOfRange) {
    auto [value, status] = stringToAny("long", "9223372036854775808");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// ulong  (range [0, 18446744073709551615])
//------------------------------------------------------------------------------

TEST(StringToAnyTests, ulong_Success) {
    auto [value, status] = stringToAny("ulong", "18446744073709551615");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(Amino::any_cast<Amino::ulong_t>(value), Amino::ulong_t{18446744073709551615ULL});
}

TEST(StringToAnyTests, ulong_Failure_Negative) {
    auto [value, status] = stringToAny("ulong", "-1");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// float
//------------------------------------------------------------------------------

TEST(StringToAnyTests, float_Success) {
    auto [value, status] = stringToAny("float", "3.14");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_FLOAT_EQ(Amino::any_cast<Amino::float_t>(value), 3.14f);
}

TEST(StringToAnyTests, float_Success_Negative) {
    auto [value, status] = stringToAny("float", "-1.5");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_FLOAT_EQ(Amino::any_cast<Amino::float_t>(value), -1.5f);
}

TEST(StringToAnyTests, float_Failure_InvalidInput) {
    auto [value, status] = stringToAny("float", "abc");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

TEST(StringToAnyTests, float_Failure_NaN) {
    auto [value, status] = stringToAny("float", "nan");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// double
//------------------------------------------------------------------------------

TEST(StringToAnyTests, double_Success) {
    auto [value, status] = stringToAny("double", "2.718281828");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_DOUBLE_EQ(Amino::any_cast<Amino::double_t>(value), 2.718281828);
}

TEST(StringToAnyTests, double_Failure_InvalidInput) {
    auto [value, status] = stringToAny("double", "inf");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// string
//------------------------------------------------------------------------------

TEST(StringToAnyTests, string_Success) {
    auto [value, status] = stringToAny("string", "hello world");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(Amino::any_cast<Amino::String>(value), "hello world");
}

TEST(StringToAnyTests, string_Success_Empty) {
    auto [value, status] = stringToAny("string", "");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(Amino::any_cast<Amino::String>(value), "");
}

//------------------------------------------------------------------------------
// Math::float2
//------------------------------------------------------------------------------

TEST(StringToAnyTests, Math_float2_Success) {
    auto [value, status] = stringToAny("Math::float2", "(1.0, 2.5)");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    const auto v = Amino::any_cast<Bifrost::Math::float2>(value);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.5f);
}

TEST(StringToAnyTests, Math_float2_Failure_InvalidInput) {
    auto [value, status] = stringToAny("Math::float2", "(1.0)");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// Math::float3
//------------------------------------------------------------------------------

TEST(StringToAnyTests, Math_float3_Success) {
    auto [value, status] = stringToAny("Math::float3", "(1.0, 2.0, 3.0)");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    const auto v = Amino::any_cast<Bifrost::Math::float3>(value);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
    EXPECT_FLOAT_EQ(v.z, 3.0f);
}

TEST(StringToAnyTests, Math_float3_Failure_InvalidInput) {
    auto [value, status] = stringToAny("Math::float3", "1.0, 2.0, 3.0");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// Math::float4
//------------------------------------------------------------------------------

TEST(StringToAnyTests, Math_float4_Success) {
    auto [value, status] = stringToAny("Math::float4", "(1.0, 2.0, 3.0, 4.0)");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    const auto v = Amino::any_cast<Bifrost::Math::float4>(value);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
    EXPECT_FLOAT_EQ(v.z, 3.0f);
    EXPECT_FLOAT_EQ(v.w, 4.0f);
}

TEST(StringToAnyTests, Math_float4_Failure_InvalidInput) {
    auto [value, status] = stringToAny("Math::float4", "(1.0, 2.0, 3.0)");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// Math::double2
//------------------------------------------------------------------------------

TEST(StringToAnyTests, Math_double2_Success) {
    auto [value, status] = stringToAny("Math::double2", "(1.1, 2.2)");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    const auto v = Amino::any_cast<Bifrost::Math::double2>(value);
    EXPECT_DOUBLE_EQ(v.x, 1.1);
    EXPECT_DOUBLE_EQ(v.y, 2.2);
}

TEST(StringToAnyTests, Math_double2_Failure_InvalidInput) {
    auto [value, status] = stringToAny("Math::double2", "(abc, 2.2)");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// Math::double3
//------------------------------------------------------------------------------

TEST(StringToAnyTests, Math_double3_Success) {
    auto [value, status] = stringToAny("Math::double3", "(1.0, 2.0, 3.0)");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    const auto v = Amino::any_cast<Bifrost::Math::double3>(value);
    EXPECT_DOUBLE_EQ(v.x, 1.0);
    EXPECT_DOUBLE_EQ(v.y, 2.0);
    EXPECT_DOUBLE_EQ(v.z, 3.0);
}

//------------------------------------------------------------------------------
// Math::double4
//------------------------------------------------------------------------------

TEST(StringToAnyTests, Math_double4_Success) {
    auto [value, status] = stringToAny("Math::double4", "(1.0, 2.0, 3.0, 4.0)");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    const auto v = Amino::any_cast<Bifrost::Math::double4>(value);
    EXPECT_DOUBLE_EQ(v.x, 1.0);
    EXPECT_DOUBLE_EQ(v.y, 2.0);
    EXPECT_DOUBLE_EQ(v.z, 3.0);
    EXPECT_DOUBLE_EQ(v.w, 4.0);
}

//------------------------------------------------------------------------------
// Math::int2
//------------------------------------------------------------------------------

TEST(StringToAnyTests, Math_int2_Success) {
    auto [value, status] = stringToAny("Math::int2", "(10, 20)");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    const auto v = Amino::any_cast<Bifrost::Math::int2>(value);
    EXPECT_EQ(v.x, 10);
    EXPECT_EQ(v.y, 20);
}

TEST(StringToAnyTests, Math_int2_Failure_InvalidInput) {
    auto [value, status] = stringToAny("Math::int2", "(10, 3.14)");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}

//------------------------------------------------------------------------------
// Math::int3
//------------------------------------------------------------------------------

TEST(StringToAnyTests, Math_int3_Success) {
    auto [value, status] = stringToAny("Math::int3", "(1, 2, 3)");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    const auto v = Amino::any_cast<Bifrost::Math::int3>(value);
    EXPECT_EQ(v.x, 1);
    EXPECT_EQ(v.y, 2);
    EXPECT_EQ(v.z, 3);
}

//------------------------------------------------------------------------------
// Math::int4
//------------------------------------------------------------------------------

TEST(StringToAnyTests, Math_int4_Success) {
    auto [value, status] = stringToAny("Math::int4", "(1, 2, 3, 4)");
    EXPECT_EQ(status, ConversionStatus::kSuccess);
    ASSERT_TRUE(value.has_value());
    const auto v = Amino::any_cast<Bifrost::Math::int4>(value);
    EXPECT_EQ(v.x, 1);
    EXPECT_EQ(v.y, 2);
    EXPECT_EQ(v.z, 3);
    EXPECT_EQ(v.w, 4);
}

TEST(StringToAnyTests, Math_int4_Failure_InvalidInput) {
    auto [value, status] = stringToAny("Math::int4", "(1, 2, 3)");
    EXPECT_EQ(status, ConversionStatus::kFailure_InvalidInput);
    EXPECT_FALSE(value.has_value());
}
