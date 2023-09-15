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

#include "usd_attribute_nodedefs.h"

#include <Amino/Core/String.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>

#include <cstdint>

#include "return_guard.h"
#include "usd_type_converter.h"
#include "usd_utils.h"

using namespace USDUtils;
using namespace USDTypeConverters;

//==============================================================================
// CONVERSION TRAITS AND FUNCTIONS
//==============================================================================

namespace {

PXR_NS::UsdAttribute get_attribute_or_throw(Amino::String const& prim_path,
                                         const Amino::String& attribute_name,
                                         BifrostUsd::Stage const& stage) {
    auto pxr_prim = get_prim_at_path(prim_path, stage);
    if (!pxr_prim) {
        auto msg = "Invalid prim path: " + std::string(prim_path.c_str());
        throw std::runtime_error(std::move(msg));
    }
    auto pxr_attribute =
        pxr_prim.GetAttribute(PXR_NS::TfToken(attribute_name.c_str()));
    if (!pxr_attribute) {
        auto msg = "Invalid attribute name: " + std::string(prim_path.c_str());
        throw std::runtime_error(std::move(msg));
    }
    return pxr_attribute;
}

template <typename T>
bool set_attribute_metadata_impl(const Amino::String& prim_path,
                                 const Amino::String& attribute_name,
                                 const Amino::String& key,
                                 T&&                  value,
                                 BifrostUsd::Stage& stage) {
    if (!stage) return false;

    try {
        VariantEditContext ctx(stage);
        auto               pxr_attribute =
            get_attribute_or_throw(prim_path, attribute_name, stage);

        return pxr_attribute.SetMetadata(GetSdfFieldKey(key), toPxr(value));

    } catch (std::exception& e) {
        log_exception("set_attribute_metadata", e);
    }
    return false;
}

template <typename T>
bool get_attribute_metadata_impl(const BifrostUsd::Stage& stage,
                                 const Amino::String&       prim_path,
                                 const Amino::String&       attribute_name,
                                 const Amino::String&       key,
                                 const T&                   default_and_type,
                                 T&                         value) {
    if (!stage) return false;

    try {
        VariantEditContext ctx(stage);
        auto               pxr_attribute =
            get_attribute_or_throw(prim_path, attribute_name, stage);
        if (pxr_attribute.GetMetadata(USDUtils::GetSdfFieldKey(key), &value)) {
            return true;
        } else {
            value = default_and_type;
        }
    } catch (std::exception& e) {
        log_exception("get_attribute_metadata", e);
        value = default_and_type;
    }
    return false;
}

} // namespace

//==============================================================================
// NODE DEFINITION IMPLEMENTATIONS
//==============================================================================

bool USD::Attribute::create_prim_attribute(
    BifrostUsd::Stage&                 stage,
    const Amino::String&                 prim_path,
    const Amino::String&                 name,
    const BifrostUsd::SdfValueTypeName type_name,
    const bool                           custom,
    const BifrostUsd::SdfVariability   variablity) {
    if (!stage) return false;

    try {
        VariantEditContext ctx(stage);

        auto pxr_prim = get_prim_at_path(prim_path, stage);
        if (!pxr_prim) return false;

        auto attrib = pxr_prim.CreateAttribute(
            PXR_NS::TfToken(name.c_str()), GetSdfValueTypeName(type_name), custom,
            GetSdfVariability(variablity));
        return attrib.IsValid();

    } catch (std::exception& e) {
        log_exception("create_prim_attribute", e);
    }
    return false;
}

bool USD::Attribute::clear_attribute(BifrostUsd::Stage& stage,
                                     const Amino::String& prim_path,
                                     const Amino::String& name) {
    if (!stage) return false;
    try {
        auto pxr_prim = get_prim_at_path(prim_path, stage);
        if (!pxr_prim) return false;

        auto attrib = pxr_prim.GetAttribute(PXR_NS::TfToken(name.c_str()));
        if (attrib) return attrib.Clear();

    } catch (std::exception& e) {
        log_exception("clear_attribute", e);
    }
    return false;
}

void USD::Attribute::block_attribute(BifrostUsd::Stage& stage,
                                     const Amino::String& prim_path,
                                     const Amino::String& name) {
    if (!stage) return;
    try {
        auto pxr_prim = get_prim_at_path(prim_path, stage);
        if (!pxr_prim) return;

        auto attrib = pxr_prim.GetAttribute(PXR_NS::TfToken(name.c_str()));
        if (attrib) {
            attrib.Block();
        }

    } catch (std::exception& e) {
        log_exception("block_attribute", e);
    }
}

bool USD::Attribute::create_primvar(
    BifrostUsd::Stage&                            stage,
    const Amino::String&                            prim_path,
    const Amino::String&                            name,
    const BifrostUsd::SdfValueTypeName            type_name,
    const BifrostUsd::UsdGeomPrimvarInterpolation interpolation,
    const int                                       element_size) {
    if (!stage) return false;

    try {
        VariantEditContext ctx(stage);

        auto pxr_prim = get_prim_at_path(prim_path, stage);
        if (!pxr_prim) return false;

        auto                primvar_api = PXR_NS::UsdGeomPrimvarsAPI(pxr_prim);
        PXR_NS::UsdGeomPrimvar primvar     = primvar_api.CreatePrimvar(
            PXR_NS::TfToken(name.c_str()), GetSdfValueTypeName(type_name),
            GetUsdGeomPrimvarInterpolation(interpolation), element_size);

        if (primvar) return true;

    } catch (std::exception& e) {
        log_exception("create_primvar", e);
    }
    return false;
}

bool USD::Attribute::get_prim_attribute(
    Amino::Ptr<BifrostUsd::Prim>              prim,
    const Amino::String&                        attribute_name,
    Amino::MutablePtr<BifrostUsd::Attribute>& attribute) {
    assert(prim);
    attribute = Amino::newMutablePtr<BifrostUsd::Attribute>();
    if (!*prim) return false;

    try {
        auto pxr_attribute =
            (*prim)->GetAttribute(PXR_NS::TfToken(attribute_name.c_str()));
        if (!pxr_attribute) return false;

        *attribute = {pxr_attribute, std::move(prim)};
        return true;

    } catch (std::exception& e) {
        log_exception("get_prim_attribute", e);
    }
    return false;
}

namespace {
template <typename DESTTYPE>
bool get_attribute_data(const BifrostUsd::Attribute& attribute,
                        const float                    frame,
                        DESTTYPE&                      value) {
    PxrType_t<DESTTYPE> result;
    bool success = attribute->Get(&result, static_cast<double>(frame));
    value        = fromPxr(result);
    return success;
}
template <>
bool get_attribute_data(const BifrostUsd::Attribute& attribute,
                        const float                    frame,
                        Amino::String&                 value) {
    value          = Amino::String(); // set default
    auto type_name = attribute->GetTypeName();
    if (type_name == PXR_NS::SdfValueTypeNames->Asset) {
        PXR_NS::SdfAssetPath result;
        bool success = attribute->Get(&result, static_cast<double>(frame));
        if (success) value = result.GetAssetPath().c_str();
        return success;
    } else if (type_name == PXR_NS::SdfValueTypeNames->Token) {
        PXR_NS::TfToken result;
        bool success = attribute->Get(&result, static_cast<double>(frame));
        if (success) value = result.GetText();
        return success;
    }
    PxrType_t<Amino::String> result;
    bool success = attribute->Get(&result, static_cast<double>(frame));
    if (success) value = fromPxr(result);
    return success;
}
template <>
bool get_attribute_data(const BifrostUsd::Attribute& attribute,
                        const float                    frame,
                        Amino::Array<Amino::String>&   value) {
    value          = Amino::Array<Amino::String>(); // set default
    auto type_name = attribute->GetTypeName();
    // get as asset array, token array or regular string array
    if (type_name == PXR_NS::SdfValueTypeNames->AssetArray) {
        PXR_NS::VtArray<PXR_NS::SdfAssetPath> result;
        bool success = attribute->Get(&result, static_cast<double>(frame));
        if (success) {
            auto out = Amino::Array<Amino::String>(result.size());
            for (unsigned i = 0; i < result.size(); ++i) {
                out[i] = result[i].GetAssetPath().c_str();
            }
            value = out;
        }
        return success;
    } else if (type_name == PXR_NS::SdfValueTypeNames->TokenArray) {
        PXR_NS::VtTokenArray result;
        bool success = attribute->Get(&result, static_cast<double>(frame));
        if (success) {
            auto out = Amino::Array<Amino::String>(result.size());
            for (unsigned i = 0; i < result.size(); ++i) {
                out[i] = result[i].GetText();
            }
            value = out;
        }
        return success;
    }

    PxrType_t<Amino::Array<Amino::String>> result;
    bool success = attribute->Get(&result, static_cast<double>(frame));
    if (success) value = fromPxr(result);
    return success;
}
template <>
bool get_attribute_data(const BifrostUsd::Attribute& attribute,
                        const float                    frame,
                        Bifrost::Math::float4&         value) {
    value          = Bifrost::Math::float4(); // set default
    auto type_name = attribute->GetTypeName();
    if (type_name == PXR_NS::SdfValueTypeNames->Quatf) {
        PXR_NS::GfQuatf result;
        bool success = attribute->Get(&result, static_cast<double>(frame));
        if (success) {
            auto const& imaginary = result.GetImaginary();
            value.w               = result.GetReal();
            value.x               = imaginary[0];
            value.y               = imaginary[1];
            value.z               = imaginary[2];
        }
        return success;

    } else if (type_name == PXR_NS::SdfValueTypeNames->Quath) {
        PXR_NS::GfQuath result;
        bool success = attribute->Get(&result, static_cast<double>(frame));
        if (success) {
            auto const& imaginary = result.GetImaginary();
            value.w               = result.GetReal();
            value.x               = imaginary[0];
            value.y               = imaginary[1];
            value.z               = imaginary[2];
        }
        return success;
    }
    PxrType_t<Bifrost::Math::float4> result;
    bool success = attribute->Get(&result, static_cast<double>(frame));
    if (success) value = fromPxr(result);
    return success;
}
template <>
bool get_attribute_data(const BifrostUsd::Attribute&       attribute,
                        const float                          frame,
                        Amino::Array<Bifrost::Math::float4>& value) {
    value          = Amino::Array<Bifrost::Math::float4>(); // set default
    auto type_name = attribute->GetTypeName();
    if (type_name == PXR_NS::SdfValueTypeNames->QuatfArray) {
        PXR_NS::VtQuatfArray result;
        bool success = attribute->Get(&result, static_cast<double>(frame));
        if (success) {
            auto out = Amino::Array<Bifrost::Math::float4>(result.size());
            for (unsigned i = 0; i < result.size(); ++i) {
                auto const& src       = result[i];
                auto const& imaginary = src.GetImaginary();
                out[i].w              = src.GetReal();
                out[i].x              = imaginary[0];
                out[i].y              = imaginary[1];
                out[i].z              = imaginary[2];
            }
            value = out;
        }
        return success;
    } else if (type_name == PXR_NS::SdfValueTypeNames->QuathArray) {
        PXR_NS::VtQuathArray result;
        bool success = attribute->Get(&result, static_cast<double>(frame));
        if (success) {
            auto out = Amino::Array<Bifrost::Math::float4>(result.size());
            for (unsigned i = 0; i < result.size(); ++i) {
                auto const& src       = result[i];
                auto const& imaginary = src.GetImaginary();
                out[i].w              = src.GetReal();
                out[i].x              = imaginary[0];
                out[i].y              = imaginary[1];
                out[i].z              = imaginary[2];
            }
            value = out;
        }
        return success;
    }

    PxrType_t<Amino::Array<Bifrost::Math::float4>> result;
    bool success = attribute->Get(&result, static_cast<double>(frame));
    if (success) value = fromPxr(result);
    return success;
}
template <>
bool get_attribute_data(const BifrostUsd::Attribute& attribute,
                        const float                    frame,
                        Bifrost::Math::double4&        value) {
    value          = Bifrost::Math::double4(); // set default
    auto type_name = attribute->GetTypeName();
    if (type_name == PXR_NS::SdfValueTypeNames->Quatd) {
        PXR_NS::GfQuatd result;
        bool success = attribute->Get(&result, static_cast<double>(frame));
        if (success) {
            auto const& imaginary = result.GetImaginary();
            value.w               = result.GetReal();
            value.x               = imaginary[0];
            value.y               = imaginary[1];
            value.z               = imaginary[2];
        }
        return success;
    }

    PxrType_t<Bifrost::Math::double4> result;
    bool success = attribute->Get(&result, static_cast<double>(frame));
    if (success) value = fromPxr(result);
    return success;
}
template <>
bool get_attribute_data(const BifrostUsd::Attribute&        attribute,
                        const float                           frame,
                        Amino::Array<Bifrost::Math::double4>& value) {
    value          = Amino::Array<Bifrost::Math::double4>(); // set default
    auto type_name = attribute->GetTypeName();
    if (type_name == PXR_NS::SdfValueTypeNames->QuatdArray) {
        PXR_NS::VtQuatdArray result;
        bool success = attribute->Get(&result, static_cast<double>(frame));
        if (success) {
            auto out = Amino::Array<Bifrost::Math::double4>(result.size());
            for (unsigned i = 0; i < result.size(); ++i) {
                auto const& src       = result[i];
                auto const& imaginary = src.GetImaginary();
                out[i].w              = src.GetReal();
                out[i].x              = imaginary[0];
                out[i].y              = imaginary[1];
                out[i].z              = imaginary[2];
            }
            value = out;
        }
        return success;
    }

    PxrType_t<Amino::Array<Bifrost::Math::double4>> result;
    bool success = attribute->Get(&result, static_cast<double>(frame));
    if (success) value = fromPxr(result);
    return success;
}
template <typename DESTTYPE>
bool get_prim_attribute_data_impl(const BifrostUsd::Attribute& attribute,
                                  const float                    frame,
                                  DESTTYPE&                      value) {
    if (!attribute) return false;
    try {
        return get_attribute_data(attribute, frame, value);
    } catch (std::exception& e) {
        log_exception("get_prim_attribute_data", e);
    }
    return false;
}
} // namespace

#define IMPLEMENT_GET_PRIM_ATTRIBUTE_DATA(TYPE)                            \
    bool USD::Attribute::get_prim_attribute_data(                          \
        const BifrostUsd::Attribute& attribute, TYPE, const float frame, \
        TYPE& value) {                                                     \
        return get_prim_attribute_data_impl(attribute, frame, value);      \
    }
FOR_EACH_SUPPORTED_BUILTIN_ATTRIBUTE(IMPLEMENT_GET_PRIM_ATTRIBUTE_DATA)
#undef IMPLEMENT_GET_PRIM_ATTRIBUTE_DATA

#define IMPLEMENT_GET_PRIM_ATTRIBUTE_DATA(TYPE)                       \
    bool USD::Attribute::get_prim_attribute_data(                     \
        const BifrostUsd::Attribute& attribute, const TYPE&,        \
        const float frame, TYPE& value) {                             \
        return get_prim_attribute_data_impl(attribute, frame, value); \
    }
FOR_EACH_SUPPORTED_STRUCT_ATTRIBUTE(IMPLEMENT_GET_PRIM_ATTRIBUTE_DATA)
#undef IMPLEMENT_GET_PRIM_ATTRIBUTE_DATA

#define IMPLEMENT_GET_PRIM_ATTRIBUTE_DATA(TYPE)                              \
    bool USD::Attribute::get_prim_attribute_data(                            \
        const BifrostUsd::Attribute& attribute, const Amino::Array<TYPE>&, \
        const float frame, Amino::MutablePtr<Amino::Array<TYPE>>& value) {   \
        value = Amino::newMutablePtr<Amino::Array<TYPE>>();                  \
        return get_prim_attribute_data_impl(attribute, frame, *value);       \
    }
FOR_EACH_SUPPORTED_ARRAY_ATTRIBUTE(IMPLEMENT_GET_PRIM_ATTRIBUTE_DATA)
#undef IMPLEMENT_GET_PRIM_ATTRIBUTE_DATA

namespace {
template <typename TYPE>
bool set_attribute(PXR_NS::UsdAttribute& pxr_attribute,
                   const TYPE&        value,
                   PXR_NS::UsdTimeCode   time) {
    return pxr_attribute.Set(toPxr(value), time);
}
template <>
bool set_attribute(PXR_NS::UsdAttribute&   pxr_attribute,
                   const Amino::String& value,
                   PXR_NS::UsdTimeCode     time) {
    auto type_name = pxr_attribute.GetTypeName();
    // set as asset, token or regular string
    if (type_name == PXR_NS::SdfValueTypeNames->Asset) {
        return pxr_attribute.Set(PXR_NS::SdfAssetPath(value.c_str()), time);
    } else if (type_name == PXR_NS::SdfValueTypeNames->Token) {
        return pxr_attribute.Set(PXR_NS::TfToken(value.c_str()), time);
    }
    return pxr_attribute.Set(toPxr(value), time);
}
template <>
bool set_attribute(PXR_NS::UsdAttribute&                 pxr_attribute,
                   const Amino::Array<Amino::String>& value,
                   PXR_NS::UsdTimeCode                   time) {
    auto type_name = pxr_attribute.GetTypeName();
    // set as asset array, token array or regular string array
    if (type_name == PXR_NS::SdfValueTypeNames->AssetArray) {
        // Create the new attribute
        PXR_NS::VtArray<PXR_NS::SdfAssetPath> pxr_array{value.size()};
        for (size_t i = 0; i < value.size(); i++) {
            pxr_array[i] = PXR_NS::SdfAssetPath(value[i].c_str());
        }
        return pxr_attribute.Set(pxr_array, time);
    } else if (type_name == PXR_NS::SdfValueTypeNames->TokenArray) {
        // Create the new attribute
        PXR_NS::VtTokenArray pxr_array{value.size()};
        for (size_t i = 0; i < value.size(); i++) {
            pxr_array[i] = PXR_NS::TfToken(value[i].c_str());
        }
        return pxr_attribute.Set(pxr_array, time);
    }
    return pxr_attribute.Set(toPxr(value), time);
}
template <>
bool set_attribute(PXR_NS::UsdAttribute&           pxr_attribute,
                   const Bifrost::Math::float4& value,
                   PXR_NS::UsdTimeCode             time) {
    auto type_name = pxr_attribute.GetTypeName();
    // set as quaternion or regular vec4 array
    if (type_name == PXR_NS::SdfValueTypeNames->Quatf) {
        auto pxr_value = PXR_NS::GfQuatf(value.w, value.x, value.y, value.z);
        return pxr_attribute.Set(pxr_value, time);
    } else if (type_name == PXR_NS::SdfValueTypeNames->Quath) {
        auto pxr_value = PXR_NS::GfQuath(value.w, value.x, value.y, value.z);
        return pxr_attribute.Set(pxr_value, time);
    }
    return pxr_attribute.Set(toPxr(value), time);
}
template <>
bool set_attribute(PXR_NS::UsdAttribute&                         pxr_attribute,
                   const Amino::Array<Bifrost::Math::float4>& value,
                   PXR_NS::UsdTimeCode                           time) {
    auto type_name = pxr_attribute.GetTypeName();
    // set as quaternion or regular vec4 array
    if (type_name == PXR_NS::SdfValueTypeNames->QuatfArray) {
        PXR_NS::VtQuatfArray pxr_array{value.size()};
        for (unsigned i = 0; i < value.size(); ++i) {
            auto const& src = value[i];
            pxr_array[i]    = PXR_NS::GfQuatf(src.w, src.x, src.y, src.z);
        }
        return pxr_attribute.Set(pxr_array, time);
    } else if (type_name == PXR_NS::SdfValueTypeNames->QuathArray) {
        PXR_NS::VtQuathArray pxr_array{value.size()};
        for (unsigned i = 0; i < value.size(); ++i) {
            auto const& src = value[i];
            pxr_array[i]    = PXR_NS::GfQuath(src.w, src.x, src.y, src.z);
        }
        return pxr_attribute.Set(pxr_array, time);
    } else if (type_name == PXR_NS::SdfValueTypeNames->Float4Array) {
        return pxr_attribute.Set(toPxr(value), time);
    }
    return false;
}
template <>
bool set_attribute(PXR_NS::UsdAttribute&            pxr_attribute,
                   const Bifrost::Math::double4& value,
                   PXR_NS::UsdTimeCode              time) {
    auto type_name = pxr_attribute.GetTypeName();
    // set as quaternion or regular vec4 array
    if (type_name == PXR_NS::SdfValueTypeNames->Quatd) {
        auto pxr_value = PXR_NS::GfQuatd(value.w, value.x, value.y, value.z);
        return pxr_attribute.Set(pxr_value, time);
    }
    return pxr_attribute.Set(toPxr(value), time);
}
bool set_attribute(PXR_NS::UsdAttribute&                          pxr_attribute,
                   const Amino::Array<Bifrost::Math::double4>& value,
                   PXR_NS::UsdTimeCode                            time) {
    auto type_name = pxr_attribute.GetTypeName();
    // set as quaternion or regular vec4 array
    if (type_name == PXR_NS::SdfValueTypeNames->QuatdArray) {
        PXR_NS::VtQuatdArray pxr_array{value.size()};
        for (unsigned i = 0; i < value.size(); ++i) {
            auto const& src = value[i];
            pxr_array[i]    = PXR_NS::GfQuatd(src.w, src.x, src.y, src.z);
        }
        return pxr_attribute.Set(pxr_array, time);
    }
    return pxr_attribute.Set(toPxr(value), time);
}
template <typename TYPE>
bool set_prim_attribute_impl(const Amino::String& prim_path,
                             const Amino::String& name,
                             const TYPE&          value,
                             const bool           use_frame,
                             const float          frame,
                             BifrostUsd::Stage& stage) {
    if (!stage) return false;
    try {
        VariantEditContext ctx(stage);

        auto pxr_prim = get_prim_at_path(prim_path, stage);
        if (!pxr_prim) return false;
        auto pxr_attribute = pxr_prim.GetAttribute(PXR_NS::TfToken(name.c_str()));
        if (!pxr_attribute) return false;
        auto time = use_frame ? PXR_NS::UsdTimeCode(static_cast<double>(frame))
                              : PXR_NS::UsdTimeCode::Default();
        return set_attribute(pxr_attribute, value, time);
    } catch (std::exception& e) {
        log_exception("get_prim_attribute_data", e);
    }
    return false;
}
} // namespace

/// \cond false
/// Doxygen is confused with those macros...
#define IMPLEMENT_SET_PRIM_ATTRIBUTE(TYPE)                                \
    bool USD::Attribute::set_prim_attribute(                              \
        BifrostUsd::Stage& stage, const Amino::String& prim_path,       \
        const Amino::String& name, TYPE value, const bool use_frame,      \
        const float frame) {                                              \
        return set_prim_attribute_impl(prim_path, name, value, use_frame, \
                                       frame, stage);                     \
    }
FOR_EACH_SUPPORTED_BUILTIN_ATTRIBUTE(IMPLEMENT_SET_PRIM_ATTRIBUTE)
#undef IMPLEMENT_SET_PRIM_ATTRIBUTE

#define IMPLEMENT_SET_PRIM_ATTRIBUTE(TYPE)                                  \
    bool USD::Attribute::set_prim_attribute(                                \
        BifrostUsd::Stage& stage, const Amino::String& prim_path,         \
        const Amino::String& name, const TYPE& value, const bool use_frame, \
        const float frame) {                                                \
        return set_prim_attribute_impl(prim_path, name, value, use_frame,   \
                                       frame, stage);                       \
    }
FOR_EACH_SUPPORTED_STRUCT_ATTRIBUTE(IMPLEMENT_SET_PRIM_ATTRIBUTE)
#undef IMPLEMENT_SET_PRIM_ATTRIBUTE

#define IMPLEMENT_SET_PRIM_ATTRIBUTE(TYPE)                                \
    bool USD::Attribute::set_prim_attribute(                              \
        BifrostUsd::Stage& stage, const Amino::String& prim_path,       \
        const Amino::String& name, const Amino::Array<TYPE>& value,       \
        const bool use_frame, const float frame) {                        \
        return set_prim_attribute_impl(prim_path, name, value, use_frame, \
                                       frame, stage);                     \
    }
FOR_EACH_SUPPORTED_ARRAY_ATTRIBUTE(IMPLEMENT_SET_PRIM_ATTRIBUTE)
#undef IMPLEMENT_SET_PRIM_ATTRIBUTE
/// \endcond

bool USD::Attribute::add_attribute_connection(
    BifrostUsd::Stage&                stage,
    const Amino::String&                prim_path,
    const Amino::String&                attribute_name,
    const Amino::String&                source,
    const BifrostUsd::UsdListPosition position) {
    if (!stage) return false;
    try {
        VariantEditContext ctx(stage);
        auto               pxr_attribute =
            get_attribute_or_throw(prim_path, attribute_name, stage);
        return pxr_attribute.AddConnection(PXR_NS::SdfPath(source.c_str()),
                                           GetUsdListPosition(position));

    } catch (std::exception& e) {
        log_exception("add_attribute_connection", e);
    }
    return false;
}

bool USD::Attribute::remove_attribute_connection(
    BifrostUsd::Stage&   stage,
    const Amino::String& prim_path,
    const Amino::String& attribute_name,
    const Amino::String& source) {
    if (!stage) return false;
    try {
        VariantEditContext ctx(stage);
        auto               pxr_attribute =
            get_attribute_or_throw(prim_path, attribute_name, stage);
        return pxr_attribute.RemoveConnection(PXR_NS::SdfPath(source.c_str()));

    } catch (std::exception& e) {
        log_exception("remove_attribute_connection", e);
    }
    return false;
}

bool USD::Attribute::clear_attribute_connections(
    BifrostUsd::Stage&   stage,
    const Amino::String& prim_path,
    const Amino::String& attribute_name) {
    if (!stage) return false;
    try {
        VariantEditContext ctx(stage);
        auto               pxr_attribute =
            get_attribute_or_throw(prim_path, attribute_name, stage);
        return pxr_attribute.ClearConnections();

    } catch (std::exception& e) {
        log_exception("clear_attribute_connections", e);
    }
    return false;
}

bool USD::Attribute::set_attribute_metadata(BifrostUsd::Stage& stage,
                                            const Amino::String& prim_path,
                                            const Amino::String& attribute_name,
                                            const Amino::String& key,
                                            const Amino::String& value) {
    return set_attribute_metadata_impl(prim_path, attribute_name, key, value,
                                       stage);
}

bool USD::Attribute::set_attribute_metadata(BifrostUsd::Stage& stage,
                                            const Amino::String& prim_path,
                                            const Amino::String& attribute_name,
                                            const Amino::String& key,
                                            const Amino::bool_t& value) {
    return set_attribute_metadata_impl(prim_path, attribute_name, key, value,
                                       stage);
}

bool USD::Attribute::set_attribute_metadata(BifrostUsd::Stage& stage,
                                            const Amino::String& prim_path,
                                            const Amino::String& attribute_name,
                                            const Amino::String& key,
                                            const Amino::float_t& value) {
    return set_attribute_metadata_impl(prim_path, attribute_name, key, value,
                                       stage);
}

bool USD::Attribute::set_attribute_metadata(BifrostUsd::Stage& stage,
                                            const Amino::String& prim_path,
                                            const Amino::String& attribute_name,
                                            const Amino::String& key,
                                            const Amino::double_t& value) {
    return set_attribute_metadata_impl(prim_path, attribute_name, key, value,
                                       stage);
}

bool USD::Attribute::set_attribute_metadata(BifrostUsd::Stage& stage,
                                            const Amino::String& prim_path,
                                            const Amino::String& attribute_name,
                                            const Amino::String& key,
                                            const Amino::int_t&  value) {
    return set_attribute_metadata_impl(prim_path, attribute_name, key, value,
                                       stage);
}

bool USD::Attribute::set_attribute_metadata(BifrostUsd::Stage& stage,
                                            const Amino::String& prim_path,
                                            const Amino::String& attribute_name,
                                            const Amino::String& key,
                                            const Amino::long_t& value) {
    return set_attribute_metadata_impl(prim_path, attribute_name, key, value,
                                       stage);
}

bool USD::Attribute::set_attribute_metadata(BifrostUsd::Stage& stage,
                                            const Amino::String& prim_path,
                                            const Amino::String& attribute_name,
                                            const Amino::String& key,
                                            const Bifrost::Object& value) {
    return set_attribute_metadata_impl(prim_path, attribute_name, key, value,
                                       stage);
}

bool USD::Attribute::get_attribute_metadata(
    const BifrostUsd::Stage& stage,
    const Amino::String&       prim_path,
    const Amino::String&       attribute_name,
    const Amino::String&       key,
    const Amino::String&       default_and_type,
    Amino::String&             value) {
    if (!stage) return false;

    try {
        VariantEditContext ctx(stage);
        auto               pxr_attribute =
            get_attribute_or_throw(prim_path, attribute_name, stage);
        std::string tempVal;
        if (pxr_attribute.GetMetadata(GetSdfFieldKey(key), &tempVal)) {
            value = tempVal.c_str();
            return true;
        } else {
            value = default_and_type;
        }
    } catch (std::exception& e) {
        log_exception("get_attribute_metadata", e);
        value = default_and_type;
    }
    return false;
}

bool USD::Attribute::get_attribute_metadata(
    const BifrostUsd::Stage& stage,
    const Amino::String&       prim_path,
    const Amino::String&       attribute_name,
    const Amino::String&       key,
    const Amino::bool_t&       default_and_type,
    Amino::bool_t&             value) {
    return get_attribute_metadata_impl(stage, prim_path, attribute_name, key,
                                       default_and_type, value);
}

bool USD::Attribute::get_attribute_metadata(
    const BifrostUsd::Stage& stage,
    const Amino::String&       prim_path,
    const Amino::String&       attribute_name,
    const Amino::String&       key,
    const Amino::float_t&      default_and_type,
    Amino::float_t&            value) {
    return get_attribute_metadata_impl(stage, prim_path, attribute_name, key,
                                       default_and_type, value);
}

bool USD::Attribute::get_attribute_metadata(
    const BifrostUsd::Stage& stage,
    const Amino::String&       prim_path,
    const Amino::String&       attribute_name,
    const Amino::String&       key,
    const Amino::double_t&     default_and_type,
    Amino::double_t&           value) {
    return get_attribute_metadata_impl(stage, prim_path, attribute_name, key,
                                       default_and_type, value);
}

bool USD::Attribute::get_attribute_metadata(
    const BifrostUsd::Stage& stage,
    const Amino::String&       prim_path,
    const Amino::String&       attribute_name,
    const Amino::String&       key,
    const Amino::int_t&        default_and_type,
    Amino::int_t&              value) {
    return get_attribute_metadata_impl(stage, prim_path, attribute_name, key,
                                       default_and_type, value);
}

bool USD::Attribute::get_attribute_metadata(
    const BifrostUsd::Stage& stage,
    const Amino::String&       prim_path,
    const Amino::String&       attribute_name,
    const Amino::String&       key,
    const Amino::long_t&       default_and_type,
    Amino::long_t&             value) {
    return get_attribute_metadata_impl(stage, prim_path, attribute_name, key,
                                       default_and_type, value);
}

bool USD::Attribute::get_attribute_metadata(
    const BifrostUsd::Stage&         stage,
    const Amino::String&               prim_path,
    const Amino::String&               attribute_name,
    const Amino::String&               key,
    const Amino::Ptr<Bifrost::Object>& default_and_type,
    Amino::Ptr<Bifrost::Object>&       value) {
    auto value_returns = createReturnGuard(
        value, [&default_and_type]() { return default_and_type; });

    if (!stage) return false;

    try {
        VariantEditContext ctx(stage);
        auto               pxr_attribute =
            get_attribute_or_throw(prim_path, attribute_name, stage);
        PXR_NS::VtDictionary temp;
        if (pxr_attribute.GetMetadata(GetSdfFieldKey(key), &temp)) {
            value = fromPxr(temp);
            return true;
        } else {
            value = default_and_type;
        }

    } catch (std::exception& e) {
        log_exception("get_attribute_metadata", e);
    }

    return false;
}
