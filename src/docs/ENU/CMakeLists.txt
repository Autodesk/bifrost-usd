#-
#*****************************************************************************
# Copyright 2024 Autodesk, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#*****************************************************************************
#+


set(doc_files
    USD_Attribute_add_attribute_connection.md
    USD_Attribute_block_attribute.md
    USD_Attribute_clear_attribute.md
    USD_Attribute_clear_attribute_connections.md
    USD_Attribute_create_prim_attribute.md
    USD_Attribute_create_primvar.md
    USD_Attribute_define_usd_attribute.md
    USD_Attribute_define_usd_display_color.md
    USD_Attribute_define_usd_transform.md
    USD_Attribute_get_attribute_metadata.md
    USD_Attribute_get_prim_attribute.md
    USD_Attribute_get_prim_attribute_connections.md
    USD_Attribute_get_prim_attribute_data.md
    USD_Attribute_get_prim_attribute_type.md
    USD_Attribute_get_usd_attribute_value.md
    USD_Attribute_remove_attribute_connection.md
    USD_Attribute_set_attribute_metadata.md
    USD_Attribute_set_prim_attribute.md
    USD_Collection_get_all_collection_names.md
    USD_Collection_get_excludes_paths.md
    USD_Collection_get_includes_paths.md
    USD_Collection_get_or_create_collection.md
    USD_Layer_add_sublayer.md
    USD_Layer_create_layer.md
    USD_Layer_create_usd_layer.md
    USD_Layer_duplicate_layer.md
    USD_Layer_export_layer_to_file.md
    USD_Layer_export_layer_to_string.md
    USD_Layer_get_layer.md
    USD_Layer_get_layer_display_name.md
    USD_Layer_get_layer_file_path.md
    USD_Layer_get_layer_identifier.md
    USD_Layer_get_root_layer.md
    USD_Layer_get_sublayer_paths.md
    USD_Layer_open_layer.md
    USD_Layer_open_usd_layer.md
    USD_Layer_replace_layer.md
    USD_Layer_set_layer_permission.md
    USD_PointInstancer_define_usd_point_instancer.md
    USD_PointInstancer_delete_usd_point_instances.md
    USD_PointInstancer_get_usd_point_instancer_attributes.md
    USD_PointInstancer_replace_usd_point_instancer_proto.md
    USD_PointInstancer_set_usd_point_instances_invisible.md
    USD_PointInstancer_usd_point_instancer_scope.md
    USD_Prim_add_applied_schema.md
    USD_Prim_add_inherit_prim.md
    USD_Prim_add_payload_prim.md
    USD_Prim_add_reference_prim.md
    USD_Prim_add_relationship_target.md
    USD_Prim_add_specialize_prim.md
    USD_Prim_clear_relationship_targets.md
    USD_Prim_compute_usdgeom_extent.md
    USD_Prim_create_class_prim.md
    USD_Prim_create_prim.md
    USD_Prim_create_prim_relationship.md
    USD_Prim_create_usd_prim.md
    USD_Prim_define_usd_curves.md
    USD_Prim_define_usd_geom_subset.md
    USD_Prim_define_usd_mesh.md
    USD_Prim_define_usd_point_instancer.md # Kept only for old scenes since USD::Prim::define_usd_point_instancer is now internal.
    USD_Prim_define_usd_prim.md
    USD_Prim_define_usd_prim_hierarchy.md
    USD_Prim_define_usd_reference.md
    USD_Prim_define_usd_reference_from_file.md
    USD_Prim_define_usd_relationship.md
    USD_Prim_define_usd_skeleton.md
    USD_Prim_define_usd_skeleton_animation.md
    USD_Prim_get_all_attribute_names.md
    USD_Prim_get_applied_schemas.md
    USD_Prim_get_authored_attribute_names.md
    USD_Prim_get_forwarded_relationship_targets.md
    USD_Prim_get_last_modified_prim.md
    USD_Prim_get_prim_asset_info.md
    USD_Prim_get_prim_at_path.md
    USD_Prim_get_prim_children.md
    USD_Prim_get_prim_in_prototype.md
    USD_Prim_get_prim_instances.md
    USD_Prim_get_prim_kind.md
    USD_Prim_get_prim_metadata.md
    USD_Prim_get_prim_path.md
    USD_Prim_get_prim_type.md
    USD_Prim_get_prototype_prims.md
    USD_Prim_get_relationship_targets.md
    USD_Prim_get_usd_geom_points.md
    USD_Prim_get_usd_geom_xform_vectors.md
    USD_Prim_override_prim.md
    USD_Prim_prim_clear_instanceable.md
    USD_Prim_prim_get_prototype.md
    USD_Prim_prim_has_authored_instanceable.md
    USD_Prim_prim_is_in_prototype.md
    USD_Prim_prim_is_instance.md
    USD_Prim_prim_is_instance_proxy.md
    USD_Prim_prim_is_instanceable.md
    USD_Prim_prim_is_prototype.md
    USD_Prim_remove_applied_schema.md
    USD_Prim_remove_payload_prim.md
    USD_Prim_remove_reference_prim.md
    USD_Prim_remove_relationship_target.md
    USD_Prim_rotate_prim.md
    USD_Prim_scale_prim.md
    USD_Prim_set_prim_active.md
    USD_Prim_set_prim_asset_info.md
    USD_Prim_set_prim_instanceable.md
    USD_Prim_set_prim_kind.md
    USD_Prim_set_prim_metadata.md
    USD_Prim_set_prim_pivot.md
    USD_Prim_set_prim_purpose.md
    USD_Prim_set_relationship_targets.md
    USD_Prim_translate_prim.md
    USD_Prim_usd_point_instancer.md
    USD_Prim_usd_volume.md
    USD_Shading_bind_material.md
    USD_Shading_define_usd_preview_surface.md
    USD_Shading_get_material_path.md
    USD_Shading_unbind_material.md
    USD_Stage_add_to_stage.md
    USD_Stage_create_usd_stage.md
    USD_Stage_export_stage_to_file.md
    USD_Stage_export_stage_to_string.md
    USD_Stage_get_default_prim.md
    USD_Stage_get_edit_layer.md
    USD_Stage_get_stage_metadata.md
    USD_Stage_open_stage_from_cache.md
    USD_Stage_open_stage_from_layer.md
    USD_Stage_open_usd_stage.md
    USD_Stage_save_stage.md
    USD_Stage_save_usd_stage.md
    USD_Stage_send_stage_to_cache.md
    USD_Stage_set_default_prim.md
    USD_Stage_set_edit_layer.md
    USD_Stage_set_stage_metadata.md
    USD_Stage_set_stage_time_code.md
    USD_Stage_set_stage_up_axis.md
    USD_VariantSet_add_variant.md
    USD_VariantSet_add_variant_set.md
    USD_VariantSet_clear_variant_selection.md
    USD_VariantSet_define_usd_variant_set.md
    USD_VariantSet_get_variant_sets.md
    USD_VariantSet_get_variants.md
    USD_VariantSet_set_variant_selection.md
)

foreach(doc_file ${doc_files})
    configure_file(
        ${doc_file}
        ${BIFROST_USD_OUTPUT_DOCS_DIR}/ENU/${doc_file}
        COPYONLY
    )
endforeach()

install(FILES ${doc_files}
    DESTINATION "${BIFROST_USD_PACK_INSTALL_RES_DOCS_DIR}/ENU")
