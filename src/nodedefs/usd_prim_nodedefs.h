//-
// Copyright 2023 Autodesk, Inc.
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

/// \file  usd_prim_nodedefs.h
/// \brief Bifrost USD Prim nodes.

#ifndef USD_PRIM_NODEDEFS_H
#define USD_PRIM_NODEDEFS_H

#include <Amino/Core/Array.h>
#include <Amino/Core/Ptr.h>
#include <Bifrost/Math/Types.h>
#include <Bifrost/Object/Object.h>
#include <BifrostUsd/Layer.h>
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
/// \defgroup Prim Prim
namespace Prim {

/// \ingroup Prim
/// \defgroup get_prim_at_path get_prim_at_path node
///
/// \brief Returns the USD prim at the specified path in the stage.
///
/// \param [in] stage The USD stage.
/// \param [in] path The path to the USD prim.
/// \param [out] prim The USD Prim at that path.
/// \returns true if the prim is valid.
USD_NODEDEF_DECL
bool get_prim_at_path(Amino::Ptr<BifrostUsd::Stage>       stage,
                      const Amino::String&                   path,
                      Amino::MutablePtr<BifrostUsd::Prim>& prim)
    USDNODE_DOC_ICON_X("get_prim_at_path",
                       "get_prim_at_path",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup get_prim_children get_prim_children node
///
/// \brief Returns the array of children of a prim.
///
/// \param [in] stage The USD stage.
/// \param [in] prim_path The USD prim path.
/// \param [out] children The array of children.
USD_NODEDEF_DECL
void get_prim_children(
    Amino::Ptr<BifrostUsd::Stage>       stage,
    const Amino::String&                   prim_path,
    const BifrostUsd::PrimDescendantMode descendant_mode,
    Amino::MutablePtr<Amino::Array<Amino::Ptr<BifrostUsd::Prim>>>& children)
    USDNODE_DOC_ICON("get_prim_children", "get_prim_children", "usd.svg");

/// \ingroup Prim
/// \defgroup get_prim_path get_prim_path node
///
/// \brief Returns the path of a USD prim.
///
/// \param [in] prim The USD prim.
/// \param [out] path The path of that USD prim.
USD_NODEDEF_DECL
void get_prim_path(const BifrostUsd::Prim& prim, Amino::String& path)
    USDNODE_DOC_ICON("get_prim_path", "get_prim_path", "usd.svg");

/// \ingroup Prim
/// \defgroup get_last_modified_prim get_last_modified_prim node
///
/// \brief Returns the last USD prim authored in the stage.
///
/// \param [in] stage The USD ptage.
/// \param [out] prim The last modified USD prim.
/// \returns true if the prim is valid.
USD_NODEDEF_DECL
bool get_last_modified_prim(Amino::Ptr<BifrostUsd::Stage>       stage,
                            Amino::MutablePtr<BifrostUsd::Prim>& prim)
    USDNODE_DOC_ICON_X("get_last_modified_prim",
                       "get_last_modified_prim",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup get_prim_type get_prim_type node
///
/// \brief Returns the type name of a USD prim.
///
/// \param [in] prim The USD prim.
/// \param [out] type_name The USD prim type name.
USD_NODEDEF_DECL
void get_prim_type(const BifrostUsd::Prim& prim, Amino::String& type_name)
    USDNODE_DOC_ICON("get_prim_type", "get_prim_type", "usd.svg");

/// \ingroup Prim
/// \defgroup get_all_attribute_names get_all_attribute_names node
///
/// \brief Returns a list of all attribute names in a USD prim.
///
/// \param [in] prim The USD prim.
/// \param [out] names An array of strings containing the USD prim's attributes' names.
USD_NODEDEF_DECL
void get_all_attribute_names(const BifrostUsd::Prim&                         prim,
                             Amino::MutablePtr<Amino::Array<Amino::String>>& names)
    USDNODE_DOC_ICON("get_all_attribute_names",
                     "get_all_attribute_names",
                     "usd.svg");

/// \ingroup Prim
/// \defgroup get_authored_attribute_names get_authored_attribute_names node
///
/// \brief Returns a list of authored attribute names in a USD prim.
///
/// \param [in] prim The USD prim.
/// \param [out] names An array of strings containing the USD prim's attributes' names.
USD_NODEDEF_DECL
void get_authored_attribute_names(
    const BifrostUsd::Prim&                         prim,
    Amino::MutablePtr<Amino::Array<Amino::String>>& names)
    USDNODE_DOC_ICON("get_authored_attribute_names",
                     "get_authored_attribute_names",
                     "usd.svg");

/// \ingroup Prim
/// \defgroup create_prim create_prim node
///
/// \brief Creates a prim of the given type (optional) at the given prim path
/// in the given stage.
///
/// \param [in] path The path to the USD prim.
/// \param [in] type The type of the USD prim (optional).
/// \param [in] stage The USD stage in which to define the prim.
USD_NODEDEF_DECL
void create_prim(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                 const Amino::String&       path,
                 const Amino::String&       type)
    USDNODE_INTERNAL("create_prim", "create_prim");

/// \ingroup Prim
/// \defgroup create_class_prim create_class_prim node
///
/// \brief Creates a class prim in the given stage.
///
/// \param [in] stage The USD stage in which to create a class prim.
/// \param [in] path The path to the USD prim.
USD_NODEDEF_DECL
void create_class_prim(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                       const Amino::String&       path)
    USDNODE_DOC_ICON("create_class_prim", "create_class_prim", "usd.svg");

/// \ingroup Prim
/// \defgroup override_prim override_prim node
///
/// \brief Returns a new copy of the stage with an override prim added.
///
/// \param [in] stage The USD stage.
/// \param [in] path The path to the USD prim.
/// \param [out] out_stage The USD stage with the new prim.
USD_NODEDEF_DECL
void override_prim(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                   const Amino::String&       path)
    USDNODE_DOC_ICON("override_prim", "override_prim", "usd.svg");

/// \ingroup Prim
/// \defgroup add_applied_schema add_applied_schema node
///
/// \brief Adds the applied API schema to the prim.
///
/// \param [in] stage The USD stage holding the prim.
/// \param [in] prim_path The path to the prim the schema will be added to.
/// \param [in] applied_schema_name The applied schema name.
///             This does not require that the name token refer to
///              a valid API schema type.
/// \returns true if the schema is added successfully or if
///          it is already applied in current edit target.
USD_NODEDEF_DECL
bool add_applied_schema(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                        const Amino::String&     prim_path,
                        const Amino::String&     applied_schema_name)
    USDNODE_DOC_ICON_X("add_applied_schema",
                       "add_applied_schema",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup remove_applied_schema remove_applied_schema node
///
/// \brief Removes the applied API schema from the prim.
///
/// \param [in] stage The USD stage holding the prim.
/// \param [in] prim_path The path to the prim the schema will be removed from.
/// \param [in] applied_schema_name The applied schema name.
/// \returns true if the schema is removed successfully or if
///               it is already deleted in current edit target.
USD_NODEDEF_DECL
bool remove_applied_schema(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                           const Amino::String&     prim_path,
                           const Amino::String&     applied_schema_name)
    USDNODE_DOC_ICON_X("remove_applied_schema",
                       "remove_applied_schema",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup add_reference_prim add_reference_prim node
///
/// \brief Adds a prim reference in the given stage.
///
/// \param [in] stage The USD stage in which to add a reference prim.
/// \param [in] prim_path The prim that will reference something.
/// \param [in] reference_layer The referenced layer (empty if internal).
/// \param [in] reference_prim_path The prim that will be referenced.
/// \param [in] layer_offset The layer time offset.
/// \param [in] layer_scale The layer offset scale factor.
/// \param [in] reference_position The position in the reference list.
/// \returns true if the reference was added successfully.
USD_NODEDEF_DECL
bool add_reference_prim(
    BifrostUsd::Stage& stage          USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const BifrostUsd::Layer&          reference_layer,
    const Amino::String&                reference_prim_path,
    const double layer_offset           AMINO_ANNOTATE("Amino::Port value=0.0"),
    const double layer_scale            AMINO_ANNOTATE("Amino::Port value=1.0"),
    const BifrostUsd::UsdListPosition reference_position)
    USDNODE_DOC_ICON_X("add_reference_prim",
                       "add_reference_prim",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup remove_reference_prim remove_reference_prim node
///
/// \brief Removes the given prim reference from the given stage.
///
/// \param [in] stage The USD stage in which to remove the reference prim.
/// \param [in] prim_path The prim path from where to remove a reference.
/// \param [in] reference_layer_identifier The file of the reference to remove (empty
///             if internal).
/// \param [in] reference_prim_path The prim path of the reference to remove.
/// \param [in] layer_offset The layer time offset.
/// \param [in] clear_all Removes every reference, ignoring the other parameters.
/// \returns true if the reference was removed successfully.
USD_NODEDEF_DECL
bool remove_reference_prim(
    BifrostUsd::Stage& stage                      USDPORT_INOUT("out_stage"),
    const Amino::String&                            prim_path,
    const Amino::String& reference_layer_identifier USDNODE_FILE_BROWSER_OPEN,
    const Amino::String&                            reference_prim_path,
    const double layer_offset AMINO_ANNOTATE("Amino::Port value=0.0"),
    const bool clear_all) USDNODE_DOC_ICON_X("remove_reference_prim",
                                             "remove_reference_prim",
                                             "usd.svg",
                                             "outName=success");

/// \ingroup Prim
/// \defgroup remove_payload_prim remove_payload_prim node
///
/// \brief Removes the given prim payload from the given stage.
///
/// \param [in] stage The USD stage.
/// \param [in] prim_path The prim path from where to remove a reference.
/// \param [in] payload_layer_identifier The file of the reference to remove (empty
///             if internal)
/// \param [in] reference_prim_path The prim path of the reference to remove
/// \param [in] layer_offset The layer time offset
/// \param [in] clear_all Remove every references, ignoring the other parameters
/// \param [in] out_stage The USD stage in which to remove the payload prim.
/// \returns true if the reference was removed successfully.
USD_NODEDEF_DECL
bool remove_payload_prim(
    BifrostUsd::Stage& stage                    USDPORT_INOUT("out_stage"),
    const Amino::String&                          prim_path,
    const Amino::String& payload_layer_identifier USDNODE_FILE_BROWSER_OPEN,
    const Amino::String&                          payload_prim_path,
    const double layer_offset AMINO_ANNOTATE("Amino::Port value=0.0"),
    const bool clear_all) USDNODE_DOC_ICON_X("remove_payload_prim",
                                             "remove_payload_prim",
                                             "usd.svg",
                                             "outName=success");

/// \ingroup Prim
/// \defgroup add_payload_prim add_payload_prim node
///
/// \brief Adds a payload arc to a prim.
///
/// \param [out] stage The USD stage in which to add the payload prim.
/// \param [in] prim_path The prim that will reference something.
/// \param [in] payload_layer The payload layer (empty if internal).
/// \param [in] payload_prim_path The prim that will be payloaded.
/// \param [in] layer_offset The layer time offset.
/// \param [in] layer_scale The layer offset scale factor.
/// \param [in] payload_position The position in the payload list.
/// \returns true if the payload was added successfully.
USD_NODEDEF_DECL
bool add_payload_prim(
    BifrostUsd::Stage& stage          USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const BifrostUsd::Layer&          payload_layer,
    const Amino::String&                payload_prim_path,
    const double layer_offset           AMINO_ANNOTATE("Amino::Port value=0.0"),
    const double layer_scale            AMINO_ANNOTATE("Amino::Port value=1.0"),
    const BifrostUsd::UsdListPosition payload_position)
    USDNODE_DOC_ICON_X("add_payload_prim",
                       "add_payload_prim",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup add_inherit_prim add_inherit_prim node
///
/// \brief Adds an "inherits" arc to a prim.
///
/// \param [in] stage The USD stage in which to add a prim with an Inherits arc.
/// \param [in] prim_path The prim that will inherit.
/// \param [in] inherited_prim_path The prim that will be inherited.
/// \param [in] inherit_position The position in the inherit list.
/// \returns true if the inherit was added successfully.
USD_NODEDEF_DECL
bool add_inherit_prim(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                      const Amino::String&       prim_path,
                      const Amino::String&       inherited_prim_path,
                      const BifrostUsd::UsdListPosition inherit_position) //
    USDNODE_DOC_ICON_X("add_inherit_prim",
                       "add_inherit_prim",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup add_specialize_prim add_specialize_prim node
///
/// \brief Adds a "specializes" arc to a prim.
///
/// \param [in] stage The USD stage in which to add an arc on the given prim.
/// \param [in] prim_path The prim that will specialize.
/// \param [in] specialized_prim_path The prim that will be specialized.
/// \param [in] specialize_position The position in the specialize list.
/// \returns true if the inherit was added successfully.
USD_NODEDEF_DECL
bool add_specialize_prim(
    BifrostUsd::Stage& stage          USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const Amino::String&                specialized_prim_path,
    const BifrostUsd::UsdListPosition specialize_position)
    USDNODE_DOC_ICON_X("add_specialize_prim",
                       "add_specialize_prim",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup create_prim_relationship create_prim_relationship node
///
/// \brief Returns a new copy of the stage with a relationship added to a
/// prim.
///
/// \param [in] stage The USD stage in which to create a prim relationship.
/// \param [in] prim_path The prim that will own the relationship.
/// \param [in] rel_name The relationship name.
/// \param [in] custom Declares that the new relationship is user-defined.
/// \param [in] target The relationship target path.
/// \param [in] target_position The position of the relationship in the list.
/// \returns true if the relationship was added successfully.
USD_NODEDEF_DECL
bool create_prim_relationship(
    BifrostUsd::Stage& stage          USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const Amino::String&                rel_name,
    const bool                          custom,
    const Amino::String&                target,
    const BifrostUsd::UsdListPosition target_position)
    USDNODE_DOC_ICON_X("create_prim_relationship",
                       "create_prim_relationship",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup add_relationship_target add_relationship_target node
///
/// \brief Adds a relationship target.
///
/// \param [in] stage The USD stage in which to add a relationship target.
/// \param [in] prim_path The prim that has the relationship.
/// \param [in] rel_name The relationship name.
/// \param [in] target The relationship target path.
/// \param [in] target_position The position in the list.
/// \returns true if the relationship target was added successfully.
USD_NODEDEF_DECL
bool add_relationship_target(
    BifrostUsd::Stage& stage          USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const Amino::String&                rel_name,
    const Amino::String&                target,
    const BifrostUsd::UsdListPosition target_position)
    USDNODE_DOC_ICON_X("add_relationship_target",
                       "add_relationship_target",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup remove_relationship_target remove_relationship_target node
///
/// \brief Removes a relationship target.
///
/// \param [in] stage The USD stage in which to remove a relationship target.
/// \param [in] prim_path The prim that has the relationship.
/// \param [in] rel_name The relationship name.
/// \param [in] target The relationship target path.
/// \returns true if the relationship target was removed successfully.
USD_NODEDEF_DECL
bool remove_relationship_target(BifrostUsd::Stage& stage
                                                     USDPORT_INOUT("out_stage"),
                                const Amino::String& prim_path,
                                const Amino::String& rel_name,
                                const Amino::String& target)
    USDNODE_DOC_ICON_X("remove_relationship_target",
                       "remove_relationship_target",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup set_relationship_targets set_relationship_targets node.
///
/// \brief Sets the target list explicitly to the list provided.
/// Note that this fails if any of the targets are invalid.
///
/// \param [in] stage The USD stage in which to set relationship targets.
/// \param [in] prim_path The prim that has the relationship.
/// \param [in] rel_name The relationship name.
/// \param [in] targets The relationship target paths.
/// \returns true if the relationship targets was set successfully.
USD_NODEDEF_DECL
bool set_relationship_targets(BifrostUsd::Stage& stage
                                                   USDPORT_INOUT("out_stage"),
                              const Amino::String& prim_path,
                              const Amino::String& rel_name,
                              const Amino::Array<Amino::String>& targets)
    USDNODE_DOC_ICON_X("set_relationship_targets",
                       "set_relationship_targets",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup clear_relationship_targets clear_relationship_targets node.
///
/// \brief Removes all opinions about the target list from the current edit
/// target.
///
/// \param [in] stage The USD stage in which to clear a relationship target.
/// \param [in] prim_path The prim that has the relationship.
/// \param [in] rel_name The relationship name.
/// \param [in] remove_spec Additionally removes the spec. You should leave the spec if you want to preserve metadata.
/// \returns true if the relationship targets was cleared successfully.
USD_NODEDEF_DECL
bool clear_relationship_targets(
    BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
    const Amino::String&       prim_path,
    const Amino::String&       rel_name,
    const bool remove_spec) USDNODE_DOC_ICON_X("clear_relationship_targets",
                                               "clear_relationship_targets",
                                               "usd.svg",
                                               "outName=success");

/// \ingroup Prim
/// \defgroup get_relationship_targets get_relationship_targets node.
///
/// \brief Composes this relationship's targets and returns the list.
///
/// \param [in] stage The USD stage in which to get relationship targets.
/// \param [in] prim_path The prim that has the relationship.
/// \param [in] rel_name The relationship name.
/// \param [in] targets The relationship target paths.
/// \returns true if the relationship targets were gotten successfully.
USD_NODEDEF_DECL
bool get_relationship_targets(
    BifrostUsd::Stage& stage                      USDPORT_INOUT("out_stage"),
    const Amino::String&                            prim_path,
    const Amino::String&                            rel_name,
    Amino::MutablePtr<Amino::Array<Amino::String>>& targets)
    USDNODE_DOC_ICON_X("get_relationship_targets",
                       "get_relationship_targets",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup get_forwarded_relationship_targets get_forwarded_relationship_targets node.
///
/// \brief Composes this relationship's ultimate targets, taking into
/// account "relationship forwarding" and returns the list.
///
/// \param [in] stage The USD stage in which to get relationship targets.
/// \param [in] prim_path The prim that has the relationship.
/// \param [in] rel_name The relationship name.
/// \param [in] targets The relationship target paths.
/// \returns true if the relationship targets were returned successfully.
USD_NODEDEF_DECL
bool get_forwarded_relationship_targets(
    BifrostUsd::Stage& stage                      USDPORT_INOUT("out_stage"),
    const Amino::String&                            prim_path,
    const Amino::String&                            rel_name,
    Amino::MutablePtr<Amino::Array<Amino::String>>& targets)
    USDNODE_DOC_ICON_X("get_forwarded_relationship_targets",
                       "get_forwarded_relationship_targets",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup prim_is_instanceable prim_is_instanceable node.
///
/// \brief Returns whether this prim has been marked as instanceable.
///
/// \param [in] stage The USD stage.
/// \param [in] prim_path The path to the USD prim.
/// \returns true if this prim has been marked as instanceable.
USD_NODEDEF_DECL
bool prim_is_instanceable(const BifrostUsd::Stage& stage,
                          const Amino::String&       prim_path)
    USDNODE_DOC_ICON_X("prim_is_instanceable",
                       "prim_is_instanceable",
                       "usd.svg",
                       "outName=is_instanceable");

/// \ingroup Prim
/// \defgroup set_prim_instanceable set_prim_instanceable node.
///
/// \brief Sets "instanceable" metadata for this prim.
///
/// \param [in] stage The USD stage in which to set instanciability of the prim.
/// \param [in] prim_path The path to the USD prim.
/// \param [in] instanceable The instanceable metadata for this prim.
/// \returns true if the prim was successfully set as instanceable.
USD_NODEDEF_DECL
bool set_prim_instanceable(
    BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
    const Amino::String&       prim_path,
    const bool instanceable) USDNODE_DOC_ICON_X("set_prim_instanceable",
                                                "set_prim_instanceable",
                                                "usd.svg",
                                                "outName=success");

/// \ingroup Prim
/// \defgroup prim_clear_instanceable prim_clear_instanceable node.
///
/// \brief Removes the authored "instanceable" opinion for this prim.
///
/// \param [in] stage The USD stage in which to clear the instanciability.
/// \param [in] prim_path The path to the USD prim with
/// metadata.
/// \returns true if the instanceable metatdata was successfully cleared.
USD_NODEDEF_DECL
bool prim_clear_instanceable(BifrostUsd::Stage& stage
                                                  USDPORT_INOUT("out_stage"),
                             const Amino::String& prim_path)
    USDNODE_DOC_ICON_X("prim_clear_instanceable",
                       "prim_clear_instanceable",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup prim_has_authored_instanceable prim_has_authored_instanceable node
///
/// \brief Returns true if this prim has an authored opinion for
/// "instanceable", false otherwise.
///
/// \param [in] stage The USD stage.
/// \param [in] prim_path The path to the USD prim.
/// \returns true if this prim has an authored opinion for 'instanceable'.
USD_NODEDEF_DECL
bool prim_has_authored_instanceable(const BifrostUsd::Stage& stage,
                                    const Amino::String&       prim_path)
    USDNODE_DOC_ICON_X("prim_has_authored_instanceable",
                       "prim_has_authored_instanceable",
                       "usd.svg",
                       "outName=has_authored_instanceable");

/// \ingroup Prim
/// \defgroup prim_is_instance prim_is_instance node
///
/// \brief Returns true if the specified prim is an instance of a prototype,
///  false otherwise.
///
/// \param [in] stage The USD stage.
/// \param [in] prim_path The path to the USD prim.
/// \returns true if this prim is an instance of a prototype.
USD_NODEDEF_DECL
bool prim_is_instance(const BifrostUsd::Stage& stage,
                      const Amino::String&       prim_path)
    USDNODE_DOC_ICON_X("prim_is_instance",
                       "prim_is_instance",
                       "usd.svg",
                       "outName=is_instance");

/// \ingroup Prim
/// \defgroup prim_is_instance_proxy prim_is_instance_proxy node
///
/// \brief Returns true if the specified prim is an instance proxy,
///  false otherwise.
///
/// \param [in] stage The USD stage.
/// \param [in] prim_path The path to the USD prim.
/// \returns true if this prim is an instance proxy.
USD_NODEDEF_DECL
bool prim_is_instance_proxy(const BifrostUsd::Stage& stage,
                            const Amino::String&       prim_path)
    USDNODE_DOC_ICON_X("prim_is_instance_proxy",
                       "prim_is_instance_proxy",
                       "usd.svg",
                       "outName=is_instance_proxy");

/// \ingroup Prim
/// \defgroup prim_is_prototype prim_is_prototype node
///
/// \brief Returns true if the specified prim is a prototype prim, false otherwise.
///
/// \param [in] stage The USD stage.
/// \param [in] prim_path The path to the USD prim.
/// \returns true if this prim is a prototype prim.
USD_NODEDEF_DECL
bool prim_is_prototype(const BifrostUsd::Stage& stage,
                       const Amino::String&       prim_path)
    USDNODE_DOC_ICON_X("prim_is_prototype",
                       "prim_is_prototype",
                       "usd.svg",
                       "outName=is_prototype");

/// \ingroup Prim
/// \defgroup prim_is_in_prototype prim_is_in_prototype node
///
/// \brief Returns true if the specified prim is located in a subtree of prims rooted
/// at a prototype prim, false otherwise.
///
/// \param [in] stage The USD stage.
/// \param [in] prim_path The path to the USD prim.
/// \returns true if this prim is located in a subtree of prims rooted at a
///          prototype prim.
USD_NODEDEF_DECL
bool prim_is_in_prototype(const BifrostUsd::Stage& stage,
                          const Amino::String&       prim_path)
    USDNODE_DOC_ICON_X("prim_is_in_prototype",
                       "prim_is_in_prototype",
                       "usd.svg",
                       "outName=is_in_prototype");

/// \ingroup Prim
/// \defgroup prim_get_prototype prim_get_prototype node
///
/// \brief Returns the prim path to to the corresponding prototype
/// if this prim is an instance, or an empty path otherwise.
///
/// \param [in] stage The USD stage.
/// \param [in] prim_path The path to the USD prim.
/// \param [out] proto_path The prim path of the prototype for this instance.
USD_NODEDEF_DECL
void prim_get_prototype(const BifrostUsd::Stage& stage,
                        const Amino::String&       prim_path,
                        Amino::String&             proto_path)
    USDNODE_DOC_ICON("prim_get_prototype", "prim_get_prototype", "usd.svg");

/// \ingroup Prim
/// \defgroup get_prim_in_prototype get_prim_in_prototype node
///
/// \brief Returns the prim path of the corresponding prim in the
/// instance's prototype if this prim is an instance proxy.
///
/// \param [in] stage The USD stage.
/// \param [in] prim_path The path to the USD prim.
/// \param [out] prim_in_proto_path The prim path of the corresponding prim in
///                                  the instance's prototype.
USD_NODEDEF_DECL
void get_prim_in_prototype(const BifrostUsd::Stage& stage,
                           const Amino::String&       prim_path,
                           Amino::String&             prim_in_proto_path)
    USDNODE_DOC_ICON("get_prim_in_prototype",
                     "get_prim_in_prototype",
                     "usd.svg");

/// \ingroup Prim
/// \defgroup get_prim_instances get_prim_instances node
///
/// \brief Returns all prims that are instances of this prototype,
/// if this prim is a prototype prim.
///
/// Note that this function will return prims in prototypes for instances
/// that are nested beneath other instances.
///
/// \param [in] stage The USD stage
/// \param [in] proto_prim_path The path to a prototype USD prim.
/// \param [out] instances_paths Returns the paths for all the prototype
///                                prims in the stage.
USD_NODEDEF_DECL
void get_prim_instances(
    const BifrostUsd::Stage&                      stage,
    const Amino::String&                            proto_prim_path,
    Amino::MutablePtr<Amino::Array<Amino::String>>& instances_paths)
    USDNODE_DOC_ICON("get_prim_instances", "get_prim_instances", "usd.svg");

/// \ingroup Prim
/// \defgroup get_prototype_prims get_prototype_prims node
///
/// \brief Returns all prototype prims in this stage.
///
/// \param [in] stage The USD stage.
/// \param [out] proto_prim_paths Returns the paths for all the prototype
///                                prims in the stage.
USD_NODEDEF_DECL
void get_prototype_prims(
    const BifrostUsd::Stage&                      stage,
    Amino::MutablePtr<Amino::Array<Amino::String>>& proto_prim_paths)
    USDNODE_DOC_ICON("get_prototype_prims", "get_prototype_prims", "usd.svg");

/// \ingroup Prim
/// \defgroup set_prim_purpose set_prim_purpose node
///
/// \brief Authors a purpose for the specified prim.
///
/// \param [in] stage The USD stage in which to set the prim purpose.
/// \param [in] path The path to the USD prim.
/// \param [in] purpose The purpose of the USD prim.
USD_NODEDEF_DECL
void set_prim_purpose(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                      const Amino::String&       path,
                      const BifrostUsd::ImageablePurpose purpose)
    USDNODE_DOC_ICON("set_prim_purpose", "set_prim_purpose", "usd.svg");

/// \ingroup Prim
/// \defgroup set_prim_kind set_prim_kind node
///
/// \brief Authors a kind for the specified prim.
///
/// \param [in] path The path to the USD prim.
/// \param [in] kind The kind of the USD prim.
/// \param [in] stage The USD stage in which to set the prim kind.
USD_NODEDEF_DECL
void set_prim_kind(const Amino::String&       path,
                   const Amino::String&       kind,
                   BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"))
    USDNODE_INTERNAL("set_prim_kind", "set_prim_kind");

/// \ingroup Prim
/// \defgroup set_prim_asset_info set_prim_asset_info node
///
/// \brief Sets the model's info dictionary for the specified prim.
///
/// \param [in] stage The USD stage.
/// \param [in] path The path to the USD prim.
/// \param [in] asset_identifier The asset identifier.
/// \param [in] asset_name The name of the asset.
/// \param [in] asset_version The version of the asset.
USD_NODEDEF_DECL
void set_prim_asset_info(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                         const Amino::String&     path,
                         const Amino::String&     asset_identifier,
                         const Amino::String&     asset_name,
                         const Amino::String&     asset_version)
    USDNODE_DOC_ICON("set_prim_asset_info", "set_prim_asset_info", "usd.svg");

/// \ingroup Prim
/// \defgroup get_prim_asset_info get_prim_asset_info node
///
/// \brief Returns the asset identifier, name, and version.
///
/// \param [in] stage The USD stage.
/// \param [in] path The path to the USD prim.
/// \param [out] asset_identifier The asset identifier.
/// \param [out] asset_name The name of the asset.
/// \param [out] asset_version The version of the asset.
USD_NODEDEF_DECL
void get_prim_asset_info(const BifrostUsd::Stage& stage,
                         const Amino::String&     path,
                         Amino::String&     asset_identifier,
                         Amino::String&     asset_name,
                         Amino::String&     asset_version)
    USDNODE_DOC_ICON("get_prim_asset_info", "get_prim_asset_info", "usd.svg");

/// \ingroup Prim
/// \defgroup set_prim_active set_prim_active node
///
/// \brief Authors "active" metadata for this prim at the current EditTarget.
///
/// \param [in] stage The USD stage in which to set the active prim.
/// \param [in] path The path to the USD prim.
/// \param [in] active Whether the prim is active.
/// \returns true if the prim was suceessfully activated or de-activated.
USD_NODEDEF_DECL
bool set_prim_active(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                     const Amino::String&       path,
                     const bool                 active) //
    USDNODE_DOC_ICON_X("set_prim_active",
                       "set_prim_active",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup set_prim_metadata set_prim_metadata node
///
/// \brief Sets the value of a prim's metadata.
///
/// \param [in] path The path to the USD prim.
/// \param [in] key The metadatum key.
/// \param [in] value The metadatum value.
/// \param [in] stage The stage in which to set the metadatum.
#define SET_PRIM_METADATA(VALUE_TYPE)                                \
    USD_NODEDEF_DECL                                                 \
    bool set_prim_metadata(                                          \
        BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),       \
        const Amino::String& path, const Amino::String& key,         \
        const VALUE_TYPE& value)                                     \
        USDNODE_INTERNAL_X("set_prim_metadata", "set_prim_metadata", \
                           "outName=success");

SET_PRIM_METADATA(Amino::String)
SET_PRIM_METADATA(Amino::bool_t)
SET_PRIM_METADATA(Amino::float_t)
SET_PRIM_METADATA(Amino::double_t)
SET_PRIM_METADATA(Amino::int_t)
SET_PRIM_METADATA(Amino::long_t)
SET_PRIM_METADATA(Bifrost::Object)

/// \ingroup Prim
/// \defgroup get_prim_metadata get_prim_metadata node
///
/// \brief Gets the value of a prim's metadata.
///
/// \param [in] path The path to the USD prim.
/// \param [in] key The metadatum key.
/// \param [in] stage The USD stage.
/// \param [out] value The metadatum value.
#define GET_PRIM_METADATA(VALUE_TYPE)                                 \
    USD_NODEDEF_DECL                                                  \
    bool get_prim_metadata(                                           \
        const BifrostUsd::Stage& stage, const Amino::String& path,  \
        const Amino::String& key, const VALUE_TYPE& default_and_type, \
        VALUE_TYPE& value)                                            \
        USDNODE_INTERNAL_X("get_prim_metadata", "get_prim_metadata",  \
                           "outName=success");

GET_PRIM_METADATA(Amino::String)
GET_PRIM_METADATA(Amino::bool_t)
GET_PRIM_METADATA(Amino::float_t)
GET_PRIM_METADATA(Amino::double_t)
GET_PRIM_METADATA(Amino::int_t)
GET_PRIM_METADATA(Amino::long_t)
GET_PRIM_METADATA(Amino::Ptr<Bifrost::Object>)

} // namespace Prim
} // namespace USD

#endif // USD_PRIM_NODEDEFS_H
