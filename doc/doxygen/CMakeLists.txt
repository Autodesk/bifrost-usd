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

#==============================================================================
# DOXYGEN CONFIGURATION
#==============================================================================

option(
    BIFROST_USD_APIDOC_ENABLE_DIAGRAMS
    "Generates class and callgraph diagrams for the doxygen documentation (requires GraphViz)"
    OFF)

find_package(Doxygen)
if(NOT DOXYGEN_FOUND)
    message(WARNING
        "Doxygen isn't found on this machine. The API documentation will not be built.")
    return()
endif()

function(bifrost_usd_build_doc doxy_doc_type)

    #
    # Note: Most of the following variables are pasted into Doxyfile.in at
    # configuration time.
    #
    set(doxy_output_root     ${CMAKE_CURRENT_BINARY_DIR}/${doxy_doc_type})
    set(doxyfile_in          ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in      )
    set(doxyfile             ${doxy_output_root}/Doxyfile)
    set(doxy_html_index_file ${doxy_output_root}/html/index.html)
    set(doxy_zip_file        ${CMAKE_CURRENT_BINARY_DIR}/${doxy_doc_type}-doc.zip)

    set(doxy_strip_from_path ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})

    if(BIFROST_USD_APIDOC_ENABLE_DIAGRAMS)
        set(doxy_enable_graph YES)
    else()
        set(doxy_enable_graph NO)
    endif()

    get_property(doxy_output_dir_include GLOBAL PROPERTY BIFUSD_DOXYGEN_INCLUDE)
    if(doxy_doc_type STREQUAL nodedefs)
        set(doxy_input  ${BIFUSD_OUTPUT_INCLUDE_DIR}/nodedefs)

        file(GLOB_RECURSE srcfiles
            ${PROJECT_SOURCE_DIR}/src/nodedefs/*.h
            ${PROJECT_SOURCE_DIR}/src/nodedefs/*.cpp
            ${PROJECT_SOURCE_DIR}/src/nodedefs/*.dox)

        list(APPEND srcfiles ${doxy_output_dir_include})
    elseif(doxy_doc_type STREQUAL sdk)
        set(doxy_input  ${BIFUSD_OUTPUT_INCLUDE_DIR}/BifrostUsd)

        file(GLOB_RECURSE srcfiles
            ${PROJECT_SOURCE_DIR}/src/BifrostUsd/*.h
            ${PROJECT_SOURCE_DIR}/src/BifrostUsd/*.cpp
            ${PROJECT_SOURCE_DIR}/src/BifrostUsd/*.dox)

        list(APPEND srcfiles ${doxy_output_dir_include})
    else()
        message(FATAL_ERROR "Unknown doxy_doc_type ${doxy_doc_type}")
        return()
    endif()

    get_property(doxy_exclude GLOBAL PROPERTY BIFUSD_DOXYGEN_EXCLUDE)
    string(REPLACE ";" " " doxy_input           "${doxy_input}")
    string(REPLACE ";" " " doxy_exclude         "${doxy_exclude}")
    string(REPLACE ";" " " doxy_strip_from_path "${doxy_strip_from_path}")

    configure_file(${doxyfile_in} ${doxyfile} @ONLY)

    add_custom_command(
        OUTPUT ${doxy_html_index_file}
        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
        # The following should be ${doxyfile} only but it will break the
        # dependency.  The optimal solution would be creating a
        # custom_command for ${doxyfile} generation but I still have to
        # figure out how...
        MAIN_DEPENDENCY ${doxyfile} ${doxyfile_in}
        DEPENDS ${DOXYGEN_EXECUTABLE} ${srcfiles}
        COMMENT "Generating HTML ${doxy_doc_type} API documentation")

    add_custom_target(
        ${BIFUSD_PACKAGE_NAME}_apidoc-${doxy_doc_type}-html
        DEPENDS ${doxy_html_index_file})

    add_custom_command(
        OUTPUT ${doxy_zip_file}
        COMMAND ${CMAKE_COMMAND} -E tar c ${doxy_zip_file} --format=zip --
                ${doxy_output_root}/html
        MAIN_DEPENDENCY ${doxy_html_index_file}
        DEPENDS ${BIFUSD_PACKAGE_NAME}_apidoc-${doxy_doc_type}-html ${srcfiles}
        COMMENT "Generating Zipped ${doxy_doc_type}-doc.zip API documentation")

    add_custom_target(
        ${BIFUSD_PACKAGE_NAME}_apidoc-${doxy_doc_type}
        DEPENDS ${doxy_zip_file})

    install(
            FILES       ${doxy_zip_file}
            DESTINATION ${BIFUSD_INSTALL_DOCS_DIR}
            OPTIONAL)

    add_custom_command(
        TARGET ${BIFUSD_PACKAGE_NAME}_apidoc-clean
        COMMAND ${CMAKE_COMMAND} -E remove_directory
        ${doxy_output_root}/html)

endfunction()

add_custom_target(${BIFUSD_PACKAGE_NAME}_apidoc-clean)

# build the doc
bifrost_usd_build_doc(nodedefs)
bifrost_usd_build_doc(sdk)

# Note: Add ALL as dependancy to build the docs by default
add_custom_target(${BIFUSD_PACKAGE_NAME}_apidoc DEPENDS ${BIFUSD_PACKAGE_NAME}_apidoc-nodedefs ${BIFUSD_PACKAGE_NAME}_apidoc-sdk)

# Add properties to be able to add external documentation
# Note: Install dirs are relative and build dirs are absolute
set_target_properties( ${BIFUSD_PACKAGE_NAME}_apidoc PROPERTIES
    BIFROST_USD_PACK_NODE_DOXYGEN_BUILD_DIR   ${CMAKE_CURRENT_BINARY_DIR}/nodedefs
    BIFROST_USD_PACK_NODE_MARKDOWN_BUILD_DIR  ${CMAKE_CURRENT_BINARY_DIR}/../markdown/nodes
    BIFROST_USD_PACK_NODE_DOC_INSTALL_DIR     ${BIFROST_USD_PACK_INSTALL_RES_DOCS_DIR}
    BIFROST_USD_PACK_NODE_JSON_BUILD_DIR      ${PROJECT_BINARY_DIR}/${BIFROST_USD_PACK_INSTALL_RES_JSON_DIR})
