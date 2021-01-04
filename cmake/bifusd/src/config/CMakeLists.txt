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

# Set the name of the bifusd config target
set(BIFUSD_CONFIG_TARGET ${BIFUSD_PACKAGE_NAME}BifusdConfig CACHE INTERNAL
    "Name of BifusdConfig target")

if( TARGET ${BIFUSD_CONFIG_TARGET})
    return()
endif()

set(config_src_dir ${BIFUSD_TOOLS_DIR}/src/config)

#==============================================================================
# Config Configuration
#==============================================================================

set(header_only_srcfiles
    CfgCompilerMacros.h
    CfgWarningMacros.h )


#==============================================================================
# TARGET LIBRARIES
#==============================================================================

bifusd_install_headers(${BIFUSD_CONFIG_TARGET}
                        SOURCE            ${config_src_dir}
                        FILES             ${header_only_srcfiles}
                        HEADER_ONLY_FILES ${header_only_srcfiles}
                        DESTINATION       "bifusd/config")

