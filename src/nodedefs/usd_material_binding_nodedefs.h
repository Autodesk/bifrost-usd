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
/// \file  usd_material_binding_nodedefs.h
/// \brief Bifrost USD Shade nodes.

#ifndef USD_MATERIAL_BINDING_NODEDEFS_H
#define USD_MATERIAL_BINDING_NODEDEFS_H

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
/// \defgroup Shading Shading
namespace Shading {

/// \ingroup Shading
/// \defgroup get_material_path get_material_path node
///
/// \brief Returns the material path of a USD prim.
///
/// \param [in] prim The prim you want to get the material path from.
/// \param [in] material_purpose Specifies the purpose for which you want to get the material.
///             The binding applies only to the specified material purpose.
/// \param [in] compute_bound_material Computes the resolved bound material for this prim.
/// \param [out] path The path to the material or an empty string if no material is found.
/// \returns true if a material path is found, false otherwise.
USD_NODEDEF_DECL
bool get_material_path(
    const BifrostUsd::Prim& prim,
    const BifrostUsd::MaterialPurpose material_purpose
        AMINO_ANNOTATE("Amino::Port value=BifrostUsd::MaterialPurpose::All"),
    const bool compute_bound_material AMINO_ANNOTATE("Amino::Port value=false"),
    Amino::String& path) USDNODE_DOC_ICON_X("get_material_path",
                                            "get_material_path",
                                            "usd.svg",
                                            "outName=success");
/// \ingroup Shading
/// \defgroup bind_material bind_material node
///
/// \brief Binds a material to a prim directly, or to a collection on the prim.
/// The \p collection_name must be specified to bind to a collection. \p binding_name can stay empty.
///
/// \param [in] stage The stage holding the prim and material.
/// \param [in] prim_path The path of the prim you want to bind the material to,
///             or on which there is a collection you want to bind the material
///             to.
/// \param [in] material_path The path of the material.
/// \param [in] binding_strength The material binding strength.
/// \param [in] material_purpose If not equal to "All", the binding applies only to the specified material purpose.
/// \param [in] collection_prim_path The prim path holding the collection. If empty, the prim_path input will be used.
/// \param [in] collection_name The name of the collection you want to assign the material to.
/// \param [in] binding_name When binding to a collection, establishes an identity for the binding that is unique on the prim.
///             This is ignored when creating a direct binding.
/// \returns true on success, false otherwise.
USD_NODEDEF_DECL
bool bind_material(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                   const Amino::String&     prim_path,
                   const Amino::String&     material_path,
                   const BifrostUsd::MaterialBindingStrength binding_strength,
                   const BifrostUsd::MaterialPurpose         material_purpose,
                   const Amino::String& collection_prim_path,
                   const Amino::String& collection_name,
                   const Amino::String& binding_name)
    USDNODE_DOC_ICON_X("bind_material",
                       "bind_material",
                       "usd.svg",
                       "outName=success");

/// \ingroup Shading
/// \defgroup unbind_material unbind_material node
///
/// \brief Unbinds a direct or collection-based binding.
/// The \p binding_name must be specified to unbind a collection.
///
/// \param [in] stage The stage holding the prim and material.
/// \param [in] prim_path The path of the prim you want to unbind the material from.
///             For collections, this is the path to the prim holding the collection.
/// \param [in] material_purpose Unbinds only the specified material purpose.
/// \param [in] binding_name The binding name of the collection.
/// \returns true if a material binding is removed, false otherwise.
USD_NODEDEF_DECL
bool unbind_material(
    BifrostUsd::Stage& stage           USDPORT_INOUT("out_stage"),
    const Amino::String&               prim_path,
    const BifrostUsd::MaterialPurpose  material_purpose,
    const Amino::String&               binding_name)
    USDNODE_DOC_ICON_X("unbind_material",
                       "unbind_material",
                       "usd.svg",
                       "outName=success");

} // namespace Shading
} // namespace USD

#endif // USD_MATERIAL_BINDING_NODEDEFS_H
