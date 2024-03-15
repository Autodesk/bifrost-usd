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
/// \file  usd_stage_nodedefs.h
/// \brief Bifrost USD Stage nodes.

#ifndef USD_STAGE_NODEDEFS_H
#define USD_STAGE_NODEDEFS_H

#include <Bifrost/Object/Object.h>
#include <Amino/Core/Array.h>
#include <Amino/Core/BuiltInTypes.h>
#include <Amino/Core/Ptr.h>
#include <BifrostUsd/Enum.h>
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
/// \defgroup Stage Stage
namespace Stage {
/// \ingroup Stage

/// \defgroup open_stage_from_layer open_stage_from_layer node
///
/// \brief Opens a stage from a given layer.
///
/// \param [in] root_layer The layer that will be the root in the stage.
/// \param [in] mask Loads only the specified prims.
///                  If this is empty, all the prims are loaded.
/// \param [in] load Controls the behavior of USD payload arcs.
///                  LoadAll loads all loadable prims, and
///                  LoadNone records but does not traverse payload arcs
///                  (useful on large scenes to override something without
///                  pulling everything).
/// \param [in] layer_index The sublayer index to set as the stage's EditTarget.
///                  The last element in the list of sublayers is the
///                  strongest of the sublayers in the Pixar USD root layer.
///                  If the sublayer index is -1 or if it does not identify an
///                  existing sublayer, the root layer is set as the EditTarget
///                  of the opened stage.
///
/// \param [out] stage The USD stage.
USD_NODEDEF_DECL
void open_stage_from_layer(const BifrostUsd::Layer&                 root_layer,
                           const Amino::Array<Amino::String>&       mask,
                           const BifrostUsd::InitialLoadSet         load,
                           const int                                layer_index
                               AMINO_ANNOTATE("Amino::Port value=-1"),
                           Amino::MutablePtr<BifrostUsd::Stage>&    stage)
    USDNODE_DOC_ICON("open_stage_from_layer",
                     "open_stage_from_layer",
                     "usd.svg");

/// \ingroup Stage
/// \defgroup open_stage_from_cache open_stage_from_cache node
///
/// \brief Opens from the cache the stage that is associated to the given ID.
///
/// \param [in] id The ID associated to a stage in the cache.
/// \param [in] layer_index The sublayer index to set as the stage's EditTarget.
///                  The last element in the list of sublayers is the
///                  strongest of the sublayers in the Pixar USD root layer.
///                  If the sublayer index is -1 or if it does not identify an
///                  existing sublayer, the root layer is set as the EditTarget
///                  of the opened stage.
///
/// \param [out] stage The USD stage.
USD_NODEDEF_DECL
void open_stage_from_cache(const Amino::long_t              id AMINO_ANNOTATE("Amino::Port metadata=[{UiSoftMin, string, 0}]"),
                           const int                        layer_index
                               AMINO_ANNOTATE("Amino::Port value=-1"),
                           Amino::Ptr<BifrostUsd::Stage>&   stage)
    USDNODE_DOC_ICON("open_stage_from_cache",
                     "open_stage_from_cache",
                     "usd.svg");

/// \ingroup Stage
/// \defgroup set_edit_layer set_edit_layer node
///
/// \brief This node sets the stage's EditTarget to the specified layer.
///
/// \param [in] stage The stage in which to set the edit layer.
/// \param [in] layer_index The sublayer index to set as the stage's EditTarget.
///                  The last element in the list of sublayers is the
///                  strongest of the sublayers in the Pixar USD root layer.
///                  The index -1 identifies the root layer.
///                  If the index is not -1 and does not identify an existing
///                  sublayer, this node does nothing and the current stage's
///                  EditTarget is preserved.
///
USD_NODEDEF_DECL
void set_edit_layer(BifrostUsd::Stage&  stage USDPORT_INOUT("out_stage"),
                    const int           layer_index)
    USDNODE_DOC_ICON("set_edit_layer", "set_edit_layer", "usd.svg");

/// \ingroup Stage
/// \defgroup get_edit_layer get_edit_layer node
///
/// \brief This node gets the layer targeted by the stage.
///
/// \param [in] stage The stage from which to get the edit layer.
/// \param [in] read_only If enabled, the original layer is returned.
///                  Since it is not a copy, it can be useful to retrieve the
///                  layer index of an in-memory layer by comparing layer
///                  identifiers. If you need to modify the returned layer, you
///                  must turn it off or connect a set_layer_permission node
///                  after this node.
/// \param [out] edit_layer The returned layer.
USD_NODEDEF_DECL
void get_edit_layer(const BifrostUsd::Stage& stage,
                    const bool read_only
                        AMINO_ANNOTATE("Amino::Port value=true"),
                    Amino::Ptr<BifrostUsd::Layer>& edit_layer)
    USDNODE_DOC_ICON("get_edit_layer", "get_edit_layer", "usd.svg");

/// \ingroup Stage
/// \defgroup set_default_prim set_default_prim node
///
/// \brief This node sets the stage's default prim.
///
/// \param [in] stage The USD stage.
/// \param [in] prim_path The path of the default prim.
/// \param [out] stage The modified USD stage.
USD_NODEDEF_DECL
void set_default_prim(BifrostUsd::Stage&    stage USDPORT_INOUT("out_stage"),
                      const Amino::String&  prim_path)
    USDNODE_INTERNAL("set_default_prim", "set_default_prim");

/// \ingroup Stage
/// \defgroup get_default_prim get_default_prim node
///
/// \brief This node gets the stage's default prim.
///
/// \param [in] stage The USD stage.
/// \param [out] prim_path The default prim path.
USD_NODEDEF_DECL
void get_default_prim(const BifrostUsd::Stage&  stage,
                      Amino::String&            prim_path)
    USDNODE_DOC_ICON("get_default_prim", "get_default_prim", "usd.svg");

/// \ingroup Stage
/// \defgroup set_stage_up_axis set_stage_up_axis node
///
/// \brief This node sets the stage's up-axis.
///
/// \param [in] stage The stage on which to set the up-axis.
/// \param [in] axis The up axis.
/// \returns true if the up-axis was successfully set.
USD_NODEDEF_DECL
bool set_stage_up_axis(BifrostUsd::Stage&       stage USDPORT_INOUT("out_stage"),
                       const BifrostUsd::UpAxis axis)
    USDNODE_DOC_ICON_X("set_stage_up_axis",
                       "set_stage_up_axis",
                       "usd.svg",
                       "outName=success");

/// \ingroup Stage
/// \defgroup set_stage_time_code set_stage_time_code node
///
/// \brief This node writes USD time code data into the root layer.
///
/// \param [in] stage The stage in which to set the time code.
/// \param [in] start The start time.
/// \param [in] end The end time.
USD_NODEDEF_DECL
void set_stage_time_code(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                         const float        start,
                         const float        end)
    USDNODE_DOC_ICON("set_stage_time_code", "set_stage_time_code", "usd.svg");

/// \ingroup Stage
/// \defgroup set_stage_metadata set_stage_metadata node
///
/// \brief This node sets the value of a stage metadatum if the stage's
/// current UsdEditTarget is the root or session layer.
///
/// \param [in] key The metadatum key.
/// \param [in] value The metadatum value.
/// \param [in] stage The stage in which to set the metadatum.
/// \returns true if the metadata was set successfully.
#define SET_STAGE_METADATA(VALUE_TYPE)                                 \
    USD_NODEDEF_DECL                                                   \
    bool set_stage_metadata(                                           \
        BifrostUsd::Stage&      stage USDPORT_INOUT("out_stage"),      \
        const Amino::String&    key,                                   \
        const VALUE_TYPE&       value)                                 \
        USDNODE_INTERNAL_X("set_stage_metadata", "set_stage_metadata", \
                           "outName=success");

SET_STAGE_METADATA(Amino::String)
SET_STAGE_METADATA(Amino::float_t)
SET_STAGE_METADATA(Amino::double_t)
SET_STAGE_METADATA(Amino::int_t)
SET_STAGE_METADATA(Amino::long_t)
SET_STAGE_METADATA(Amino::bool_t)
SET_STAGE_METADATA(Bifrost::Object)

/// \todo Amino does not accept InOut ports on scalars (BIFROST-6207)

/// \ingroup Stage
/// \defgroup get_stage_metadata get_stage_metadata node
///
/// \brief This node gets the value of a stage metadatum if the stage's
/// current UsdEditTarget is the root or session layer.
///
/// \param [in] key The metadatum key.
/// \param [in] stage The USD stage.
/// \param [out] value The metadatum value.
/// \returns true if the metadata was gotten successfully.
#define GET_STAGE_METADATA(VALUE_TYPE)                                 \
    USD_NODEDEF_DECL                                                   \
    bool get_stage_metadata(                                           \
        const BifrostUsd::Stage&    stage,                             \
        const Amino::String&        key,                               \
        const VALUE_TYPE&           default_and_type,                  \
        VALUE_TYPE&                 value)                             \
        USDNODE_INTERNAL_X("get_stage_metadata", "get_stage_metadata", \
                           "outName=success");

GET_STAGE_METADATA(Amino::String)
GET_STAGE_METADATA(Amino::float_t)
GET_STAGE_METADATA(Amino::double_t)
GET_STAGE_METADATA(Amino::int_t)
GET_STAGE_METADATA(Amino::long_t)
GET_STAGE_METADATA(Amino::bool_t)
GET_STAGE_METADATA(Amino::Ptr<Bifrost::Object>)

/// \todo Amino does not accept InOut ports on scalars (BIFROST-6207)

/// \ingroup Stage
/// \defgroup save_stage save_stage node
///
/// \brief This node writes the composite scene as a flattened USD text representation.
///
/// \param [in] stage The USD stage.
/// \param [in] file The file path to set (optional).
/// \returns true if the save was successful.
USD_NODEDEF_DECL
bool save_stage(const BifrostUsd::Stage&    stage,
                const Amino::String&        file  USDNODE_FILE_BROWSER_SAVE)
    USDNODE_INTERNAL("save_stage", "save_stage");

/// \ingroup Stage
/// \defgroup send_stage_to_cache send_stage_to_cache node
///
/// \brief This sends the stage to the cache and returns its associated ID.
///
/// \param [in] stage The USD stage.
/// \returns The USD stage's cache ID.
USD_NODEDEF_DECL
Amino::long_t send_stage_to_cache(const Amino::Ptr<BifrostUsd::Stage>& stage)
    USDNODE_DOC_ICON_X("send_stage_to_cache",
                       "send_stage_to_cache",
                       "usd.svg",
                       "outName=id");

/// \ingroup Stage
/// \defgroup export_stage_to_string export_stage_to_string node
///
/// \brief This node returns the composite scene as a flattened USD text representation
/// as a string.
///
/// \param [in] stage The USD stage.
/// \param [out] flattened_stage The flattened USD text representation.
USD_NODEDEF_DECL
void export_stage_to_string(const BifrostUsd::Stage&    stage,
                            Amino::String&              result)
    USDNODE_DOC_ICON("export_stage_to_string",
                     "export_stage_to_string",
                     "usd.svg");

/// \ingroup Stage
/// \defgroup export_stage_to_file export_stage_to_file node
///
/// \brief This node writes the composite scene as a flattened USD text representation
/// into the given file.
///
/// \param [in] stage The USD stage.
/// \param [in] file The file to save the flattened stage.
/// \returns true if the save was successful.
USD_NODEDEF_DECL
bool export_stage_to_file(const BifrostUsd::Stage&  stage,
                          const Amino::String&      file  USDNODE_FILE_BROWSER_SAVE)
    USDNODE_DOC_ICON_X("export_stage_to_file",
                       "export_stage_to_file",
                       "usd.svg",
                       "outName=success");

} // namespace Stage
} // namespace USD

#endif // USD_STAGE_NODEDEFS_H
