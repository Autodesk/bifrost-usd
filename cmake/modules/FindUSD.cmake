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

#------------------------------------------------------------------------------
#
# Creates an IMPORTED library named USD.
#
# Linking against usd will cause the proper include directories
# to be added to your target.
#
function(init_usd)
    # Setting-up to call find_package

    # Depending how USD CMake files are generated, the path
    # to python may be parameterized.  Ex.: USD coming with
    # Maya USD parameterizes Python...
    set(PYTHON_INCLUDE_DIR "${BIFUSD_PYTHON_INCLUDE_DIRS}")
    set(PYTHON_LIBRARIES "${BIFUSD_PYTHON_LIBRARIES}")

    set(PXR_USD_LOCATION "${USD_LOCATION}")
    set(PXR_USD_LOCATION "${USD_LOCATION}" PARENT_SCOPE)

    find_package(pxr PATHS ${USD_LOCATION} REQUIRED CONFIG NO_DEFAULT_PATH)
    if(pxr_FOUND)
        message( STATUS "-- Found USD: ${USD_LOCATION}")
    endif()

    set( USD_MAJOR_VERSION ${PXR_MAJOR_VERSION} PARENT_SCOPE)
    set( USD_MINOR_VERSION ${PXR_MINOR_VERSION} PARENT_SCOPE)
    set( USD_PATCH_VERSION ${PXR_PATCH_VERSION} PARENT_SCOPE)
    set( USD_VERSION "${PXR_MAJOR_VERSION}.${PXR_MINOR_VERSION}.${PXR_PATCH_VERSION}" PARENT_SCOPE)
endfunction()

# Using a function to scope variables and avoid polluting the global namespace!
init_usd()
