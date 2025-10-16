#-
#*****************************************************************************
# Copyright 2025 Autodesk, Inc.
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
# Json config file to load nodes, watchpoints, ...
#
# Translation tables have their own config file that is under the application
# plugin folder, ex.: maya_plugin, because they depend on the application's api.
#==============================================================================

set( usd_config_main_json_tpl       ${BIFUSD_OUTPUT_ROOT_DIR}/usd_pack_config.json.tpl )
set( usd_config_comp_dev_json_tpl   ${BIFUSD_OUTPUT_ROOT_DIR}/usd_pack_config_compound_dev.json.tpl )

set( usd_config_main_json           ${BIFUSD_OUTPUT_ROOT_DIR}/usd_pack_config.json )
set( usd_config_comp_dev_json       ${BIFUSD_OUTPUT_ROOT_DIR}/usd_pack_config_compound_dev.json )


# The name and version of the package/library are first substituted in the
# input config files (see the @xyz@ variables in `usd_pack_config.json.in` and
# `usd_pack_config_compound_dev.json.in`).
# Namely, the @BIFUSD_PACKAGE_NAME@ variable is used to define the name of
# this package/library.
configure_file(usd_pack_config.json.in              ${usd_config_main_json_tpl} @ONLY )
configure_file(usd_pack_config_compound_dev.json.in ${usd_config_comp_dev_json_tpl} @ONLY )

# After the previous steps, only the ${xyz} variables remain in the generated config files.

# Define some overrides for the CMake variables used to replace the ${xyz} variables in the main
# config file. Others are left unchanged and are defined in setup.cmake.
set(BIFROST_USD_CONFIG_GRAPHS "")

# Define the CMake variables to replace the ${xyz} variables in the compound_dev config:
set(USD_SRC_COMPOUND_DEV_CONFIG    ${BIFUSD_COMPOUND_DEV_SOURCE_DIR}/compound-dev.json)
set(USD_SRC_COMPOUND_DEV_COMPOUNDS ${BIFUSD_COMPOUND_DEV_SOURCE_DIR}/src/compounds)
set(USD_SRC_COMPOUND_DEV_GRAPHS    ${BIFUSD_COMPOUND_DEV_SOURCE_DIR}/src/graphs)

# Now substitute the ${xyz} variables (e.g. the json and library locations).
configure_file(${usd_config_main_json_tpl}      ${usd_config_main_json} )
configure_file(${usd_config_comp_dev_json_tpl}  ${usd_config_comp_dev_json} )

install(FILES ${usd_config_main_json} DESTINATION       ${BIFUSD_INSTALL_ROOT_DIR} )
install(FILES ${usd_config_comp_dev_json} DESTINATION   ${BIFUSD_INSTALL_ROOT_DIR} )

# Add the location of the intermediate main config template file.
# The ${xyz} variables in that file can be later substituted by CMake configure_file
set_target_properties(${BIFUSD_PACKAGE_NAME}_config_info PROPERTIES
                       BIFROST_USD_PACK_CONFIG_FILE_TPL "${usd_config_main_json_tpl}"
                       BIFROST_USD_PACK_INSTALL_RES_CONFIG_DIR "${BIFROST_USD_PACK_INSTALL_RES_CONFIG_DIR}")
