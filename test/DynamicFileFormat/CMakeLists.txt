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
    BifrostUsdDffDiagnostics
    BifrostUsdGraphExecutor
    BifrostUsd
    BifrostUsdTestUtils
)
set(pxr_libs
    usd
    usdGeom
)

#------------------------------------------------------------------------------
# Bifrost Lib Config files
#------------------------------------------------------------------------------

# Config files for regular tests
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

# Minimum set of config files reusable by any test that just needs the Amino
# library to initialize (no usd_pack, no DFF compounds, no test-specific extras).
# Each consumer appends its own test-specific config files at the use site.
if(IS_BIFUSD_STANDALONE)
    set(minimum_lib_config_files
        ${BIFROST_LOCATION}/resources/standalone_config.json
        ${BIFUSD_EXTRA_CONFIG_FILES}
    )
else()
    if(NOT EXISTS "${AMINO_LOCATION}/amino_config.json")
        message(FATAL_ERROR
            "Expected amino_config.json at '${AMINO_LOCATION}/amino_config.json' "
            "but file not found. AMINO_LOCATION is '${AMINO_LOCATION}'.")
    endif()
    set(minimum_lib_config_files "${AMINO_LOCATION}/amino_config.json")
endif()

#------------------------------------------------------------------------------
# Test files
#------------------------------------------------------------------------------
set(test_files
    testFileFormat_compoundArgs.cpp
    testFileFormat_contents.cpp
    testFileFormat_globalsArgs.cpp
    testFileFormat_inputsArgs.cpp
    testFileFormat_outputsArgs.cpp
    testFileFormat_settingsArgs.cpp
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

# DFF plugin tests are quite slow to execute at this time, especially with
# sanitizers or valgrind. Setting a longer timeout to avoid spurious test failures.
# TODO Change the compounds being used by these tests to reduce significantly
#      the execution time and remove the need for a long timeout.
set(test_timeout 3600)

# These tests are too slow under Debug + Valgrind to complete within a
# reasonable per-test timeout. They are already covered by the RelWithDebInfo
# Valgrind run, so skip them in Debug to prevent spurious failures.
if(NOT (BIFUSD_VALGRIND_ENABLED AND BIFUSD_IS_DEBUG))
    foreach(test_file ${test_files})
        configure_bifusd_unittest(
            ${test_file} "DynamicFileFormat"
            EXTRA_SRC_FILES
                ${CMAKE_CURRENT_SOURCE_DIR}/dffTestLayerHelpers.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/dffTestDiagnostics.cpp
            INCLUDE_DIRS     ${CMAKE_CURRENT_SOURCE_DIR}/../../src/DynamicFileFormat
            LINK_LIBS       ${libs} ${pxr_libs}
            SHARED_LIB_DIRS ${BIFUSD_EXTRA_BUILD_AND_TEST_PATHS} ${Maya_SHARED_LIB_DIRS}
            ENV_VARS        ${bifrost_env_config} ${usd_env_config} ${asan_options}
            OPTIONS         $<${BIFUSD_IS_MSC}:/wd4251 /wd4273>
            TIMEOUT         ${test_timeout}
        )
        get_unittest_target(unittest_target "DynamicFileFormat" ${test_file})
        # bifusd_append_test_path_env_var takes care of handling the path separator
        # (':' for unix, escaped ';' for windows) to not be confused for CMake list:
        bifusd_append_test_path_env_var(${unittest_target} "BIFROST_LIB_CONFIG_FILES" "${lib_config_files}")
    endforeach()
endif()

#------------------------------------------------------------------------------
# Config files for library-reload test (testFileFormat_reloadLibrary.cpp)
#------------------------------------------------------------------------------

set(dff_empty_config_file ${CMAKE_CURRENT_SOURCE_DIR}/empty_config.json)
set(dff_extra_config_file_tpl ${CMAKE_CURRENT_SOURCE_DIR}/extra_compounds_config.json.in)
set(dff_placeholder_config_file "${BIFUSD_OUTPUT_ROOT_DIR}/test_dff_reload_placeholder_config.json")

# Resolve the absolute path to the extra compounds directory (same directory
# used by test/GraphExecutor - the less.json compound lives there).
set(DFF_EXTRA_COMPOUNDS_REL "../compounds/GraphExecutor/extra")
get_filename_component(
    DFF_EXTRA_COMPOUNDS ${DFF_EXTRA_COMPOUNDS_REL}
    ABSOLUTE BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
)
set(dff_extra_config_file ${BIFUSD_OUTPUT_ROOT_DIR}/dff_extra_compounds_config.json)
configure_file(${dff_extra_config_file_tpl} ${dff_extra_config_file})

# Copy the empty config into the placeholder location at configure time.
# The test will overwrite it at runtime with the extra compounds config.
configure_file(
    ${dff_empty_config_file}
    ${dff_placeholder_config_file}
    COPYONLY
)

#------------------------------------------------------------------------------
# testFileFormat_reloadLibrary
#------------------------------------------------------------------------------

# These tests are too slow under Debug + Valgrind to complete within a
# reasonable per-test timeout. They are already covered by the RelWithDebInfo
# Valgrind run, so skip them in Debug to prevent spurious failures.
if(NOT (BIFUSD_VALGRIND_ENABLED AND BIFUSD_IS_DEBUG))
    configure_bifusd_unittest(
        testFileFormat_reloadLibrary.cpp "DynamicFileFormat"
        EXTRA_SRC_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/dffTestLayerHelpers.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/dffTestDiagnostics.cpp
        INCLUDE_DIRS     ${CMAKE_CURRENT_SOURCE_DIR}/../../src/DynamicFileFormat
        LINK_LIBS       ${libs} ${pxr_libs}
        SHARED_LIB_DIRS ${BIFUSD_EXTRA_BUILD_AND_TEST_PATHS} ${Maya_SHARED_LIB_DIRS}
        ENV_VARS        ${bifrost_env_config} ${usd_env_config} ${asan_options}
        OPTIONS         $<${BIFUSD_IS_MSC}:/wd4251 /wd4273>
        DEFINITIONS     PLACEHOLDER_CONFIG_FILE="${dff_placeholder_config_file}"
                        EMPTY_CONFIG_FILE="${dff_empty_config_file}"
                        EXTRA_CONFIG_FILE="${dff_extra_config_file}"
        TIMEOUT         ${test_timeout}
    )
    get_unittest_target(unittest_target "DynamicFileFormat" testFileFormat_reloadLibrary.cpp)
    bifusd_append_test_path_env_var(
        ${unittest_target} "BIFROST_LIB_CONFIG_FILES"
        "${minimum_lib_config_files};${dff_placeholder_config_file}"
    )
endif()

#------------------------------------------------------------------------------
# Test files that have no USD dependencies
#------------------------------------------------------------------------------

set(test_files_no_usd
    testParsingTools_scalars.cpp
    testParsingTools_vectors.cpp
    testParsingTools_others.cpp
    testStringToAny.cpp
)

set(parsing_tools_src_dir
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/DynamicFileFormat)

set(test_extra_src_files
    ${parsing_tools_src_dir}/parsingTools.cpp
    ${parsing_tools_src_dir}/stringToAny.cpp
)

foreach(test_file ${test_files_no_usd})
    configure_bifusd_unittest(
        ${test_file} "DynamicFileFormat"
        LINK_LIBS           ${libs}
        EXTRA_SRC_FILES     ${test_extra_src_files}
        INCLUDE_DIRS        ${parsing_tools_src_dir}
        SHARED_LIB_DIRS     ${BIFUSD_EXTRA_BUILD_AND_TEST_PATHS}
        ENV_VARS            ${bifrost_env_config} ${asan_options}
        OPTIONS             $<${BIFUSD_IS_MSC}:/wd4251 /wd4273>
    )
endforeach()

#------------------------------------------------------------------------------
# mayapy test to check if the Maya USD plugin is loading BifrostDynamicFileFormat plugin.
#------------------------------------------------------------------------------
if( NEED_MAYA_USD_PLUG_INFO AND MAYA_USD_PLUGIN_DIR )
    set(test_file testMayaUsdDynamicFileFormat.py)
    set(unittest_target DynamicFileFormat_${test_file})
    add_test(NAME ${unittest_target}
        COMMAND "${MAYA_PY_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/${test_file}"
    )

    if(IS_BIFUSD_STANDALONE)
        bifusd_append_test_path_env_var(${unittest_target} "BIFROST_DISABLE_PACKS" "usd_pack usdMayaTranslations")
        bifusd_append_test_path_env_var(${unittest_target} "BIFROST_LIB_CONFIG_FILES" "${lib_config_files}")
        bifusd_append_test_path_env_var(${unittest_target} "MAYA_PXR_PLUGINPATH_NAME"
            "${CMAKE_INSTALL_PREFIX}" # directory containing mayaUsdPlugInfo.json
        )
        bifusd_append_test_path_env_var(${unittest_target} "MAYA_MODULE_PATH"
            "${MAYA_USD_PLUGIN_DIR}" # to find mayausd.mod
        )
    else()
        message(STATUS "Testing $test_file is not yet supported from BMP. Skipping.")
    endif()
endif()
