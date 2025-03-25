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

set(usd_icons
    add_layer.svg
    bind_material.svg
    def_point_instancer.svg
    define_reference.svg
    export_layer.svg
    file_usd.svg
    import_layer.svg
    make_target.svg
    move.svg
    point_instancer.svg
    prim_active.svg
    prim_att.svg
    relationship.svg
    rotate.svg
    scale.svg
    usd_add.svg
    usd_curve.svg
    usd_default.svg
    usd_get.svg
    usd_layers.svg
    usd_mesh.svg
    usd_pill.svg
    usd_set.svg
    usd_stage.svg
    usd_transform.svg
    usd_variant.svg
    write_usd_file.svg
)

foreach(icon_file ${usd_icons})
    configure_file(
        ${icon_file}
        ${BIFROST_USD_OUTPUT_ICONS_DIR}/${icon_file}
        COPYONLY)
endforeach()

install(FILES ${usd_icons}
    DESTINATION "${BIFROST_USD_PACK_INSTALL_RES_ICONS_DIR}")
