//-/// Copyright 2023 Autodesk, Inc.
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

PXR_NS::UsdListPosition GetUsdListPosition(
    const BifrostUsd::UsdListPosition position) {
    switch (position) {
        case BifrostUsd::UsdListPositionFrontOfPrependList:
            return PXR_NS::UsdListPositionFrontOfPrependList;
        case BifrostUsd::UsdListPositionBackOfPrependList:
            return PXR_NS::UsdListPositionBackOfPrependList;
        case BifrostUsd::UsdListPositionFrontOfAppendList:
            return PXR_NS::UsdListPositionFrontOfAppendList;
        case BifrostUsd::UsdListPositionBackOfAppendList:
            return PXR_NS::UsdListPositionBackOfAppendList;
    }
    return PXR_NS::UsdListPositionBackOfPrependList;
}

PXR_NS::UsdGeomXformCommonAPI::RotationOrder GetUsdRotationOrder(
    const Bifrost::Math::rotation_order order) {
    switch (order) {
        case Bifrost::Math::rotation_order::XYZ:
            return PXR_NS::UsdGeomXformCommonAPI::RotationOrderXYZ;
        case Bifrost::Math::rotation_order::XZY:
            return PXR_NS::UsdGeomXformCommonAPI::RotationOrderXZY;
        case Bifrost::Math::rotation_order::YXZ:
            return PXR_NS::UsdGeomXformCommonAPI::RotationOrderYXZ;
        case Bifrost::Math::rotation_order::YZX:
            return PXR_NS::UsdGeomXformCommonAPI::RotationOrderYZX;
        case Bifrost::Math::rotation_order::ZXY:
            return PXR_NS::UsdGeomXformCommonAPI::RotationOrderZXY;
        case Bifrost::Math::rotation_order::ZYX:
            return PXR_NS::UsdGeomXformCommonAPI::RotationOrderZYX;
    }
    return PXR_NS::UsdGeomXformCommonAPI::RotationOrderXYZ;
}

Bifrost::Math::rotation_order GetRotationOrder(
    const PXR_NS::UsdGeomXformCommonAPI::RotationOrder order) {
    switch (order) {
        case PXR_NS::UsdGeomXformCommonAPI::RotationOrderXYZ:
            return Bifrost::Math::rotation_order::XYZ;
        case PXR_NS::UsdGeomXformCommonAPI::RotationOrderXZY:
            return Bifrost::Math::rotation_order::XZY;
        case PXR_NS::UsdGeomXformCommonAPI::RotationOrderYXZ:
            return Bifrost::Math::rotation_order::YXZ;
        case PXR_NS::UsdGeomXformCommonAPI::RotationOrderYZX:
            return Bifrost::Math::rotation_order::YZX;
        case PXR_NS::UsdGeomXformCommonAPI::RotationOrderZXY:
            return Bifrost::Math::rotation_order::ZXY;
        case PXR_NS::UsdGeomXformCommonAPI::RotationOrderZYX:
            return Bifrost::Math::rotation_order::ZYX;
    }
    return Bifrost::Math::rotation_order::XYZ;
}

PXR_NS::GfVec3d GetVec3d(const Bifrost::Math::double3& vec) {
    return PXR_NS::GfVec3d(vec.x, vec.y, vec.z);
}

PXR_NS::GfVec3f GetVec3f(const Bifrost::Math::float3& vec) {
    return PXR_NS::GfVec3f(vec.x, vec.y, vec.z);
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
                PXR_NS::VtVec3fArray&                         dest) {
    dest.resize(src.size());
    for (size_t i = 0; i < src.size(); ++i) {
        dest[i][0] = src[i].x;
        dest[i][1] = src[i].y;
        dest[i][2] = src[i].z;
    }
}

void copy_array(const Amino::Array<Bifrost::Math::float4>& src,
                PXR_NS::VtVec4fArray&                         dest) {
    dest.resize(src.size());
    for (size_t i = 0; i < src.size(); ++i) {
        dest[i][0] = src[i].x;
        dest[i][1] = src[i].y;
        dest[i][2] = src[i].z;
        dest[i][3] = src[i].w;
    }
}

void copy_array(const Amino::Array<Bifrost::Math::float4>& src,
                PXR_NS::VtQuathArray&                         dest) {
    dest.resize(src.size());
    for (size_t i = 0; i < src.size(); ++i) {
        dest[i].SetReal(PXR_NS::pxr_half::half(src[i].w));
        dest[i].SetImaginary(PXR_NS::pxr_half::half(src[i].x),
                             PXR_NS::pxr_half::half(src[i].y),
                             PXR_NS::pxr_half::half(src[i].z));
    }
}

PXR_NS::UsdPrim get_prim_at_path(const Amino::String&       path,
                              const BifrostUsd::Stage& stage) {
    assert(stage.isValid());
    if (stage.isValid()) {
        Amino::String prim_path = resolve_prim_path(path, stage);

        auto prim = stage->GetPrimAtPath(PXR_NS::SdfPath(prim_path.c_str()));
        return prim;
    }

    return PXR_NS::UsdPrim(); // invalid prim
}

PXR_NS::UsdPrim get_prim_or_throw(Amino::String const&       prim_path,
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

PXR_NS::SdfVariability GetSdfVariability(
    const BifrostUsd::SdfVariability variablity) {
    switch (variablity) {
        case BifrostUsd::SdfVariability::Varying: return PXR_NS::SdfVariabilityVarying;
        case BifrostUsd::SdfVariability::Uniform: return PXR_NS::SdfVariabilityUniform;
    }
    return PXR_NS::SdfVariabilityVarying;
}

PXR_NS::SdfValueTypeName GetSdfValueTypeName(
    const BifrostUsd::SdfValueTypeName type_name) {
    switch (type_name) {
        case BifrostUsd::SdfValueTypeName::Asset: return PXR_NS::SdfValueTypeNames->Asset;
        case BifrostUsd::SdfValueTypeName::AssetArray:
            return PXR_NS::SdfValueTypeNames->AssetArray;
        case BifrostUsd::SdfValueTypeName::Bool: return PXR_NS::SdfValueTypeNames->Bool;
        case BifrostUsd::SdfValueTypeName::BoolArray: return PXR_NS::SdfValueTypeNames->BoolArray;
        case BifrostUsd::SdfValueTypeName::Color3f: return PXR_NS::SdfValueTypeNames->Color3f;
        case BifrostUsd::SdfValueTypeName::Color3fArray:
            return PXR_NS::SdfValueTypeNames->Color3fArray;
        case BifrostUsd::SdfValueTypeName::Double: return PXR_NS::SdfValueTypeNames->Double;
        case BifrostUsd::SdfValueTypeName::DoubleArray:
            return PXR_NS::SdfValueTypeNames->DoubleArray;
        case BifrostUsd::SdfValueTypeName::Double2: return PXR_NS::SdfValueTypeNames->Double2;
        case BifrostUsd::SdfValueTypeName::Double2Array:
            return PXR_NS::SdfValueTypeNames->Double2Array;
        case BifrostUsd::SdfValueTypeName::Double3: return PXR_NS::SdfValueTypeNames->Double3;
        case BifrostUsd::SdfValueTypeName::Double3Array:
            return PXR_NS::SdfValueTypeNames->Double3Array;
        case BifrostUsd::SdfValueTypeName::Double4: return PXR_NS::SdfValueTypeNames->Double4;
        case BifrostUsd::SdfValueTypeName::Double4Array:
            return PXR_NS::SdfValueTypeNames->Double4Array;
        case BifrostUsd::SdfValueTypeName::Float: return PXR_NS::SdfValueTypeNames->Float;
        case BifrostUsd::SdfValueTypeName::FloatArray:
            return PXR_NS::SdfValueTypeNames->FloatArray;
        case BifrostUsd::SdfValueTypeName::Float2: return PXR_NS::SdfValueTypeNames->Float2;
        case BifrostUsd::SdfValueTypeName::Float2Array:
            return PXR_NS::SdfValueTypeNames->Float2Array;
        case BifrostUsd::SdfValueTypeName::Float3: return PXR_NS::SdfValueTypeNames->Float3;
        case BifrostUsd::SdfValueTypeName::Float3Array:
            return PXR_NS::SdfValueTypeNames->Float3Array;
        case BifrostUsd::SdfValueTypeName::Float4: return PXR_NS::SdfValueTypeNames->Float4;
        case BifrostUsd::SdfValueTypeName::Float4Array:
            return PXR_NS::SdfValueTypeNames->Float4Array;
        case BifrostUsd::SdfValueTypeName::Half:
            return PXR_NS::SdfValueTypeNames->Half;
        case BifrostUsd::SdfValueTypeName::HalfArray:
            return PXR_NS::SdfValueTypeNames->HalfArray;
        case BifrostUsd::SdfValueTypeName::Half2:
            return PXR_NS::SdfValueTypeNames->Half2;
        case BifrostUsd::SdfValueTypeName::Half2Array:
            return PXR_NS::SdfValueTypeNames->Half2Array;
        case BifrostUsd::SdfValueTypeName::Half3:
            return PXR_NS::SdfValueTypeNames->Half3;
        case BifrostUsd::SdfValueTypeName::Half3Array:
            return PXR_NS::SdfValueTypeNames->Half3Array;
        case BifrostUsd::SdfValueTypeName::Half4:
            return PXR_NS::SdfValueTypeNames->Half4;
        case BifrostUsd::SdfValueTypeName::Half4Array:
            return PXR_NS::SdfValueTypeNames->Half4Array;
        case BifrostUsd::SdfValueTypeName::Int: return PXR_NS::SdfValueTypeNames->Int;
        case BifrostUsd::SdfValueTypeName::IntArray: return PXR_NS::SdfValueTypeNames->IntArray;
        case BifrostUsd::SdfValueTypeName::Int64: return PXR_NS::SdfValueTypeNames->Int64;
        case BifrostUsd::SdfValueTypeName::Int64Array:
            return PXR_NS::SdfValueTypeNames->Int64Array;
        case BifrostUsd::SdfValueTypeName::Matrix2d:
            return PXR_NS::SdfValueTypeNames->Matrix2d;
        case BifrostUsd::SdfValueTypeName::Matrix2dArray:
            return PXR_NS::SdfValueTypeNames->Matrix2dArray;
        case BifrostUsd::SdfValueTypeName::Matrix3d:
            return PXR_NS::SdfValueTypeNames->Matrix3d;
        case BifrostUsd::SdfValueTypeName::Matrix3dArray:
            return PXR_NS::SdfValueTypeNames->Matrix3dArray;
        case BifrostUsd::SdfValueTypeName::Matrix4d:
            return PXR_NS::SdfValueTypeNames->Matrix4d;
        case BifrostUsd::SdfValueTypeName::Matrix4dArray:
            return PXR_NS::SdfValueTypeNames->Matrix4dArray;
        case BifrostUsd::SdfValueTypeName::Normal3f: return PXR_NS::SdfValueTypeNames->Normal3f;
        case BifrostUsd::SdfValueTypeName::Normal3fArray:
            return PXR_NS::SdfValueTypeNames->Normal3fArray;
        case BifrostUsd::SdfValueTypeName::Quatd: return PXR_NS::SdfValueTypeNames->Quatd;
        case BifrostUsd::SdfValueTypeName::QuatdArray:
            return PXR_NS::SdfValueTypeNames->QuatdArray;
        case BifrostUsd::SdfValueTypeName::Quatf: return PXR_NS::SdfValueTypeNames->Quatf;
        case BifrostUsd::SdfValueTypeName::QuatfArray:
            return PXR_NS::SdfValueTypeNames->QuatfArray;
        case BifrostUsd::SdfValueTypeName::Quath: return PXR_NS::SdfValueTypeNames->Quath;
        case BifrostUsd::SdfValueTypeName::QuathArray:
            return PXR_NS::SdfValueTypeNames->QuathArray;
        case BifrostUsd::SdfValueTypeName::String: return PXR_NS::SdfValueTypeNames->String;
        case BifrostUsd::SdfValueTypeName::StringArray:
            return PXR_NS::SdfValueTypeNames->StringArray;
        case BifrostUsd::SdfValueTypeName::TexCoord2f:
            return PXR_NS::SdfValueTypeNames->TexCoord2f;
        case BifrostUsd::SdfValueTypeName::TexCoord2fArray:
            return PXR_NS::SdfValueTypeNames->TexCoord2fArray;
        case BifrostUsd::SdfValueTypeName::Token: return PXR_NS::SdfValueTypeNames->Token;
        case BifrostUsd::SdfValueTypeName::TokenArray:
            return PXR_NS::SdfValueTypeNames->TokenArray;
        case BifrostUsd::SdfValueTypeName::UChar: return PXR_NS::SdfValueTypeNames->UChar;
        case BifrostUsd::SdfValueTypeName::UCharArray:
            return PXR_NS::SdfValueTypeNames->UCharArray;
        case BifrostUsd::SdfValueTypeName::UInt: return PXR_NS::SdfValueTypeNames->UInt;
        case BifrostUsd::SdfValueTypeName::UIntArray: return PXR_NS::SdfValueTypeNames->UIntArray;
        case BifrostUsd::SdfValueTypeName::UInt64: return PXR_NS::SdfValueTypeNames->UInt64;
        case BifrostUsd::SdfValueTypeName::UInt64Array:
            return PXR_NS::SdfValueTypeNames->UInt64Array;
    }
    return PXR_NS::SdfValueTypeName();
}

PXR_NS::TfToken GetUsdGeomPrimvarInterpolation(
    const BifrostUsd::UsdGeomPrimvarInterpolation interpolation) {
    switch (interpolation) {
        case BifrostUsd::UsdGeomPrimvarInterpolation::PrimVarConstant: return PXR_NS::UsdGeomTokens->constant;
        case BifrostUsd::UsdGeomPrimvarInterpolation::PrimVarUniform: return PXR_NS::UsdGeomTokens->uniform;
        case BifrostUsd::UsdGeomPrimvarInterpolation::PrimVarVarying: return PXR_NS::UsdGeomTokens->varying;
        case BifrostUsd::UsdGeomPrimvarInterpolation::PrimVarVertex: return PXR_NS::UsdGeomTokens->vertex;
        case BifrostUsd::UsdGeomPrimvarInterpolation::PrimVarFaceVarying:
            return PXR_NS::UsdGeomTokens->faceVarying;
    }
    return PXR_NS::TfToken();
}

PXR_NS::TfToken GetSdfFieldKey(const Amino::String& key) {
    if (key == "comment") {
        return PXR_NS::SdfFieldKeys->Comment;
    } else if (key == "customData") {
        return PXR_NS::SdfFieldKeys->CustomData;
    } else if (key == "documentation") {
        return PXR_NS::SdfFieldKeys->Documentation;
    }

    return PXR_NS::TfToken(key.c_str());
}

PXR_NS::VtDictionary BifrostObjectToVtDictionary(const Bifrost::Object& object) {
    PXR_NS::VtDictionary result;
    auto keys = object.keys();
    for (const auto& objKey : *keys) {
        auto any = object.getProperty(objKey);

        if (auto dictionaryPayload =
                Amino::any_cast<Amino::Ptr<Bifrost::Object>>(&any)) {
            result[objKey.c_str()] =
                BifrostObjectToVtDictionary(**dictionaryPayload);
        } else if (auto floatPayload = Amino::any_cast<Amino::float_t>(&any)) {
            result[objKey.c_str()] = PXR_NS::VtValue(toPxr(*floatPayload));
        } else if (auto doublePayload =
                       Amino::any_cast<Amino::double_t>(&any)) {
            result[objKey.c_str()] = PXR_NS::VtValue(toPxr(*doublePayload));
        } else if (auto stringPayload = Amino::any_cast<Amino::String>(&any)) {
            result[objKey.c_str()] = PXR_NS::VtValue(toPxr(*stringPayload));
        } else if (auto intPayload = Amino::any_cast<Amino::int_t>(&any)) {
            result[objKey.c_str()] = PXR_NS::VtValue(toPxr(*intPayload));
        } else if (auto longPayload = Amino::any_cast<Amino::long_t>(&any)) {
            result[objKey.c_str()] = PXR_NS::VtValue(toPxr(*longPayload));
        } else if (auto boolPayload = Amino::any_cast<Amino::bool_t>(&any)) {
            result[objKey.c_str()] = PXR_NS::VtValue(toPxr(*boolPayload));
        }
    }
    return result;
}

template <class T>
void setObjectProperty(const std::string&  key,
                       const PXR_NS::VtValue& value,
                       Bifrost::Object&    object) {
    if (value.IsHolding<T>()) {
        object.setProperty(key.c_str(), fromPxr<T>(value.UncheckedGet<T>()));
    }
}

template <>
void setObjectProperty<std::string>(const std::string&  key,
                                    const PXR_NS::VtValue& value,
                                    Bifrost::Object&    object) {
    if (value.IsHolding<std::string>()) {
        object.setProperty(key.c_str(),
                           fromPxr(value.UncheckedGet<std::string>()));
    }
}

template <>
void setObjectProperty<PXR_NS::VtDictionary>(const std::string&  key,
                                          const PXR_NS::VtValue& value,
                                          Bifrost::Object&    object) {
    if (value.IsHolding<PXR_NS::VtDictionary>()) {
        object.setProperty(key.c_str(),
                           VtDictionaryToBifrostObject(
                               value.UncheckedGet<PXR_NS::VtDictionary>()));
    }
}

auto VtDictionaryToBifrostObject(const PXR_NS::VtDictionary& dict)
    -> decltype(Bifrost::createObject()) {
    auto result = Bifrost::createObject();
    for (auto const& item : dict) {
        setObjectProperty<PXR_NS::VtDictionary>(item.first, item.second, *result);
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

PXR_NS::TfToken GetImageablePurpose(const BifrostUsd::ImageablePurpose purpose) {
    switch (purpose) {
        case BifrostUsd::ImageablePurpose::Default: return PXR_NS::UsdGeomTokens->default_;
        case BifrostUsd::ImageablePurpose::Render: return PXR_NS::UsdGeomTokens->render;
        case BifrostUsd::ImageablePurpose::Proxy: return PXR_NS::UsdGeomTokens->proxy;
        case BifrostUsd::ImageablePurpose::Guide: return PXR_NS::UsdGeomTokens->guide;
    }
    return PXR_NS::TfToken();
}

PXR_NS::TfToken GetMaterialBindingStrength(
    const BifrostUsd::MaterialBindingStrength strength) {
    switch (strength) {
        case BifrostUsd::MaterialBindingStrength::FallbackStrength:
            return PXR_NS::UsdShadeTokens->fallbackStrength;
        case BifrostUsd::MaterialBindingStrength::StrongerThanDescendants:
            return PXR_NS::UsdShadeTokens->strongerThanDescendants;
        case BifrostUsd::MaterialBindingStrength::WeakerThanDescendants:
            return PXR_NS::UsdShadeTokens->weakerThanDescendants;
    }
    return PXR_NS::UsdShadeTokens->fallbackStrength;
}

PXR_NS::TfToken GetMaterialPurpose(const BifrostUsd::MaterialPurpose purpose) {
    switch (purpose) {
        case BifrostUsd::MaterialPurpose::All:
            return PXR_NS::UsdShadeTokens->allPurpose;
        case BifrostUsd::MaterialPurpose::Preview:
            return PXR_NS::UsdShadeTokens->preview;
        case BifrostUsd::MaterialPurpose::Full:
            return PXR_NS::UsdShadeTokens->full;
    }
    return PXR_NS::UsdShadeTokens->allPurpose;
}

PXR_NS::TfToken GetExpansionRule(const BifrostUsd::ExpansionRule rule) {
    switch (rule) {
        case BifrostUsd::ExpansionRule::Default: return PXR_NS::TfToken();
        case BifrostUsd::ExpansionRule::ExplicitOnly:
            return PXR_NS::UsdTokens->explicitOnly;
        case BifrostUsd::ExpansionRule::ExpandPrims:
            return PXR_NS::UsdTokens->expandPrims;
        case BifrostUsd::ExpansionRule::ExpandPrimsAndProperties:
            return PXR_NS::UsdTokens->expandPrimsAndProperties;
    }
    return PXR_NS::TfToken();
}
} // namespace USDUtils
