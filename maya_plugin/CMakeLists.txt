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

if(NOT IS_BIFUSD_STANDALONE)
    # This code acts like a mini project setup when calling directly the maya_plugin cmake.
    # That is, when an external sub-project, not main project,
    # does a direct add_subdirectory(maya_plugin)

    #==============================================================================
    # Define common build variables
    #==============================================================================
    set(BIFUSD_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/../cmake")
    set(BIFUSD_TOOLS_DIR "${BIFUSD_CMAKE_DIR}/bifusd")

    # A calling package (other CMake) can set the package name
    if(NOT BIFUSD_PACKAGE_NAME)
        set(BIFUSD_PACKAGE_NAME "usd_pack")
        # Need to "export" parent scope because this may be needed by the caller
        set(BIFUSD_PACKAGE_NAME ${BIFUSD_PACKAGE_NAME} PARENT_SCOPE)
    endif()
    # A calling package (other CMake) can set the translation table name
    if(NOT BIFUSD_MAYA_TRANSLATIONS_NAME)
        set(BIFUSD_MAYA_TRANSLATIONS_NAME "usdMayaTranslations")
        # Need to "export" parent scope because this may be needed by the caller
        set(BIFUSD_MAYA_TRANSLATIONS_NAME ${BIFUSD_MAYA_TRANSLATIONS_NAME} PARENT_SCOPE)
    endif()

    set(BIFUSD_CONFIG_TARGET ${BIFUSD_PACKAGE_NAME}BifusdConfig)
    include(${BIFUSD_CMAKE_DIR}/version.info)

    set(BIFUSD_MAJOR_VERSION ${BIFROST_USD_PACK_MAJOR_VERSION})
    set(BIFUSD_MINOR_VERSION ${BIFROST_USD_PACK_MINOR_VERSION})
    set(BIFUSD_PATCH_LEVEL   ${BIFROST_USD_PACK_PATCH_LEVEL})
    set(BIFUSD_PRODUCT_NAME  "\"Usd_pack\"")

    #==============================================================================
    # Target/Property for json files
    #==============================================================================
    # This target and property needs to be forward declared before utils.cmake.
    # because bifusd_header_parser refers to them. More deps/properties are added
    # to the target later, including in setup.cmake.
    #
    if( NOT TARGET ${BIFUSD_PACKAGE_NAME}_config_info)
        add_custom_target(${BIFUSD_PACKAGE_NAME}_config_info)
    endif()

    #==============================================================================
    # General utility functions
    #==============================================================================

    # This defines functions and macros that used later on; in particular
    # in setup.cmake and the compiler setup.

    include(${BIFUSD_TOOLS_DIR}/utils.cmake)
    include(${BIFUSD_TOOLS_DIR}/setup.cmake)
    include(${BIFUSD_TOOLS_DIR}/version.cmake)
    include(${BIFUSD_CMAKE_DIR}/setup.cmake)
    include(${BIFUSD_CMAKE_DIR}/utils.cmake)

    set(BIFUSD_USE_PYTHON_VERSION 3 CACHE STRING "The Python version used.")
    include(${BIFUSD_TOOLS_DIR}/python.cmake)
    if(BIFUSD_PYTHON_INCLUDED)
        bifusd_print_python_settings()
    endif()

    include(${BIFUSD_TOOLS_DIR}/package.cmake)

    #==============================================================================
    # TESTING
    #==============================================================================
    # Tests require our own TBBInitGuard that needs to be built with the TBB that
    # is shipped with USD, so we first need to detect what/where is the USD's TBB:
    include(${BIFUSD_TOOLS_DIR}/bifusd_detect_usd_tbb.cmake)
    bifusd_detect_usd_tbb(usd FALSE) # verbose=FALSE

    # Add our TBBInitGuard to properly initialize and terminate TBB's scheduler:
    add_subdirectory(${BIFUSD_TOOLS_DIR}/src/tbb tbb)

    include(${BIFUSD_TOOLS_DIR}/gtest.cmake)

    # Note: Versioning static lib is currently  exported to be able
    # to add_subdirectory( maya_plugin ) directly and link statically
    # with the version info...  This is subject to revision
endif()

#==============================================================================
# Do the maya plugin
#==============================================================================

# Do not skip tests unless demanded
# Some Maya environement may not be complete
# Ex.: On Linux with older Maya's you need
# libXp.so.6 and it may not be installed...
if( NOT DEFINED BIFUSD_MAYA_SKIP_TESTS )
    set(BIFUSD_MAYA_SKIP_TESTS 0 PARENT_SCOPE)
endif()

add_subdirectory(src)
add_subdirectory(test)
