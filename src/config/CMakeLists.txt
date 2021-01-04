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
# Json config file to load nodes, watchpoints, ...
#
# Translation tables have their own config file that is under the application
# plugin folder, ex.: maya_plugin, because they depend on the application's api.
#==============================================================================

set( usd_config_json_tpl ${BIFUSD_OUTPUT_ROOT_DIR}/usd_pack_config.json.tpl )
set( usd_config_json ${BIFUSD_OUTPUT_ROOT_DIR}/usd_pack_config.json )

# The name and version of the package/library are first substituted in the
# generated config file (see usd_pack_config.json.in @xyz@ variables).
# Namely, the @BIFUSD_PACKAGE_NAME@ variable is used to define the name of
# this package/library.
configure_file(usd_pack_config.json.in ${usd_config_json_tpl} @ONLY )

# After the previous step, only the ${xyz} variables remain.
# Now substitute the json and library locations.
# (see usd_pack_config.json.in)
configure_file(${usd_config_json_tpl} ${usd_config_json} )

install(FILES ${usd_config_json} DESTINATION ${BIFUSD_INSTALL_ROOT_DIR} )

# Add the config template file location.
# The variables in that file can be substituted by CMake configure_file
set_target_properties(${BIFUSD_PACKAGE_NAME}_config_info PROPERTIES
                       BIFROST_USD_PACK_CONFIG_FILE_TPL "${usd_config_json_tpl}"
                       BIFROST_USD_PACK_INSTALL_RES_CONFIG_DIR "${BIFROST_USD_PACK_INSTALL_RES_CONFIG_DIR}")
