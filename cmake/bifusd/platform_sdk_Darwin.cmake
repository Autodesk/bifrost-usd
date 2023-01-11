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
#
# NOTE: This file will be called before the language is enabled.
# Either before project() or enable_language().
# In Bifusd Cmake utils we call it before enable_language() be used.

# Check languages are not not enabled...
get_property(languages GLOBAL PROPERTY ENABLED_LANGUAGES)
# Remove the NONE case
list(REMOVE_ITEM languages NONE )
list( LENGTH languages nb_languages)
if( nb_languages )
    message( FATAL_ERROR "Darwin Platform SDK must be set before language. Language is set calling CMAKE project() or enable_language() ")
endif()

# Setup OSX specific flags.
# If binary arch not set explicitaly use same as system
if (NOT BIFUSD_OSX_BINARY_ARCH)
    if ("${CMAKE_HOST_SYSTEM_PROCESSOR}" STREQUAL "arm64")
        set(BIFUSD_OSX_BINARY_ARCH "arm64")
    else()
        set(BIFUSD_OSX_BINARY_ARCH "x64")
    endif()
endif()

if ("${BIFUSD_OSX_BINARY_ARCH}" STREQUAL "ub2")
    bifusd_reset_cache_entry(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
elseif ("${BIFUSD_OSX_BINARY_ARCH}" STREQUAL "x64")
    bifusd_reset_cache_entry(CMAKE_OSX_ARCHITECTURES "x86_64")
elseif ("${BIFUSD_OSX_BINARY_ARCH}" STREQUAL "arm64")
    bifusd_reset_cache_entry(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    message(FATAL_ERROR "Binary architecture ${BIFUSD_OSX_BINARY_ARCH} not supported.")
endif()

# Set target min OS - will be adjusted.
if( NOT BIFUSD_OSX_MIN_OS)
    set(osx_min_os "11.0")
else()
    set(osx_min_os ${BIFUSD_OSX_MIN_OS})
endif()

if ( NOT BIFUSD_OSX_ACTIVE_SDK )
    execute_process(COMMAND "xcrun" "--sdk" "macosx" "--show-sdk-path"
            OUTPUT_VARIABLE sdk_location
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE retcode
            ERROR_QUIET
            )
    if( NOT "${retcode}" STREQUAL "0" )
        message(FATAL_ERROR "Could not find OSX active SDK.")
    endif()

    execute_process(COMMAND "xcrun" "--sdk" "macosx" "--show-sdk-version"
            OUTPUT_VARIABLE ACTIVE_SDK
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE retcode
            ERROR_QUIET
            )
    if( NOT "${retcode}" STREQUAL "0" )
        message(FATAL_ERROR "Could not find OSX active SDK version.")
    endif()

    if( osx_min_os GREATER ACTIVE_SDK )
        set(osx_min_os ${ACTIVE_SDK})
        message( STATUS "Resetting min os to active SDK ${ACTIVE_SDK}")
    endif()
    bifusd_reset_cache_entry(CMAKE_OSX_SYSROOT ${sdk_location})
else()
    execute_process(COMMAND "xcrun" "--sdk" "macosx${BIFUSD_OSX_ACTIVE_SDK}" "--show-sdk-path"
            OUTPUT_VARIABLE sdk_location
            RESULT_VARIABLE retcode
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            )

    if("${retcode}" STREQUAL "0")
        bifusd_reset_cache_entry(CMAKE_OSX_SYSROOT ${sdk_location})
        message( STATUS "Setting OSX active SDK: ${BIFUSD_OSX_ACTIVE_SDK}")
    else()
        message(FATAL_ERROR "Could not find OSX base SDK 'macosx${BIFUSD_OSX_ACTIVE_SDK}'")
    endif()

    if( osx_min_os GREATER BIFUSD_OSX_ACTIVE_SDK )
        set(osx_min_os ${BIFUSD_OSX_ACTIVE_SDK})
        message( STATUS "Resetting min os to desired SDK ${BIFUSD_OSX_ACTIVE_SDK}")

    endif()

endif()

# Deployment set according to SDK found above
bifusd_reset_cache_entry(CMAKE_OSX_DEPLOYMENT_TARGET ${osx_min_os})
