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


#ifndef USD_PRIM_NODEDEFS_H
#define USD_PRIM_NODEDEFS_H

#include <Amino/Core/Array.h>
#include <Amino/Core/BuiltInTypes.h>
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

namespace USD {

namespace Prim {

USD_NODEDEF_DECL
bool get_prim_at_path(Amino::Ptr<BifrostUsd::Stage>       stage,
                      const Amino::String&                   path,
                      Amino::MutablePtr<BifrostUsd::Prim>& prim)
    USDNODE_DOC_ICON_X("get_prim_at_path",
                       "USD_Prim_get_prim_at_path.md",
                       "usd_pill.svg",
                       "outName=success");

USD_NODEDEF_DECL
void get_prim_children(
    Amino::Ptr<BifrostUsd::Stage>       stage,
    const Amino::String&                   prim_path,
    const BifrostUsd::PrimDescendantMode descendant_mode,
    const bool traverse_instances        AMINO_ANNOTATE("Amino::Port value=false"),
    Amino::MutablePtr<Amino::Array<Amino::Ptr<BifrostUsd::Prim>>>& children)
    USDNODE_DOC_ICON("get_prim_children", "USD_Prim_get_prim_children.md", "usd_pill.svg");

USD_NODEDEF_DECL
void get_prim_path(const BifrostUsd::Prim& prim, Amino::String& path)
    USDNODE_DOC_ICON("get_prim_path", "USD_Prim_get_prim_path.md", "usd_pill.svg");

USD_NODEDEF_DECL
void get_prim_parent(const BifrostUsd::Prim& prim,
                     Amino::MutablePtr<BifrostUsd::Prim>& parent,
                     Amino::String& parent_path)
    USDNODE_DOC_ICON("get_prim_parent", "USD_Prim_get_prim_parent.md", "usd_pill.svg");

USD_NODEDEF_DECL
bool get_last_modified_prim(Amino::Ptr<BifrostUsd::Stage>       stage,
                            Amino::MutablePtr<BifrostUsd::Prim>& prim)
    USDNODE_DOC_ICON_X("get_last_modified_prim",
                       "USD_Prim_get_last_modified_prim.md",
                       "usd_pill.svg",
                       "outName=success");

USD_NODEDEF_DECL
void get_prim_type(const BifrostUsd::Prim& prim, Amino::String& type_name)
    USDNODE_DOC_ICON("get_prim_type", "USD_Prim_get_prim_type.md", "usd_pill.svg");

USD_NODEDEF_DECL
void get_all_attribute_names(const BifrostUsd::Prim&                         prim,
                             Amino::MutablePtr<Amino::Array<Amino::String>>& names)
    USDNODE_DOC_ICON("get_all_attribute_names",
                     "USD_Prim_get_all_attribute_names.md",
                     "prim_att.svg");

USD_NODEDEF_DECL
void get_authored_attribute_names(
    const BifrostUsd::Prim&                         prim,
    Amino::MutablePtr<Amino::Array<Amino::String>>& names)
    USDNODE_DOC_ICON("get_authored_attribute_names",
                     "USD_Prim_get_authored_attribute_names.md",
                     "prim_att.svg");

USD_NODEDEF_DECL
void create_prim(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                 const Amino::String&       path,
                 const Amino::String&       type)
    USDNODE_INTERNAL("create_prim", "USD_Prim_create_prim.md");

USD_NODEDEF_DECL
void create_class_prim(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                       const Amino::String&       path)
    USDNODE_DOC_ICON("create_class_prim", "USD_Prim_create_class_prim.md", "usd_pill.svg");

USD_NODEDEF_DECL
void override_prim(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                   const Amino::String&       path)
    USDNODE_DOC_ICON("override_prim", "USD_Prim_override_prim.md", "usd_pill.svg");

USD_NODEDEF_DECL
bool add_applied_schema(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                        const Amino::String&     prim_path,
                        const Amino::String&     applied_schema_name)
    USDNODE_DOC_ICON_X("add_applied_schema",
                       "USD_Prim_add_applied_schema.md",
                       "usd_pill.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool remove_applied_schema(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                           const Amino::String&     prim_path,
                           const Amino::String&     applied_schema_name)
    USDNODE_DOC_ICON_X("remove_applied_schema",
                       "USD_Prim_remove_applied_schema.md",
                       "usd_pill.svg",
                       "outName=success");

USD_NODEDEF_DECL
void get_applied_schemas(const BifrostUsd::Prim&                         prim,
                         Amino::MutablePtr<Amino::Array<Amino::String>>& names)
    USDNODE_DOC_ICON("get_applied_schemas", "USD_Prim_get_applied_schemas.md", "usd_pill.svg");

USD_NODEDEF_DECL
bool add_reference_prim(
    BifrostUsd::Stage& stage          USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const BifrostUsd::Layer&          reference_layer,
    const Amino::String&                reference_prim_path,
    const double layer_offset           AMINO_ANNOTATE("Amino::Port value=0.0"),
    const double layer_scale            AMINO_ANNOTATE("Amino::Port value=1.0"),
    const BifrostUsd::UsdListPosition reference_position,
    const Amino::String& anchor_path=Amino::String{})
    USDNODE_DOC_ICON_X("add_reference_prim",
                       "USD_Prim_add_reference_prim.md",
                       "define_reference.svg",
                       "outName=success,Amino::DefaultOverload");

USD_NODEDEF_DECL
bool add_reference_prim(
    BifrostUsd::Stage& stage            USDPORT_INOUT("out_stage"),
    const Amino::String&                  prim_path,
    const Amino::String& reference_layer  USDNODE_FILE_BROWSER_OPEN,
    const Amino::String&                  reference_prim_path,
    const double layer_offset             AMINO_ANNOTATE("Amino::Port value=0.0"),
    const double layer_scale              AMINO_ANNOTATE("Amino::Port value=1.0"),
    const BifrostUsd::UsdListPosition reference_position,
    const Amino::String& anchor_path=Amino::String{})
    USDNODE_DOC_ICON_X("add_reference_prim",
                       "USD_Prim_add_reference_prim.md",
                       "define_reference.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool remove_reference_prim(
    BifrostUsd::Stage& stage                      USDPORT_INOUT("out_stage"),
    const Amino::String&                            prim_path,
    const Amino::String& reference_layer_identifier USDNODE_FILE_BROWSER_OPEN,
    const Amino::String&                            reference_prim_path,
    const double layer_offset AMINO_ANNOTATE("Amino::Port value=0.0"),
    const bool clear_all) USDNODE_DOC_ICON_X("remove_reference_prim",
                                             "USD_Prim_remove_reference_prim.md",
                                             "define_reference.svg",
                                             "outName=success");

USD_NODEDEF_DECL
bool remove_payload_prim(
    BifrostUsd::Stage& stage                    USDPORT_INOUT("out_stage"),
    const Amino::String&                          prim_path,
    const Amino::String& payload_layer_identifier USDNODE_FILE_BROWSER_OPEN,
    const Amino::String&                          payload_prim_path,
    const double layer_offset AMINO_ANNOTATE("Amino::Port value=0.0"),
    const bool clear_all) USDNODE_DOC_ICON_X("remove_payload_prim",
                                             "USD_Prim_remove_payload_prim.md",
                                             "define_reference.svg",
                                             "outName=success");

USD_NODEDEF_DECL
bool add_payload_prim(
    BifrostUsd::Stage& stage          USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const BifrostUsd::Layer&          payload_layer,
    const Amino::String&                payload_prim_path,
    const double layer_offset           AMINO_ANNOTATE("Amino::Port value=0.0"),
    const double layer_scale            AMINO_ANNOTATE("Amino::Port value=1.0"),
    const BifrostUsd::UsdListPosition payload_position,
    const Amino::String& anchor_path=Amino::String{})
    USDNODE_DOC_ICON_X("add_payload_prim",
                       "USD_Prim_add_payload_prim.md",
                       "define_reference.svg",
                       "outName=success,Amino::DefaultOverload");

USD_NODEDEF_DECL
bool add_payload_prim(
    BifrostUsd::Stage& stage          USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const Amino::String& payload_layer  USDNODE_FILE_BROWSER_OPEN,
    const Amino::String&                payload_prim_path,
    const double layer_offset           AMINO_ANNOTATE("Amino::Port value=0.0"),
    const double layer_scale            AMINO_ANNOTATE("Amino::Port value=1.0"),
    const BifrostUsd::UsdListPosition payload_position,
    const Amino::String& anchor_path=Amino::String{})
    USDNODE_DOC_ICON_X("add_payload_prim",
                       "USD_Prim_add_payload_prim.md",
                       "define_reference.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool add_inherit_prim(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                      const Amino::String&       prim_path,
                      const Amino::String&       inherited_prim_path,
                      const BifrostUsd::UsdListPosition inherit_position) //
    USDNODE_DOC_ICON_X("add_inherit_prim",
                       "USD_Prim_add_inherit_prim.md",
                       "define_reference.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool add_specialize_prim(
    BifrostUsd::Stage& stage          USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const Amino::String&                specialized_prim_path,
    const BifrostUsd::UsdListPosition specialize_position)
    USDNODE_DOC_ICON_X("add_specialize_prim",
                       "USD_Prim_add_specialize_prim.md",
                       "define_reference.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool create_prim_relationship(
    BifrostUsd::Stage& stage          USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const Amino::String&                rel_name,
    const bool                          custom,
    const Amino::String&                target,
    const BifrostUsd::UsdListPosition target_position)
    USDNODE_DOC_ICON_X("create_prim_relationship",
                       "USD_Prim_create_prim_relationship.md",
                       "relationship.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool add_relationship_target(
    BifrostUsd::Stage& stage          USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const Amino::String&                rel_name,
    const Amino::String&                target,
    const BifrostUsd::UsdListPosition target_position)
    USDNODE_DOC_ICON_X("add_relationship_target",
                       "USD_Prim_add_relationship_target.md",
                       "relationship.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool remove_relationship_target(BifrostUsd::Stage& stage
                                                     USDPORT_INOUT("out_stage"),
                                const Amino::String& prim_path,
                                const Amino::String& rel_name,
                                const Amino::String& target)
    USDNODE_DOC_ICON_X("remove_relationship_target",
                       "USD_Prim_remove_relationship_target.md",
                       "relationship.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool set_relationship_targets(BifrostUsd::Stage& stage
                                                   USDPORT_INOUT("out_stage"),
                              const Amino::String& prim_path,
                              const Amino::String& rel_name,
                              const Amino::Array<Amino::String>& targets)
    USDNODE_DOC_ICON_X("set_relationship_targets",
                       "USD_Prim_set_relationship_targets.md",
                       "relationship.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool clear_relationship_targets(
    BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
    const Amino::String&       prim_path,
    const Amino::String&       rel_name,
    const bool remove_spec) USDNODE_DOC_ICON_X("clear_relationship_targets",
                                               "USD_Prim_clear_relationship_targets.md",
                                               "relationship.svg",
                                               "outName=success");

USD_NODEDEF_DECL
bool get_relationship_targets(
    BifrostUsd::Stage& stage                      USDPORT_INOUT("out_stage"),
    const Amino::String&                            prim_path,
    const Amino::String&                            rel_name,
    Amino::MutablePtr<Amino::Array<Amino::String>>& targets)
    USDNODE_DOC_ICON_X("get_relationship_targets",
                       "USD_Prim_get_relationship_targets.md",
                       "relationship.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool get_forwarded_relationship_targets(
    BifrostUsd::Stage& stage                      USDPORT_INOUT("out_stage"),
    const Amino::String&                            prim_path,
    const Amino::String&                            rel_name,
    Amino::MutablePtr<Amino::Array<Amino::String>>& targets)
    USDNODE_DOC_ICON_X("get_forwarded_relationship_targets",
                       "USD_Prim_get_forwarded_relationship_targets.md",
                       "relationship.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool prim_is_instanceable(const BifrostUsd::Stage& stage,
                          const Amino::String&       prim_path)
    USDNODE_DOC_ICON_X("prim_is_instanceable",
                       "USD_Prim_prim_is_instanceable.md",
                       "usd_pill.svg",
                       "outName=is_instanceable");

USD_NODEDEF_DECL
bool set_prim_instanceable(
    BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
    const Amino::String&       prim_path,
    const bool instanceable) USDNODE_DOC_ICON_X("set_prim_instanceable",
                                                "USD_Prim_set_prim_instanceable.md",
                                                "usd_pill.svg",
                                                "outName=success");

USD_NODEDEF_DECL
bool prim_clear_instanceable(BifrostUsd::Stage& stage
                                                  USDPORT_INOUT("out_stage"),
                             const Amino::String& prim_path)
    USDNODE_DOC_ICON_X("prim_clear_instanceable",
                       "USD_Prim_prim_clear_instanceable.md",
                       "usd_pill.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool prim_has_authored_instanceable(const BifrostUsd::Stage& stage,
                                    const Amino::String&       prim_path)
    USDNODE_DOC_ICON_X("prim_has_authored_instanceable",
                       "USD_Prim_prim_has_authored_instanceable.md",
                       "usd_pill.svg",
                       "outName=has_authored_instanceable");

USD_NODEDEF_DECL
bool prim_is_instance(const BifrostUsd::Stage& stage,
                      const Amino::String&       prim_path)
    USDNODE_DOC_ICON_X("prim_is_instance",
                       "USD_Prim_prim_is_instance.md",
                       "usd_pill.svg",
                       "outName=is_instance");

USD_NODEDEF_DECL
bool prim_is_instance_proxy(const BifrostUsd::Stage& stage,
                            const Amino::String&       prim_path)
    USDNODE_DOC_ICON_X("prim_is_instance_proxy",
                       "USD_Prim_prim_is_instance_proxy.md",
                       "usd_pill.svg",
                       "outName=is_instance_proxy");

USD_NODEDEF_DECL
bool prim_is_prototype(const BifrostUsd::Stage& stage,
                       const Amino::String&       prim_path)
    USDNODE_DOC_ICON_X("prim_is_prototype",
                       "USD_Prim_prim_is_prototype.md",
                       "usd_pill.svg",
                       "outName=is_prototype");

USD_NODEDEF_DECL
bool prim_is_in_prototype(const BifrostUsd::Stage& stage,
                          const Amino::String&       prim_path)
    USDNODE_DOC_ICON_X("prim_is_in_prototype",
                       "USD_Prim_prim_is_in_prototype.md",
                       "usd_pill.svg",
                       "outName=is_in_prototype");

USD_NODEDEF_DECL
void prim_get_prototype(const BifrostUsd::Stage& stage,
                        const Amino::String&       prim_path,
                        Amino::String&             proto_path)
    USDNODE_DOC_ICON("prim_get_prototype", "USD_Prim_prim_get_prototype.md", "usd_pill.svg");

USD_NODEDEF_DECL
void get_prim_in_prototype(const BifrostUsd::Stage& stage,
                           const Amino::String&       prim_path,
                           Amino::String&             prim_in_proto_path)
    USDNODE_DOC_ICON("get_prim_in_prototype",
                     "USD_Prim_get_prim_in_prototype.md",
                     "usd_pill.svg");

USD_NODEDEF_DECL
void get_prim_instances(
    const BifrostUsd::Stage&                      stage,
    const Amino::String&                            proto_prim_path,
    Amino::MutablePtr<Amino::Array<Amino::String>>& instances_paths)
    USDNODE_DOC_ICON("get_prim_instances", "USD_Prim_get_prim_instances.md", "usd_pill.svg");

USD_NODEDEF_DECL
void get_prototype_prims(
    const BifrostUsd::Stage&                      stage,
    Amino::MutablePtr<Amino::Array<Amino::String>>& proto_prim_paths)
    USDNODE_DOC_ICON("get_prototype_prims", "USD_Prim_get_prototype_prims.md", "usd_pill.svg");

USD_NODEDEF_DECL
void set_prim_purpose(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                      const Amino::String&       path,
                      const BifrostUsd::ImageablePurpose purpose)
    USDNODE_DOC_ICON("set_prim_purpose", "USD_Prim_set_prim_purpose.md", "usd_pill.svg");

USD_NODEDEF_DECL
void set_prim_kind(const Amino::String&       path,
                   const Amino::String&       kind,
                   BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"))
    USDNODE_INTERNAL("set_prim_kind", "USD_Prim_set_prim_kind.md");

USD_NODEDEF_DECL
void get_prim_kind(const BifrostUsd::Prim& prim, Amino::String& kind)
    USDNODE_DOC_ICON("get_prim_kind", "USD_Prim_get_prim_kind.md", "usd_pill.svg");

USD_NODEDEF_DECL
void set_prim_asset_info(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                         const Amino::String&     path,
                         const Amino::String&     asset_identifier,
                         const Amino::String&     asset_name,
                         const Amino::String&     asset_version)
    USDNODE_DOC_ICON("set_prim_asset_info", "USD_Prim_set_prim_asset_info.md", "usd_pill.svg");

USD_NODEDEF_DECL
void get_prim_asset_info(const BifrostUsd::Stage& stage,
                         const Amino::String&     path,
                         Amino::String&     asset_identifier,
                         Amino::String&     asset_name,
                         Amino::String&     asset_version)
    USDNODE_DOC_ICON("get_prim_asset_info", "USD_Prim_get_prim_asset_info.md", "usd_pill.svg");

USD_NODEDEF_DECL
bool set_prim_active(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                     const Amino::String&       path,
                     const bool                 active) //
    USDNODE_DOC_ICON_X("set_prim_active",
                       "USD_Prim_set_prim_active.md",
                       "prim_active.svg",
                       "outName=success");

#define SET_PRIM_METADATA(VALUE_TYPE)                                       \
    USD_NODEDEF_DECL                                                        \
    bool set_prim_metadata(                                                 \
        BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),                \
        const Amino::String& path, const Amino::String& key,                \
        const VALUE_TYPE& value)                                            \
        USDNODE_DOC_ICON_X("set_prim_metadata",                             \
                           "USD_Prim_set_prim_metadata.md", "prim_att.svg", \
                           "outName=success");

SET_PRIM_METADATA(Amino::String)
SET_PRIM_METADATA(Amino::bool_t)
SET_PRIM_METADATA(Amino::float_t)
SET_PRIM_METADATA(Amino::double_t)
SET_PRIM_METADATA(Amino::int_t)
SET_PRIM_METADATA(Amino::long_t)
SET_PRIM_METADATA(Bifrost::Object)
SET_PRIM_METADATA(Amino::Array<Amino::String>)

#define SET_PRIM_METADATA_BY_DICT_KEY(VALUE_TYPE)                       \
    USD_NODEDEF_DECL                                                    \
    bool set_prim_metadata_by_dict_key(                                 \
        BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),            \
        const Amino::String& path, const Amino::String& key,            \
        const Amino::String& key_path, const VALUE_TYPE& value)         \
        USDNODE_DOC_ICON_X("set_prim_metadata_by_dict_key",             \
                           "USD_Prim_set_prim_metadata_by_dict_key.md", \
                           "prim_att.svg", "outName=success");

SET_PRIM_METADATA_BY_DICT_KEY(Amino::String)
SET_PRIM_METADATA_BY_DICT_KEY(Amino::bool_t)
SET_PRIM_METADATA_BY_DICT_KEY(Amino::float_t)
SET_PRIM_METADATA_BY_DICT_KEY(Amino::double_t)
SET_PRIM_METADATA_BY_DICT_KEY(Amino::int_t)
SET_PRIM_METADATA_BY_DICT_KEY(Amino::long_t)
SET_PRIM_METADATA_BY_DICT_KEY(Bifrost::Object)
SET_PRIM_METADATA_BY_DICT_KEY(Amino::Array<Amino::String>)

#define GET_PRIM_METADATA(VALUE_TYPE)                                       \
    USD_NODEDEF_DECL                                                        \
    bool get_prim_metadata(                                                 \
        const BifrostUsd::Stage& stage, const Amino::String& path,          \
        const Amino::String& key, const VALUE_TYPE& default_and_type,       \
        VALUE_TYPE& value)                                                  \
        USDNODE_DOC_ICON_X("get_prim_metadata",                             \
                           "USD_Prim_get_prim_metadata.md", "prim_att.svg", \
                           "outName=success");

GET_PRIM_METADATA(Amino::String)
GET_PRIM_METADATA(Amino::bool_t)
GET_PRIM_METADATA(Amino::float_t)
GET_PRIM_METADATA(Amino::double_t)
GET_PRIM_METADATA(Amino::int_t)
GET_PRIM_METADATA(Amino::long_t)
GET_PRIM_METADATA(Amino::Ptr<Bifrost::Object>)
GET_PRIM_METADATA(Amino::Ptr<Amino::Array<Amino::String>>)

} // namespace Prim
} // namespace USD

#endif // USD_PRIM_NODEDEFS_H
