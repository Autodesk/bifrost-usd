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

#include "usd_material_binding_nodedefs.h"

#include <Amino/Core/String.h>

#include "usd_type_converter.h"
#include "usd_utils.h"

// Note: To silence warnings coming from USD library
#include <bifusd/config/CfgWarningMacros.h>
BIFUSD_WARNING_PUSH

BIFUSD_WARNING_DISABLE_MSC(4003)
BIFUSD_WARNING_DISABLE_MSC(4267)

#include <pxr/usd/usdShade/materialBindingAPI.h>

BIFUSD_WARNING_POP


using namespace USDUtils;
using namespace USDTypeConverters;

namespace {} // namespace

bool USD::Shading::get_material_path(
    const BifrostUsd::Prim&            prim,
    const BifrostUsd::MaterialPurpose  material_purpose,
    const bool                         compute_bound_material,
    Amino::String&                     path) {

    auto materialBindingAPI =
        PXR_NS::UsdShadeMaterialBindingAPI::Apply(prim.getPxrPrim());
    if (materialBindingAPI) {
        auto materialPurpose = USDUtils::GetMaterialPurpose(material_purpose);
        auto materialPath    = PXR_NS::SdfPath{};
        if (compute_bound_material) {
            materialPath =
                materialBindingAPI.ComputeBoundMaterial(materialPurpose)
                    .GetPath();
        } else {
            materialPath = materialBindingAPI.GetDirectBinding(materialPurpose)
                               .GetMaterialPath();
        }
        path = materialPath.GetText();
        return !materialPath.IsEmpty();
    }
    return false;
}

bool USD::Shading::bind_material(
    BifrostUsd::Stage& stage                  USDPORT_INOUT("out_stage"),
    const Amino::String&                      prim_path,
    const Amino::String&                      material_path,
    const BifrostUsd::MaterialBindingStrength binding_strength,
    const BifrostUsd::MaterialPurpose         material_purpose,
    const Amino::String&                      collection_prim_path,
    const Amino::String&                      collection_name,
    const Amino::String&                      binding_name)
{
    if (!stage) return false;

    try {
        auto pxr_geo_prim = USDUtils::get_prim_or_throw(prim_path, stage);
        auto pxr_mat_prim = USDUtils::get_prim_or_throw(material_path, stage);

        VariantEditContext ctx(stage);

        auto material = PXR_NS::UsdShadeMaterial::Get(stage.getStagePtr(),
                                                   pxr_mat_prim.GetPath());
        if (!material) {
            auto msg = "material_path " + pxr_mat_prim.GetPath().GetString() +
                       " is not a UsdShadeMaterial";
            throw std::runtime_error(std::move(msg));
        }

        auto materialPurpose = USDUtils::GetMaterialPurpose(material_purpose);

        auto materialBindingAPI =
            PXR_NS::UsdShadeMaterialBindingAPI::Apply(pxr_geo_prim);
        if (collection_name.empty()) {
            return materialBindingAPI.Bind(
                material,
                USDUtils::GetMaterialBindingStrength(binding_strength),
                materialPurpose);
        } else {
            auto pxr_collection_prim =
                collection_prim_path.empty()
                    ? pxr_geo_prim
                    : USDUtils::get_prim_or_throw(collection_prim_path, stage);
            auto collectionAPI = PXR_NS::UsdCollectionAPI::Get(
                pxr_collection_prim, PXR_NS::TfToken{collection_name.c_str()});
            if (!collectionAPI) {
                auto msg =
                    "No collection " + std::string{collection_name.c_str()} +
                    " found on prim " + pxr_collection_prim.GetPath().GetString();
                throw std::runtime_error(std::move(msg));
            }

            return materialBindingAPI.Bind(
                collectionAPI, material, PXR_NS::TfToken{binding_name.c_str()},
                USDUtils::GetMaterialBindingStrength(binding_strength),
                materialPurpose);
        }

    } catch (std::exception& e) {
        log_exception("USD::Shading::bind_material", e);
    }
    return false;
}

bool USD::Shading::unbind_material(
    BifrostUsd::Stage& stage          USDPORT_INOUT("out_stage"),
    const Amino::String&              prim_path,
    const BifrostUsd::MaterialPurpose material_purpose,
    const Amino::String&              binding_name)

{
    if (!stage) return false;

    try {
        auto pxr_geo_prim = USDUtils::get_prim_or_throw(prim_path, stage);

        VariantEditContext ctx(stage);
        auto               materialBindingAPI =
            PXR_NS::UsdShadeMaterialBindingAPI::Apply(pxr_geo_prim);

        auto materialPurpose = USDUtils::GetMaterialPurpose(material_purpose);
        if (binding_name.empty()) {
            return materialBindingAPI.UnbindDirectBinding(materialPurpose);
        } else {
            return materialBindingAPI.UnbindCollectionBinding(
                PXR_NS::TfToken{binding_name.c_str()}, materialPurpose);
        }
    } catch (std::exception& e) {
        log_exception("USD::Shading::unbind_material", e);
    }

    return false;
}
