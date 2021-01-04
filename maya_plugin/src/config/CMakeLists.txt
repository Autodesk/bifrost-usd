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

set( usd_maya_json_tpl ${BIFUSD_OUTPUT_ROOT_DIR}/usd_maya_translation.json.tpl )
set( usd_maya_json ${BIFUSD_OUTPUT_ROOT_DIR}/usd_maya_translation.json )

# Substitute version information.
# See usd_maya_translation.json.in file
configure_file(usd_maya_translation.json.in ${usd_maya_json_tpl} @ONLY )

# Substitute location of libs - only ${xyz} vars remain after the previous step.
# See usd_maya_translation.json.in file
configure_file(${usd_maya_json_tpl} ${usd_maya_json} )
install(FILES ${usd_maya_json} DESTINATION ${BIFUSD_INSTALL_ROOT_DIR} )

# Add the config template file location.
# The variables in that file can be substituted by CMake configure_file.
set_target_properties(${BIFUSD_PACKAGE_NAME}_config_info PROPERTIES
                       BIFROST_USD_PACK_MAYA_CONFIG_FILE_TPL "${usd_maya_json_tpl}")

# This config includes the nodedefs config and the translation table config
install(FILES plugin_config.json
DESTINATION ${BIFUSD_INSTALL_ROOT_DIR} )
