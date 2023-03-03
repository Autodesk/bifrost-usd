//-
// Copyright 2023 Autodesk, Inc.
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

#include "usd_prim_nodedefs.h"

#include <Amino/Core/String.h>
#include <Bifrost/FileUtils/FileUtils.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/inherits.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/specializes.h>

#include "return_guard.h"
#include "usd_type_converter.h"
#include "usd_utils.h"

// Note: To silence warnings coming from USD library
#include <bifusd/config/CfgWarningMacros.h>
BIFUSD_WARNING_PUSH
BIFUSD_WARNING_DISABLE_MSC(4003)
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/payloads.h>
#include <pxr/usd/usdGeom/imageable.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <pxr/usd/usdVol/field3DAsset.h>
#include <pxr/usd/usdVol/openVDBAsset.h>
#include <pxr/usd/usdVol/volume.h>
BIFUSD_WARNING_POP

using namespace USDUtils;
using namespace USDTypeConverters;

namespace {

template <typename T>
bool set_prim_metadata_impl(const Amino::String& path,
                            const Amino::String& key,
                            T&&                  value,
                            BifrostUsd::Stage& stage) {
    if (!stage) return false;

    try {
        auto pxr_prim = USDUtils::get_prim_or_throw(path, stage);

        VariantEditContext ctx(stage);

        return pxr_prim.SetMetadata(GetSdfFieldKey(key), toPxr(value));

    } catch (std::exception& e) {
        log_exception("set_prim_metadata", e);
    }
    return false;
}

template <typename T>
bool get_prim_metadata_impl(const BifrostUsd::Stage& stage,
                            const Amino::String&       path,
                            const Amino::String&       key,
                            const T&                   default_and_type,
                            T&                         value) {
    if (!stage) return false;

    try {
        auto               pxr_prim = USDUtils::get_prim_or_throw(path, stage);
        VariantEditContext ctx(stage);
        auto               pxr_key = GetSdfFieldKey(key);
        PxrType_t<T>       pxrValue;
        if (pxr_prim.GetMetadata(pxr_key, &pxrValue)) {
            value = fromPxr(pxrValue);
            return true;
        } else {
            value = default_and_type;
        }
    } catch (std::exception& e) {
        log_exception("get_prim_metadata", e);
        value = default_and_type;
    }
    return false;
}

} // namespace

bool USD::Prim::get_prim_at_path(Amino::Ptr<BifrostUsd::Stage>        stage,
                                 const Amino::String&                   path,
                                 Amino::MutablePtr<BifrostUsd::Prim>& prim) {
    assert(stage);
    prim = Amino::newMutablePtr<BifrostUsd::Prim>();
    assert(!*prim);
    if (!*stage) return false;

    try {
        pxr::SdfPath sdf_path(path.c_str());
        auto         pxr_prim = USDUtils::get_prim_at_path(path, *stage);
        *prim                 = BifrostUsd::Prim{pxr_prim, std::move(stage)};
        return pxr_prim.IsValid();

    } catch (std::exception& e) {
        log_exception("get_prim_at_path", e);
    }

    assert(prim);
    return false;
}

void USD::Prim::get_prim_children(
    Amino::Ptr<BifrostUsd::Stage>        stage,
    const Amino::String&                   prim_path,
    const BifrostUsd::PrimDescendantMode descendant_mode,
    Amino::MutablePtr<Amino::Array<Amino::Ptr<BifrostUsd::Prim>>>& children) {
    assert(stage);
    children =
        Amino::newMutablePtr<Amino::Array<Amino::Ptr<BifrostUsd::Prim>>>();
    if (!stage) return;

    try {
        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, *stage);
        if (!pxr_prim) return;

        auto setChildren = [&children, &stage](auto const& usd_children) {
            for (const auto& child_prim : usd_children) {
                (*children).push_back(
                    Amino::newClassPtr<BifrostUsd::Prim>(child_prim, stage));
            }
        };

        // Use a switch. If BifrostUsd::PrimDescendantMode were to support
        // more enum kinds, this switch compilation would fail (because of
        // missing cases) which would prompt the developper to implement the
        // missing cases.
        switch (descendant_mode) {
            case BifrostUsd::UsdPrimChildren:
                setChildren(pxr_prim.GetChildren());
                break;
            case BifrostUsd::UsdPrimAllChildren:
                setChildren(pxr_prim.GetAllChildren());
                break;
            case BifrostUsd::UsdPrimDescendants:
                setChildren(pxr_prim.GetDescendants());
                break;
            case BifrostUsd::UsdPrimAllDescendants:
                setChildren(pxr_prim.GetAllDescendants());
                break;
        }
    } catch (std::exception& e) {
        log_exception("get_prim_children", e);
    }
}

void USD::Prim::get_prim_path(const BifrostUsd::Prim& prim,
                              Amino::String&            path) {
    try {
        if (prim) {
            path = prim->GetPrimPath().GetText();
        }
    } catch (std::exception& e) {
        log_exception("get_prim_path", e);
    }
}

bool USD::Prim::get_last_modified_prim(
    Amino::Ptr<BifrostUsd::Stage>        stage,
    Amino::MutablePtr<BifrostUsd::Prim>& prim) {
    assert(stage);
    prim = Amino::newMutablePtr<BifrostUsd::Prim>();
    if (!*stage) return false;

    try {
        Amino::String const& primPath = stage->last_modified_prim;
        if (primPath.empty()) return false;

        auto pxr_prim = (*stage)->GetPrimAtPath(pxr::SdfPath(primPath.c_str()));
        if (!pxr_prim.IsValid()) return false;

        *prim = BifrostUsd::Prim{pxr_prim, std::move(stage)};

    } catch (std::exception& e) {
        log_exception("get_last_modified_prim", e);
    }
    return false;
}

void USD::Prim::get_prim_type(const BifrostUsd::Prim& prim,
                              Amino::String&            type_name) {
    try {
        if (prim) {
            type_name = prim->GetTypeName().GetText();
        }
    } catch (std::exception& e) {
        log_exception("get_prim_type", e);
    }
}

void USD::Prim::get_all_attribute_names(const BifrostUsd::Prim&                         prim,
                                        Amino::MutablePtr<Amino::Array<Amino::String>>& names) {
    names = Amino::newMutablePtr<Amino::Array<Amino::String>>();
    try {
        if (prim) {
            std::vector<pxr::UsdAttribute> usdAttributes = prim->GetAttributes();
            names->resize(usdAttributes.size());
            for (size_t i = 0; i < usdAttributes.size(); ++i) {
                (*names)[i] = usdAttributes[i].GetName().GetText();
            }
        }
    } catch (std::exception& e) {
        log_exception("get_all_attribute_names", e);
    }
}

void USD::Prim::get_authored_attribute_names(
    const BifrostUsd::Prim&                         prim,
    Amino::MutablePtr<Amino::Array<Amino::String>>& names) {
    names = Amino::newMutablePtr<Amino::Array<Amino::String>>();
    try {
        if (prim) {
            std::vector<pxr::UsdAttribute> usdAttributes =
                prim->GetAuthoredAttributes();
            names->resize(usdAttributes.size());
            for (size_t i = 0; i < usdAttributes.size(); ++i) {
                (*names)[i] = usdAttributes[i].GetName().GetText();
            }
        }
    } catch (std::exception& e) {
        log_exception("get_authored_attribute_names", e);
    }
}

void USD::Prim::create_prim(BifrostUsd::Stage& stage,
                            const Amino::String& path,
                            const Amino::String& type) {
    if (!stage || path.empty()) {
        return;
    }

    try {
        VariantEditContext ctx(stage);

        Amino::String resolvedPath = USDUtils::resolve_prim_path(path, stage);
        pxr::SdfPath  sdf_path(resolvedPath.c_str());
        stage->DefinePrim(sdf_path, pxr::TfToken(type.c_str()));
        stage.last_modified_prim = resolvedPath;

    } catch (std::exception& e) {
        log_exception("create_prim", e);
    }
}

void USD::Prim::create_class_prim(BifrostUsd::Stage& stage,
                                  const Amino::String& path) {
    if (!stage) return;

    try {
        VariantEditContext ctx(stage);

        Amino::String resolvedPath = USDUtils::resolve_prim_path(path, stage);
        pxr::SdfPath  sdf_path(resolvedPath.c_str());
        stage->CreateClassPrim(sdf_path);
        stage.last_modified_prim = resolvedPath;

    } catch (std::exception& e) {
        log_exception("create_class_prim", e);
    }
}

void USD::Prim::override_prim(BifrostUsd::Stage& stage,
                              const Amino::String& path) {
    if (!stage) return;

    try {
        VariantEditContext ctx(stage);

        Amino::String resolvedPath = USDUtils::resolve_prim_path(path, stage);
        pxr::SdfPath  sdf_path(resolvedPath.c_str());
        stage->OverridePrim(sdf_path);
        stage.last_modified_prim = resolvedPath;

    } catch (std::exception& e) {
        log_exception("override_prim", e);
    }
}

bool USD::Prim::add_applied_schema(BifrostUsd::Stage&   stage,
                                   const Amino::String& prim_path,
                                   const Amino::String& applied_schema_name) {
    if (!stage) return false;
    try {
        auto pxr_prim = USDUtils::get_prim_or_throw(prim_path, stage);
        VariantEditContext ctx(stage);
        stage.last_modified_prim = pxr_prim.GetPath().GetText();
        return pxr_prim.AddAppliedSchema(
            pxr::TfToken(applied_schema_name.c_str()));
    } catch (std::exception& e) {
        log_exception("add_applied_schema", e);
    }
    return false;
}

bool USD::Prim::remove_applied_schema(BifrostUsd::Stage&   stage,
                                   const Amino::String& prim_path,
                                   const Amino::String& applied_schema_name) {
    if (!stage) return false;
    try {
        auto pxr_prim = USDUtils::get_prim_or_throw(prim_path, stage);
        VariantEditContext ctx(stage);
        stage.last_modified_prim = pxr_prim.GetPath().GetText();
        return pxr_prim.RemoveAppliedSchema(
            pxr::TfToken(applied_schema_name.c_str()));
    } catch (std::exception& e) {
        log_exception("remove_applied_schema", e);
    }
    return false;
}

bool USD::Prim::add_reference_prim(
    BifrostUsd::Stage&                stage,
    const Amino::String&                prim_path,
    const BifrostUsd::Layer&          reference_layer,
    const Amino::String&                reference_prim_path,
    const double                        layer_offset,
    const double                        layer_scale,
    const BifrostUsd::UsdListPosition reference_position) {
    if (!stage) return false;
    try {
        auto pxr_prim = USDUtils::get_prim_or_throw(prim_path, stage);

        VariantEditContext ctx(stage);

        stage.last_modified_prim = pxr_prim.GetPath().GetText();
        if (!reference_layer.isValid()) {
            pxr::SdfPath ref_prim_path(reference_prim_path.c_str());
            return pxr_prim.GetReferences().AddInternalReference(
                ref_prim_path, pxr::SdfLayerOffset(layer_offset, layer_scale),
                GetUsdListPosition(reference_position));
        } else {
            std::string identifier = reference_layer->GetIdentifier().c_str();
            if (reference_prim_path.empty()) {
                return pxr_prim.GetReferences().AddReference(
                    identifier, pxr::SdfLayerOffset(layer_offset, layer_scale),
                    GetUsdListPosition(reference_position));
            } else {
                pxr::SdfPath ref_prim_path(reference_prim_path.c_str());
                return pxr_prim.GetReferences().AddReference(
                    identifier, ref_prim_path,
                    pxr::SdfLayerOffset(layer_offset, layer_scale),
                    GetUsdListPosition(reference_position));
            }
        }

    } catch (std::exception& e) {
        log_exception("add_reference_prim", e);
    }
    return false;
}

bool USD::Prim::remove_reference_prim(
    BifrostUsd::Stage& stage,
    const Amino::String& prim_path,
    const Amino::String& reference_layer_identifier,
    const Amino::String& reference_prim_path,
    const double         layer_offset,
    const bool           clear_all) {
    if (!stage) return false;

    try {
        auto pxr_prim = USDUtils::get_prim_or_throw(prim_path, stage);

        VariantEditContext ctx(stage);

        stage.last_modified_prim = pxr_prim.GetPath().GetText();

        if (clear_all) {
            return pxr_prim.GetReferences().ClearReferences();
        } else {
            auto pxr_reference_prim_path = pxr::SdfPath::EmptyPath();
            if (!reference_prim_path.empty()) {
                pxr_reference_prim_path =
                    pxr::SdfPath(reference_prim_path.c_str());
            }
            auto ref = pxr::SdfReference(reference_layer_identifier.c_str(),
                                         pxr_reference_prim_path,
                                         pxr::SdfLayerOffset(layer_offset));
            return pxr_prim.GetReferences().RemoveReference(ref);
        }
    } catch (std::exception& e) {
        log_exception("remove_reference_prim", e);
    }
    return false;
}

bool USD::Prim::remove_payload_prim(
    BifrostUsd::Stage& stage,
    const Amino::String& prim_path,
    const Amino::String& payload_layer_identifier,
    const Amino::String& payload_prim_path,
    const double         layer_offset,
    const bool           clear_all) {
    if (!stage) return false;
    try {
        auto pxr_prim = USDUtils::get_prim_or_throw(prim_path, stage);

        VariantEditContext ctx(stage);

        stage.last_modified_prim = pxr_prim.GetPath().GetText();

        if (clear_all) {
            return pxr_prim.GetPayloads().ClearPayloads();
        } else {
            auto pxr_payload_prim_path = pxr::SdfPath::EmptyPath();
            if (!payload_prim_path.empty()) {
                pxr_payload_prim_path = pxr::SdfPath(payload_prim_path.c_str());
            }
            auto ref = pxr::SdfPayload(payload_layer_identifier.c_str(),
                                       pxr_payload_prim_path,
                                       pxr::SdfLayerOffset(layer_offset));
            return pxr_prim.GetPayloads().RemovePayload(ref);
        }

    } catch (std::exception& e) {
        log_exception("remove_payload_prim", e);
    }
    return false;
}

bool USD::Prim::add_payload_prim(
    BifrostUsd::Stage&                stage,
    const Amino::String&                prim_path,
    const BifrostUsd::Layer&          payload_layer,
    const Amino::String&                payload_prim_path,
    const double                        layer_offset,
    const double                        layer_scale,
    const BifrostUsd::UsdListPosition payload_position) {
    if (!stage) return false;
    try {
        auto pxr_prim = USDUtils::get_prim_or_throw(prim_path, stage);

        VariantEditContext ctx(stage);

        stage.last_modified_prim = pxr_prim.GetPath().GetText();
        if (!payload_layer.isValid()) {
            pxr::SdfPath pld_prim_path(payload_prim_path.c_str());
            return pxr_prim.GetPayloads().AddInternalPayload(
                pld_prim_path, pxr::SdfLayerOffset(layer_offset, layer_scale),
                GetUsdListPosition(payload_position));
        } else {
            std::string identifier = payload_layer->GetIdentifier().c_str();
            if (payload_prim_path.empty()) {
                return pxr_prim.GetPayloads().AddPayload(
                    identifier, pxr::SdfLayerOffset(layer_offset, layer_scale),
                    GetUsdListPosition(payload_position));
            } else {
                pxr::SdfPath pld_prim_path(payload_prim_path.c_str());
                return pxr_prim.GetPayloads().AddPayload(
                    identifier, pld_prim_path,
                    pxr::SdfLayerOffset(layer_offset, layer_scale),
                    GetUsdListPosition(payload_position));
            }
        }
    } catch (std::exception& e) {
        log_exception("add_payload_prim", e);
    }
    return false;
}

bool USD::Prim::add_inherit_prim(
    BifrostUsd::Stage&                stage,
    const Amino::String&                prim_path,
    const Amino::String&                inherited_prim_path,
    const BifrostUsd::UsdListPosition inherit_position) {
    if (!stage) return false;

    bool success = false;
    try {
        auto pxr_prim = USDUtils::get_prim_or_throw(prim_path, stage);
        if (!pxr_prim) return false;

        VariantEditContext ctx(stage);

        pxr::SdfPath in_prim_path(inherited_prim_path.c_str());
        success = pxr_prim.GetInherits().AddInherit(
            in_prim_path, GetUsdListPosition(inherit_position));
        if (success) {
            stage.last_modified_prim = pxr_prim.GetPath().GetText();
        }

    } catch (std::exception& e) {
        log_exception("add_inherit_prim", e);
    }
    return success;
}

bool USD::Prim::add_specialize_prim(
    BifrostUsd::Stage&                stage,
    const Amino::String&                prim_path,
    const Amino::String&                specialized_prim_path,
    const BifrostUsd::UsdListPosition specialize_position) {
    if (!stage) return false;

    bool success = false;
    try {
        auto pxr_prim = USDUtils::get_prim_or_throw(prim_path, stage);

        VariantEditContext ctx(stage);

        pxr::SdfPath in_prim_path(specialized_prim_path.c_str());
        success = pxr_prim.GetSpecializes().AddSpecialize(
            in_prim_path, GetUsdListPosition(specialize_position));
        if (success) {
            stage.last_modified_prim = pxr_prim.GetPath().GetText();
        }

    } catch (std::exception& e) {
        log_exception("add_specialize_prim", e);
    }
    return success;
}

bool USD::Prim::create_prim_relationship(
    BifrostUsd::Stage&                stage,
    const Amino::String&                prim_path,
    const Amino::String&                rel_name,
    const bool                          custom,
    const Amino::String&                target,
    const BifrostUsd::UsdListPosition target_position) {
    if (!stage) return false;

    bool success = false;
    try {
        VariantEditContext ctx(stage);

        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        if (!pxr_prim) return false;

        auto rel =
            pxr_prim.CreateRelationship(pxr::TfToken(rel_name.c_str()), custom);
        if (!rel) return false;
        success = rel.AddTarget(pxr::SdfPath(target.c_str()),
                                GetUsdListPosition(target_position));

        if (success) {
            stage.last_modified_prim = pxr_prim.GetPath().GetText();
        }

    } catch (std::exception& e) {
        log_exception("create_prim_relationship", e);
    }
    return success;
}

bool USD::Prim::add_relationship_target(
    BifrostUsd::Stage&                stage,
    const Amino::String&                prim_path,
    const Amino::String&                rel_name,
    const Amino::String&                target,
    const BifrostUsd::UsdListPosition target_position) {
    if (!stage) return false;

    bool success = false;
    try {
        VariantEditContext ctx(stage);

        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        if (!pxr_prim) return false;

        auto rel = pxr_prim.GetRelationship(pxr::TfToken(rel_name.c_str()));
        if (!rel) return false;
        success = rel.AddTarget(pxr::SdfPath(target.c_str()),
                                GetUsdListPosition(target_position));

        if (success) {
            stage.last_modified_prim = pxr_prim.GetPath().GetText();
        }

    } catch (std::exception& e) {
        log_exception("add_relationship_target", e);
    }
    return success;
}

bool USD::Prim::remove_relationship_target(BifrostUsd::Stage& stage,
                                           const Amino::String& prim_path,
                                           const Amino::String& rel_name,
                                           const Amino::String& target) {
    if (!stage) return false;

    bool success = false;
    try {
        VariantEditContext ctx(stage);

        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        if (!pxr_prim) return false;

        auto rel = pxr_prim.GetRelationship(pxr::TfToken(rel_name.c_str()));
        if (!rel) return false;

        success = rel.RemoveTarget(pxr::SdfPath(target.c_str()));
        if (success) {
            stage.last_modified_prim = pxr_prim.GetPath().GetText();
        }

    } catch (std::exception& e) {
        log_exception("remove_relationship_target", e);
    }
    return success;
}

bool USD::Prim::set_relationship_targets(
    BifrostUsd::Stage&               stage,
    const Amino::String&               prim_path,
    const Amino::String&               rel_name,
    const Amino::Array<Amino::String>& targets) {
    if (!stage) return false;

    bool success = false;
    try {
        VariantEditContext ctx(stage);

        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        if (!pxr_prim) return false;

        auto rel = pxr_prim.GetRelationship(pxr::TfToken(rel_name.c_str()));
        if (!rel) return false;

        pxr::SdfPathVector pxrTargets = {};
        for (size_t i = 0; i < targets.size(); ++i) {
            pxrTargets.push_back(pxr::SdfPath(targets[i].c_str()));
        }

        success = rel.SetTargets(pxrTargets);
        if (success) {
            stage.last_modified_prim = pxr_prim.GetPath().GetText();
        }

    } catch (std::exception& e) {
        log_exception("set_relationship_targets", e);
    }
    return success;
}

bool USD::Prim::clear_relationship_targets(BifrostUsd::Stage& stage,
                                           const Amino::String& prim_path,
                                           const Amino::String& rel_name,
                                           const bool           remove_spec) {
    if (!stage) return false;

    bool success = false;
    try {
        VariantEditContext ctx(stage);

        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        if (!pxr_prim) return false;

        auto rel = pxr_prim.GetRelationship(pxr::TfToken(rel_name.c_str()));
        if (!rel) return false;

        success = rel.ClearTargets(remove_spec);
        if (success) {
            stage.last_modified_prim = pxr_prim.GetPath().GetText();
        }

    } catch (std::exception& e) {
        log_exception("clear_relationship_targets", e);
    }
    return success;
}

bool USD::Prim::get_relationship_targets(
    BifrostUsd::Stage&                            stage,
    const Amino::String&                            prim_path,
    const Amino::String&                            rel_name,
    Amino::MutablePtr<Amino::Array<Amino::String>>& targets) {
    targets = Amino::newMutablePtr<Amino::Array<Amino::String>>();
    if (!stage) return false;

    bool success = false;
    try {
        VariantEditContext ctx(stage);

        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        if (!pxr_prim) return false;

        auto rel = pxr_prim.GetRelationship(pxr::TfToken(rel_name.c_str()));
        if (!rel) return false;

        pxr::SdfPathVector pxrTargets = {};
        success                       = rel.GetTargets(&pxrTargets);

        if (success) {
            targets->resize(pxrTargets.size());
            for (size_t i = 0; i < pxrTargets.size(); ++i) {
                (*targets)[i] = pxrTargets[i].GetPrimPath().GetText();
            }
            stage.last_modified_prim = pxr_prim.GetPath().GetText();
        }

    } catch (std::exception& e) {
        log_exception("get_relationship_targets", e);
    }
    return success;
}

bool USD::Prim::get_forwarded_relationship_targets(
    BifrostUsd::Stage&                            stage,
    const Amino::String&                            prim_path,
    const Amino::String&                            rel_name,
    Amino::MutablePtr<Amino::Array<Amino::String>>& targets) {
    targets = Amino::newMutablePtr<Amino::Array<Amino::String>>();
    if (!stage) return false;

    bool success = false;
    try {
        VariantEditContext ctx(stage);

        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        if (!pxr_prim) return false;

        auto rel = pxr_prim.GetRelationship(pxr::TfToken(rel_name.c_str()));
        if (!rel) return false;

        pxr::SdfPathVector pxrTargets = {};
        success                       = rel.GetForwardedTargets(&pxrTargets);

        if (success) {
            targets->resize(pxrTargets.size());
            for (size_t i = 0; i < pxrTargets.size(); ++i) {
                (*targets)[i] = pxrTargets[i].GetPrimPath().GetText();
            }
            stage.last_modified_prim = pxr_prim.GetPath().GetText();
        }

    } catch (std::exception& e) {
        log_exception("get_forwarded_relationship_targets", e);
    }
    return success;
}

bool USD::Prim::prim_is_instanceable(const BifrostUsd::Stage& stage,
                                     const Amino::String&       prim_path) {
    if (!stage) return false;
    try {
        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        return pxr_prim && pxr_prim.IsInstanceable();

    } catch (std::exception& e) {
        log_exception("prim_is_instanceable", e);
    }
    return false;
}

bool USD::Prim::set_prim_instanceable(BifrostUsd::Stage& stage,
                                      const Amino::String& prim_path,
                                      const bool           instanceable) {
    if (!stage) return false;
    try {
        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        return pxr_prim && pxr_prim.SetInstanceable(instanceable);

    } catch (std::exception& e) {
        log_exception("set_prim_instanceable", e);
    }
    return false;
}

bool USD::Prim::prim_clear_instanceable(BifrostUsd::Stage& stage,
                                        const Amino::String& prim_path) {
    if (!stage) return false;
    try {
        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        return pxr_prim && pxr_prim.ClearInstanceable();

    } catch (std::exception& e) {
        log_exception("prim_clear_instanceable", e);
    }
    return false;
}

bool USD::Prim::prim_has_authored_instanceable(const BifrostUsd::Stage& stage,
                                               const Amino::String& prim_path) {
    if (!stage) return false;

    try {
        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        return pxr_prim && pxr_prim.HasAuthoredInstanceable();

    } catch (std::exception& e) {
        log_exception("prim_has_authored_instanceable", e);
    }
    return false;
}

bool USD::Prim::prim_is_instance(const BifrostUsd::Stage& stage,
                                 const Amino::String&       prim_path) {
    if (!stage) return false;
    try {
        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        return pxr_prim && pxr_prim.IsInstance();

    } catch (std::exception& e) {
        log_exception("prim_is_instance", e);
    }
    return false;
}

bool USD::Prim::prim_is_instance_proxy(const BifrostUsd::Stage& stage,
                                       const Amino::String&       prim_path) {
    if (!stage) return false;

    try {
        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        return pxr_prim && pxr_prim.IsInstanceProxy();

    } catch (std::exception& e) {
        log_exception("prim_is_instance_proxy", e);
    }
    return false;
}

bool USD::Prim::prim_is_prototype(const BifrostUsd::Stage& stage,
                                  const Amino::String&       prim_path) {
    if (!stage) return false;

    try {
        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        return pxr_prim && pxr_prim.IsPrototype();

    } catch (std::exception& e) {
        log_exception("prim_is_prototype", e);
    }
    return false;
}

bool USD::Prim::prim_is_in_prototype(const BifrostUsd::Stage& stage,
                                     const Amino::String&       prim_path) {
    if (!stage) return false;

    try {
        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        return pxr_prim && pxr_prim.IsInPrototype();

    } catch (std::exception& e) {
        log_exception("prim_is_in_prototype", e);
    }
    return false;
}

void USD::Prim::prim_get_prototype(const BifrostUsd::Stage& stage,
                                   const Amino::String&       prim_path,
                                   Amino::String&             proto_path) {
    if (!stage) return;
    assert(stage);

    try {
        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        if (!pxr_prim) return;

        auto proto = pxr_prim.GetPrototype();
        if (proto) return;

        proto_path = proto.GetPrimPath().GetText();

    } catch (std::exception& e) {
        log_exception("prim_get_prototype", e);
    }
}

void USD::Prim::get_prim_in_prototype(const BifrostUsd::Stage& stage,
                                      const Amino::String&       prim_path,
                                      Amino::String& prim_in_proto_path) {
    if (!stage) return;

    try {
        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        if (!pxr_prim) return;

        auto prim_in_proto = pxr_prim.GetPrimInPrototype();

        if (!prim_in_proto) return;

        prim_in_proto_path = prim_in_proto.GetPrimPath().GetText();

    } catch (std::exception& e) {
        log_exception("get_prim_in_prototype", e);
    }
}

void USD::Prim::get_prim_instances(
    const BifrostUsd::Stage&                      stage,
    const Amino::String&                            proto_prim_path,
    Amino::MutablePtr<Amino::Array<Amino::String>>& instances_paths) {
    instances_paths = Amino::newMutablePtr<Amino::Array<Amino::String>>();

    if (!stage) return;

    try {
        auto pxr_prim =
            stage->GetPrimAtPath(pxr::SdfPath(proto_prim_path.c_str()));
        if (pxr_prim) {
            auto instances = pxr_prim.GetInstances();

            instances_paths->resize(instances.size());
            for (size_t i = 0; i < instances.size(); ++i) {
                (*instances_paths)[i] = instances[i].GetPrimPath().GetText();
            }
        }

    } catch (std::exception& e) {
        log_exception("get_prim_instances", e);
    }
}

void USD::Prim::get_prototype_prims(
    const BifrostUsd::Stage&                      stage,
    Amino::MutablePtr<Amino::Array<Amino::String>>& proto_prim_paths) {
    proto_prim_paths = Amino::newMutablePtr<Amino::Array<Amino::String>>();
    if (!stage) return;

    try {
        auto prototypes = stage->GetPrototypes();

        proto_prim_paths->resize(prototypes.size());
        for (size_t i = 0; i < prototypes.size(); ++i) {
            (*proto_prim_paths)[i] = prototypes[i].GetPrimPath().GetText();
        }

    } catch (std::exception& e) {
        log_exception("get_prototype_prims", e);
    }
}

void USD::Prim::set_prim_purpose(BifrostUsd::Stage&                 stage,
                                 const Amino::String&                 prim_path,
                                 const BifrostUsd::ImageablePurpose purpose) {
    assert(stage);

    try {
        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        if (!pxr_prim) {
            std::cerr << "set_prim_purpose failed: " << prim_path.c_str()
                      << " is not a valid prim path" << std::endl;
            return;
        }

        if (!pxr_prim.IsA<pxr::UsdGeomImageable>()) {
            std::cerr << "set_prim_purpose failed: " << prim_path.c_str()
                      << " is not an Imageable prim" << std::endl;
            return;
        }

        VariantEditContext ctx(stage);

        auto imageable   = pxr::UsdGeomImageable(pxr_prim);
        auto pxr_purpose = USDUtils::GetImageablePurpose(purpose);
        if (pxr_purpose.IsEmpty())
            throw std::runtime_error("Invalid purpose enum value");
        imageable.GetPurposeAttr().Set(pxr_purpose);

    } catch (std::exception& e) {
        log_exception("set_prim_purpose", e);
    }
}

void USD::Prim::set_prim_kind(const Amino::String& prim_path,
                              const Amino::String& kind,
                              BifrostUsd::Stage& stage) {
    if (!stage) return;
    try {
        auto pxr_prim = USDUtils::get_prim_or_throw(prim_path, stage);

        VariantEditContext ctx(stage);

        auto modelAPI = pxr::UsdModelAPI(pxr_prim);
        modelAPI.SetKind(pxr::TfToken(kind.c_str()));

    } catch (std::exception& e) {
        log_exception("set_prim_kind", e);
    }
}

void USD::Prim::set_prim_asset_info(BifrostUsd::Stage&   stage,
                                    const Amino::String& prim_path,
                                    const Amino::String& asset_identifier,
                                    const Amino::String& asset_name,
                                    const Amino::String& asset_version) {
    if (!stage) return;
    try {
        auto pxr_prim = USDUtils::get_prim_or_throw(prim_path, stage);

        VariantEditContext ctx(stage);

        auto modelAPI = pxr::UsdModelAPI(pxr_prim);
        // if the string parameter is empty we don't want to
        // call the modelAPI matching method as it would erase
        // any existing value.
        if (!asset_identifier.empty()) {
            modelAPI.SetAssetIdentifier(
                // We use the SdfAssetPath constructor using one parameter.
                // The SdfAssetPath constructor using two parameters is useless
                // in our scenario as UsdModelAPI::GetAssetIdentifier
                // returns an SdfAssetPath with an empty resolvedPath.
                // The following python snippet (tested with USD 22.08)
                // illustrates this behaviour:
                // \code
                // >>> model = Usd.ModelAPI(prim)
                // >>> asset_path = Sdf.AssetPath("foo.usd", "/abc/foo/foo.usd")
                // >>> asset_path
                // Sdf.AssetPath('foo.usd', '/abc/foo/foo.usd')
                // >>> model.SetAssetIdentifier(asset_path)
                // >>> model.GetAssetIdentifier()
                // Sdf.AssetPath('foo.usd')
                // \endcode
                pxr::SdfAssetPath(asset_identifier.c_str()));
        }
        if (!asset_name.empty()) {
            modelAPI.SetAssetName(asset_name.c_str());
        }
        if (!asset_version.empty()) {
            modelAPI.SetAssetVersion(asset_version.c_str());
        }
    } catch (std::exception& e) {
        log_exception("set_prim_asset_info", e);
    }
}

void USD::Prim::get_prim_asset_info(const BifrostUsd::Stage& stage,
                                    const Amino::String&     prim_path,
                                    Amino::String&           asset_identifier,
                                    Amino::String&           asset_name,
                                    Amino::String&           asset_version) {
    if (!stage) return;
    try {
        auto pxr_prim = USDUtils::get_prim_or_throw(prim_path, stage);

        VariantEditContext ctx(stage);

        auto modelAPI = pxr::UsdModelAPI(pxr_prim);

        pxr::SdfAssetPath asset_path;
        modelAPI.GetAssetIdentifier(&asset_path);
        asset_identifier = asset_path.GetAssetPath().c_str();

        std::string name;
        modelAPI.GetAssetName(&name);
        asset_name = name.c_str();

        std::string version;
        modelAPI.GetAssetVersion(&version);
        asset_version = version.c_str();

    } catch (std::exception& e) {
        log_exception("get_prim_asset_info", e);
    }
}

bool USD::Prim::set_prim_active(BifrostUsd::Stage& stage,
                                const Amino::String& path,
                                const bool           active) {
    if (!stage) return false;

    try {
        auto pxr_prim = USDUtils::get_prim_or_throw(path, stage);

        VariantEditContext ctx(stage);

        return pxr_prim.SetActive(active);

    } catch (std::exception& e) {
        log_exception("set_prim_active", e);
    }
    return false;
}

bool USD::Prim::set_prim_metadata(BifrostUsd::Stage& stage,
                                  const Amino::String& path,
                                  const Amino::String& key,
                                  const Amino::String& value) {
    return set_prim_metadata_impl(path, key, value, stage);
}

bool USD::Prim::set_prim_metadata(BifrostUsd::Stage& stage,
                                  const Amino::String& path,
                                  const Amino::String& key,
                                  const Amino::bool_t& value) {
    return set_prim_metadata_impl(path, key, value, stage);
}

bool USD::Prim::set_prim_metadata(BifrostUsd::Stage&  stage,
                                  const Amino::String&  path,
                                  const Amino::String&  key,
                                  const Amino::float_t& value) {
    return set_prim_metadata_impl(path, key, value, stage);
}

bool USD::Prim::set_prim_metadata(BifrostUsd::Stage&   stage,
                                  const Amino::String&   path,
                                  const Amino::String&   key,
                                  const Amino::double_t& value) {
    return set_prim_metadata_impl(path, key, value, stage);
}

bool USD::Prim::set_prim_metadata(BifrostUsd::Stage& stage,
                                  const Amino::String& path,
                                  const Amino::String& key,
                                  const Amino::int_t&  value) {
    return set_prim_metadata_impl(path, key, value, stage);
}

bool USD::Prim::set_prim_metadata(BifrostUsd::Stage& stage,
                                  const Amino::String& path,
                                  const Amino::String& key,
                                  const Amino::long_t& value) {
    return set_prim_metadata_impl(path, key, value, stage);
}

bool USD::Prim::set_prim_metadata(BifrostUsd::Stage&   stage,
                                  const Amino::String&   path,
                                  const Amino::String&   key,
                                  const Bifrost::Object& value) {
    return set_prim_metadata_impl(path, key, value, stage);
}

bool USD::Prim::get_prim_metadata(const BifrostUsd::Stage& stage,
                                  const Amino::String&       path,
                                  const Amino::String&       key,
                                  const Amino::String&       default_and_type,
                                  Amino::String&             value) {
    if (!stage) return false;

    try {
        auto               pxr_prim = USDUtils::get_prim_or_throw(path, stage);
        VariantEditContext ctx(stage);
        auto               pxr_key = GetSdfFieldKey(key);
        std::string        tempVal;
        if (pxr_prim.GetMetadata(pxr_key, &tempVal)) {
            value = tempVal.c_str();
            return true;
        } else {
            value = default_and_type;
        }
    } catch (std::exception& e) {
        log_exception("get_prim_metadata", e);
        value = default_and_type;
    }
    return false;
}

bool USD::Prim::get_prim_metadata(const BifrostUsd::Stage& stage,
                                  const Amino::String&       path,
                                  const Amino::String&       key,
                                  const Amino::bool_t&       default_and_type,
                                  Amino::bool_t&             value) {
    return get_prim_metadata_impl(stage, path, key, default_and_type, value);
}

bool USD::Prim::get_prim_metadata(const BifrostUsd::Stage& stage,
                                  const Amino::String&       path,
                                  const Amino::String&       key,
                                  const Amino::float_t&      default_and_type,
                                  Amino::float_t&            value) {
    return get_prim_metadata_impl(stage, path, key, default_and_type, value);
}

bool USD::Prim::get_prim_metadata(const BifrostUsd::Stage& stage,
                                  const Amino::String&       path,
                                  const Amino::String&       key,
                                  const Amino::double_t&     default_and_type,
                                  Amino::double_t&           value) {
    return get_prim_metadata_impl(stage, path, key, default_and_type, value);
}

bool USD::Prim::get_prim_metadata(const BifrostUsd::Stage& stage,
                                  const Amino::String&       path,
                                  const Amino::String&       key,
                                  const Amino::int_t&        default_and_type,
                                  Amino::int_t&              value) {
    return get_prim_metadata_impl(stage, path, key, default_and_type, value);
}

bool USD::Prim::get_prim_metadata(const BifrostUsd::Stage& stage,
                                  const Amino::String&       path,
                                  const Amino::String&       key,
                                  const Amino::long_t&       default_and_type,
                                  Amino::long_t&             value) {
    return get_prim_metadata_impl(stage, path, key, default_and_type, value);
}

bool USD::Prim::get_prim_metadata(
    const BifrostUsd::Stage&         stage,
    const Amino::String&               path,
    const Amino::String&               key,
    const Amino::Ptr<Bifrost::Object>& default_and_type,
    Amino::Ptr<Bifrost::Object>&       value) {
    auto value_returns = createReturnGuard(
        value, [&default_and_type]() { return default_and_type; });

    if (!stage) return false;

    try {
        auto               pxr_prim = USDUtils::get_prim_or_throw(path, stage);
        VariantEditContext ctx(stage);
        auto               pxr_key = GetSdfFieldKey(key);
        pxr::VtDictionary  temp;

        if (pxr_prim.GetMetadata(pxr_key, &temp)) {
            value = fromPxr(temp);
            return true;
        } else {
            value = default_and_type;
        }

    } catch (std::exception& e) {
        log_exception("get_prim_metadata", e);
    }

    return false;
}
