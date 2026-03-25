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

namespace USD {

namespace Shading {

USD_NODEDEF_DECL
bool get_material_path(
    const BifrostUsd::Prim& prim,
    const BifrostUsd::MaterialPurpose material_purpose
        AMINO_ANNOTATE("Amino::Port value=All"),
    const bool compute_bound_material AMINO_ANNOTATE("Amino::Port value=false"),
    Amino::String& path) USDNODE_DOC_ICON_X("get_material_path",
                                            "USD_Shading_get_material_path.md",
                                            "usd_default.svg",
                                            "outName=success");

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
                       "USD_Shading_bind_material.md",
                       "bind_material.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool unbind_material(
    BifrostUsd::Stage& stage           USDPORT_INOUT("out_stage"),
    const Amino::String&               prim_path,
    const BifrostUsd::MaterialPurpose  material_purpose,
    const Amino::String&               binding_name)
    USDNODE_DOC_ICON_X("unbind_material",
                       "USD_Shading_unbind_material.md",
                       "usd_default.svg",
                       "outName=success");

} // namespace Shading
} // namespace USD

#endif // USD_MATERIAL_BINDING_NODEDEFS_H
