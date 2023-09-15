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
/// \file  usd_type_converter.h
/// \brief Bifrost USD conversion from Amino types to USD and vice versa.

#ifndef ADSK_USD_TYPE_CONVERTER_H
#define ADSK_USD_TYPE_CONVERTER_H

#include <Amino/Core/Array.h>
#include <Amino/Core/String.h>
#include <Bifrost/Math/Types.h>

#include "usd_utils.h"

namespace USDTypeConverters {

// Traits to get Pxr types from Bifrost/Amino types.

#define FOR_EACH_TYPE_PAIR(MACRO)               \
    MACRO(Amino::bool_t, bool)                  \
    MACRO(Amino::uchar_t, unsigned char)        \
    MACRO(Amino::int_t, int)                    \
    MACRO(Amino::uint_t, unsigned int)          \
    MACRO(Amino::long_t, std::int64_t)          \
    MACRO(Amino::ulong_t, std::uint64_t)        \
    MACRO(Amino::float_t, float)                \
    MACRO(Amino::double_t, double)              \
    MACRO(Amino::String, std::string)           \
    MACRO(Bifrost::Math::float2, PXR_NS::GfVec2f)  \
    MACRO(Bifrost::Math::float3, PXR_NS::GfVec3f)  \
    MACRO(Bifrost::Math::float4, PXR_NS::GfVec4f)  \
    MACRO(Bifrost::Math::double2, PXR_NS::GfVec2d) \
    MACRO(Bifrost::Math::double3, PXR_NS::GfVec3d) \
    MACRO(Bifrost::Math::double4, PXR_NS::GfVec4d) \
    MACRO(Bifrost::Math::double4x4, PXR_NS::GfMatrix4d)

template <typename T>
struct type_identity { // std::type_identity in C++20
    using type = T;
};

template <typename T>
struct PxrType {};
template <typename T>
struct BfType {};

template <typename T>
using PxrType_t = typename PxrType<T>::type;
template <typename T>
using BfType_t = typename BfType<T>::type;

#define DEFINE_TRAITS(BF_TYPE, PXR_TYPE)                         \
    template <>                                                  \
    struct PxrType<BF_TYPE> : public type_identity<PXR_TYPE> {}; \
    template <>                                                  \
    struct BfType<PXR_TYPE> : public type_identity<BF_TYPE> {};
FOR_EACH_TYPE_PAIR(DEFINE_TRAITS)
#undef DEFINE_TRAITS

template <>
struct PxrType<Bifrost::Object> : public type_identity<PXR_NS::VtDictionary> {};
template <>
struct BfType<PXR_NS::VtDictionary>
    : public type_identity<Amino::MutablePtr<Bifrost::Object>> {};

template <typename T>
struct PxrType<Amino::Array<T>>
    : public type_identity<PXR_NS::VtArray<PxrType_t<T>>> {};
template <typename T>
struct BfType<PXR_NS::VtArray<T>>
    : public type_identity<Amino::Array<BfType_t<T>>> {};

// Type conversions from Pxr types to Bifrost/Amino types, and vice versa.

template <typename T>
inline BfType_t<T> fromPxr(const T& src) {
    return src;
}
template <>
inline BfType_t<std::string> fromPxr(const std::string& src) {
    return src.c_str();
}
template <>
inline BfType_t<PXR_NS::GfVec2f> fromPxr(const PXR_NS::GfVec2f& src) {
    return {src[0], src[1]};
}
template <>
inline BfType_t<PXR_NS::GfVec3f> fromPxr(const PXR_NS::GfVec3f& src) {
    return {src[0], src[1], src[2]};
}
template <>
inline BfType_t<PXR_NS::GfVec4f> fromPxr(const PXR_NS::GfVec4f& src) {
    return {src[0], src[1], src[2], src[3]};
}
template <>
inline BfType_t<PXR_NS::GfVec2d> fromPxr(const PXR_NS::GfVec2d& src) {
    return {src[0], src[1]};
}
template <>
inline BfType_t<PXR_NS::GfVec3d> fromPxr(const PXR_NS::GfVec3d& src) {
    return {src[0], src[1], src[2]};
}
template <>
inline BfType_t<PXR_NS::GfVec4d> fromPxr(const PXR_NS::GfVec4d& src) {
    return {src[0], src[1], src[2], src[3]};
}
template <>
inline BfType_t<PXR_NS::GfMatrix4d> fromPxr(const PXR_NS::GfMatrix4d& src) {
    double data[4][4];
    src.Get(data);
    return {{data[0][0], data[0][1], data[0][2], data[0][3]},
             {data[1][0], data[1][1], data[1][2], data[1][3]},
             {data[2][0], data[2][1], data[2][2], data[2][3]},
             {data[3][0], data[3][1], data[3][2], data[3][3]}};
}
template <>
inline BfType_t<PXR_NS::VtDictionary> fromPxr(const PXR_NS::VtDictionary& src) {
    return USDUtils::VtDictionaryToBifrostObject(src);
}
template <typename T>
inline BfType_t<PXR_NS::VtArray<T>> fromPxr(const PXR_NS::VtArray<T>& src) {
    BfType_t<PXR_NS::VtArray<T>> dest;
    dest.resize(src.size());
    for (size_t i = 0; i < src.size(); i++) {
        dest[i] = fromPxr(src[i]);
    }
    return dest;
}

template <typename T>
inline PxrType_t<T> toPxr(const T& src) {
    return src;
}
template <>
inline PxrType_t<Amino::String> toPxr(const Amino::String& src) {
    return src.c_str();
}
template <>
inline PxrType_t<Bifrost::Math::float2> toPxr(
    const Bifrost::Math::float2& src) {
    return {src.x, src.y};
}
template <>
inline PxrType_t<Bifrost::Math::float3> toPxr(
    const Bifrost::Math::float3& src) {
    return {src.x, src.y, src.z};
}
template <>
inline PxrType_t<Bifrost::Math::float4> toPxr(
    const Bifrost::Math::float4& src) {
    return {src.x, src.y, src.z, src.w};
}
template <>
inline PxrType_t<Bifrost::Math::double2> toPxr(
    const Bifrost::Math::double2& src) {
    return {src.x, src.y};
}
template <>
inline PxrType_t<Bifrost::Math::double3> toPxr(
    const Bifrost::Math::double3& src) {
    return {src.x, src.y, src.z};
}
template <>
inline PxrType_t<Bifrost::Math::double4> toPxr(
    const Bifrost::Math::double4& src) {
    return {src.x, src.y, src.z, src.w};
}
template <>
inline PxrType_t<Bifrost::Math::double4x4> toPxr(
    const Bifrost::Math::double4x4& src) {
    return {src.c0.x, src.c0.y, src.c0.z, src.c0.w, src.c1.x, src.c1.y,
            src.c1.z, src.c1.w, src.c2.x, src.c2.y, src.c2.z, src.c2.w,
            src.c3.x, src.c3.y, src.c3.z, src.c3.w};    
}
template <>
inline PxrType_t<Bifrost::Object> toPxr(const Bifrost::Object& src) {
    return USDUtils::BifrostObjectToVtDictionary(src);
}
template <typename T>
inline PxrType_t<Amino::Array<T>> toPxr(const Amino::Array<T>& src) {
    PxrType_t<Amino::Array<T>> dest;
    dest.resize(src.size());
    for (size_t i = 0; i < src.size(); i++) {
        dest[i] = toPxr(src[i]);
    }
    return dest;
}

} // namespace USDTypeConverters

#endif // ADSK_USD_TYPE_CONVERTER_H
