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
#include <Amino/Core/String.h>
#include <gtest/gtest.h>
#include <nodedefs/usd_pack/usd_attribute_nodedefs.h>
#include <nodedefs/usd_pack/usd_geom_nodedefs.h>
#include <nodedefs/usd_pack/usd_prim_nodedefs.h>
#include <nodedefs/usd_pack/usd_stage_nodedefs.h>
#include <utils/test/testUtils.h>

BIFUSD_WARNING_PUSH
BIFUSD_WARNING_DISABLE_MSC(4003)
#include <pxr/usd/usd/references.h>
BIFUSD_WARNING_POP

#include <cstdlib>
#include <string>
#include <type_traits>

using namespace BifrostUsd::TestUtils;

TEST(AttributeNodeDefs, create_prim_attribute) {
    BifrostUsd::Stage stage;
    auto                primPath = pxr::SdfPath("/a");
    stage->DefinePrim(primPath);

    const auto name       = Amino::String{"my_float"};
    const auto type_name  = BifrostUsd::SdfValueTypeName::Float;
    const bool custom     = true;
    const auto variablity = BifrostUsd::SdfVariability::Varying;

    auto success = USD::Attribute::create_prim_attribute(
        stage, primPath.GetText(), name, type_name, custom, variablity);
    ASSERT_TRUE(success);

    auto prim = stage->GetPrimAtPath(primPath);
    auto attr = prim.GetAttribute(pxr::TfToken("my_float"));
    ASSERT_TRUE(attr);
    ASSERT_EQ(attr.GetTypeName(), pxr::SdfValueTypeNames->Float);
}

TEST(AttributeNodeDefs, clear_attribute) {
    BifrostUsd::Stage stage;
    auto                primPath = pxr::SdfPath("/a");
    auto                prim     = stage->DefinePrim(primPath);
    auto                attr     = prim.CreateAttribute(pxr::TfToken("foo"),
                                     pxr::SdfValueTypeNames->Token);
    attr.Set(pxr::TfToken("bar"));
    pxr::TfToken result;
    attr.Get(&result);
    ASSERT_EQ(result, pxr::TfToken("bar"));

    const auto name = Amino::String{"foo"};
    auto       success =
        USD::Attribute::clear_attribute(stage, primPath.GetText(), name);
    ASSERT_TRUE(success);

    prim = stage->GetPrimAtPath(primPath);
    attr = prim.GetAttribute(pxr::TfToken("foo"));
    pxr::TfToken empty;
    ASSERT_FALSE(attr.Get(&empty));
    ASSERT_TRUE(empty.IsEmpty());
}

TEST(AttributeNodeDefs, block_attribute) {
    BifrostUsd::Stage stage;
    auto                primPath = pxr::SdfPath("/Sphere");
    auto                prim     = stage->DefinePrim(primPath);

    const auto defAttrTk = pxr::TfToken("size");
    auto       defAttr =
        prim.CreateAttribute(defAttrTk, pxr::SdfValueTypeNames->Double);
    defAttr.Set<double>(1.0);

    auto localRefPrimPath = pxr::SdfPath("/SphereOver");
    auto localRefPrim     = stage->OverridePrim(localRefPrimPath);
    localRefPrim.GetReferences().AddInternalReference(primPath);
    auto localRefAttr =
        localRefPrim.CreateAttribute(defAttrTk, pxr::SdfValueTypeNames->Double);

    double localRefAttrValue;
    localRefAttr.Get(&localRefAttrValue);
    ASSERT_EQ(localRefAttrValue, 1.0);

    USD::Attribute::block_attribute(stage, localRefPrimPath.GetText(),
                                    defAttrTk.GetText());

    auto prim2         = stage->GetPrimAtPath(localRefPrimPath);
    auto localRefAttr2 = prim2.GetAttribute(defAttrTk);

    double localRefAttrValue2 = -1;
    // Since the attribute is blocked, it does not have any value and so the Get
    // call does nothing
    ASSERT_FALSE(localRefAttr2.Get(&localRefAttrValue2));
    ASSERT_EQ(localRefAttrValue2, -1);
}

TEST(AttributeNodeDefs, create_primvar) {
    BifrostUsd::Stage stage;
    auto                primPath = pxr::SdfPath("/Sphere");
    auto                prim     = stage->DefinePrim(primPath);

    auto success = USD::Attribute::create_primvar(
        stage, primPath.GetText(), "density", BifrostUsd::SdfValueTypeName::Float,
        BifrostUsd::UsdGeomPrimvarInterpolation::PrimVarVarying, -1);
    ASSERT_TRUE(success);

    prim      = stage->GetPrimAtPath(primPath);
    auto attr = prim.GetAttribute(pxr::TfToken("primvars:density"));
    ASSERT_TRUE(attr);
}

TEST(AttributeNodeDefs, get_prim_attribute) {
    auto stage    = Amino::newMutablePtr<BifrostUsd::Stage>();
    auto primPath = pxr::SdfPath("/a");
    auto prim     = stage->get().DefinePrim(primPath);

    auto attr = prim.CreateAttribute(pxr::TfToken("foo"),
                                     pxr::SdfValueTypeNames->Token);
    attr.Set(pxr::TfToken("bar"));

    auto primInterface =
        Amino::newClassPtr<BifrostUsd::Prim>(prim, std::move(stage));
    Amino::MutablePtr<BifrostUsd::Attribute> attribute;
    auto                                       success =
        USD::Attribute::get_prim_attribute(primInterface, "foo", attribute);
    ASSERT_TRUE(success);
    ASSERT_TRUE(attribute);

    prim = primInterface->getPxrPrim();
    attr = prim.GetAttribute(pxr::TfToken("foo"));
    pxr::TfToken result;
    ASSERT_TRUE(attr.Get(&result));
    ASSERT_EQ(result, pxr::TfToken("bar"));
}

TEST(AttributeNodeDefs, set_and_get_prim_attribute) {
    auto stage_mut = Amino::newMutablePtr<BifrostUsd::Stage>();
    auto primPath  = pxr::SdfPath("/a");
    auto prim      = stage_mut->get().DefinePrim(primPath);

    // create supported USD attributes
    prim.CreateAttribute(pxr::TfToken("my_asset"),
                         pxr::SdfValueTypeNames->Asset);
    prim.CreateAttribute(pxr::TfToken("my_bool"), pxr::SdfValueTypeNames->Bool);
    prim.CreateAttribute(pxr::TfToken("my_color3f"),
                         pxr::SdfValueTypeNames->Color3f);
    prim.CreateAttribute(pxr::TfToken("my_double"),
                         pxr::SdfValueTypeNames->Double);
    prim.CreateAttribute(pxr::TfToken("my_double2"),
                         pxr::SdfValueTypeNames->Double2);
    prim.CreateAttribute(pxr::TfToken("my_double3"),
                         pxr::SdfValueTypeNames->Double3);
    prim.CreateAttribute(pxr::TfToken("my_double4"),
                         pxr::SdfValueTypeNames->Double4);
    prim.CreateAttribute(pxr::TfToken("my_float"),
                         pxr::SdfValueTypeNames->Float);
    prim.CreateAttribute(pxr::TfToken("my_float2"),
                         pxr::SdfValueTypeNames->Float2);
    prim.CreateAttribute(pxr::TfToken("my_float3"),
                         pxr::SdfValueTypeNames->Float3);
    prim.CreateAttribute(pxr::TfToken("my_float4"),
                         pxr::SdfValueTypeNames->Float4);
    prim.CreateAttribute(pxr::TfToken("my_int"), pxr::SdfValueTypeNames->Int);
    prim.CreateAttribute(pxr::TfToken("my_int64"),
                         pxr::SdfValueTypeNames->Int64);
    prim.CreateAttribute(pxr::TfToken("my_normal3f"),
                         pxr::SdfValueTypeNames->Normal3f);
    prim.CreateAttribute(pxr::TfToken("my_quatd"),
                         pxr::SdfValueTypeNames->Quatd);
    prim.CreateAttribute(pxr::TfToken("my_quatf"),
                         pxr::SdfValueTypeNames->Quatf);
    prim.CreateAttribute(pxr::TfToken("my_quath"),
                         pxr::SdfValueTypeNames->Quath);
    prim.CreateAttribute(pxr::TfToken("my_string"),
                         pxr::SdfValueTypeNames->String);
    prim.CreateAttribute(pxr::TfToken("my_texcoord2f"),
                         pxr::SdfValueTypeNames->TexCoord2f);
    prim.CreateAttribute(pxr::TfToken("my_token"),
                         pxr::SdfValueTypeNames->Token);
    prim.CreateAttribute(pxr::TfToken("my_uchar"),
                         pxr::SdfValueTypeNames->UChar);
    prim.CreateAttribute(pxr::TfToken("my_uint"), pxr::SdfValueTypeNames->UInt);
    prim.CreateAttribute(pxr::TfToken("my_uint64"),
                         pxr::SdfValueTypeNames->UInt64);
    prim.CreateAttribute(pxr::TfToken("my_double4x4"),
                         pxr::SdfValueTypeNames->Matrix4d);

    // create supported USD attributes arrays
    prim.CreateAttribute(pxr::TfToken("my_assetArray"),
                         pxr::SdfValueTypeNames->AssetArray);
    prim.CreateAttribute(pxr::TfToken("my_boolArray"),
                         pxr::SdfValueTypeNames->BoolArray);
    prim.CreateAttribute(pxr::TfToken("my_color3fArray"),
                         pxr::SdfValueTypeNames->Color3fArray);
    prim.CreateAttribute(pxr::TfToken("my_doubleArray"),
                         pxr::SdfValueTypeNames->DoubleArray);
    prim.CreateAttribute(pxr::TfToken("my_double2Array"),
                         pxr::SdfValueTypeNames->Double2Array);
    prim.CreateAttribute(pxr::TfToken("my_double3Array"),
                         pxr::SdfValueTypeNames->Double3Array);
    prim.CreateAttribute(pxr::TfToken("my_double4Array"),
                         pxr::SdfValueTypeNames->Double4Array);
    prim.CreateAttribute(pxr::TfToken("my_floatArray"),
                         pxr::SdfValueTypeNames->FloatArray);
    prim.CreateAttribute(pxr::TfToken("my_float2Array"),
                         pxr::SdfValueTypeNames->Float2Array);
    prim.CreateAttribute(pxr::TfToken("my_float3Array"),
                         pxr::SdfValueTypeNames->Float3Array);
    prim.CreateAttribute(pxr::TfToken("my_float4Array"),
                         pxr::SdfValueTypeNames->Float4Array);
    prim.CreateAttribute(pxr::TfToken("my_intArray"),
                         pxr::SdfValueTypeNames->IntArray);
    prim.CreateAttribute(pxr::TfToken("my_int64Array"),
                         pxr::SdfValueTypeNames->Int64Array);
    prim.CreateAttribute(pxr::TfToken("my_normal3fArray"),
                         pxr::SdfValueTypeNames->Normal3fArray);
    prim.CreateAttribute(pxr::TfToken("my_quatdArray"),
                         pxr::SdfValueTypeNames->QuatdArray);
    prim.CreateAttribute(pxr::TfToken("my_quatfArray"),
                         pxr::SdfValueTypeNames->QuatfArray);
    prim.CreateAttribute(pxr::TfToken("my_quathArray"),
                         pxr::SdfValueTypeNames->QuathArray);
    prim.CreateAttribute(pxr::TfToken("my_stringArray"),
                         pxr::SdfValueTypeNames->StringArray);
    prim.CreateAttribute(pxr::TfToken("my_texcoord2fArray"),
                         pxr::SdfValueTypeNames->TexCoord2fArray);
    prim.CreateAttribute(pxr::TfToken("my_tokenArray"),
                         pxr::SdfValueTypeNames->TokenArray);
    prim.CreateAttribute(pxr::TfToken("my_ucharArray"),
                         pxr::SdfValueTypeNames->UCharArray);
    prim.CreateAttribute(pxr::TfToken("my_uintArray"),
                         pxr::SdfValueTypeNames->UIntArray);
    prim.CreateAttribute(pxr::TfToken("my_uint64Array"),
                         pxr::SdfValueTypeNames->UInt64Array);
    prim.CreateAttribute(pxr::TfToken("my_double4x4Array"),
                         pxr::SdfValueTypeNames->Matrix4dArray);

    auto useFrame = false;
    auto frame    = 1.0f;

    auto test_set_attribute = [&](const Amino::String& attribName,
                                  auto&                attribValue) {
        auto success = USD::Attribute::set_prim_attribute(
            *stage_mut, primPath.GetText(), attribName, attribValue, useFrame,
            frame);
        ASSERT_TRUE(success) << attribName.c_str();
    };

    // Single values
    const auto assetValue = Amino::String{"my_asset_value"};
    test_set_attribute("my_asset", assetValue);

    const auto boolValue = Amino::bool_t{true};
    test_set_attribute("my_bool", boolValue);

    const auto color3fValue = Bifrost::Math::float3{1.f, 2.f, 3.f};
    test_set_attribute("my_color3f", color3fValue);

    const auto doubleValue = Amino::double_t{1.0};
    test_set_attribute("my_double", doubleValue);

    const auto double2Value = Bifrost::Math::double2{1.0, 2.0};
    test_set_attribute("my_double2", double2Value);

    const auto double3Value = Bifrost::Math::double3{1.0, 2.0, 3.0};
    test_set_attribute("my_double3", double3Value);

    const auto double4Value = Bifrost::Math::double4{1.0, 2.0, 3.0, 4.0};
    test_set_attribute("my_double4", double4Value);

    const auto floatValue = Amino::float_t{1.f};
    test_set_attribute("my_float", floatValue);

    const auto float2Value = Bifrost::Math::float2{1.f, 2.f};
    test_set_attribute("my_float2", float2Value);

    const auto float3Value = Bifrost::Math::float3{1.f, 2.f, 3.f};
    test_set_attribute("my_float3", float3Value);

    const auto float4Value = Bifrost::Math::float4{1.f, 2.f, 3.f, 4.f};
    test_set_attribute("my_float4", float4Value);

    const auto intValue = Amino::int_t{-1};
    test_set_attribute("my_int", intValue);

    const auto int64Value = Amino::long_t{1};
    test_set_attribute("my_int64", int64Value);

    test_set_attribute("my_normal3f", float3Value);

    test_set_attribute("my_quatd", double4Value);

    test_set_attribute("my_quatf", float4Value);

    test_set_attribute("my_quath", float4Value);

    const auto stringValue = Amino::String{"my_string_value"};
    test_set_attribute("my_string", stringValue);

    test_set_attribute("my_texcoord2f", float2Value);

    const auto tokenValue = Amino::String{"my_token_value"};
    test_set_attribute("my_token", tokenValue);

    const auto ucharValue = Amino::uchar_t{255};
    test_set_attribute("my_uchar", ucharValue);

    const auto uintValue = Amino::uint_t{1};
    test_set_attribute("my_uint", uintValue);

    const auto uint64Value = Amino::ulong_t{1};
    test_set_attribute("my_uint64", uint64Value);

    const auto double4x4Value = Bifrost::Math::double4x4{
        {0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15}};
    test_set_attribute("my_double4x4", double4x4Value);

    // Arrays
    const auto stringArrayValue =
        Amino::Array<Amino::String>{"one", "two", "three"};
    test_set_attribute("my_assetArray", stringArrayValue);

    const auto boolArrayValue = Amino::Array<Amino::bool_t>{false, true};
    test_set_attribute("my_boolArray", boolArrayValue);

    auto pt1                    = Bifrost::Math::float3();
    pt1.x                       = 1.f;
    pt1.y                       = 2.f;
    pt1.z                       = 2.f;
    const auto float3ArrayValue = Amino::Array<Bifrost::Math::float3>{pt1};
    test_set_attribute("my_color3fArray", float3ArrayValue);

    const auto doubleArrayValue = Amino::Array<Amino::double_t>{.1, .2};
    test_set_attribute("my_doubleArray", doubleArrayValue);

    auto db2                     = Bifrost::Math::double2();
    db2.x                        = 1;
    db2.y                        = 2;
    const auto double2ArrayValue = Amino::Array<Bifrost::Math::double2>{db2};
    test_set_attribute("my_double2Array", double2ArrayValue);

    auto db3                     = Bifrost::Math::double3();
    db3.x                        = 1;
    db3.y                        = 2;
    const auto double3ArrayValue = Amino::Array<Bifrost::Math::double3>{db3};
    test_set_attribute("my_double3Array", double3ArrayValue);

    auto db4                     = Bifrost::Math::double4();
    db4.x                        = 1;
    db4.y                        = 2;
    const auto double4ArrayValue = Amino::Array<Bifrost::Math::double4>{db4};
    test_set_attribute("my_double4Array", double4ArrayValue);

    const auto floatArrayValue = Amino::Array<Amino::float_t>{.1f, .2f};
    test_set_attribute("my_floatArray", floatArrayValue);

    auto uv1                    = Bifrost::Math::float2();
    uv1.x                       = 1.f;
    uv1.y                       = 2.f;
    const auto float2ArrayValue = Amino::Array<Bifrost::Math::float2>{uv1};
    test_set_attribute("my_float2Array", float2ArrayValue);

    test_set_attribute("my_float3Array", float3ArrayValue);

    auto q1                     = Bifrost::Math::float4();
    q1.x                        = 1.f;
    q1.y                        = 2.f;
    q1.z                        = 3.f;
    q1.w                        = 4.f;
    const auto float4ArrayValue = Amino::Array<Bifrost::Math::float4>{q1};
    test_set_attribute("my_float4Array", float4ArrayValue);

    const auto intArrayValue = Amino::Array<Amino::int_t>{-1, 2};
    test_set_attribute("my_intArray", intArrayValue);

    const auto int64ArrayValue = Amino::Array<Amino::long_t>{-1, 2};
    test_set_attribute("my_int64Array", int64ArrayValue);

    test_set_attribute("my_normal3fArray", float3ArrayValue);

    test_set_attribute("my_quatdArray", double4ArrayValue);

    test_set_attribute("my_quatfArray", float4ArrayValue);

    test_set_attribute("my_quathArray", float4ArrayValue);

    test_set_attribute("my_stringArray", stringArrayValue);

    test_set_attribute("my_texcoord2fArray", float2ArrayValue);

    test_set_attribute("my_tokenArray", stringArrayValue);

    const auto ucharArrayValue = Amino::Array<Amino::uchar_t>{0, 255};
    test_set_attribute("my_ucharArray", ucharArrayValue);

    const auto uintArrayValue = Amino::Array<Amino::uint_t>{1, 2};
    test_set_attribute("my_uintArray", uintArrayValue);

    const auto uint64ArrayValue = Amino::Array<Amino::ulong_t>{1, 2};
    test_set_attribute("my_uint64Array", uint64ArrayValue);

    const auto m0 = Bifrost::Math::double4x4{
        {0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15}};
    const auto double4x4ArrayValue = Amino::Array<Bifrost::Math::double4x4>{m0};
    test_set_attribute("my_double4x4Array", double4x4ArrayValue);

    Amino::Ptr<BifrostUsd::Stage> stage = std::move(stage_mut);

    auto test_get_attribute = [&](const Amino::String& attribName,
                                  auto& attribValue, auto& attribValueResult) {
        auto attribute = prim.GetAttribute(pxr::TfToken(attribName.c_str()));
        const auto primInterface =
            Amino::newClassPtr<BifrostUsd::Prim>(prim, stage);
        BifrostUsd::Attribute attributeInterface(attribute, primInterface);

        std::remove_reference_t<decltype(attribValue)> valueType{};

        bool success = USD::Attribute::get_prim_attribute_data(
            attributeInterface, valueType, frame, attribValueResult);
        ASSERT_TRUE(success);
        ASSERT_EQ(attribValue, attribValueResult);
    };

    // Single value tests

    auto test_get_scalar2_attribute = [&](const Amino::String& attribName,
                                          auto&                attribValue,
                                          auto& attribValueResult) {
        auto attribute = prim.GetAttribute(pxr::TfToken(attribName.c_str()));
        const auto primInterface =
            Amino::newClassPtr<BifrostUsd::Prim>(prim, stage);
        BifrostUsd::Attribute attributeInterface{attribute, primInterface};

        std::remove_reference_t<decltype(attribValue)> valueType{};

        bool success = USD::Attribute::get_prim_attribute_data(
            attributeInterface, valueType, frame, attribValueResult);
        ASSERT_TRUE(success);
        ASSERT_EQ(attribValue.x, attribValueResult.x);
        ASSERT_EQ(attribValue.y, attribValueResult.y);
    };

    auto test_get_scalar3_attribute = [&](const Amino::String& attribName,
                                          auto&                attribValue,
                                          auto& attribValueResult) {
        auto attribute = prim.GetAttribute(pxr::TfToken(attribName.c_str()));
        const auto primInterface =
            Amino::newClassPtr<BifrostUsd::Prim>(prim, stage);
        BifrostUsd::Attribute attributeInterface{attribute, primInterface};

        std::remove_reference_t<decltype(attribValue)> valueType{};

        bool success = USD::Attribute::get_prim_attribute_data(
            attributeInterface, valueType, frame, attribValueResult);
        ASSERT_TRUE(success);
        ASSERT_EQ(attribValue.x, attribValueResult.x);
        ASSERT_EQ(attribValue.y, attribValueResult.y);
        ASSERT_EQ(attribValue.z, attribValueResult.z);
    };

    auto test_get_scalar4_attribute = [&](const Amino::String& attribName,
                                          auto&                attribValue,
                                          auto& attribValueResult) {
        auto attribute = prim.GetAttribute(pxr::TfToken(attribName.c_str()));
        const auto primInterface =
            Amino::newClassPtr<BifrostUsd::Prim>(prim, stage);
        BifrostUsd::Attribute attributeInterface{attribute, primInterface};

        std::remove_reference_t<decltype(attribValue)> valueType{};

        bool success = USD::Attribute::get_prim_attribute_data(
            attributeInterface, valueType, frame, attribValueResult);
        ASSERT_TRUE(success);
        ASSERT_EQ(attribValue.x, attribValueResult.x);
        ASSERT_EQ(attribValue.y, attribValueResult.y);
        ASSERT_EQ(attribValue.z, attribValueResult.z);
        ASSERT_EQ(attribValue.w, attribValueResult.w);
    };

    auto test_get_scalar4x4_attribute = [&](const Amino::String& attribName,
                                            auto&                attribValue,
                                            auto& attribValueResult) {
        auto attribute = prim.GetAttribute(pxr::TfToken(attribName.c_str()));
        const auto primInterface =
            Amino::newClassPtr<BifrostUsd::Prim>(prim, stage);
        BifrostUsd::Attribute attributeInterface{attribute, primInterface};

        std::remove_reference_t<decltype(attribValue)> valueType{};

        bool success = USD::Attribute::get_prim_attribute_data(
            attributeInterface, valueType, frame, attribValueResult);
        ASSERT_TRUE(success);

        ASSERT_EQ(attribValue.c0.x, attribValueResult.c0.x);
        ASSERT_EQ(attribValue.c0.y, attribValueResult.c0.y);
        ASSERT_EQ(attribValue.c0.z, attribValueResult.c0.z);
        ASSERT_EQ(attribValue.c0.w, attribValueResult.c0.w);

        ASSERT_EQ(attribValue.c1.x, attribValueResult.c1.x);
        ASSERT_EQ(attribValue.c1.y, attribValueResult.c1.y);
        ASSERT_EQ(attribValue.c1.z, attribValueResult.c1.z);
        ASSERT_EQ(attribValue.c1.w, attribValueResult.c1.w);

        ASSERT_EQ(attribValue.c2.x, attribValueResult.c2.x);
        ASSERT_EQ(attribValue.c2.y, attribValueResult.c2.y);
        ASSERT_EQ(attribValue.c2.z, attribValueResult.c2.z);
        ASSERT_EQ(attribValue.c2.w, attribValueResult.c2.w);

        ASSERT_EQ(attribValue.c3.x, attribValueResult.c3.x);
        ASSERT_EQ(attribValue.c3.y, attribValueResult.c3.y);
        ASSERT_EQ(attribValue.c3.z, attribValueResult.c3.z);
        ASSERT_EQ(attribValue.c3.w, attribValueResult.c3.w);
    };

    auto assetValueResult = Amino::String{""};
    test_get_attribute("my_asset", assetValue, assetValueResult);

    auto boolValueResult = Amino::bool_t{false};
    test_get_attribute("my_bool", boolValue, boolValueResult);

    auto color3fValueResult = Bifrost::Math::float3();
    test_get_scalar3_attribute("my_color3f", color3fValue, color3fValueResult);

    auto doubleValueResult = Amino::double_t{0};
    test_get_attribute("my_double", doubleValue, doubleValueResult);

    auto double2ValueResult = Bifrost::Math::double2();
    test_get_scalar2_attribute("my_double2", double2Value, double2ValueResult);

    auto double3ValueResult = Bifrost::Math::double3();
    test_get_scalar3_attribute("my_double3", double3Value, double3ValueResult);

    auto double4ValueResult = Bifrost::Math::double4();
    test_get_scalar4_attribute("my_double4", double4Value, double4ValueResult);

    auto floatValueResult = Amino::float_t{0};
    test_get_attribute("my_float", floatValue, floatValueResult);

    auto float2ValueResult = Bifrost::Math::float2();
    test_get_scalar2_attribute("my_float2", float2Value, float2ValueResult);

    auto float3ValueResult = Bifrost::Math::float3();
    test_get_scalar3_attribute("my_float3", float3Value, float3ValueResult);

    auto float4ValueResult = Bifrost::Math::float4();
    test_get_scalar4_attribute("my_float4", float4Value, float4ValueResult);

    auto intValueResult = Amino::int_t{0};
    test_get_attribute("my_int", intValue, intValueResult);

    auto int64ValueResult = Amino::long_t{0};
    test_get_attribute("my_int64", int64Value, int64ValueResult);

    auto normal3fValueResult = Bifrost::Math::float3();
    test_get_scalar3_attribute("my_normal3f", float3Value, normal3fValueResult);

    auto quatdValueResult = Bifrost::Math::double4();
    test_get_scalar4_attribute("my_quatd", double4Value, quatdValueResult);

    auto quatfValueResult = Bifrost::Math::float4();
    test_get_scalar4_attribute("my_quatf", float4Value, quatfValueResult);

    auto quathValueResult = Bifrost::Math::float4();
    test_get_scalar4_attribute("my_quath", float4Value, quathValueResult);

    auto stringValueResult = Amino::String{""};
    test_get_attribute("my_string", stringValue, stringValueResult);

    auto texcoord2fValueResult = Bifrost::Math::float2();
    test_get_scalar2_attribute("my_texcoord2f", float2Value,
                               texcoord2fValueResult);

    auto tokenValueResult = Amino::String{""};
    test_get_attribute("my_token", tokenValue, tokenValueResult);

    auto ucharValueResult = Amino::uchar_t{0};
    test_get_attribute("my_uchar", ucharValue, ucharValueResult);

    auto uintValueResult = Amino::uint_t{0};
    test_get_attribute("my_uint", uintValue, uintValueResult);

    auto uint64ValueResult = Amino::ulong_t{0};
    test_get_attribute("my_uint64", uint64Value, uint64ValueResult);

    auto double4x4ValueResult = Bifrost::Math::double4x4{};
    test_get_scalar4x4_attribute("my_double4x4", double4x4Value, double4x4ValueResult);


    // Array tests

    auto test_get_scalar_attribute_array = [&](const Amino::String& attribName,
                                               auto&                attribValue,
                                               auto& attribValueResult) {
        auto attribute = prim.GetAttribute(pxr::TfToken(attribName.c_str()));
        const auto primInterface =
            Amino::newClassPtr<BifrostUsd::Prim>(prim, stage);
        BifrostUsd::Attribute attributeInterface{attribute, primInterface};

        std::remove_reference_t<decltype(attribValue)> valueType{};

        bool success = USD::Attribute::get_prim_attribute_data(
            attributeInterface, valueType, frame, attribValueResult);
        ASSERT_TRUE(success);
        auto result = *attribValueResult;
        for (size_t i = 0; i < result.size(); ++i) {
            ASSERT_EQ(attribValue[i], result[i]);
        }
    };

    auto test_get_scalar2_attribute_array = [&](const Amino::String& attribName,
                                                auto& attribValue,
                                                auto& attribValueResult) {
        auto attribute = prim.GetAttribute(pxr::TfToken(attribName.c_str()));
        const auto primInterface =
            Amino::newClassPtr<BifrostUsd::Prim>(prim, stage);
        BifrostUsd::Attribute attributeInterface{attribute, primInterface};

        std::remove_reference_t<decltype(attribValue)> valueType{};

        bool success = USD::Attribute::get_prim_attribute_data(
            attributeInterface, valueType, frame, attribValueResult);
        ASSERT_TRUE(success);
        auto result = *attribValueResult;
        for (size_t i = 0; i < result.size(); ++i) {
            ASSERT_EQ(attribValue[i].x, result[i].x);
            ASSERT_EQ(attribValue[i].y, result[i].y);
        }
    };

    auto test_get_scalar3_attribute_array = [&](const Amino::String& attribName,
                                                auto& attribValue,
                                                auto& attribValueResult) {
        auto attribute = prim.GetAttribute(pxr::TfToken(attribName.c_str()));
        const auto primInterface =
            Amino::newClassPtr<BifrostUsd::Prim>(prim, stage);
        BifrostUsd::Attribute attributeInterface{attribute, primInterface};

        std::remove_reference_t<decltype(attribValue)> valueType{};

        bool success = USD::Attribute::get_prim_attribute_data(
            attributeInterface, valueType, frame, attribValueResult);
        ASSERT_TRUE(success);
        auto result = *attribValueResult;
        for (size_t i = 0; i < result.size(); ++i) {
            ASSERT_EQ(attribValue[i].x, result[i].x);
            ASSERT_EQ(attribValue[i].y, result[i].y);
            ASSERT_EQ(attribValue[i].z, result[i].z);
        }
    };

    auto test_get_scalar4x4_attribute_array =
        [&](const Amino::String& attribName, auto& attribValue,
            auto& attribValueResult) {
            auto attribute =
                prim.GetAttribute(pxr::TfToken(attribName.c_str()));
            const auto primInterface =
                Amino::newClassPtr<BifrostUsd::Prim>(prim, stage);
            BifrostUsd::Attribute attributeInterface{attribute, primInterface};

            std::remove_reference_t<decltype(attribValue)> valueType{};

            bool success = USD::Attribute::get_prim_attribute_data(
                attributeInterface, valueType, frame, attribValueResult);
            ASSERT_TRUE(success);
            auto result = *attribValueResult;
            for (size_t i = 0; i < result.size(); ++i) {
                ASSERT_EQ(attribValue[i].c0.x, result[i].c0.x);
                ASSERT_EQ(attribValue[i].c0.y, result[i].c0.y);
                ASSERT_EQ(attribValue[i].c0.z, result[i].c0.z);
                ASSERT_EQ(attribValue[i].c0.w, result[i].c0.w);

                ASSERT_EQ(attribValue[i].c1.x, result[i].c1.x);
                ASSERT_EQ(attribValue[i].c1.y, result[i].c1.y);
                ASSERT_EQ(attribValue[i].c1.z, result[i].c1.z);
                ASSERT_EQ(attribValue[i].c1.w, result[i].c1.w);

                ASSERT_EQ(attribValue[i].c2.x, result[i].c2.x);
                ASSERT_EQ(attribValue[i].c2.y, result[i].c2.y);
                ASSERT_EQ(attribValue[i].c2.z, result[i].c2.z);
                ASSERT_EQ(attribValue[i].c2.w, result[i].c2.w);

                ASSERT_EQ(attribValue[i].c3.x, result[i].c3.x);
                ASSERT_EQ(attribValue[i].c3.y, result[i].c3.y);
                ASSERT_EQ(attribValue[i].c3.z, result[i].c3.z);
                ASSERT_EQ(attribValue[i].c3.w, result[i].c3.w);
            }
        };

    auto test_get_scalar4_attribute_array = [&](const Amino::String& attribName,
                                                auto& attribValue,
                                                auto& attribValueResult) {
        auto attribute = prim.GetAttribute(pxr::TfToken(attribName.c_str()));
        const auto primInterface =
            Amino::newClassPtr<BifrostUsd::Prim>(prim, stage);
        BifrostUsd::Attribute attributeInterface = {attribute, primInterface};

        std::remove_reference_t<decltype(attribValue)> valueType{};

        bool success = USD::Attribute::get_prim_attribute_data(
            attributeInterface, valueType, frame, attribValueResult);
        ASSERT_TRUE(success);
        auto result = *attribValueResult;
        for (size_t i = 0; i < result.size(); ++i) {
            ASSERT_EQ(attribValue[i].x, result[i].x);
            ASSERT_EQ(attribValue[i].y, result[i].y);
            ASSERT_EQ(attribValue[i].z, result[i].z);
            ASSERT_EQ(attribValue[i].w, result[i].w);
        }
    };

    Amino::MutablePtr<Amino::Array<Amino::String>> assetArrayValueResult;
    test_get_scalar_attribute_array("my_assetArray", stringArrayValue,
                                    assetArrayValueResult);

    Amino::MutablePtr<Amino::Array<Amino::bool_t>> boolArrayValueResult;
    test_get_scalar_attribute_array("my_boolArray", boolArrayValue,
                                    boolArrayValueResult);

    Amino::MutablePtr<Amino::Array<Bifrost::Math::float3>>
        color3fArrayValueResult;
    test_get_scalar3_attribute_array("my_color3fArray", float3ArrayValue,
                                     color3fArrayValueResult);

    Amino::MutablePtr<Amino::Array<Amino::double_t>> doubleArrayValueResult;
    test_get_scalar_attribute_array("my_doubleArray", doubleArrayValue,
                                    doubleArrayValueResult);

    Amino::MutablePtr<Amino::Array<Bifrost::Math::double2>>
        double2ArrayValueResult;
    test_get_scalar2_attribute_array("my_double2Array", double2ArrayValue,
                                     double2ArrayValueResult);

    Amino::MutablePtr<Amino::Array<Bifrost::Math::double3>>
        double3ArrayValueResult;
    test_get_scalar3_attribute_array("my_double3Array", double3ArrayValue,
                                     double3ArrayValueResult);

    Amino::MutablePtr<Amino::Array<Bifrost::Math::double4>>
        double4ArrayValueResult;
    test_get_scalar4_attribute_array("my_double4Array", double4ArrayValue,
                                     double4ArrayValueResult);

    Amino::MutablePtr<Amino::Array<Amino::float_t>> floatArrayValueResult;
    test_get_scalar_attribute_array("my_floatArray", floatArrayValue,
                                    floatArrayValueResult);

    Amino::MutablePtr<Amino::Array<Bifrost::Math::float2>>
        float2ArrayValueResult;
    test_get_scalar2_attribute_array("my_float2Array", float2ArrayValue,
                                     float2ArrayValueResult);

    Amino::MutablePtr<Amino::Array<Bifrost::Math::float3>>
        float3ArrayValueResult;
    test_get_scalar3_attribute_array("my_float3Array", float3ArrayValue,
                                     float3ArrayValueResult);

    Amino::MutablePtr<Amino::Array<Bifrost::Math::float4>>
        float4ArrayValueResult;
    test_get_scalar4_attribute_array("my_float4Array", float4ArrayValue,
                                     float4ArrayValueResult);

    Amino::MutablePtr<Amino::Array<Amino::int_t>> intArrayValueResult;
    test_get_scalar_attribute_array("my_intArray", intArrayValue,
                                    intArrayValueResult);

    Amino::MutablePtr<Amino::Array<Amino::long_t>> int64ArrayValueResult;
    test_get_scalar_attribute_array("my_int64Array", int64ArrayValue,
                                    int64ArrayValueResult);

    Amino::MutablePtr<Amino::Array<Bifrost::Math::float3>>
        normal3fArrayValueResult;
    test_get_scalar3_attribute_array("my_normal3fArray", float3ArrayValue,
                                     normal3fArrayValueResult);

    Amino::MutablePtr<Amino::Array<Bifrost::Math::double4>>
        quatdArrayValueResult;
    test_get_scalar4_attribute_array("my_quatdArray", double4ArrayValue,
                                     quatdArrayValueResult);

    Amino::MutablePtr<Amino::Array<Bifrost::Math::float4>>
        quatfArrayValueResult;
    test_get_scalar4_attribute_array("my_quatfArray", float4ArrayValue,
                                     quatfArrayValueResult);

    Amino::MutablePtr<Amino::Array<Bifrost::Math::float4>>
        quathArrayValueResult;
    test_get_scalar4_attribute_array("my_quathArray", float4ArrayValue,
                                     quathArrayValueResult);

    Amino::MutablePtr<Amino::Array<Amino::String>> stringArrayValueResult;
    test_get_scalar_attribute_array("my_stringArray", stringArrayValue,
                                    stringArrayValueResult);

    Amino::MutablePtr<Amino::Array<Bifrost::Math::float2>>
        texcoord2fArrayValueResult;
    test_get_scalar2_attribute_array("my_texcoord2fArray", float2ArrayValue,
                                     texcoord2fArrayValueResult);

    Amino::MutablePtr<Amino::Array<Amino::String>> tokenArrayValueResult;
    test_get_scalar_attribute_array("my_tokenArray", stringArrayValue,
                                    tokenArrayValueResult);

    Amino::MutablePtr<Amino::Array<Amino::uchar_t>> ucharArrayValueResult;
    test_get_scalar_attribute_array("my_ucharArray", ucharArrayValue,
                                    ucharArrayValueResult);

    Amino::MutablePtr<Amino::Array<Amino::uint_t>> uintArrayValueResult;
    test_get_scalar_attribute_array("my_uintArray", uintArrayValue,
                                    uintArrayValueResult);

    Amino::MutablePtr<Amino::Array<Amino::ulong_t>> uint64ArrayValueResult;
    test_get_scalar_attribute_array("my_uint64Array", uint64ArrayValue,
                                    uint64ArrayValueResult);

    Amino::MutablePtr<Amino::Array<Bifrost::Math::double4x4>>
        double4x4ArrayValueResult;
    test_get_scalar4x4_attribute_array("my_double4x4Array", double4x4ArrayValue,
                                       double4x4ArrayValueResult);
}

TEST(PrimNodeDefs, attribute_metadata) {
    BifrostUsd::Stage stage{getResourcePath("helloworld.usd")};
    ASSERT_TRUE(stage);
    auto primPath    = Amino::String{"/hello"};
    auto pxrPrimPath = pxr::SdfPath(primPath.c_str());
    auto prim        = stage->GetPrimAtPath(pxrPrimPath);
    auto attrName    = Amino::String{"my_float"};
    auto attr        = prim.CreateAttribute(pxr::TfToken(attrName.c_str()),
                                     pxr::SdfValueTypeNames->Float);
    // Test String
    {
        auto docVal  = Amino::String{"This is my documentation"};
        bool success = USD::Attribute::set_attribute_metadata(
            stage, primPath, attrName, "documentation", docVal);
        ASSERT_TRUE(success);
        ASSERT_TRUE(stage);
        Amino::String strDefault = "Oups an error!";
        Amino::String strResult;
        success = USD::Attribute::get_attribute_metadata(
            stage, primPath, attrName, "documentation", strDefault, strResult);
        ASSERT_TRUE(success);
        ASSERT_STREQ(docVal.c_str(), strResult.c_str());
    }
    // Test bool
    {
        bool hiddenVal = true;
        bool success   = USD::Attribute::set_attribute_metadata(
            stage, primPath, attrName, "hidden", hiddenVal);
        ASSERT_TRUE(success);
        ASSERT_TRUE(stage);
        bool boolDefault = false;
        bool boolResult  = false;
        success          = USD::Attribute::get_attribute_metadata(
            stage, primPath, attrName, "hidden", boolDefault, boolResult);
        ASSERT_TRUE(success);
        ASSERT_EQ(hiddenVal, boolResult);
    }
    // Test Bifrost::Object
    {
        auto         objValue    = Bifrost::createObject();
        Amino::int_t intValue    = 7;
        auto         subObjValue = Bifrost::createObject();
        subObjValue->setProperty("my_int", intValue);
        Amino::long_t   int64Value = 10333222111;
        Amino::String   strValue("foo");
        Amino::float_t  fltValue  = 749.3f;
        Amino::double_t dblValue  = 1001.0;
        bool            boolValue = true;
        objValue->setProperty("my_subObject", std::move(subObjValue));
        objValue->setProperty("my_int64", int64Value);
        objValue->setProperty("my_string", strValue);
        objValue->setProperty("my_float", fltValue);
        objValue->setProperty("my_double", dblValue);
        objValue->setProperty("my_bool", boolValue);
        bool success = USD::Attribute::set_attribute_metadata(
            stage, primPath, attrName, "customData", *objValue);
        ASSERT_TRUE(success);
        ASSERT_TRUE(stage);

        Amino::Ptr<Bifrost::Object> objDefault = Bifrost::createObject();
        Amino::Ptr<Bifrost::Object> objResult;

        success = USD::Attribute::get_attribute_metadata(
            stage, primPath, attrName, "customData", objDefault, objResult);
        ASSERT_TRUE(success);
        ASSERT_TRUE(objResult);
        auto any         = objResult->getProperty("my_subObject");
        auto mySubObject = Amino::any_cast<Amino::Ptr<Bifrost::Object>>(&any);
        ASSERT_TRUE(*mySubObject);
        any        = (*mySubObject)->getProperty("my_int");
        auto myInt = Amino::any_cast<Amino::int_t>(&any);
        ASSERT_TRUE(myInt);
        ASSERT_EQ(*myInt, intValue);
        any          = objResult->getProperty("my_int64");
        auto myInt64 = Amino::any_cast<Amino::long_t>(&any);
        ASSERT_TRUE(myInt64);
        ASSERT_EQ(*myInt64, int64Value);
        any           = objResult->getProperty("my_string");
        auto myString = Amino::any_cast<Amino::String>(&any);
        ASSERT_TRUE(myString);
        ASSERT_STREQ((*myString).c_str(), strValue.c_str());
        any          = objResult->getProperty("my_float");
        auto myFloat = Amino::any_cast<Amino::float_t>(&any);
        ASSERT_TRUE(myFloat);
        ASSERT_FLOAT_EQ(*myFloat, fltValue);
        any           = objResult->getProperty("my_double");
        auto myDouble = Amino::any_cast<Amino::double_t>(&any);
        ASSERT_TRUE(myDouble);
        ASSERT_DOUBLE_EQ(*myDouble, dblValue);
        any         = objResult->getProperty("my_bool");
        auto myBool = Amino::any_cast<Amino::bool_t>(&any);
        ASSERT_TRUE(myBool);
        ASSERT_EQ(*myBool, boolValue);
    }
}

TEST(AttributeNodeDefs, add_attribute_connection) {
    auto stage          = Amino::newMutablePtr<BifrostUsd::Stage>();
    auto targetPrimPath = pxr::SdfPath("/target");
    auto targetPrim     = stage->get().DefinePrim(targetPrimPath);

    auto targetAttrName = pxr::TfToken("target");
    auto targetAttr     = targetPrim.CreateAttribute(targetAttrName,
                                                 pxr::SdfValueTypeNames->Token);
    targetAttr.Set(targetAttrName);

    auto sourcePrimPath = pxr::SdfPath("/source");
    auto sourcePrim     = stage->get().DefinePrim(sourcePrimPath);

    auto sourceAttrName = pxr::TfToken("source");
    auto sourceAttr     = sourcePrim.CreateAttribute(sourceAttrName,
                                                 pxr::SdfValueTypeNames->Token);
    sourceAttr.Set(sourceAttrName);

    auto sourceAttrPath = sourcePrimPath.AppendProperty(sourceAttrName);
    auto position       = BifrostUsd::UsdListPositionFrontOfPrependList;

    ASSERT_TRUE(USD::Attribute::add_attribute_connection(
        *stage, targetPrimPath.GetText(), targetAttrName.GetText(),
        sourceAttrPath.GetText(), position));

    targetPrim = stage->get().GetPrimAtPath(targetPrimPath);
    targetAttr = targetPrim.GetAttribute(targetAttrName);
    pxr::SdfPathVector sources;

    ASSERT_TRUE(targetAttr.GetConnections(&sources));
    ASSERT_EQ(sources[0], sourceAttrPath);
}

TEST(AttributeNodeDefs, remove_attribute_connection) {
    auto stage          = Amino::newMutablePtr<BifrostUsd::Stage>();
    auto targetPrimPath = pxr::SdfPath("/target");
    auto targetPrim     = stage->get().DefinePrim(targetPrimPath);

    auto targetAttrName = pxr::TfToken("target");
    auto targetAttr     = targetPrim.CreateAttribute(targetAttrName,
                                                 pxr::SdfValueTypeNames->Token);
    targetAttr.Set(targetAttrName);

    auto sourcePrimPath = pxr::SdfPath("/source");
    auto sourcePrim     = stage->get().DefinePrim(sourcePrimPath);

    auto sourceAttrName = pxr::TfToken("source");
    auto sourceAttr     = sourcePrim.CreateAttribute(sourceAttrName,
                                                 pxr::SdfValueTypeNames->Token);
    sourceAttr.Set(sourceAttrName);

    auto sourceAttrPath = sourcePrimPath.AppendProperty(sourceAttrName);

    targetAttr.AddConnection(sourceAttrPath,
                             pxr::UsdListPositionFrontOfPrependList);
    ASSERT_TRUE(USD::Attribute::remove_attribute_connection(
        *stage, targetPrimPath.GetText(), targetAttrName.GetText(),
        sourceAttrPath.GetText()));

    targetPrim = stage->get().GetPrimAtPath(targetPrimPath);
    targetAttr = targetPrim.GetAttribute(targetAttrName);
    pxr::SdfPathVector sources;
    ASSERT_TRUE(targetAttr.GetConnections(&sources));
    ASSERT_EQ(sources.size(), 0);
}

TEST(AttributeNodeDefs, clear_attribute_connections) {
    auto stage          = Amino::newMutablePtr<BifrostUsd::Stage>();
    auto targetPrimPath = pxr::SdfPath("/target");
    auto targetPrim     = stage->get().DefinePrim(targetPrimPath);

    auto targetAttrName = pxr::TfToken("target");
    auto targetAttr     = targetPrim.CreateAttribute(targetAttrName,
                                                 pxr::SdfValueTypeNames->Token);
    targetAttr.Set(targetAttrName);

    auto sourcePrimPath = pxr::SdfPath("/source");
    auto sourcePrim     = stage->get().DefinePrim(sourcePrimPath);

    auto sourceAttrName = pxr::TfToken("source");
    auto sourceAttr     = sourcePrim.CreateAttribute(sourceAttrName,
                                                 pxr::SdfValueTypeNames->Token);
    sourceAttr.Set(sourceAttrName);

    auto sourceAttrPath = sourcePrimPath.AppendProperty(sourceAttrName);

    targetAttr.AddConnection(sourceAttrPath,
                             pxr::UsdListPositionFrontOfPrependList);

    ASSERT_TRUE(USD::Attribute::clear_attribute_connections(
        *stage, targetPrimPath.GetText(), targetAttrName.GetText()));

    targetPrim = stage->get().GetPrimAtPath(targetPrimPath);
    targetAttr = targetPrim.GetAttribute(targetAttrName);
    pxr::SdfPathVector sources;
    ASSERT_FALSE(targetAttr.GetConnections(&sources));
    ASSERT_EQ(sources.size(), 0);
}
