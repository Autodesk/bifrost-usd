#-
#*****************************************************************************
# Copyright 2022 Autodesk, Inc.
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

if(bifusd_compiler_linux_included)
    message(WARNING "compiler_Linux.cmake included twice!")
    return()
endif()
set(bifusd_compiler_linux_included true)

# This file is actually included multiple times through the
# CMAKE_USER_MAKE_RULES_OVERRIDE mechanism. It is included as part of CMake's
# make processing and again each time try_compile() is invoked. When invoked
# through try_compile(), none of CMake variable are available unless the value
# is explcitly listed below!
#
# See: https://cmake.org/cmake/help/v3.6/variable/CMAKE_TRY_COMPILE_PLATFORM_VARIABLES.html
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
    BIFUSD_TOOLS_DIR
    BIFUSD_EXPORTS_FILE
    BIFUSD_CLANG_COMPILER_BINDIR
    )

# Setup bifusd variables containing information about compiler versions
include(${BIFUSD_TOOLS_DIR}/compiler_config.cmake)

#==============================================================================
# COMPILER CONFIGURATION ON LINUX
#==============================================================================

# include common compiler options
# Since both CMAKE_CXX_COMPILER AND CMAKE_C_COMPILER can be set
# but only one of the _ID variant can be set, check the latter
# to figure out if we're called for the C compiler, or the CXX one.
#
if ( NOT ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "" ))
	include(${BIFUSD_TOOLS_DIR}/compiler_${CMAKE_CXX_COMPILER_ID}.cmake)
elseif ( NOT ( "${CMAKE_C_COMPILER_ID}" STREQUAL "" ))
	include(${BIFUSD_TOOLS_DIR}/compiler_${CMAKE_C_COMPILER_ID}.cmake)
else()
	message(WARNING "compiler_Linux.cmake - Unknown compiler")
	return()
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    get_filename_component(gcc_bin_dir ${CMAKE_LINKER} DIRECTORY) # .../usr/bin
    get_filename_component(gcc_bin_dir ${gcc_bin_dir}  DIRECTORY) # .../usr

    list(APPEND cxx_flags
        -stdlib=libstdc++
        --gcc-toolchain=${gcc_bin_dir}
        )

    function(bifusd_clang_compiler_bindir_init)
        get_filename_component(bindir ${CMAKE_CXX_COMPILER} DIRECTORY)
        set(BIFUSD_CLANG_COMPILER_BINDIR ${bindir} CACHE PATH
            "Path to the 'bin' directory of the clang installation")
    endfunction()
    bifusd_clang_compiler_bindir_init()
endif()

#------------------------------------------------------------------------------
# Shared linker flags
#------------------------------------------------------------------------------

# Internally used shared libraries should avoid STB_GNU_UNIQUE symbols.
set(BIFUSD_INTERNAL_SHARED_LINKER_FLAGS
    -Wl,--no-gnu-unique
    # Configure gcc to use gold instead of the regular ld.
    -fuse-ld=gold
)

set(BIFUSD_EXTERNAL_SHARED_LINKER_FLAGS
    -Wl,--no-gnu-unique
    # Configure gcc to use gold instead of the regular ld.
    -fuse-ld=gold
)

# Optionally, externally available (deliverable) shared libraries should filter
# the exported symbols. These are simply any symbol in the product namespace or
# involving a type in the product namespace.
if (BIFUSD_EXPORTS_FILE)
    list(APPEND BIFUSD_EXTERNAL_SHARED_LINKER_FLAGS
        -Wl,--version-script=${BIFUSD_EXPORTS_FILE})
endif()

string(REPLACE ";" " " BIFUSD_INTERNAL_SHARED_LINKER_FLAGS "${BIFUSD_INTERNAL_SHARED_LINKER_FLAGS}")
string(REPLACE ";" " " BIFUSD_EXTERNAL_SHARED_LINKER_FLAGS "${BIFUSD_EXTERNAL_SHARED_LINKER_FLAGS}")

# Set the default compiler flags
string(REPLACE ";" " " cxx_flags "${cxx_flags}")
set(CMAKE_CXX_FLAGS_INIT "${cxx_flags}")

# Set the default shared linker flags
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${BIFUSD_EXTERNAL_SHARED_LINKER_FLAGS}")

#------------------------------------------------------------------------------
# Compile definitions that are not stored in CMake cache variables.
#------------------------------------------------------------------------------

add_definitions(
    -D__STDC_CONSTANT_MACROS
    -D__STDC_FORMAT_MACROS
    -D__STDC_LIMIT_MACROS
)
