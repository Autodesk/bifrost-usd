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

cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

message(STATUS "CMake version: ${CMAKE_VERSION}")

# A calling package (other CMake) can set the package name
# Note: The USD Pack that is shipped with Bifrost is named `usd_pack` and it is
#       loaded by default by Bifrost.
#       Here we must use a different name for this custom/standalone build of
#       the pack, so the USD Pack from Bifrost's installation directory can be
#       disabled when needed and replaced by this one.
if( NOT BIFUSD_PACKAGE_NAME )
    set(BIFUSD_PACKAGE_NAME bifrost_usd_pack)
endif()

set(IS_BIFUSD_STANDALONE ON CACHE BOOL "Indicate if this is a standalone build of Bifrost USD")

# C++ 17 not extensions - by default
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/version.info)

# Enable no language yet.
project(${BIFUSD_PACKAGE_NAME}
        VERSION ${BIFROST_USD_PACK_MAJOR_VERSION}.${BIFROST_USD_PACK_MINOR_VERSION}.${BIFROST_USD_PACK_PATCH_LEVEL}
        LANGUAGES NONE)

include(CMakePrintHelpers)

# Generates a 'compile_commands.json' file containing the exact compilation
# command for all translation units of the project in a machine-readable form.
#
# Useful for many offline tools and very useful for debugging the build.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

#==============================================================================
# Define common build variables
#==============================================================================
set(BIFUSD_CMAKE_DIR "${PROJECT_SOURCE_DIR}/cmake")
set(BIFUSD_TOOLS_DIR "${BIFUSD_CMAKE_DIR}/bifusd")

set(BIFUSD_MAJOR_VERSION ${BIFROST_USD_PACK_MAJOR_VERSION})
set(BIFUSD_MINOR_VERSION ${BIFROST_USD_PACK_MINOR_VERSION})
set(BIFUSD_PATCH_LEVEL   ${BIFROST_USD_PACK_PATCH_LEVEL})
set(BIFUSD_PRODUCT_NAME  "\"Bifrost USD pack\"")

#==============================================================================
# Target/Property for json files
#==============================================================================
# This target and property needs to be forward declared before utils.cmake.
# because bifusd_header_parser refers to them. More deps/properties are added
# to the target later.
#
if( NOT TARGET ${BIFUSD_PACKAGE_NAME}_config_info)
    add_custom_target(${BIFUSD_PACKAGE_NAME}_config_info)
endif()

# Add property to collect the list of generated json files.
set_target_properties(${BIFUSD_PACKAGE_NAME}_config_info PROPERTIES
                       BIFROST_USD_PACK_ALL_JSON_FILES "")

#==============================================================================
# General setup
# This defines functions and macros that used later on; in particular
# in setup.cmake and the compiler setup.
#==============================================================================
include(${BIFUSD_TOOLS_DIR}/utils.cmake)
include(${BIFUSD_TOOLS_DIR}/setup.cmake)
include(${BIFUSD_TOOLS_DIR}/version.cmake)

#==============================================================================
# PACKAGES USED WHILE BUILDING
#==============================================================================
set(BIFUSD_USE_PYTHON_VERSION 3 CACHE STRING "The Python version used.")
include(${BIFUSD_TOOLS_DIR}/python.cmake)

#==============================================================================
# Local setup
#==============================================================================
include(cmake/setup.cmake)
include(cmake/utils.cmake)

# Standalone build setup the compiler
# Else use the parent environment's compiler
if(IS_BIFUSD_STANDALONE)
    #==============================================================================
    # COMPILER
    #==============================================================================

    # Include the file configuring the compiler used on the given platform.
    # Enabling the languages and set the compiler flags after the project
    # has been configured follows the suggestion in:
    #
    # http://www.cmake.org/Bug/print_bug_page.php?bug_id=11942

    set(CMAKE_USER_MAKE_RULES_OVERRIDE "${BIFUSD_TOOLS_DIR}/compiler_${CMAKE_SYSTEM_NAME}.cmake")

    #==============================================================================
    # LANGUAGES
    #==============================================================================
    # The platform setup needs to be called before enable_language
    # It will setup the platform's SDK location, when necessary.
    include(${BIFUSD_TOOLS_DIR}/platform_sdk_${CMAKE_SYSTEM_NAME}.cmake)

    # Enabling the languages and set the compiler flags after the project
    # has been configured follows the suggestion in:
    #
    # http://www.cmake.org/Bug/print_bug_page.php?bug_id=11942
    enable_language(CXX)

    # Finalize the various build variants now that the C++ language has been
    # enabled.
    include(${BIFUSD_TOOLS_DIR}/final_compiler_setup.cmake)
else()
    include(${BIFUSD_TOOLS_DIR}/compiler_config.cmake)
endif()

#==============================================================================
# PACKAGING - uses CPack
#==============================================================================

include(${BIFUSD_TOOLS_DIR}/package.cmake)

#==============================================================================
# 3rd-parties
#==============================================================================

#==============================================================================
# TESTING
#==============================================================================
include(${BIFUSD_TOOLS_DIR}/gtest.cmake)

#==============================================================================
# Import/Find installed packages
#==============================================================================

find_package(Bifrost REQUIRED SDK)
find_package(USD REQUIRED)

#==============================================================================
# BIFUSD SUBDIRECTORIES
#==============================================================================

add_subdirectory(${BIFUSD_TOOLS_DIR}/src/config)
add_subdirectory(${BIFUSD_TOOLS_DIR}/src/versioning)

#==============================================================================
# SUBDIRECTORIES
#==============================================================================
print_settings()

add_subdirectory(src)
add_subdirectory(doc)

# Test resources can be set from the outside.
# This allows having the test data in a downloaded archive, for example.
if( NOT BIFUSD_TEST_RESOURCES_LOCATION )
    set( BIFUSD_TEST_RESOURCES_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/test")
endif()
enable_testing()
add_subdirectory(utils/test)
add_subdirectory(test)

#==============================================================================
# HOST PLUGINS - TRANSLATION TABLES AND OTHERS
#==============================================================================
#
# Maya
if( MAYA_RUNTIME_LOCATION)
    find_package(Maya REQUIRED)
    find_package(Bifrost REQUIRED Maya)
    add_subdirectory(maya_plugin)
endif()

#==============================================================================
# Bifrost Hydra
# NOT PART OF PUBLIC BIFROST SDK YET
# NOT PART OF PUBLIC BIFROST SDK YET
# NOT PART OF PUBLIC BIFROST SDK YET
#==============================================================================
#
if( BIFUSD_BUILD_HYDRA )
    add_subdirectory(bifrost_hydra)
endif()

#==============================================================================
# Configure "clang_tidy" target
#==============================================================================
#
if( CLAND_TIDY_LOCATION )
    include(${BIFUSD_TOOLS_DIR}/clang-tidy.cmake)
    bifrost_usd_clang_tidy()
endif()

#==============================================================================
# FINALIZE
#==============================================================================
# Publish the CMake module files
install(
    FILES ${BIFUSD_CMAKE_DIR}/modules/FindUSD.cmake
    DESTINATION ${BIFUSD_INSTALL_CMAKE_DIR}/modules )

install(
    FILES ${BIFUSD_CMAKE_DIR}/modules/FindMaya.cmake
    DESTINATION ${BIFUSD_INSTALL_CMAKE_DIR}/modules )

include( ${BIFUSD_TOOLS_DIR}/finalize.cmake)
