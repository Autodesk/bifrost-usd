#-
#*****************************************************************************
# Copyright 2022 Autodesk, Inc.
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

# Compounds using the usd-pack external operators directly
set(node_def_compound_json
    # ----
    # The assign_usd_material compound is deprecated in favor of the
    # bind_material operator. It is set as internal to not show up in tab menu.
    assign_usd_material.json
    # ----
    create_child_prim_path.json
    create_usd_layer.json
    create_usd_prim.json
    create_usd_stage.json
    get_attribute_comment.json
    get_attribute_custom_data.json
    get_attribute_documentation.json
    get_prim_comment.json
    get_prim_custom_data.json
    get_prim_documentation.json
    get_stage_comment.json
    get_stage_custom_layer_data.json
    get_stage_documentation.json
    open_usd_layer.json
    open_usd_stage.json
    read_usd_curves.json
    read_usd_meshes.json
    save_usd_stage.json
    set_attribute_comment.json
    set_attribute_custom_data.json
    set_attribute_documentation.json
    set_prim_color.json
    set_prim_comment.json
    set_prim_custom_data.json
    set_prim_documentation.json
    set_stage_comment.json
    set_stage_custom_layer_data.json
    set_stage_documentation.json
    set_usd_default_prim.json
    set_usd_prim_kind.json
    usd_enums.json
    usd_file_path.json
    # ---- compounds describing USD actions with Bifrost::Objects ----
    add_to_stage.json
    build_prim_from_definition.json
    create_usd_attribute_from_def.json
    define_usd_attribute.json
    define_usd_curves.json
    define_usd_display_color.json
    define_usd_material_tag.json
    define_usd_mesh.json
    define_usd_point_instancer.json
    define_usd_preview_surface.json
    define_usd_prim.json
    define_usd_reference.json
    define_usd_reference_from_file.json
    define_usd_relationship.json
    define_usd_transform.json
    define_usd_variant_set.json
    duplicate_usd_prim_definition.json
    get_usd_attribute_value.json
)

foreach(json_file ${node_def_compound_json})
    configure_file(
        ${json_file}
        ${BIFROST_USD_OUTPUT_JSON_DIR}/${json_file}
        COPYONLY)
endforeach()

set(all_jsons ${node_def_compound_json})
list(TRANSFORM all_jsons PREPEND "${BIFROST_USD_OUTPUT_JSON_DIR}/")
set_property(TARGET ${BIFUSD_PACKAGE_NAME}_config_info
             APPEND PROPERTY BIFROST_USD_PACK_ALL_JSON_FILES "${all_jsons}")
add_custom_target(bifrost_usd_compound_json_files DEPENDS ${all_jsons})

install(
    FILES ${node_def_compound_json}
    DESTINATION "${BIFROST_USD_PACK_INSTALL_RES_JSON_DIR}")
