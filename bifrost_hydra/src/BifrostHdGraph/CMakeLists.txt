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

set(private_libs
    Amino::Core
    BifrostHdEngine
    BifrostHdTranslators
    hd
    hdGp
)

set(src_files
    BifrostGraphGenerativeProcedural.cpp
    BifrostGraphGenerativeProceduralPlugin.cpp
)

bifusd_set_extra_rpaths(extra_rpaths)

bifusd_create_shared_lib(
    BifrostHdGraph         "BIFROST_HD_GRAPH_BUILD_DLL"
    PUBLIC_INSTALL
    PUBLIC_DEFINITIONS     $<${BIFUSD_IS_MSC}: BOOST_LIB_TOOLSET="vc142">
    PRIVATE_DEFINITIONS    $<${BIFUSD_IS_DEBUG}: TBB_USE_DEBUG BOOST_DEBUG_PYTHON BOOST_LINKING_PYTHON>

    PRIVATE_OPTIONS        $<${BIFUSD_IS_MSC}:/wd4251 /wd4273 /wd4003>
                           $<${BIFUSD_IS_GCC}: -Wno-deprecated -Wno-unused-macros>
                           $<${BIFUSD_IS_CLANG}: -Wno-deprecated -Wno-unused-macros>

    PUBLIC_OPTIONS         $<${BIFUSD_IS_MSC}:/wd4251 /wd4273>
    SRC_FILES              ${src_files}
    PRIVATE_LINK_LIBS      ${private_libs}
    EXTRA_RPATH            ${extra_rpaths}
)

set(plugin_info_json ${BIFROST_USD_OUTPUT_PROC_DIR}/plugInfo.json)

# NOTE: To keep on the radar... For some reason, for tests to work, the relative path to add in the plugInfo.json needs
# to be computed from the directory that contains the directory that contains plugInfo.json.
if( BIFUSD_IS_WINDOWS)
    file( RELATIVE_PATH BIFROST_HYDRA_REL_SHARED_LIB_DIR ${BIFROST_USD_OUTPUT_PROC_DIR}/.. ${BIFUSD_OUTPUT_BIN_DIR} )
else()
    file( RELATIVE_PATH BIFROST_HYDRA_REL_SHARED_LIB_DIR ${BIFROST_USD_OUTPUT_PROC_DIR}/.. ${BIFUSD_OUTPUT_LIB_DIR} )
endif()

configure_file(plugInfo.json.in ${plugin_info_json} @ONLY)

install(
    FILES ${plugin_info_json}
    DESTINATION "${BIFROST_USD_PACK_INSTALL_RES_PROC_DIR}")
