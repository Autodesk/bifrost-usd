#-
#*****************************************************************************
# Copyright 2023 Autodesk, Inc.
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

set( bifrost_hydra_json_tpl ${BIFUSD_OUTPUT_ROOT_DIR}/bifrost_hydra_translation.json.tpl )
set( bifrost_hydra_json ${BIFUSD_OUTPUT_ROOT_DIR}/bifrost_hydra_translation.json )

# Substitute version information.
# See bifrost_hydra_translation.json.in file
configure_file(bifrost_hydra_translation.json.in ${bifrost_hydra_json_tpl} @ONLY )

# Substitute location of libs - only ${xyz} vars remain after the previous step.
# See bifrost_hydra_translation.json.in file
configure_file(${bifrost_hydra_json_tpl} ${bifrost_hydra_json} )
install(FILES ${bifrost_hydra_json} DESTINATION ${BIFUSD_INSTALL_ROOT_DIR} )
