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

/// \file  usd_layer_nodedefs.h
/// \brief Bifrost USD Layer nodes.

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

/// \defgroup USD USD
namespace USD {

/// \ingroup USD
/// \defgroup Layer Layer
namespace Layer {

/// \ingroup Layer
/// \defgroup get_root_layer get_root_layer node
///
/// \brief This node returns the stage's root layer.
///
/// \param [in] stage The USD stage.
/// \param [out] layer The root layer of the stage.
USD_NODEDEF_DECL
void get_root_layer(const BifrostUsd::Stage&        stage,
                    Amino::Ptr<BifrostUsd::Layer>& layer)
    USDNODE_DOC_ICON("get_root_layer", "get_root_layer", "usd.svg");

/// \ingroup Layer
/// \defgroup get_layer get_layer node
///
/// \brief This node returns a sublayer from the stage's root layer.
/// The last element in the list of sublayers is the strongest of the
/// sublayers in the Pixar USD root layer.
///
/// \param [in] stage The USD stage.
/// \param [in] layer_index The sublayer index. If -1, the root layer is returned.
/// \param [out] layer The returned layer.
USD_NODEDEF_DECL
void get_layer(const BifrostUsd::Stage&       stage,
               const int                      layer_index,
               Amino::Ptr<BifrostUsd::Layer>& layer)
    USDNODE_DOC_ICON("get_layer", "get_layer", "usd.svg");

/// \ingroup Layer
/// \defgroup replace_layer replace_layer node
///
/// \brief This node replaces the sublayer from the stage's root layer at the
/// specified index.
///
/// The last element in the list of sublayers is the strongest of the
/// sublayers in the Pixar USD root layer.
///
/// \param [in,out] stage The USD stage in which to replace the sublayer.
/// \param [in] sublayer_index The sublayer index (of the root layer) to replace.
/// \param [in] new_layer The new layer that will replace the existing one.
/// \returns true if the sublayer was replaced successfully.
USD_NODEDEF_DECL
bool replace_layer(BifrostUsd::Stage&       stage USDPORT_INOUT("out_stage"),
                   const int                sublayer_index,
                   const BifrostUsd::Layer& new_layer)
    USDNODE_DOC_ICON_X("replace_layer",
                       "replace_layer",
                       "usd.svg",
                       "outName=success");

/// \ingroup Layer
/// \defgroup create_layer create_layer node
///
/// \brief This creates a new anonymous layer.
///
/// \param [in] save_file The file path used when saving the layer.
/// \param [out] layer The created anonymous layer.
USD_NODEDEF_DECL
void create_layer(const Amino::String& save_file USDNODE_FILE_BROWSER_SAVE,
                  Amino::MutablePtr<BifrostUsd::Layer>& layer)
    USDNODE_INTERNAL("create_layer", "create_layer");

/// \ingroup Layer
/// \defgroup open_layer open_layer node
///
/// \brief This creates a new anonymous layer from a file on disk.
///
/// \param [in] file The file path of the layer to open as anonymous.
/// \param [in] save_file The file path used when saving the layer.
/// \param [in] read_only To set the layer as non editable.
/// \param [out] layer The created anonymous layer.
USD_NODEDEF_DECL
void open_layer(const Amino::String& file      USDNODE_FILE_BROWSER_OPEN,
                const Amino::String& save_file USDNODE_FILE_BROWSER_SAVE,
                const bool read_only AMINO_ANNOTATE("Amino::Port value=false"),
                Amino::MutablePtr<BifrostUsd::Layer>& layer)
    USDNODE_DOC_ICON("open_layer", "open_layer", "usd.svg");

/// \ingroup Layer
/// \defgroup duplicate_layer duplicate_layer node
///
/// \brief This node duplicate the input layer.
///
/// \param [in] layer The USD layer.
/// \param [in] save_file The file to save the new layer.
/// \param [out] new_layer The USD layer duplicating the input layer.
USD_NODEDEF_DECL
void duplicate_layer(const BifrostUsd::Layer&     layer,
                     const Amino::String& save_file USDNODE_FILE_BROWSER_SAVE,
                     Amino::MutablePtr<BifrostUsd::Layer>& new_layer)
    USDNODE_DOC_ICON("duplicate_layer", "duplicate_layer", "usd.svg");

/// \ingroup Layer
/// \defgroup get_layer_identifier get_layer_identifier node
///
/// \brief This node returns the layer identifier.
///
/// \param [in] layer The USD layer.
/// \param [out] identifier The identifier of the layer.
USD_NODEDEF_DECL
void get_layer_identifier(const BifrostUsd::Layer& layer,
                          Amino::String&             identifier)
    USDNODE_DOC_ICON("get_layer_identifier", "get_layer_identifier", "usd.svg");

/// \ingroup Layer
/// \defgroup get_layer_file_path get_layer_file_path node
///
/// \brief This node returns the file path used to save the layer.
///
/// \param [in] layer The USD layer.
/// \param [out] file The file path used to save the layer.
USD_NODEDEF_DECL
void get_layer_file_path(const BifrostUsd::Layer& layer, Amino::String& file)
    USDNODE_DOC_ICON("get_layer_file_path", "get_layer_file_path", "usd.svg");

/// \ingroup Layer
/// \defgroup export_layer_to_string export_layer_to_string node
///
/// \brief This node exports a layer to a string.
///
/// \param [in] layer The layer to export.
/// \param [in] export_sub_layers Exports any sublayers.
/// \param [out] result The layer in text format.
USD_NODEDEF_DECL
void export_layer_to_string(const BifrostUsd::Layer& layer,
                            const bool                 export_sub_layers,
                            Amino::String&             result)
    USDNODE_DOC_ICON("export_layer_to_string",
                     "export_layer_to_string",
                     "usd.svg");

/// \ingroup Layer
/// \defgroup export_layer_to_file export_layer_to_file node
///
/// \brief This node writes the composite scene as a flattened USD text representation
/// into the given file.
///
/// \param [in] layer The USD layer.
/// \param [in] file The file to save the layer.
/// \param [in] relative_path Stores sublayers with their relative paths.
/// \returns true if layer was exported to file successfully.
USD_NODEDEF_DECL
bool export_layer_to_file(const BifrostUsd::Layer& layer,
                          const Amino::String& file  USDNODE_FILE_BROWSER_SAVE,
                          const bool relative_path
                              AMINO_ANNOTATE("Amino::Port value=true"))
    USDNODE_DOC_ICON_X("export_layer_to_file",
                       "export_layer_to_file",
                       "usd.svg",
                       "outName=success");

/// \ingroup Layer
/// \defgroup get_sublayer_paths get_sublayer_paths node
///
/// \brief This node returns the sublayer paths directly included in the stage.
/// The returned paths are listed from the weakest to the strongest of the
/// sublayers in the USD Layer.
///
/// \param [in] stage The USD stage.
/// \param [out] sub_layer_paths The layer paths.
USD_NODEDEF_DECL
void get_sublayer_paths(
    const BifrostUsd::Stage&                      stage,
    Amino::MutablePtr<Amino::Array<Amino::String>>& sub_layer_paths)
    USDNODE_DOC_ICON("get_sublayer_paths", "get_sublayer_paths", "usd.svg");

/// \ingroup Layer
/// \defgroup add_sublayer add_sublayer node
///
/// \brief This node adds a sublayer at the end of the sublayers of the given
/// layer. The newly added sublayer becomes the strongest of the sublayers
/// in the Pixar USD root layer.
///
/// \param [in] sub_layer The sublayer to add to the layer.
/// \param [in,out] layer The layer in which to add the sublayer.
USD_NODEDEF_DECL
void add_sublayer(const BifrostUsd::Layer& sub_layer,
                  BifrostUsd::Layer& layer USDPORT_INOUT("new_layer"))
    USDNODE_DOC_ICON_X("add_sublayer",
                       "add_sublayer",
                       "usd.svg",
                       "Amino::DefaultOverload");

/// \ingroup Layer
/// \defgroup add_sublayer add_sublayer node
///
/// \brief This node adds sublayers at the end of the sublayers of the given
/// layer. The last element in the given array of sublayers becomes the
/// strongest of the sublayers in the Pixar USD root layer.
///
/// \param [in] sub_layer The sublayers to add to the layer.
/// \param [in,out] layer The layer in which to add the sublayers.
USD_NODEDEF_DECL
void add_sublayer(
    const Amino::Array<Amino::Ptr<BifrostUsd::Layer>>& sub_layer,
    BifrostUsd::Layer& layer USDPORT_INOUT("new_layer"))
    USDNODE_DOC_ICON("add_sublayer", "add_sublayer", "usd.svg");

} // namespace Layer
} // namespace USD

#endif // USD_LAYER_NODEDEFS_H
