//-
// Copyright 2024 Autodesk, Inc.
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

namespace USD {

namespace VariantSet {

USD_NODEDEF_DECL
void add_variant_set(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                     const Amino::String&       prim_path,
                     const Amino::String&       variant_set_name)
    USDNODE_DOC_ICON("add_variant_set", "USD_VariantSet_add_variant_set.md", "usd_variant.svg");

USD_NODEDEF_DECL
void add_variant(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                const Amino::String& prim_path,
                 const Amino::String& variant_set_name,
                 const Amino::String& variant_name,
                 const bool set_variant_selection
                     AMINO_ANNOTATE("Amino::Port value=true"))
    USDNODE_DOC_ICON("add_variant", "USD_VariantSet_add_variant.md", "usd_variant.svg");

USD_NODEDEF_DECL
void set_variant_selection(BifrostUsd::Stage& stage
                                                USDPORT_INOUT("out_stage"),
                           const Amino::String& prim_path,
                           const Amino::String& variant_set_name,
                           const Amino::String& variant_name,
                           bool clear=false) USDNODE_DOC_ICON("set_variant_selection",
                                                        "USD_VariantSet_set_variant_selection.md",
                                                        "usd_variant.svg");

USD_NODEDEF_DECL
void clear_variant_selection(BifrostUsd::Stage& stage
                                                  USDPORT_INOUT("out_stage"),
                             const Amino::String& prim_path,
                             const Amino::String& variant_set_name)
    USDNODE_DOC_ICON("clear_variant_selection",
                     "USD_VariantSet_clear_variant_selection.md",
                     "usd_variant.svg");

USD_NODEDEF_DECL
void get_variant_sets(
    const BifrostUsd::Stage&                      stage,
    const Amino::String&                            prim_path,
    Amino::MutablePtr<Amino::Array<Amino::String>>& names)
    USDNODE_DOC_ICON("get_variant_sets",
                     "USD_VariantSet_get_variant_sets.md",
                     "usd_variant.svg");

USD_NODEDEF_DECL
void get_variants(
    const BifrostUsd::Stage&                      stage,
    const Amino::String&                            prim_path,
    const Amino::String&                            variant_set_name,
    Amino::MutablePtr<Amino::Array<Amino::String>>& names)
    USDNODE_DOC_ICON("get_variants",
                     "USD_VariantSet_get_variants.md",
                     "usd_variant.svg");

USD_NODEDEF_DECL
void get_variant_selection(
    const BifrostUsd::Stage& stage,
    const Amino::String&     prim_path,
    const Amino::String&     variant_set_name,
    Amino::String&           selection)
    USDNODE_DOC_ICON("get_variant_selection",
                     "USD_VariantSet_get_variant_selection.md",
                     "usd_variant.svg");

} // namespace VariantSet
} // namespace USD

#endif // USD_VARIANT_SET_NODEDEFS_H
