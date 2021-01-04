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
/// \file  usd_variantset_nodedefs.h
/// \brief Bifrost USD Variant Set nodes.

#ifndef USD_VARIANT_SET_NODEDEFS_H
#define USD_VARIANT_SET_NODEDEFS_H

#include <Amino/Core/Array.h>
#include <Amino/Core/Ptr.h>
#include <BifrostUsd/Prim.h>
#include <BifrostUsd/Stage.h>

#include "header_parser_macros.h"
#include "nodedef_export.h"

// forward declarations
namespace Amino {
class String;
} // namespace Amino

/// \defgroup USD USD
namespace USD {

/// \ingroup USD
/// \defgroup VariantSet VariantSet
namespace VariantSet {

/// \ingroup VariantSet
/// \defgroup add_variant_set add_variant_set node
///
/// \brief This node adds a new variant set for the USD prim on the given stage.
///
/// \param [in] stage The USD stage on which to add the variant set on the prim.
/// \param [in] prim_path The USD prim path to create a variant set for.
/// \param [in] variant_set_name The name of the variant set.
USD_NODEDEF_DECL
void add_variant_set(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                     const Amino::String&       prim_path,
                     const Amino::String&       variant_set_name)
    USDNODE_DOC_ICON("add_variant_set", "add_variant_set", "usd.svg");

/// \ingroup VariantSet
/// \defgroup add_variant add_variant node
///
/// \brief This node adds a new variant to the variant set on the given stage.
///
/// \param [in] stage The USD stage on which to add the variant.
/// \param [in] prim_path The path to the prim to add variants to.
/// \param [in] variant_set_name The variant set name to add variants to.
/// \param [in] variant_name The variant's name.
USD_NODEDEF_DECL
void add_variant(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                const Amino::String& prim_path,
                 const Amino::String& variant_set_name,
                 const Amino::String& variant_name,
                 const bool set_variant_selection
                     AMINO_ANNOTATE("Amino::Port value=true"))
    USDNODE_DOC_ICON("add_variant", "add_variant", "usd.svg");

/// \ingroup VariantSet
/// \defgroup set_variant_selection set_variant_selection node
///
/// \brief This node selects a given variant on the given variant set of the given
/// stage.
///
/// \param [in] stage The USD stage on which to select the given variant.
/// \param [in] prim_path The path to the prim to set the variant selection to.
/// \param [in] variant_set_name The variant set for which to select a variant.
/// \param [in] variant_name The variant's name.
/// \param [in] clear Clears the selection of the variant set instead.
USD_NODEDEF_DECL
void set_variant_selection(BifrostUsd::Stage& stage
                                                USDPORT_INOUT("out_stage"),
                           const Amino::String& prim_path,
                           const Amino::String& variant_set_name,
                           const Amino::String& variant_name,
                           bool clear) USDNODE_DOC_ICON("set_variant_selection",
                                                        "set_variant_selection",
                                                        "usd.svg");

/// \ingroup VariantSet
/// \defgroup get_variant_sets get_variant_sets node
///
/// \brief This node returns the names of all the variant sets present on a prim.
///
/// \param [in] stage The USD stage holding the prim you want to query.
/// \param [in] prim_path The path to the prim to get the variant set names from.
/// \param [out] names The variant set names.
USD_NODEDEF_DECL
void get_variant_sets(
    const BifrostUsd::Stage&                      stage,
    const Amino::String&                            prim_path,
    Amino::MutablePtr<Amino::Array<Amino::String>>& names)
    USDNODE_DOC_ICON("get_variant_sets",
                     "get_variant_sets",
                     "usd.svg");

/// \ingroup VariantSet
/// \defgroup get_variants get_variants node
///
/// \brief This node returns the names of all the variants present in a variant set.
///
/// \param [in] stage The USD stage holding the prim you want to query.
/// \param [in] prim_path The path to the prim with the variant set.
/// \param [in] variant_set_name The name of the variant set for which to get variant names.
/// \param [out] names The variant names.
USD_NODEDEF_DECL
void get_variants(
    const BifrostUsd::Stage&                      stage,
    const Amino::String&                            prim_path,
    const Amino::String&                            variant_set_name,
    Amino::MutablePtr<Amino::Array<Amino::String>>& names)
    USDNODE_DOC_ICON("get_variants",
                     "get_variants",
                     "usd.svg");

} // namespace VariantSet
} // namespace USD

#endif // USD_VARIANT_SET_NODEDEFS_H
