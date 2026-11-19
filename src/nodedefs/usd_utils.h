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

#ifndef ADSK_USD_UTILS_H
#define ADSK_USD_UTILS_H

#include "logger.h"
#include "nodedef_export.h"

#include <Amino/Core/Array.h>
#include <Bifrost/Math/Types.h>
#include <Bifrost/Object/Object.h>
#include <BifrostUsd/Enum.h>
#include <BifrostUsd/Stage.h>

// Note: To silence warnings coming from USD library
#include <bifusd/config/CfgWarningMacros.h>

#include <bifusd/config/CfgCompilerMacros.h> // for BIFUSD_IS_MSC

#include <iostream>
#include <utility>

BIFUSD_WARNING_PUSH
BIFUSD_WARNING_DISABLE_MSC(4003)
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <pxr/usd/usdShade/tokens.h>
#include <pxr/usd/usdVol/volume.h>

BIFUSD_WARNING_POP

namespace USDUtils {

PXR_NS::UsdListPosition GetUsdListPosition(
    const BifrostUsd::UsdListPosition position);

PXR_NS::UsdGeomXformCommonAPI::RotationOrder GetUsdRotationOrder(
    const Bifrost::Math::rotation_order order);

Bifrost::Math::rotation_order GetRotationOrder(
    const PXR_NS::UsdGeomXformCommonAPI::RotationOrder order);

PXR_NS::GfVec3d GetVec3d(const Bifrost::Math::double3& vec);

PXR_NS::GfVec3f GetVec3f(const Bifrost::Math::float3& vec);

Amino::String ToString(const Bifrost::Math::float3& vec);

Amino::String ToString(const Bifrost::Math::double3& vec);

template <class AMINOTYPE, class USDTYPE>
void copy_array(const AMINOTYPE& src, USDTYPE& dest) {
    dest.resize(src.size());
    for (size_t i = 0; i < src.size(); ++i) {
        dest[i] = src[i];
    }
}

void copy_array(const Amino::Array<Bifrost::Math::float3>& src,
                PXR_NS::VtVec3fArray&                         dest);
void copy_array(const Amino::Array<Bifrost::Math::float4>& src,
                PXR_NS::VtVec4fArray&                         dest);

void copy_array(const Amino::Array<Bifrost::Math::float4>& src,
                PXR_NS::VtQuathArray&                         dest);

template <class VOLUME_FIELD_ASSET_TYPE>
void set_volume_field_relationship(VOLUME_FIELD_ASSET_TYPE& fieldPrim,
                                   const PXR_NS::TfToken&      field_name,
                                   const PXR_NS::TfToken&      relationship_name,
                                   const PXR_NS::SdfAssetPath& field_path,
                                   PXR_NS::UsdVolVolume&       volume,
                                   const double             frame) {
    auto fieldNameAttr = fieldPrim.CreateFieldNameAttr();
    fieldNameAttr.Set(field_name, frame);

    auto filePathAttr = fieldPrim.CreateFilePathAttr();
    filePathAttr.Set(field_path, frame);

    volume.CreateFieldRelationship(relationship_name, fieldPrim.GetPath());
}

USD_NODEDEF_DECL PXR_NS::UsdPrim get_prim_at_path(const Amino::String&     path,
                                                  const BifrostUsd::Stage& stage);

USD_NODEDEF_DECL PXR_NS::UsdPrim get_prim_or_throw(Amino::String const&     prim_path,
                                                   BifrostUsd::Stage const& stage);

USD_NODEDEF_DECL Amino::String resolve_prim_path(const Amino::String&     path,
                                                 const BifrostUsd::Stage& stage);

PXR_NS::SdfVariability GetSdfVariability(
    const BifrostUsd::SdfVariability variablity);

PXR_NS::SdfValueTypeName GetSdfValueTypeName(
    const BifrostUsd::SdfValueTypeName type_name);

bool SetSdfValueTypeName(const PXR_NS::SdfValueTypeName& pxr_type_name,
                         BifrostUsd::SdfValueTypeName&   type_name);

PXR_NS::TfToken GetUsdGeomPrimvarInterpolation(
    const BifrostUsd::UsdGeomPrimvarInterpolation interpolation);

PXR_NS::TfToken GetSdfFieldKey(const Amino::String& key);

// --- USD metadata write helpers ----------------------------------------------
//
// As of USD 0.26.05, UsdObject::SetMetadata()/SetMetadataByDictKey() take a
// non-owning VtValueRef instead of a `const VtValue&`. When the VtValue is
// handed over as an rvalue (VtValueRef::IsRValue()==true), USD extracts the
// payload by MOVE-constructing the held object *inside the USD binary*
// (VtValueRef::UncheckedRemove() -> std::move of the stored object). Passing an
// lvalue selects VtValue::operator VtValueRef() const& (IsRValue()==false), so
// USD COPIES the payload instead.
//
// The move is the cheaper path and is what we want for large payloads (e.g. a
// customData VtDictionary built from a Bifrost::Object) -- EXCEPT on the MSVC
// debug STL:
//
//   * On Windows this pack consumes USD as separate DLLs and compiles its STL
//     with iterator debugging on (_ITERATOR_DEBUG_LEVEL=1, see
//     cmake/bifusd/compiler_Windows.cmake). Every std::map -- and VtDictionary
//     IS a std::map -- then carries a heap-allocated _Container_proxy.
//   * Move-constructing such a map rewires that proxy. When USD (a separate
//     binary, built with its own toolset / iterator-debug level) runs that
//     rewiring on a proxy allocated by this pack's module, it is a cross-module
//     access violation (0xc0000005). But a COPY is safe: USD rebuilds the
//     container in its own module/heap.
//
// This is purely an MSVC-debug-STL problem. It does NOT exist on Linux/macOS
// (gcc/clang, one shared C++ runtime, no cross-module iterator-proxy ownership;
// _ITERATOR_DEBUG_LEVEL is an MSVC concept), nor in optimized MSVC builds
// (_ITERATOR_DEBUG_LEVEL==0 -> no proxies).
// So: (1) move when safe; (2) copy only on MSVC when _ITERATOR_DEBUG_LEVEL != 0
//
#if !defined(BIFUSD_IS_MSC)
#error "BIFUSD_IS_MSC must always be defined!"
#endif
#if BIFUSD_IS_MSC && !defined(_ITERATOR_DEBUG_LEVEL)
#error "_ITERATOR_DEBUG_LEVEL must be defined under MSVC before usd_utils.h!"
#endif
#if BIFUSD_IS_MSC && (_ITERATOR_DEBUG_LEVEL != 0)
inline constexpr bool kUsdMetadataMoveIsSafe = false;
#else
inline constexpr bool kUsdMetadataMoveIsSafe = true;
#endif

/// Set metadata on \p dest, moving \p value into USD when that is safe and
/// copying it otherwise. See kUsdMetadataMoveIsSafe above for the rationale.
/// Templated on the USD type so it works for anything exposing
/// SetMetadata(key, VtValueRef), and on the value type so callers can pass the
/// raw toPxr() result directly: VtValue's per-type constructor is explicit, so
/// it is direct-initialized here rather than converted implicitly at the call.
template <class USDTYPE, class VALUE>
bool setMetadata(const USDTYPE&         dest,
                 const PXR_NS::TfToken& key,
                 VALUE&&                value) {
    PXR_NS::VtValue vtValue(std::forward<VALUE>(value));
    if constexpr (kUsdMetadataMoveIsSafe) {
        return dest.SetMetadata(key, std::move(vtValue));
    } else {
        return dest.SetMetadata(key, vtValue); // copy: safe on MSVC debug STL
    }
}

/// Set metadata by dictionary key on \p dest, moving \p value into USD when
/// that is safe and copying it otherwise. See kUsdMetadataMoveIsSafe above.
template <class USDTYPE, class VALUE>
bool setMetadataByDictKey(const USDTYPE&         dest,
                          const PXR_NS::TfToken& key,
                          const PXR_NS::TfToken& keyPath,
                          VALUE&&                value) {
    PXR_NS::VtValue vtValue(std::forward<VALUE>(value));
    if constexpr (kUsdMetadataMoveIsSafe) {
        return dest.SetMetadataByDictKey(key, keyPath, std::move(vtValue));
    } else {
        return dest.SetMetadataByDictKey(key, keyPath, vtValue);
    }
}

/// Read USD metadata into a VtValue, honoring the cross-DLL ownership rule.
///
/// When we use the GetMetadata(key, T*) template to fill-in data that requires
/// USD to allocate memory (like VtDictionary, VtArray types etc.) into a local
/// variable, then when the local variable goes out of scope, the pack's var
/// destructor deletes memory through pack's CRT -> a cross-DLL memory deletion
/// that can crash on MSVC.
///
/// By comparison, GetMetadata(key, VtValue*) is a non-template USD_API
/// function. USD's binary allocates data stored in the VtValue (a VtDictionary,
/// VtArray etc.) and registers its destructor -> a fct ptr that lives in USD's
/// binary and calls USD's delete operator. This way, USD deallocates what it
/// has allocated.
///
/// IMPORTANT: access the dictionary only via const ref from UncheckedGet<>().
/// Never extract a VtDictionary by value from the returned VtValue, to make
/// sure USD continues to deallocate the memory it allocated.
template <class USDTYPE>
bool getMetadataAsVtValue(const USDTYPE&         source,
                          const PXR_NS::TfToken& key,
                          PXR_NS::VtValue&       outValue) {
    return source.GetMetadata(key, &outValue);
}

PXR_NS::VtDictionary BifrostObjectToVtDictionary(const Bifrost::Object& object);

auto VtDictionaryToBifrostObject(const PXR_NS::VtDictionary& dict)
    -> decltype(Bifrost::createObject());

USD_NODEDEF_DECL
size_t reversedSublayerIndex(const size_t index, const size_t numLayers);
USD_NODEDEF_DECL
int reversedSublayerIndex(const int index, const int numLayers);

PXR_NS::TfToken GetImageablePurpose(const BifrostUsd::ImageablePurpose purpose);

PXR_NS::TfToken GetMaterialBindingStrength(const BifrostUsd::MaterialBindingStrength strength);

PXR_NS::TfToken GetMaterialPurpose(const BifrostUsd::MaterialPurpose purpose);

PXR_NS::TfToken GetExpansionRule(const BifrostUsd::ExpansionRule rule);

} // namespace USDUtils

#endif // ADSK_USD_UTILS_H
