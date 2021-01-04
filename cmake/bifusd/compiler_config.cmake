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

if(bifusd_compiler_config_included)
    message(WARNING "compiler_config.cmake included twice!")
    return()
endif()
set(bifusd_compiler_config_included true)

#==============================================================================
# Configuration common to all compilers
#==============================================================================

set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

#==============================================================================
# Compiler versions
#==============================================================================

set(BIFUSD_IS_GCC 0)
set(BIFUSD_IS_GCC_48 0)
set(BIFUSD_IS_GCC_52 0)
set(BIFUSD_IS_GCC_61 0)
set(BIFUSD_IS_GCC_62 0)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(BIFUSD_IS_GCC 1)

    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
        OUTPUT_VARIABLE gcc_version)
    if (gcc_version VERSION_GREATER 6.2 OR gcc_version VERSION_EQUAL 6.2)
        set(BIFUSD_IS_GCC_62 1)
    elseif (gcc_version VERSION_GREATER 6.1 OR gcc_version VERSION_EQUAL 6.1)
        set(BIFUSD_IS_GCC_61 1)
    elseif (gcc_version VERSION_GREATER 5.2 OR gcc_version VERSION_EQUAL 5.2)
        set(BIFUSD_IS_GCC_52 1)
    elseif (gcc_version VERSION_GREATER 4.8 OR gcc_version VERSION_EQUAL 4.8)
        set(BIFUSD_IS_GCC_48 1)
    endif()
endif()


set(BIFUSD_IS_CLANG 0)
if ((CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang") OR
        (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    set(BIFUSD_IS_CLANG 1)
endif()


set(BIFUSD_IS_MSC 0)
set(BIFUSD_IS_MSVC_2012 0)
set(BIFUSD_IS_MSVC_2013 0)
set(BIFUSD_IS_MSVC_2015 0)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(BIFUSD_IS_MSC 1)

    if (MSVC_VERSION STRLESS "1800")
        # Consider any version prior to MSVC 2013 as MSVC 2012
        set(BIFUSD_IS_MSVC_2012 1)
    elseif (MSVC_VERSION STRLESS "1900")
        # MSVC version in the range [1800, 1900) is considered
        # as MSVC 2013
        set(BIFUSD_IS_MSVC_2013 1)
    else()
        # If the MSVC version is larger or equal to 1900,
        # consider as MSVC 2015
        set(BIFUSD_IS_MSVC_2015 1)
    endif()
endif()
