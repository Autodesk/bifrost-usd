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

#include <Amino/Core/String.h>
#include <Bifrost/FileUtils/FileUtils.h>
#include <pxr/usd/sdf/copyUtils.h>

#include <iostream>

#include "logger.h"
#include "usd_utils.h"

namespace {
/// \todo BIFROST-6381 Investigate if USD throws and whether all try/catch
/// are necessary.
void log_exception(const char* func_name, std::exception const& e) {
    if (USDUtils::Logger::errorVerboseLevel() > 0) {
        std::cerr << func_name << " failed: " << e.what() << std::endl;
    }
}
} // namespace

void USD::VariantSet::add_variant_set(BifrostUsd::Stage& stage,
                                      const Amino::String& prim_path,
                                      const Amino::String& variant_set_name) {
    try {
        if (stage) {
            auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
            if (pxr_prim) {
                pxr_prim.GetVariantSets().AddVariantSet(
                    variant_set_name.c_str());
                stage.last_modified_prim = pxr_prim.GetPath().GetText();
                stage.last_modified_variant_set_prim = stage.last_modified_prim;
                stage.last_modified_variant_set_name = variant_set_name;
            }
        }

    } catch (std::exception& e) {
        log_exception("add_variant_set", e);
    }
}

void USD::VariantSet::add_variant(BifrostUsd::Stage& stage,
                                  const Amino::String& prim_path,
                                  const Amino::String& variant_set_name,
                                  const Amino::String& variant_name,
                                  const bool           set_variant_selection) {
    try {
        if (stage) {
            auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
            if (pxr_prim) {
                Amino::String resolved_variant_set_name = variant_set_name;
                if (resolved_variant_set_name.empty()) {
                    resolved_variant_set_name =
                        stage.last_modified_variant_set_name;
                }
                if (!resolved_variant_set_name.empty()) {
                    auto pxr_variant_set = pxr_prim.GetVariantSet(
                        resolved_variant_set_name.c_str());
                    if (pxr_variant_set) {
                        pxr_variant_set.AddVariant(variant_name.c_str());

                        if (set_variant_selection) {
                            pxr_variant_set.SetVariantSelection(
                                variant_name.c_str());
                        }

                        stage.last_modified_prim = pxr_prim.GetPath().GetText();
                        stage.last_modified_variant_set_prim =
                            stage.last_modified_prim;
                        stage.last_modified_variant_set_name =
                            resolved_variant_set_name;
                        stage.last_modified_variant_name = variant_name;
                    }
                }
            }
        }

    } catch (std::exception& e) {
        log_exception("add_variant", e);
    }
}

void USD::VariantSet::set_variant_selection(
    BifrostUsd::Stage& stage,
    const Amino::String& prim_path,
    const Amino::String& variant_set_name,
    const Amino::String& variant_name,
    bool                 clear) {
    try {
        if (stage) {
            auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
            if (!pxr_prim) {
                throw std::runtime_error("Could not get prim at path " +
                                         std::string(prim_path.c_str()));
            } else {
                Amino::String resolved_variant_set_name =
                    clear ? "" : variant_set_name;
                if (resolved_variant_set_name.empty()) {
                    resolved_variant_set_name =
                        stage.last_modified_variant_set_name;
                }
                if (!resolved_variant_set_name.empty()) {
                    auto vset = pxr_prim.GetVariantSet(
                        resolved_variant_set_name.c_str());
                    stage.last_modified_variant_set_name =
                        resolved_variant_set_name;
                    Amino::String resolved_variant_name = variant_name;
                    if (resolved_variant_name.empty() && !clear) {
                        resolved_variant_name =
                            stage.last_modified_variant_name;
                    }
                    stage.last_modified_variant_name = resolved_variant_name;
                    if (!resolved_variant_name.empty() && !clear) {
                        if (!vset.SetVariantSelection(
                                resolved_variant_name.c_str())) {
                            throw std::runtime_error(
                                "Could not set Variant selection to " +
                                std::string(resolved_variant_name.c_str()));
                        }
                    } else if (clear) {
                        vset.ClearVariantSelection();
                    }
                    stage.last_modified_prim = pxr_prim.GetPath().GetText();
                    stage.last_modified_variant_set_prim =
                        stage.last_modified_prim;
                }
            }
        }

    } catch (std::exception& e) {
        log_exception("set_variant_selection", e);
    }
}

void USD::VariantSet::get_variant_sets(
    const BifrostUsd::Stage&                      stage,
    const Amino::String&                            prim_path,
    Amino::MutablePtr<Amino::Array<Amino::String>>& names) {
    names = Amino::newMutablePtr<Amino::Array<Amino::String>>();
    try {
        if (stage) {
            auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
            if (!pxr_prim) {
                throw std::runtime_error("Could not get prim at path " +
                                         std::string(prim_path.c_str()));
            } else {
                if (!pxr_prim.HasVariantSets()) {
                    return;
                }

                for (const auto& name : pxr_prim.GetVariantSets().GetNames()) {
                    names->push_back(name.c_str());
                }
            }
        }

    } catch (std::exception& e) {
        log_exception("get_variant_sets", e);
    }
}

void USD::VariantSet::get_variants(
    const BifrostUsd::Stage&                      stage,
    const Amino::String&                            prim_path,
    const Amino::String&                            variant_set_name,
    Amino::MutablePtr<Amino::Array<Amino::String>>& names) {
    names = Amino::newMutablePtr<Amino::Array<Amino::String>>();
    try {
        if (stage) {
            auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
            if (pxr_prim) {
                Amino::String resolved_variant_set_name = variant_set_name;
                if (resolved_variant_set_name.empty()) {
                    resolved_variant_set_name =
                        stage.last_modified_variant_set_name;
                }
                if (!resolved_variant_set_name.empty()) {
                    auto pxr_variant_set = pxr_prim.GetVariantSet(
                        resolved_variant_set_name.c_str());
                    if (pxr_variant_set) {
                        for (const auto& name :
                             pxr_variant_set.GetVariantNames()) {
                            names->push_back(name.c_str());
                        }
                    }
                }
            }
        }
    } catch (std::exception& e) {
        log_exception("get_variants", e);
    }
}
