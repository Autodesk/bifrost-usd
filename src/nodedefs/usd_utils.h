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
/// \file  usd_utils.h
/// \brief Bifrost USD utilitites.

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

class VariantEditContext {
public:
    explicit VariantEditContext(const BifrostUsd::Stage& stage) {
        auto variantSet = stage.getLastModifedVariantSet();
        if (variantSet) {
            auto variantEditCtx = variantSet.GetVariantEditContext();
            if (variantEditCtx.second.IsValid()) {
                new (&m_ctx) Ctx(variantSet.GetVariantEditContext());
                m_hasContext = true;
            }
        }
    }
    ~VariantEditContext() {
        if (m_hasContext) reinterpret_cast<Ctx*>(&m_ctx)->~Ctx();
    }

    VariantEditContext(VariantEditContext const&) = delete;
    VariantEditContext(VariantEditContext&&)      = delete;
    VariantEditContext& operator=(VariantEditContext const&) = delete;
    VariantEditContext& operator=(VariantEditContext&&) = delete;

private:
    using Ctx          = PXR_NS::UsdEditContext;
    using storage_type = std::aligned_storage_t<sizeof(Ctx), alignof(Ctx)>;

    // home grown optional, should use std::optional when available.
    storage_type m_ctx;
    bool         m_hasContext = false;
};

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

PXR_NS::UsdPrim get_prim_at_path(const Amino::String&       path,
                              const BifrostUsd::Stage& stage);

PXR_NS::UsdPrim get_prim_or_throw(Amino::String const&     prim_path,
                               BifrostUsd::Stage const& stage);

Amino::String resolve_prim_path(const Amino::String&       path,
                                const BifrostUsd::Stage& stage);

PXR_NS::SdfVariability GetSdfVariability(
    const BifrostUsd::SdfVariability variablity);

PXR_NS::SdfValueTypeName GetSdfValueTypeName(
    const BifrostUsd::SdfValueTypeName type_name);

PXR_NS::TfToken GetUsdGeomPrimvarInterpolation(
    const BifrostUsd::UsdGeomPrimvarInterpolation interpolation);

PXR_NS::TfToken GetSdfFieldKey(const Amino::String& key);

PXR_NS::VtDictionary BifrostObjectToVtDictionary(const Bifrost::Object& object);

auto VtDictionaryToBifrostObject(const PXR_NS::VtDictionary& dict)
    -> decltype(Bifrost::createObject());

/// The sublayer indices in Bifrost USD nodedef functions (where 0
/// designates the WEAKEST sublayer) are reversed compared to sublayer
/// indices of a Pixar Layer stack (where 0 designates the STRONGEST
/// sublayer). This function converts a sublayer index from a Bifrost
/// USD nodedef function to an index for a Pixar Layer stack,
/// or vice-versa.
///
/// \param [in] index The index of the sublayer to reverse.
///     This index must be within [0, numLayers-1]. 
/// \param [in] numLayers The number of sublayers in parent layer.
///     It must be greater or equal to 0.
/// \return The converted sublayer index.
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
