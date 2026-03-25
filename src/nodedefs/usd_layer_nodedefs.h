//-
// Copyright 2026 Autodesk, Inc.
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

#ifndef USD_LAYER_NODEDEFS_H
#define USD_LAYER_NODEDEFS_H

#include <Amino/Core/Array.h>
#include <Amino/Core/Ptr.h>
#include <BifrostUsd/Layer.h>
#include <BifrostUsd/Stage.h>

#include "header_parser_macros.h"
#include "nodedef_export.h"

// forward declarations
namespace Amino {
class String;
} // namespace Amino

namespace USD {

namespace Layer {

USD_NODEDEF_DECL
void get_root_layer(const BifrostUsd::Stage&        stage,
                    Amino::Ptr<BifrostUsd::Layer>& layer)
    USDNODE_DOC_ICON("get_root_layer", "USD_Layer_get_root_layer.md", "usd_layers.svg");

USD_NODEDEF_DECL
void get_layer(const BifrostUsd::Stage&       stage,
               const int                      layer_index,
               Amino::Ptr<BifrostUsd::Layer>& layer)
    USDNODE_DOC_ICON("get_layer", "USD_Layer_get_layer.md", "usd_layers.svg");

USD_NODEDEF_DECL
bool replace_layer(BifrostUsd::Stage&       stage USDPORT_INOUT("out_stage"),
                   const int                sublayer_index,
                   const BifrostUsd::Layer& new_layer)
    USDNODE_DOC_ICON_X("replace_layer",
                       "USD_Layer_replace_layer.md",
                       "usd_layers.svg",
                       "outName=success");

USD_NODEDEF_DECL
void create_layer(const Amino::String& save_file USDNODE_FILE_BROWSER_SAVE,
                  const Amino::String& file_format,
                  Amino::MutablePtr<BifrostUsd::Layer>& layer)
    USDNODE_INTERNAL("create_layer", "USD_Layer_create_layer.md");

USD_NODEDEF_DECL
void open_layer(const Amino::String& file      USDNODE_FILE_BROWSER_OPEN,
                const Amino::String& save_file USDNODE_FILE_BROWSER_SAVE,
                const bool read_only AMINO_ANNOTATE("Amino::Port value=false"),
                Amino::MutablePtr<BifrostUsd::Layer>& layer)
    USDNODE_INTERNAL("open_layer", "USD_Layer_open_layer.md");

USD_NODEDEF_DECL
void set_layer_permission(const bool read_only
                              AMINO_ANNOTATE("Amino::Port value=false"),
                          BifrostUsd::Layer& layer USDPORT_INOUT("out_layer"))
    USDNODE_DOC_ICON("set_layer_permission", "USD_Layer_set_layer_permission.md", "usd_layers.svg");

USD_NODEDEF_DECL
void duplicate_layer(const BifrostUsd::Layer&     layer,
                     const Amino::String& save_file USDNODE_FILE_BROWSER_SAVE,
                     Amino::MutablePtr<BifrostUsd::Layer>& new_layer)
    USDNODE_DOC_ICON("duplicate_layer", "USD_Layer_duplicate_layer.md", "usd_layers.svg");

USD_NODEDEF_DECL
void get_layer_identifier(const BifrostUsd::Layer& layer,
                          Amino::String&             identifier)
    USDNODE_DOC_ICON("get_layer_identifier", "USD_Layer_get_layer_identifier.md", "usd_layers.svg");

USD_NODEDEF_DECL
void get_layer_display_name(const BifrostUsd::Layer& layer,
                          Amino::String&             display_name)
    USDNODE_DOC_ICON("get_layer_display_name", "USD_Layer_get_layer_display_name.md", "usd_layers.svg");

USD_NODEDEF_DECL
void get_layer_file_path(const BifrostUsd::Layer& layer, Amino::String& file)
    USDNODE_DOC_ICON("get_layer_file_path", "USD_Layer_get_layer_file_path.md", "usd_layers.svg");

USD_NODEDEF_DECL
void export_layer_to_string(const BifrostUsd::Layer& layer,
                            const bool                 export_sub_layers,
                            Amino::String&             result)
    USDNODE_DOC_ICON("export_layer_to_string",
                     "USD_Layer_export_layer_to_string.md",
                     "export_layer.svg");

USD_NODEDEF_DECL
bool export_layer_to_file(const BifrostUsd::Layer& layer,
                          const Amino::String& file  USDNODE_FILE_BROWSER_SAVE,
                          const bool relative_path
                              AMINO_ANNOTATE("Amino::Port value=true"))
    USDNODE_DOC_ICON_X("export_layer_to_file",
                       "USD_Layer_export_layer_to_file.md",
                       "export_layer.svg",
                       "outName=success");

USD_NODEDEF_DECL
void get_sublayer_paths(
    const BifrostUsd::Stage&                      stage,
    Amino::MutablePtr<Amino::Array<Amino::String>>& sub_layer_paths)
    USDNODE_DOC_ICON("get_sublayer_paths", "USD_Layer_get_sublayer_paths.md", "usd_layers.svg");

USD_NODEDEF_DECL
void add_sublayer(const BifrostUsd::Layer& sub_layer,
                  BifrostUsd::Layer& layer USDPORT_INOUT("new_layer"))
    USDNODE_DOC_ICON_X("add_sublayer",
                       "USD_Layer_add_sublayer.md",
                       "add_layer.svg",
                       "Amino::DefaultOverload");

USD_NODEDEF_DECL
void add_sublayer(
    const Amino::Array<Amino::Ptr<BifrostUsd::Layer>>& sub_layer,
    BifrostUsd::Layer& layer USDPORT_INOUT("new_layer"))
    USDNODE_DOC_ICON("add_sublayer", "USD_Layer_add_sublayer.md", "add_layer.svg");

} // namespace Layer
} // namespace USD

#endif // USD_LAYER_NODEDEFS_H
