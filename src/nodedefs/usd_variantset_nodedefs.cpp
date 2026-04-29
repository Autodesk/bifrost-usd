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

#include "usd_variantset_nodedefs.h"
#include <BifrostUsd/VariantContext.h>
#include <BifrostUsd/VariantSelection.h>

#include <Amino/Core/String.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/sdf/path.h>
#include <algorithm>

#include "logger.h"
#include "usd_utils.h"

using namespace USDUtils;

void USD::VariantSet::add_variant_set(BifrostUsd::Stage&   stage,
                                      const Amino::String& prim_path,
                                      const Amino::String& variant_set_name) {
    try {
        BifrostUsd::WithVariantContext(stage, [&]() {
            if (stage) {
                auto resolvedPath =
                    USDUtils::resolve_prim_path(prim_path, stage);
                auto pxr_prim =
                    USDUtils::get_prim_or_throw(resolvedPath, stage);
                pxr_prim.GetVariantSets().AddVariantSet(
                    variant_set_name.c_str());
                stage.last_modified_prim = resolvedPath;
            }
        });
    } catch (std::exception& e) {
        log_exception("add_variant_set", e);
    }
}

void USD::VariantSet::add_variant(BifrostUsd::Stage&   stage,
                                  const Amino::String& prim_path,
                                  const Amino::String& variant_set_name,
                                  const Amino::String& variant_name,
                                  const bool           set_variant_selection) {
    try {
        BifrostUsd::WithVariantContext(stage, [&]() {
            if (stage) {
                auto pxr_prim =
                    USDUtils::get_prim_or_throw(USDUtils::resolve_prim_path(prim_path, stage), stage);

                Amino::String resolved_variant_set_name = variant_set_name;
                if (resolved_variant_set_name.empty()) {
                    resolved_variant_set_name = stage.lastModifiedVariantSet();
                }
                if (!resolved_variant_set_name.empty()) {
                    auto pxr_variant_set = pxr_prim.GetVariantSet(
                        resolved_variant_set_name.c_str());
                    if (pxr_variant_set) {
                        pxr_variant_set.AddVariant(variant_name.c_str());

                        if (set_variant_selection) {
                            pxr_variant_set.SetVariantSelection(
                                variant_name.c_str());
                            stage.variantSelection().add(
                                pxr_prim.GetPath().GetText(), variant_set_name.c_str(),
                                variant_name.c_str());
                        }

                        stage.last_modified_prim = pxr_prim.GetPath().GetText();
                        stage.variantSelection().setPrimPath(
                            stage.last_modified_prim);
                    }
                }
            }
        });

    } catch (std::exception& e) {
        log_exception("add_variant", e);
    }
}

void USD::VariantSet::set_variant_selection(
    BifrostUsd::Stage&   stage,
    const Amino::String& prim_path,
    const Amino::String& variant_set_name,
    const Amino::String& variant_name,
    bool                 clear) {
    try {
        if (stage) {
            if (variant_set_name.empty()) {
                throw std::runtime_error(
                    "Could not set Variant selection with an empty Variant Set "
                    "name");
            } else if (variant_name.empty()) {
                // If variant_name is empty, UsdVariantSet::SetVariantSelection
                // will clear the variant selection. Bifrost USD avoid this
                // behaviour to remove confusion while authoring a graph.
                throw std::runtime_error(
                    "Could not set Variant selection with an empty Variant "
                    "name. If you want to clear the variantSet selection use "
                    "the clear_variant_selection node");
            }

            if (clear) {
                stage.variantSelection().clear();
            }

            BifrostUsd::WithVariantContext(stage, [&]() {
                auto pxr_prim = USDUtils::get_prim_or_throw(prim_path, stage);

                // Check if variant_set_name is valid
                std::vector<std::string> vsetNames;
                pxr_prim.GetVariantSets().GetNames(&vsetNames);

                auto it = std::find(vsetNames.begin(), vsetNames.end(),
                                    variant_set_name.c_str());
                if (it == vsetNames.end()) {
                    throw std::runtime_error("VariantSet not found");
                }

                auto vset = pxr_prim.GetVariantSet(variant_set_name.c_str());
                if (clear) {
                    vset.ClearVariantSelection();
                }

                if (!vset.SetVariantSelection(variant_name.c_str())) {
                    throw std::runtime_error(
                        "Could not set Variant selection to " +
                        std::string(variant_name.c_str()));
                }

                stage.variantSelection().add(pxr_prim.GetPath().GetText(),
                                              variant_set_name.c_str(),
                                              variant_name.c_str());

                stage.last_modified_prim = pxr_prim.GetPath().GetText();
            });
        }

    } catch (std::exception& e) {
        log_exception("set_variant_selection", e);
    }
}

void USD::VariantSet::clear_variant_selection(
    BifrostUsd::Stage&   stage,
    const Amino::String& prim_path,
    const Amino::String& variant_set_name) {
    try {
        if (stage) {
            auto pxr_prim = USDUtils::get_prim_or_throw(prim_path, stage);
            auto vset     = pxr_prim.GetVariantSet(variant_set_name.c_str());
            vset.ClearVariantSelection();
            stage.variantSelection().clear();
        }
    } catch (std::exception& e) {
        log_exception("clear_variant_selection", e);
    }
}

void USD::VariantSet::get_variant_sets(
    const BifrostUsd::Stage&                        stage,
    const Amino::String&                            prim_path,
    Amino::MutablePtr<Amino::Array<Amino::String>>& names) {
    names = Amino::newMutablePtr<Amino::Array<Amino::String>>();
    try {
        if (stage) {
            auto pxr_prim = USDUtils::get_prim_or_throw(prim_path, stage);
            if (!pxr_prim.HasVariantSets()) {
                return;
            }

            for (const auto& name : pxr_prim.GetVariantSets().GetNames()) {
                names->push_back(name.c_str());
            }
        }

    } catch (std::exception& e) {
        log_exception("get_variant_sets", e);
    }
}

void USD::VariantSet::get_variants(
    const BifrostUsd::Stage&                        stage,
    const Amino::String&                            prim_path,
    const Amino::String&                            variant_set_name,
    Amino::MutablePtr<Amino::Array<Amino::String>>& names) {
    names = Amino::newMutablePtr<Amino::Array<Amino::String>>();
    try {
        if (stage) {
            auto pxr_prim = USDUtils::get_prim_or_throw(prim_path, stage);
            Amino::String resolved_variant_set_name = variant_set_name;
            if (resolved_variant_set_name.empty()) {
                resolved_variant_set_name = stage.lastModifiedVariantSet();
            }
            if (!resolved_variant_set_name.empty()) {
                auto pxr_variant_set =
                    pxr_prim.GetVariantSet(resolved_variant_set_name.c_str());
                if (pxr_variant_set) {
                    for (const auto& name : pxr_variant_set.GetVariantNames()) {
                        names->push_back(name.c_str());
                    }
                }
            }
        }
    } catch (std::exception& e) {
        log_exception("get_variants", e);
    }
}

void USD::VariantSet::get_variant_selection(
    const BifrostUsd::Stage& stage,
    const Amino::String&     prim_path,
    const Amino::String&     variant_set_name,
    Amino::String&           selection) {
    selection = Amino::String();
    try {
        if (stage) {
            auto pxr_prim = USDUtils::get_prim_or_throw(prim_path, stage);
            Amino::String resolved_variant_set_name = variant_set_name;
            if (resolved_variant_set_name.empty()) {
                resolved_variant_set_name = stage.lastModifiedVariantSet();
            }
            if (!resolved_variant_set_name.empty()) {
                auto pxr_variant_set =
                    pxr_prim.GetVariantSet(resolved_variant_set_name.c_str());
                if (pxr_variant_set) {
                    selection = pxr_variant_set.GetVariantSelection().c_str();
                }
            }
        }
    } catch (std::exception& e) {
        log_exception("get_variant_selection", e);
    }
}
