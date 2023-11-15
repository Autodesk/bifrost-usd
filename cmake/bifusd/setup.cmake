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
if(bifusd_setup_included)
    return()
endif()
set(bifusd_setup_included true)


#==============================================================================
# Platform-specific variables
#==============================================================================

bifusd_set_if_matches(BIFUSD_IS_WINDOWS CMAKE_SYSTEM_NAME "Windows")
bifusd_set_if_matches(BIFUSD_IS_LINUX   CMAKE_SYSTEM_NAME "Linux")
bifusd_set_if_matches(BIFUSD_IS_OSX     CMAKE_SYSTEM_NAME "Darwin")

bifusd_eval_condition(BIFUSD_IS_UNIX
                       CONDITION BIFUSD_IS_LINUX OR BIFUSD_IS_OSX)

if (BIFUSD_IS_WINDOWS)
    set(PATH_SEPARATOR ";")
elseif(BIFUSD_IS_UNIX)
    set(PATH_SEPARATOR ":")
else()
    message(FATAL_ERROR "Unsupported platform.")
endif()

if (BIFUSD_IS_WINDOWS)
    # Information retrieved from:
    #     http://www.codeguru.com/cpp/w-p/system/systeminformation/article.php/c8973/

    bifusd_set_if_matches(BIFUSD_IS_WINDOWS_7 CMAKE_SYSTEM_VERSION "6.1")
    bifusd_set_if_matches(BIFUSD_IS_WINDOWS_8 CMAKE_SYSTEM_VERSION "6.2")
endif()

#==============================================================================
# Variant variables
#==============================================================================

# Update the documentation string of CMAKE_BUILD_TYPE for GUIs
set(CMAKE_BUILD_TYPE
    "${CMAKE_BUILD_TYPE}" CACHE STRING
    "Choose the type of build, options are: Debug Release RelWithDebInfo RelWithAsserts MinSizeRel."
    FORCE)

bifusd_set_if_matches(BIFUSD_IS_RELEASE  CMAKE_BUILD_TYPE "^(Release|RelWithDebInfo|RelWithAsserts|MinSizeRel|Coverage)$")
bifusd_set_if_matches(BIFUSD_IS_DEBUG    CMAKE_BUILD_TYPE "^Debug$")

bifusd_set_if_matches(BIFUSD_HAS_DEBUG_FILES CMAKE_BUILD_TYPE "^(RelWithDebInfo|RelWithAsserts|Debug)$")

# Determine if assertions are enabled in the build.
bifusd_set_if_matches(BIFUSD_IS_ASSERTING CMAKE_BUILD_TYPE "^(Debug|RelWithAsserts)$")

# Define the strings here and use them consistently here or elsewhere
# instead of hardcoding "release","debug", or "rel" etc.
set(BIFUSD_BUILDVAR_RELEASE  "release")
set(BIFUSD_BUILDVAR_DEBUG    "debug")

# Determine the variant build directory where the libraries are located
if (BIFUSD_IS_RELEASE)
    set(BIFUSD_VARIANT_DIR      "Release")
    set(BIFUSD_SHORTVARIANT_DIR "rel")
    set(BIFUSD_BUILDVAR_NAME    "${BIFUSD_BUILDVAR_RELEASE}")
elseif(BIFUSD_IS_DEBUG)
    set(BIFUSD_VARIANT_DIR      "Debug")
    set(BIFUSD_SHORTVARIANT_DIR "dbg")
    set(BIFUSD_BUILDVAR_NAME    "${BIFUSD_BUILDVAR_DEBUG}")
else()
    message(FATAL_ERROR "Unsupported variant.")
endif()

# By default assume artifacts categorized by variant in Artifactory were deployed
# using Bifusd and are use RelWithDebInfo and Debug as their variant directory.
if (BIFUSD_IS_RELEASE)
    set(BIFUSD_ARTIFACT_VARIANT_DIR "RelWithDebInfo")
elseif (BIFUSD_IS_DEBUG)
    set(BIFUSD_ARTIFACT_VARIANT_DIR "Debug")
else()
    message(FATAL_ERROR "Unsupported variant.")
endif()

# When importing modules, use RelWithDebInfo build type for
# both Release and RelWithDebInfo.
if (BIFUSD_IS_RELEASE)
    set(BIFUSD_IMPORTED_LOCATION_PROPERTY "IMPORTED_LOCATION_RELWITHDEBINFO")
elseif (BIFUSD_IS_DEBUG)
    set(BIFUSD_IMPORTED_LOCATION_PROPERTY "IMPORTED_LOCATION_DEBUG")
else()
    message(FATAL_ERROR "Unsupported variant.")
endif()

# Define the flag identifying the build variant
if (BIFUSD_IS_DEBUG)
    add_definitions(-DBIFUSD_DEBUG)
elseif(BIFUSD_IS_RELEASE)
    add_definitions(-DBIFUSD_RELEASE)
else()
    message(FATAL_ERROR "Unsupported variant.")
endif()

# Resolve the value of a given Bifusd variable according to the explicit
# override configurations.
function(bifusd_get_resolved_variable variable result_var)
    set(value ${${variable}})

    set(${result_var} ${value} PARENT_SCOPE)
endfunction(bifusd_get_resolved_variable)

#==============================================================================
# Default linking type.
#==============================================================================
set(BIFUSD_LINK_TYPE  "static")

#==============================================================================
# Target subdirectory structure for build and install
#==============================================================================
set(BIFUSD_INSTALL_VARIANT_DIR      "${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}")

set(BIFUSD_INSTALL_ROOT_DIR         ".")
set(BIFUSD_INSTALL_CMAKE_DIR        "cmake")
set(BIFUSD_INSTALL_COMMON_DIR       "common")
set(BIFUSD_INSTALL_PLATFORM_DIR     "platform/${BIFUSD_INSTALL_VARIANT_DIR}")

# Being called by other CMake? Use BIFUSD_INSTALL_PREFIX
# It allows isolating from the other packages.
if( BIFUSD_INSTALL_PREFIX )
    set(BIFUSD_INSTALL_ROOT_DIR         "${BIFUSD_INSTALL_PREFIX}")
    set(BIFUSD_INSTALL_CMAKE_DIR        "${BIFUSD_INSTALL_PREFIX}/${BIFUSD_INSTALL_CMAKE_DIR}")
    set(BIFUSD_INSTALL_COMMON_DIR       "${BIFUSD_INSTALL_PREFIX}/${BIFUSD_INSTALL_COMMON_DIR}")
    set(BIFUSD_INSTALL_PLATFORM_DIR     "${BIFUSD_INSTALL_PREFIX}/${BIFUSD_INSTALL_PLATFORM_DIR}")
endif()

set(BIFUSD_INSTALL_INCLUDE_DIR      "${BIFUSD_INSTALL_COMMON_DIR}/include")
set(BIFUSD_INSTALL_DOCS_DIR         "${BIFUSD_INSTALL_COMMON_DIR}/docs")
set(BIFUSD_INSTALL_BIN_DIR          "${BIFUSD_INSTALL_PLATFORM_DIR}/bin")
set(BIFUSD_INSTALL_LIB_DIR          "${BIFUSD_INSTALL_PLATFORM_DIR}/lib")

#==============================================================================
# Target output directories
#==============================================================================
set(BIFUSD_OUTPUT_ROOT_DIR    "${PROJECT_BINARY_DIR}/${BIFUSD_INSTALL_ROOT_DIR}")
set(BIFUSD_OUTPUT_LIB_DIR     "${PROJECT_BINARY_DIR}/${BIFUSD_INSTALL_LIB_DIR}")
set(BIFUSD_OUTPUT_BIN_DIR     "${PROJECT_BINARY_DIR}/${BIFUSD_INSTALL_BIN_DIR}")
set(BIFUSD_OUTPUT_CMAKE_DIR   "${PROJECT_BINARY_DIR}/${BIFUSD_INSTALL_CMAKE_DIR}")
set(BIFUSD_OUTPUT_INCLUDE_DIR "${PROJECT_BINARY_DIR}/${BIFUSD_INSTALL_COMMON_DIR}/include")
set(BIFUSD_OUTPUT_DOCS_DIR    "${PROJECT_BINARY_DIR}/${BIFUSD_INSTALL_DOCS_DIR}")

#==============================================================================
# Platform directories
#==============================================================================

# Determine the platform system directory where the libraries are located
if (BIFUSD_IS_WINDOWS)
    set(BIFUSD_SYSTEM_DIR   "Windows")
    set(BIFUSD_SHORT_OSNAME "win")
elseif(BIFUSD_IS_LINUX)
    set(BIFUSD_SYSTEM_DIR   "Linux")
    set(BIFUSD_SHORT_OSNAME "lin")
elseif(BIFUSD_IS_OSX)
    set(BIFUSD_SYSTEM_DIR   "Darwin")
    set(BIFUSD_SHORT_OSNAME "osx")
else()
    message(FATAL_ERROR "Unsupported platform.")
endif()

#==============================================================================
# LINK JOB LIMIT
#==============================================================================

# Limit the number of link jobs that can be run in parallel. This is especially
# useful on Windows. The entire link process is limited by the single mspdbsrv
# process and disk I/O. By attempting to link too many things in parallel, we
# simply slow things down...
set(BIFUSD_JOB_POOL_LINK "FALSE"
    CACHE STRING "Override the number of link jobs that can be run in parallel.")

if (BIFUSD_JOB_POOL_LINK)
    message(STATUS "Number of link jobs = ${BIFUSD_JOB_POOL_LINK}")

    set_property(GLOBAL PROPERTY JOB_POOLS link_pool=${BIFUSD_JOB_POOL_LINK})
    set(CMAKE_JOB_POOL_LINK link_pool)
endif()
