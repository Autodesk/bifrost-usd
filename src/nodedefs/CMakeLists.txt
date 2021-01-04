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

#==============================================================================
# INSTALL HEADERS
#==============================================================================

set(node_def_headers
    usd_attribute_nodedefs.h
    usd_collection_nodedefs.h
    usd_geom_nodedefs.h
    usd_layer_nodedefs.h
    usd_material_binding_nodedefs.h
    usd_prim_nodedefs.h
    usd_stage_nodedefs.h
    usd_variantset_nodedefs.h
)

set(nodedefsHeaders nodedef_export.h logger.h usd_utils.h usd_type_converter.h return_guard.h ${node_def_headers} header_parser_macros.h)
bifusd_install_headers(
    usd_packNodedefsHeadersInstall
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${nodedefsHeaders}
    DESTINATION "nodedefs/usd_pack"
)

#==============================================================================
# JSON RESOURCES
#==============================================================================

bifusd_header_parser(generate_usd_pack_node_def_json
    ${BIFROST_USD_PACK_INSTALL_RES_JSON_DIR}
    DEPENDS       usdHeadersInstall usd_packNodedefsHeadersInstall
    INCLUDES      ${CMAKE_CURRENT_SOURCE_DIR}
    INPUTS        ${node_def_headers}
)

#==============================================================================
# USD nodedefs shared library
#==============================================================================

set(node_def_src_files
    logger.cpp
    usd_attribute_nodedefs.cpp
    usd_collection_nodedefs.cpp
    usd_geom_nodedefs.cpp
    usd_layer_nodedefs.cpp
    usd_material_binding_nodedefs.cpp
    usd_prim_nodedefs.cpp
    usd_stage_nodedefs.cpp
    usd_utils.cpp
    usd_variantset_nodedefs.cpp
)

set(node_def_public_libs
    Amino::Core
    Amino::Cpp
    Bifrost::Object
    BifrostUSD
    usd_packNodedefsHeadersInstall
    usdVol
    usdGeom
    usdShade
    usdUtils
)

bifusd_set_extra_rpaths(extra_rpaths)

bifusd_create_shared_lib(
    BifrostUSDNodedefs "USD_BUILD_NODEDEF_DLL"
    PUBLIC_INSTALL
    PRIVATE_DEFINITIONS     $<${BIFUSD_IS_DEBUG}: TBB_USE_DEBUG BOOST_DEBUG_PYTHON BOOST_LINKING_PYTHON>
    PUBLIC_OPTIONS          $<${BIFUSD_IS_GCC}: -Wno-deprecated>
    SRC_FILES               ${node_def_src_files}
    PUBLIC_LINK_LIBS        ${node_def_public_libs}
    EXTRA_RPATH             ${extra_rpaths}
)

#==============================================================================
# Subdirectories
#==============================================================================

add_subdirectory(compounds)

#==============================================================================
# Target for json files
#==============================================================================
add_dependencies(${BIFUSD_PACKAGE_NAME}_config_info
                    generate_usd_pack_node_def_json
                    bifrost_usd_compound_json_files )

# Add properties to be able to merge of json files externally
set_target_properties( ${BIFUSD_PACKAGE_NAME}_config_info PROPERTIES
    BIFROST_USD_PACK_ALL_JSON_FILES_DIR   ${BIFROST_USD_OUTPUT_JSON_DIR}
    BIFROST_USD_PACK_INSTALL_RES_JSON_DIR ${BIFROST_USD_PACK_INSTALL_RES_JSON_DIR})
