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

include(${BIFUSD_TOOLS_DIR}/utils.cmake)

# Helper function to configure a SDK unit test target
function(configure_bifusd_unittest unittest_src_file test_module_name)

    set(modes
        "INCLUDE_DIRS"
        "OBJECT_LIBS"
        "LINK_LIBS"
        "SHARED_LIB_DIRS"
        "DEFINITIONS"
        "OPTIONS"
        "PASS_REGEX"
        "FAIL_REGEX"
        "ENV_VARS"
        "LABELS"
        "CATEGORIES"
        "EXTRA_SRC_FILES"
        "SERIAL"
        "KLUDGE_LINK_OPTIONS"
    )

    bifusd_extract_options("${modes}" ${ARGN})

    set(categories)
    if(CATEGORIES-FOUND)
        list(APPEND categories ${CATEGORIES})
    else()
        list(APPEND categories ${BIFUSD_SANITY_TEST_CATEGORY})
    endif()

    set(link_libs
        bifusd_gtest_main
        BifrostUSDTestUtils
        ${LINK_LIBS}
    )

    get_unittest_target(unittest_target ${test_module_name} ${unittest_src_file})

    set(env_vars
       "AMINO_ALWAYS_PRINT_ERRORS=1"
        ${ENV_VARS})

    bifusd_configure_unittest(${unittest_target}
                               SRC_FILES       ${unittest_src_file}
                                               ${EXTRA_SRC_FILES}
                               INCLUDE_DIRS    ${INCLUDE_DIRS}
                               OBJECT_LIBS     ${OBJECT_LIBS}
                               LINK_LIBS       ${link_libs}
                               DEFINITIONS     ${DEFINITIONS}
                               OPTIONS         ${OPTIONS}
                               ENV_VARS        ${env_vars}
                               PASS_REGEX      ${PASS_REGEX}
                               FAIL_REGEX      ${FAIL_REGEX}
                               LABELS          ${LABELS}
                               CATEGORIES      ${categories}
                               SHARED_LIB_DIRS ${SHARED_LIB_DIRS}
                               ${SERIAL-ARGS})

    # On Linux we need the rt lib for clock_gettime function
    if(BIFUSD_IS_LINUX)
        set_target_properties( ${unittest_target} PROPERTIES LINK_FLAGS -lrt )
    endif()

    if(KLUDGE_LINK_OPTIONS)
        target_link_options( ${unittest_target} BEFORE PRIVATE ${KLUDGE_LINK_OPTIONS})
    endif()

    if (BIFUSD_IS_DEBUG)
        set_tests_properties( ${unittest_target} PROPERTIES TIMEOUT 600 )
    endif()

endfunction(configure_bifusd_unittest)

# Helper function to parse the headers to generate json files used to add
# custom types and operators in Amino library.
#
#   bifusd_header_parser(<target_name> [options] )
#   parameters:
#       target_name: name of the target to be created.
#   options:
#       HEADER_FILES: list of header files to parse (passed to cpp2json)
#       LINK_LIBS: list of libraries that represent the header dependencies (passed to cpp2json)
#       JSON_OUTPUT_DIR: (optional) output directory for the json files (default: ${BIFROST_USD_OUTPUT_JSON_DIR})
#       JSON_INSTALL_DIR: (optional) install directory for the json files (default: ${BIFROST_USD_PACK_INSTALL_RES_JSON_DIR})
#   Notes
#      DISABLE_PXR_HEADERS: disable the pxr headers parsing passed to cpp2json
#
function(bifusd_header_parser target_name )
    set(forwarded_modes
        "HEADER_FILES"
        "LINK_LIBS"
    )
    set(modes
        "JSON_OUTPUT_DIR"
        "JSON_INSTALL_DIR"
        ${forwarded_modes}
    )

    bifusd_extract_options("${modes}" ${ARGN})
    bifusd_get_options(forwarded_options ${forwarded_modes})

    # On win, tbb needs /MT (ie. _MT def) and disabling of min,
    # max in winmindefs.h as they interfere with numeric_limits::max
    set(defs)
    if (BIFUSD_IS_WINDOWS)
        set (defs _MT NOMINMAX)
    endif()

    set( json_output_dir ${BIFROST_USD_OUTPUT_JSON_DIR})
    if( ${JSON_OUTPUT_DIR-FOUND} )
        set( json_output_dir ${JSON_OUTPUT_DIR})
    endif()

    # install files go to default install location if none given
    set(json_install_dir ${BIFROST_USD_PACK_INSTALL_RES_JSON_DIR})
    if(${JSON_INSTALL_DIR-FOUND})
        set(json_install_dir ${JSON_INSTALL_DIR})
    endif()

    amino_cpp2json(${target_name}
        CPP2JSON     $<TARGET_FILE:Bifrost::cpp2json>
        DESTINATION ${json_output_dir}
        OUT_VAR     node_def_jsons
        DEFINITIONS ${defs} DISABLE_PXR_HEADERS
        ${forwarded_options}
    )

    set_property(TARGET ${BIFUSD_PACKAGE_NAME}_config_info
        APPEND PROPERTY BIFROST_USD_PACK_ALL_JSON_FILES "${node_def_jsons}")
    install(FILES ${node_def_jsons}  DESTINATION ${json_install_dir})
endfunction(bifusd_header_parser)

# Helper function to return rpaths
# If is called from outside (ECG it sets ../thirdparty/lib)
function( bifusd_set_extra_rpaths return_extra_rpaths )

    if( BIFUSD_EXTRA_INSTALL_RPATHS )
        set( extra_rpaths "${BIFUSD_EXTRA_INSTALL_RPATHS}")
    elseif(IS_BIFUSD_STANDALONE)
        file(RELATIVE_PATH bifusd_to_bifrost
                "${CMAKE_INSTALL_PREFIX}/${BIFUSD_INSTALL_LIB_DIR}"
                "${BIFROST_LOCATION}/lib")

        file(RELATIVE_PATH bifusd_to_usd
                "${CMAKE_INSTALL_PREFIX}/${BIFUSD_INSTALL_LIB_DIR}"
                "${USD_LOCATION}/lib")

        # No relative paths to the Maya runtime.
        # Maya is loaded and setup when translation tables for Maya are used.
        # Runtime is readily available...
        # file(RELATIVE_PATH bifusdmaya_to_maya
        #        "${CMAKE_INSTALL_PREFIX}/${BIFUSD_INSTALL_LIB_DIR}"
        #        "${MAYA_RUNTIME_LOCATION}/lib")

        set( extra_rpaths ${bifusd_to_bifrost} ${bifusd_to_usd})
    else()
        set( extra_rpaths "")
    endif()

    set (${return_extra_rpaths} ${extra_rpaths} PARENT_SCOPE )
endfunction( bifusd_set_extra_rpaths)
