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
    Amino::Executor
    BifusdTBB
    BifusdTBBHelpers
    BifrostUSD
    BifrostUsdGraphExecutor
    BifrostUSDNodedefs
    BifrostUSDTestUtils
    BifrostUsdTranslator
    usd
    usdGeom
)

#------------------------------------------------------------------------------
# Config files for regular tests
#------------------------------------------------------------------------------

set(config_files
    ${BIFUSD_EXTRA_CONFIG_FILES}
    ${BIFUSD_OUTPUT_ROOT_DIR}/usd_pack_config.json
    ${CMAKE_CURRENT_SOURCE_DIR}/basic_compounds_config.json
)

if(IS_BIFUSD_STANDALONE)
    list(PREPEND config_files ${BIFROST_LOCATION}/resources/standalone_config.json)
endif()

#------------------------------------------------------------------------------
# Config files for library tests
#------------------------------------------------------------------------------

set(empty_config_file ${CMAKE_CURRENT_SOURCE_DIR}/empty_config.json)
set(extra_config_file_tpl ${CMAKE_CURRENT_SOURCE_DIR}/extra_compounds_config.json.in)
set(placeholder_config_file "${BIFUSD_OUTPUT_ROOT_DIR}/test_library_placeholder_config.json")

# Substitute the ${xyz} variables in the "extra" config file template,
# and make the changes in the build output directory, so that the test can use
# the generated config file with the correct paths:
set(GRAPH_EXECUTOR_EXTRA_COMPOUNDS_REL "../compounds/GraphExecutor/extra")
get_filename_component(
    GRAPH_EXECUTOR_EXTRA_COMPOUNDS ${GRAPH_EXECUTOR_EXTRA_COMPOUNDS_REL}
    ABSOLUTE BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
)
set(extra_config_file ${BIFUSD_OUTPUT_ROOT_DIR}/extra_compounds_config.json)
configure_file(${extra_config_file_tpl} ${extra_config_file})

# Copy the "empty" config file into the "placeholder" config file in the build
# output directory.
configure_file(
    ${empty_config_file}
    ${placeholder_config_file}
    COPYONLY
)

set(library_test_config_files
    ${config_files}
    ${placeholder_config_file}
)

#------------------------------------------------------------------------------
# Regular tests
#------------------------------------------------------------------------------
set(regular_test_files
    testGraphExecutor.cpp
)

foreach(test_file ${regular_test_files})
    configure_bifusd_unittest(
        ${test_file} "GraphExecutor"
        LINK_LIBS ${libs}
        SHARED_LIB_DIRS ${BIFUSD_EXTRA_BUILD_AND_TEST_PATHS}
        DEFINITIONS USD_TEST_RESOURCES_DIR="${resources_dir}"
    )
    get_unittest_target(unittest_target "GraphExecutor" ${test_file})
    bifusd_append_test_path_env_var(${unittest_target} "BIFROST_LIB_CONFIG_FILES" "${config_files}")
endforeach()

#------------------------------------------------------------------------------
# testLibrary
#------------------------------------------------------------------------------

configure_bifusd_unittest(
    testLibrary.cpp "GraphExecutor"
    LINK_LIBS ${libs}
    SHARED_LIB_DIRS ${BIFUSD_EXTRA_BUILD_AND_TEST_PATHS}
    DEFINITIONS USD_TEST_RESOURCES_DIR="${resources_dir}"
                PLACEHOLDER_CONFIG_FILE="${placeholder_config_file}"
                EMPTY_CONFIG_FILE="${empty_config_file}"
                EXTRA_CONFIG_FILE="${extra_config_file}"
)
get_unittest_target(unittest_target "GraphExecutor" testLibrary.cpp)
bifusd_append_test_path_env_var(${unittest_target} "BIFROST_LIB_CONFIG_FILES" "${library_test_config_files}")
