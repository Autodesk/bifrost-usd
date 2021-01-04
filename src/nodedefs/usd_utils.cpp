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

#include "usd_utils.h"
#include "usd_type_converter.h"

#include <Amino/Core/String.h>
#include <Bifrost/Object/Object.h>

#include <pxr/usd/usd/tokens.h>

using namespace USDTypeConverters;

namespace USDUtils {

pxr::UsdListPosition GetUsdListPosition(
    const BifrostUsd::UsdListPosition position) {
    switch (position) {
        case BifrostUsd::UsdListPositionFrontOfPrependList:
            return pxr::UsdListPositionFrontOfPrependList;
        case BifrostUsd::UsdListPositionBackOfPrependList:
            return pxr::UsdListPositionBackOfPrependList;
        case BifrostUsd::UsdListPositionFrontOfAppendList:
            return pxr::UsdListPositionFrontOfAppendList;
        case BifrostUsd::UsdListPositionBackOfAppendList:
            return pxr::UsdListPositionBackOfAppendList;
    }
    return pxr::UsdListPositionBackOfPrependList;
}

pxr::UsdGeomXformCommonAPI::RotationOrder GetUsdRotationOrder(
    const Bifrost::Math::rotation_order order) {
    switch (order) {
        case Bifrost::Math::rotation_order::XYZ:
            return pxr::UsdGeomXformCommonAPI::RotationOrderXYZ;
        case Bifrost::Math::rotation_order::XZY:
            return pxr::UsdGeomXformCommonAPI::RotationOrderXZY;
        case Bifrost::Math::rotation_order::YXZ:
            return pxr::UsdGeomXformCommonAPI::RotationOrderYXZ;
        case Bifrost::Math::rotation_order::YZX:
            return pxr::UsdGeomXformCommonAPI::RotationOrderYZX;
        case Bifrost::Math::rotation_order::ZXY:
            return pxr::UsdGeomXformCommonAPI::RotationOrderZXY;
        case Bifrost::Math::rotation_order::ZYX:
            return pxr::UsdGeomXformCommonAPI::RotationOrderZYX;
    }
    return pxr::UsdGeomXformCommonAPI::RotationOrderXYZ;
}

Bifrost::Math::rotation_order GetRotationOrder(
    const pxr::UsdGeomXformCommonAPI::RotationOrder order) {
    switch (order) {
        case pxr::UsdGeomXformCommonAPI::RotationOrderXYZ:
            return Bifrost::Math::rotation_order::XYZ;
        case pxr::UsdGeomXformCommonAPI::RotationOrderXZY:
            return Bifrost::Math::rotation_order::XZY;
        case pxr::UsdGeomXformCommonAPI::RotationOrderYXZ:
            return Bifrost::Math::rotation_order::YXZ;
        case pxr::UsdGeomXformCommonAPI::RotationOrderYZX:
            return Bifrost::Math::rotation_order::YZX;
        case pxr::UsdGeomXformCommonAPI::RotationOrderZXY:
            return Bifrost::Math::rotation_order::ZXY;
        case pxr::UsdGeomXformCommonAPI::RotationOrderZYX:
            return Bifrost::Math::rotation_order::ZYX;
    }
    return Bifrost::Math::rotation_order::XYZ;
}

pxr::GfVec3d GetVec3d(const Bifrost::Math::double3& vec) {
    return pxr::GfVec3d(vec.x, vec.y, vec.z);
}

pxr::GfVec3f GetVec3f(const Bifrost::Math::float3& vec) {
    return pxr::GfVec3f(vec.x, vec.y, vec.z);
}

Amino::String ToString(const Bifrost::Math::float3& vec) {
    return (std::to_string(vec.x) + std::to_string(vec.y) +
            std::to_string(vec.z))
        .c_str();
}

Amino::String ToString(const Bifrost::Math::double3& vec) {
    return (std::to_string(vec.x) + std::to_string(vec.y) +
            std::to_string(vec.z))
        .c_str();
}

void copy_array(const Amino::Array<Bifrost::Math::float3>& src,
                pxr::VtVec3fArray&                         dest) {
    dest.resize(src.size());
    for (size_t i = 0; i < src.size(); ++i) {
        dest[i][0] = src[i].x;
        dest[i][1] = src[i].y;
        dest[i][2] = src[i].z;
    }
}

void copy_array(const Amino::Array<Bifrost::Math::float4>& src,
                pxr::VtVec4fArray&                         dest) {
    dest.resize(src.size());
    for (size_t i = 0; i < src.size(); ++i) {
        dest[i][0] = src[i].x;
        dest[i][1] = src[i].y;
        dest[i][2] = src[i].z;
        dest[i][3] = src[i].w;
    }
}

void copy_array(const Amino::Array<Bifrost::Math::float4>& src,
                pxr::VtQuathArray&                         dest) {
    dest.resize(src.size());
    for (size_t i = 0; i < src.size(); ++i) {
        dest[i].SetReal(pxr::pxr_half::half(src[i].w));
        dest[i].SetImaginary(pxr::pxr_half::half(src[i].x),
                             pxr::pxr_half::half(src[i].y),
                             pxr::pxr_half::half(src[i].z));
    }
}

pxr::UsdPrim get_prim_at_path(const Amino::String&       path,
                              const BifrostUsd::Stage& stage) {
    assert(stage.isValid());
    if (stage.isValid()) {
        Amino::String prim_path = resolve_prim_path(path, stage);

        auto prim = stage->GetPrimAtPath(pxr::SdfPath(prim_path.c_str()));
        return prim;
    }

    return pxr::UsdPrim(); // invalid prim
}

pxr::UsdPrim get_prim_or_throw(Amino::String const&       prim_path,
                               BifrostUsd::Stage const& stage) {
    auto pxr_prim = get_prim_at_path(prim_path, stage);
    if (!pxr_prim) {
        auto msg = "Invalid prim path: " + std::string(prim_path.c_str());
        throw std::runtime_error(std::move(msg));
    }
    return pxr_prim;
}

Amino::String resolve_prim_path(const Amino::String&       path,
                                const BifrostUsd::Stage& stage) {
    assert(stage.isValid());
    Amino::String resolvedPath = path;

    if (resolvedPath.empty()) {
        if (!stage.last_modified_prim.empty()) {
            resolvedPath = stage.last_modified_prim;
        } else {
            resolvedPath = "/";
        }
    }

    else if (path.front() != '/') {
        resolvedPath = stage.last_modified_prim + "/" + path;
    }
    return resolvedPath;
}

pxr::SdfVariability GetSdfVariability(
    const BifrostUsd::SdfVariability variablity) {
    switch (variablity) {
        case BifrostUsd::SdfVariability::Varying: return pxr::SdfVariabilityVarying;
        case BifrostUsd::SdfVariability::Uniform: return pxr::SdfVariabilityUniform;
    }
    return pxr::SdfVariabilityVarying;
}

pxr::SdfValueTypeName GetSdfValueTypeName(
    const BifrostUsd::SdfValueTypeName type_name) {
    switch (type_name) {
        case BifrostUsd::SdfValueTypeName::Asset: return pxr::SdfValueTypeNames->Asset;
        case BifrostUsd::SdfValueTypeName::AssetArray:
            return pxr::SdfValueTypeNames->AssetArray;
        case BifrostUsd::SdfValueTypeName::Bool: return pxr::SdfValueTypeNames->Bool;
        case BifrostUsd::SdfValueTypeName::BoolArray: return pxr::SdfValueTypeNames->BoolArray;
        case BifrostUsd::SdfValueTypeName::Color3f: return pxr::SdfValueTypeNames->Color3f;
        case BifrostUsd::SdfValueTypeName::Color3fArray:
            return pxr::SdfValueTypeNames->Color3fArray;
        case BifrostUsd::SdfValueTypeName::Double: return pxr::SdfValueTypeNames->Double;
        case BifrostUsd::SdfValueTypeName::DoubleArray:
            return pxr::SdfValueTypeNames->DoubleArray;
        case BifrostUsd::SdfValueTypeName::Double2: return pxr::SdfValueTypeNames->Double2;
        case BifrostUsd::SdfValueTypeName::Double2Array:
            return pxr::SdfValueTypeNames->Double2Array;
        case BifrostUsd::SdfValueTypeName::Double3: return pxr::SdfValueTypeNames->Double3;
        case BifrostUsd::SdfValueTypeName::Double3Array:
            return pxr::SdfValueTypeNames->Double3Array;
        case BifrostUsd::SdfValueTypeName::Double4: return pxr::SdfValueTypeNames->Double4;
        case BifrostUsd::SdfValueTypeName::Double4Array:
            return pxr::SdfValueTypeNames->Double4Array;
        case BifrostUsd::SdfValueTypeName::Float: return pxr::SdfValueTypeNames->Float;
        case BifrostUsd::SdfValueTypeName::FloatArray:
            return pxr::SdfValueTypeNames->FloatArray;
        case BifrostUsd::SdfValueTypeName::Float2: return pxr::SdfValueTypeNames->Float2;
        case BifrostUsd::SdfValueTypeName::Float2Array:
            return pxr::SdfValueTypeNames->Float2Array;
        case BifrostUsd::SdfValueTypeName::Float3: return pxr::SdfValueTypeNames->Float3;
        case BifrostUsd::SdfValueTypeName::Float3Array:
            return pxr::SdfValueTypeNames->Float3Array;
        case BifrostUsd::SdfValueTypeName::Float4: return pxr::SdfValueTypeNames->Float4;
        case BifrostUsd::SdfValueTypeName::Float4Array:
            return pxr::SdfValueTypeNames->Float4Array;
        case BifrostUsd::SdfValueTypeName::Int: return pxr::SdfValueTypeNames->Int;
        case BifrostUsd::SdfValueTypeName::IntArray: return pxr::SdfValueTypeNames->IntArray;
        case BifrostUsd::SdfValueTypeName::Int64: return pxr::SdfValueTypeNames->Int64;
        case BifrostUsd::SdfValueTypeName::Int64Array:
            return pxr::SdfValueTypeNames->Int64Array;
        case BifrostUsd::SdfValueTypeName::Normal3f: return pxr::SdfValueTypeNames->Normal3f;
        case BifrostUsd::SdfValueTypeName::Normal3fArray:
            return pxr::SdfValueTypeNames->Normal3fArray;
        case BifrostUsd::SdfValueTypeName::Quatd: return pxr::SdfValueTypeNames->Quatd;
        case BifrostUsd::SdfValueTypeName::QuatdArray:
            return pxr::SdfValueTypeNames->QuatdArray;
        case BifrostUsd::SdfValueTypeName::Quatf: return pxr::SdfValueTypeNames->Quatf;
        case BifrostUsd::SdfValueTypeName::QuatfArray:
            return pxr::SdfValueTypeNames->QuatfArray;
        case BifrostUsd::SdfValueTypeName::Quath: return pxr::SdfValueTypeNames->Quath;
        case BifrostUsd::SdfValueTypeName::QuathArray:
            return pxr::SdfValueTypeNames->QuathArray;
        case BifrostUsd::SdfValueTypeName::String: return pxr::SdfValueTypeNames->String;
        case BifrostUsd::SdfValueTypeName::StringArray:
            return pxr::SdfValueTypeNames->StringArray;
        case BifrostUsd::SdfValueTypeName::TexCoord2f:
            return pxr::SdfValueTypeNames->TexCoord2f;
        case BifrostUsd::SdfValueTypeName::TexCoord2fArray:
            return pxr::SdfValueTypeNames->TexCoord2fArray;
        case BifrostUsd::SdfValueTypeName::Token: return pxr::SdfValueTypeNames->Token;
        case BifrostUsd::SdfValueTypeName::TokenArray:
            return pxr::SdfValueTypeNames->TokenArray;
        case BifrostUsd::SdfValueTypeName::UChar: return pxr::SdfValueTypeNames->UChar;
        case BifrostUsd::SdfValueTypeName::UCharArray:
            return pxr::SdfValueTypeNames->UCharArray;
        case BifrostUsd::SdfValueTypeName::UInt: return pxr::SdfValueTypeNames->UInt;
        case BifrostUsd::SdfValueTypeName::UIntArray: return pxr::SdfValueTypeNames->UIntArray;
        case BifrostUsd::SdfValueTypeName::UInt64: return pxr::SdfValueTypeNames->UInt64;
        case BifrostUsd::SdfValueTypeName::UInt64Array:
            return pxr::SdfValueTypeNames->UInt64Array;
    }
    return pxr::SdfValueTypeName();
}

pxr::TfToken GetUsdGeomPrimvarInterpolation(
    const BifrostUsd::UsdGeomPrimvarInterpolation interpolation) {
    switch (interpolation) {
        case BifrostUsd::UsdGeomPrimvarInterpolation::PrimVarConstant: return pxr::UsdGeomTokens->constant;
        case BifrostUsd::UsdGeomPrimvarInterpolation::PrimVarUniform: return pxr::UsdGeomTokens->uniform;
        case BifrostUsd::UsdGeomPrimvarInterpolation::PrimVarVarying: return pxr::UsdGeomTokens->varying;
        case BifrostUsd::UsdGeomPrimvarInterpolation::PrimVarVertex: return pxr::UsdGeomTokens->vertex;
        case BifrostUsd::UsdGeomPrimvarInterpolation::PrimVarFaceVarying:
            return pxr::UsdGeomTokens->faceVarying;
    }
    return pxr::TfToken();
}

pxr::TfToken GetSdfFieldKey(const Amino::String& key) {
    if (key == "comment") {
        return pxr::SdfFieldKeys->Comment;
    } else if (key == "customData") {
        return pxr::SdfFieldKeys->CustomData;
    } else if (key == "documentation") {
        return pxr::SdfFieldKeys->Documentation;
    }

    return pxr::TfToken(key.c_str());
}

pxr::VtDictionary BifrostObjectToVtDictionary(const Bifrost::Object& object) {
    pxr::VtDictionary result;
    auto keys = object.keys();
    for (const auto& objKey : *keys) {
        auto any = object.getProperty(objKey);

        if (auto dictionaryPayload =
                Amino::any_cast<Amino::Ptr<Bifrost::Object>>(&any)) {
            result[objKey.c_str()] =
                BifrostObjectToVtDictionary(**dictionaryPayload);
        } else if (auto floatPayload = Amino::any_cast<Amino::float_t>(&any)) {
            result[objKey.c_str()] = pxr::VtValue(toPxr(*floatPayload));
        } else if (auto doublePayload =
                       Amino::any_cast<Amino::double_t>(&any)) {
            result[objKey.c_str()] = pxr::VtValue(toPxr(*doublePayload));
        } else if (auto stringPayload = Amino::any_cast<Amino::String>(&any)) {
            result[objKey.c_str()] = pxr::VtValue(toPxr(*stringPayload));
        } else if (auto intPayload = Amino::any_cast<Amino::int_t>(&any)) {
            result[objKey.c_str()] = pxr::VtValue(toPxr(*intPayload));
        } else if (auto longPayload = Amino::any_cast<Amino::long_t>(&any)) {
            result[objKey.c_str()] = pxr::VtValue(toPxr(*longPayload));
        } else if (auto boolPayload = Amino::any_cast<Amino::bool_t>(&any)) {
            result[objKey.c_str()] = pxr::VtValue(toPxr(*boolPayload));
        }
    }
    return result;
}

template <class T>
void setObjectProperty(const std::string&  key,
                       const pxr::VtValue& value,
                       Bifrost::Object&    object) {
    if (value.IsHolding<T>()) {
        object.setProperty(key.c_str(), fromPxr<T>(value.UncheckedGet<T>()));
    }
}

template <>
void setObjectProperty<std::string>(const std::string&  key,
                                    const pxr::VtValue& value,
                                    Bifrost::Object&    object) {
    if (value.IsHolding<std::string>()) {
        object.setProperty(key.c_str(),
                           fromPxr(value.UncheckedGet<std::string>()));
    }
}

template <>
void setObjectProperty<pxr::VtDictionary>(const std::string&  key,
                                          const pxr::VtValue& value,
                                          Bifrost::Object&    object) {
    if (value.IsHolding<pxr::VtDictionary>()) {
        object.setProperty(key.c_str(),
                           VtDictionaryToBifrostObject(
                               value.UncheckedGet<pxr::VtDictionary>()));
    }
}

auto VtDictionaryToBifrostObject(const pxr::VtDictionary& dict)
    -> decltype(Bifrost::createObject()) {
    auto result = Bifrost::createObject();
    for (auto const& item : dict) {
        setObjectProperty<pxr::VtDictionary>(item.first, item.second, *result);
        setObjectProperty<PxrType_t<Amino::int_t>>(item.first, item.second, *result);
        setObjectProperty<PxrType_t<Amino::long_t>>(item.first, item.second, *result);
        setObjectProperty<PxrType_t<Amino::float_t>>(item.first, item.second, *result);
        setObjectProperty<PxrType_t<Amino::double_t>>(item.first, item.second, *result);
        setObjectProperty<PxrType_t<Amino::bool_t>>(item.first, item.second, *result);
        setObjectProperty<PxrType_t<Amino::String>>(item.first, item.second, *result);
    }
    return result;
}

size_t reversedSublayerIndex(const size_t index, const size_t numLayers) {
    assert(numLayers > index);
    return ((numLayers - 1) - index);
}

int reversedSublayerIndex(const int index, const int numLayers) {
    assert(index >= 0);
    assert(numLayers > index);
    return ((numLayers - 1) - index);
}

pxr::TfToken GetImageablePurpose(const BifrostUsd::ImageablePurpose purpose) {
    switch (purpose) {
        case BifrostUsd::ImageablePurpose::Default: return pxr::UsdGeomTokens->default_;
        case BifrostUsd::ImageablePurpose::Render: return pxr::UsdGeomTokens->render;
        case BifrostUsd::ImageablePurpose::Proxy: return pxr::UsdGeomTokens->proxy;
        case BifrostUsd::ImageablePurpose::Guide: return pxr::UsdGeomTokens->guide;
    }
    return pxr::TfToken();
}

pxr::TfToken GetMaterialBindingStrength(
    const BifrostUsd::MaterialBindingStrength strength) {
    switch (strength) {
        case BifrostUsd::MaterialBindingStrength::FallbackStrength:
            return pxr::UsdShadeTokens->fallbackStrength;
        case BifrostUsd::MaterialBindingStrength::StrongerThanDescendants:
            return pxr::UsdShadeTokens->strongerThanDescendants;
        case BifrostUsd::MaterialBindingStrength::WeakerThanDescendants:
            return pxr::UsdShadeTokens->weakerThanDescendants;
    }
    return pxr::UsdShadeTokens->fallbackStrength;
}

pxr::TfToken GetMaterialPurpose(const BifrostUsd::MaterialPurpose purpose) {
    switch (purpose) {
        case BifrostUsd::MaterialPurpose::All:
            return pxr::UsdShadeTokens->allPurpose;
        case BifrostUsd::MaterialPurpose::Preview:
            return pxr::UsdShadeTokens->preview;
        case BifrostUsd::MaterialPurpose::Full:
            return pxr::UsdShadeTokens->full;
    }
    return pxr::UsdShadeTokens->allPurpose;
}

pxr::TfToken GetExpansionRule(const BifrostUsd::ExpansionRule rule) {
    switch (rule) {
        case BifrostUsd::ExpansionRule::Default: return pxr::TfToken();
        case BifrostUsd::ExpansionRule::ExplicitOnly:
            return pxr::UsdTokens->explicitOnly;
        case BifrostUsd::ExpansionRule::ExpandPrims:
            return pxr::UsdTokens->expandPrims;
        case BifrostUsd::ExpansionRule::ExpandPrimsAndProperties:
            return pxr::UsdTokens->expandPrimsAndProperties;
    }
    return pxr::TfToken();
}
} // namespace USDUtils
