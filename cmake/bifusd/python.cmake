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

if(BIFUSD_PYTHON_INCLUDED)
  return()
endif()

if( NOT BIFUSD_USE_PYTHON_VERSION)
    message( FATAL_ERROR "BIFUSD_USE_PYTHON_VERSION must be set to 2 or 3.")
endif()

set(BIFUSD_PYTHON_INCLUDED TRUE)

set( BIFUSD_HAS_PYTHON OFF)

set( PythonVersion Python${BIFUSD_USE_PYTHON_VERSION})

# Find Python
# On Linux ensure that /usr/lib64 is searched
set(need_lib64_on_linux FALSE)
set(usr_lib64 /usr/lib64 )
if(BIFUSD_IS_LINUX AND EXISTS ${usr_lib64})
    set(need_lib64_on_linux TRUE)
endif()

if(need_lib64_on_linux)
    set(OLD_CMAKE_LIBRARY_PATH  ${CMAKE_LIBRARY_PATH})
    list(APPEND CMAKE_LIBRARY_PATH ${usr_lib64})
endif()

find_package(${PythonVersion} )

if(need_lib64_on_linux)
    set(CMAKE_LIBRARY_PATH ${OLD_CMAKE_LIBRARY_PATH})
endif()

unset(need_lib64_on_linux)
unset(usr_lib64)

if(NOT ${PythonVersion}_FOUND)
  message(FATAL_ERROR "Could not find ${PythonVersion}... Please make sure ${PythonVersion} is available.")
endif()

# CMake 3.20.3 issue with find_package(Python3) when cpython is found..
#
# Context: Python3_LIBRARIES is used later in an INTERFACE_LINK_LIBRARIES 
# property and CMake failed... 
#
# CMake Error: Property INTERFACE_LINK_LIBRARIES may not contain link-type 
# keyword "optimized".  
#
# Fix: filter-out the correct python libraries when the "optimized or debug"
# keywords are present.
if (BIFUSD_IS_WINDOWS AND BIFUSD_USE_PYTHON_VERSION EQUAL 3)
  if (BIFUSD_IS_DEBUG)
    string(REGEX MATCHALL "debug;[^;]+" Python3_matches_LIBRARIES "${Python3_LIBRARIES}")
    if( ${Python3_matches_LIBRARIES})
        string(REGEX REPLACE "debug;" "" Python3_LIBRARIES "${Python3_matches_LIBRARIES}")
    endif()
  else()
    string(REGEX MATCHALL "optimized;[^;]+" Python3_matches_LIBRARIES "${Python3_LIBRARIES}")
    if( ${Python3_matches_LIBRARIES})
        string(REGEX REPLACE "optimized;" "" Python3_LIBRARIES "${Python3_matches_LIBRARIES}")
    endif()
  endif()
endif()

set(BIFUSD_PYTHON_VERSION       "${${PythonVersion}_VERSION}")
set(BIFUSD_PYTHON_VERSION_MAJOR "${${PythonVersion}_VERSION_MAJOR}")
set(BIFUSD_PYTHON_VERSION_MINOR "${${PythonVersion}_VERSION_MINOR}")
set(BIFUSD_PYTHON_VERSION_PATCH "${${PythonVersion}_VERSION_PATCH}")
set(BIFUSD_PYTHON_EXECUTABLE    "${${PythonVersion}_EXECUTABLE}")
set(BIFUSD_PYTHON_INCLUDE_DIRS  "${${PythonVersion}_INCLUDE_DIRS}")
set(BIFUSD_PYTHON_LIBRARIES     "${${PythonVersion}_LIBRARIES}")
set(BIFUSD_PYTHON_LIBRARY_DIRS  "${${PythonVersion}_LIBRARY_DIRS}")

set(BIFUSD_HAS_PYTHON ON)

function(bifusd_print_python_settings)
    if (BIFUSD_HAS_PYTHON)
        message(STATUS "Using Python ${BIFUSD_PYTHON_VERSION_MAJOR}")
        message(STATUS "    version:    ${BIFUSD_PYTHON_VERSION}")
        message(STATUS "    executable: ${BIFUSD_PYTHON_EXECUTABLE}")
        message(STATUS "    includes:   ${BIFUSD_PYTHON_INCLUDE_DIRS}")
        message(STATUS "    libs:       ${BIFUSD_PYTHON_LIBRARIES}")
        message(STATUS "    lib dirs:   ${BIFUSD_PYTHON_LIBRARY_DIRS}")
    else()
        message(STATUS "No Python version setup")
    endif()
endfunction()
