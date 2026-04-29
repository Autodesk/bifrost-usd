#-
#*****************************************************************************
# Copyright 2026 Autodesk, Inc.
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

#------------------------------------------------------------------------------
# Library dependencies
#------------------------------------------------------------------------------

set(libs
    bifusd_gtest
    BifusdTBB
    BifusdTBBHelpers
    BifrostUSD
    BifrostUSDTestUtils
)
set(pxr_libs
    usd
    usdGeom
)

#------------------------------------------------------------------------------
# test files
#------------------------------------------------------------------------------
set(test_files
    testDynamicFileFormat.cpp
)

set(lib_config_files
    ${BIFUSD_EXTRA_CONFIG_FILES}
    ${BIFUSD_OUTPUT_ROOT_DIR}/usd_pack_config.json
    ${CMAKE_CURRENT_SOURCE_DIR}/test_config.json
)

if(IS_BIFUSD_STANDALONE)
    list(PREPEND lib_config_files ${BIFROST_LOCATION}/resources/standalone_config.json )
endif()

set(bifrost_env_config
    BIFROST_LOCATION=${BIFROST_LOCATION}
)

if(IS_BIFUSD_STANDALONE)
    list(APPEND bifrost_env_config BIFROST_DISABLE_PACKS=usd_pack)
endif()

set(usd_env_config
     USD_TEST_RESOURCES_DIR=${resources_dir}/DynamicFileFormat
     PXR_PLUGINPATH_NAME=${BIFROST_USD_OUTPUT_DFF_DIR}
     USD_TEST_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
     TF_DEBUG="PLUG_REGISTRATION PLUG_LOAD"
     PXR_AR_DEFAULT_SEARCH_PATH=${resources_dir}
)

foreach(test_file ${test_files})
    configure_bifusd_unittest(
        ${test_file} "DynamicFileFormat"
        LINK_LIBS ${libs} ${pxr_libs}
        SHARED_LIB_DIRS ${BIFUSD_EXTRA_BUILD_AND_TEST_PATHS} ${Maya_SHARED_LIB_DIRS}
        ENV_VARS ${bifrost_env_config} ${usd_env_config} ${asan_options}
        OPTIONS      $<${BIFUSD_IS_MSC}:/wd4251 /wd4273>
    )
    get_unittest_target(unittest_target "DynamicFileFormat" ${test_file})
    # bifusd_append_test_path_env_var takes care of handling the path separator
    # (':' for unix, escaped ';' for windows) to not be confused for CMake list:
    bifusd_append_test_path_env_var(${unittest_target} "BIFROST_LIB_CONFIG_FILES" "${lib_config_files}")
endforeach()