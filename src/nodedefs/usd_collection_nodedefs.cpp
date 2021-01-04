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

#include "usd_collection_nodedefs.h"

#include <Amino/Core/String.h>

#include "usd_type_converter.h"
#include "usd_utils.h"

// Note: To silence warnings coming from USD library
#include <bifusd/config/CfgWarningMacros.h>
BIFUSD_WARNING_PUSH
BIFUSD_WARNING_DISABLE_MSC(4003)

#include <pxr/usd/usd/collectionAPI.h>

BIFUSD_WARNING_POP

using namespace USDUtils;
using namespace USDTypeConverters;

namespace {} // namespace

bool USD::Collection::get_or_create_collection(
    BifrostUsd::Stage& stage           USDPORT_INOUT("out_stage"),
    const Amino::String&               prim_path,
    const Amino::String&               collection_name,
    const BifrostUsd::ExpansionRule    rule,
    const Amino::Array<Amino::String>& include_paths,
    const Amino::Array<Amino::String>& exclude_paths) {
    if (!stage) return false;

    try {
        auto prim = USDUtils::get_prim_or_throw(prim_path, stage);

        VariantEditContext ctx(stage);

        auto collectionAPI = pxr::UsdCollectionAPI::Get(
            prim, pxr::TfToken{collection_name.c_str()});
        if (!collectionAPI) {
            std::string whyNot;
            bool        result = pxr::UsdCollectionAPI::CanApply(
                prim, pxr::TfToken{collection_name.c_str()}, &whyNot);
            if (!result) {
                auto msg =
                    "It is not valid to apply CollectionAPI schema to "
                    "primitive " +
                    std::string{prim_path.c_str()} + " in collection " +
                    std::string{collection_name.c_str()} + " (reason is: `" +
                    whyNot + "`)";
                throw std::runtime_error(std::move(msg));
            }
            collectionAPI = pxr::UsdCollectionAPI::Apply(
                prim, pxr::TfToken{collection_name.c_str()});
            if (!collectionAPI) {
                auto msg =
                    "An unexpected error occurred while applying CollectionAPI "
                    "schema to primitive " +
                    std::string{prim_path.c_str()} + " in collection " +
                    std::string{collection_name.c_str()};
                throw std::runtime_error(std::move(msg));
            }
        }

        if (rule != BifrostUsd::ExpansionRule::Default) {
            pxr::TfToken      token = USDUtils::GetExpansionRule(rule);
            pxr::UsdAttribute attr =
                collectionAPI.CreateExpansionRuleAttr(pxr::VtValue{token});
            if (!attr) {
                auto msg = "Unable to set expansion rule " +
                            std::string{token.GetText()} +
                            " in collection " +
                            std::string{collection_name.c_str()};
                throw std::runtime_error(std::move(msg));
            }
        }
        for (size_t i = 0; i < include_paths.size(); ++i) {
            if (!collectionAPI.IncludePath(
                    pxr::SdfPath{include_paths[i].c_str()})) {
                auto msg = "Can not add include path " +
                            std::string{include_paths[i].c_str()} +
                            " to collection " +
                            std::string{collection_name.c_str()};
                throw std::runtime_error(std::move(msg));
            }
        }

        for (size_t i = 0; i < exclude_paths.size(); ++i) {
            if (!collectionAPI.ExcludePath(
                    pxr::SdfPath{exclude_paths[i].c_str()})) {
                auto msg = "Can not add exclude path " +
                            std::string{exclude_paths[i].c_str()} +
                            " to collection " +
                            std::string{collection_name.c_str()};
                throw std::runtime_error(std::move(msg));
            }
        }

        return true;

    } catch (std::exception& e) {
        log_exception("USD::Collection::get_or_create_collection", e);
    }
    return false;
}

void USD::Collection::get_all_collection_names(
    const BifrostUsd::Prim&                         prim,
    Amino::MutablePtr<Amino::Array<Amino::String>>& names) {
    names = Amino::newMutablePtr<Amino::Array<Amino::String>>();

    if (!prim) {
        return;
    }

    auto allCollections =
        pxr::UsdCollectionAPI::GetAllCollections(prim.getPxrPrim());

    const size_t numCollections = allCollections.size();
    names->resize(numCollections);
    for (size_t i = 0; i < numCollections; ++i) {
        (*names)[i] = allCollections[i].GetName().GetText();
    }
}

void USD::Collection::get_includes_paths(
    const BifrostUsd::Prim&                         prim,
    const Amino::String&                            collection_name,
    Amino::MutablePtr<Amino::Array<Amino::String>>& paths) {
    paths = Amino::newMutablePtr<Amino::Array<Amino::String>>();

    if (!prim) {
        return;
    }

    auto collection = pxr::UsdCollectionAPI::Get(
        prim.getPxrPrim(), pxr::TfToken{collection_name.c_str()});

    pxr::SdfPathVector targets;
    if (collection.GetIncludesRel().GetTargets(&targets)) {
        const size_t numIncludesPaths = targets.size();
        paths->resize(numIncludesPaths);
        for (size_t i = 0; i < numIncludesPaths; ++i) {
            (*paths)[i] = targets[i].GetText();
        }
    }
}

void USD::Collection::get_excludes_paths(
    const BifrostUsd::Prim&                         prim,
    const Amino::String&                            collection_name,
    Amino::MutablePtr<Amino::Array<Amino::String>>& paths) {
    paths = Amino::newMutablePtr<Amino::Array<Amino::String>>();

    if (!prim) {
        return;
    }

    auto collection = pxr::UsdCollectionAPI::Get(
        prim.getPxrPrim(), pxr::TfToken{collection_name.c_str()});

    pxr::SdfPathVector targets;
    if (collection.GetExcludesRel().GetTargets(&targets)) {
        const size_t numExcludesPaths = targets.size();
        paths->resize(numExcludesPaths);
        for (size_t i = 0; i < numExcludesPaths; ++i) {
            (*paths)[i] = targets[i].GetText();
        }
    }
}
