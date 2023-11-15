#-
#*****************************************************************************
# Copyright 2023 Autodesk, Inc. All rights reserved.
#
# Use of this software is subject to the terms of the Autodesk license
# agreement provided at the time of installation or download, or which
# otherwise accompanies this software in either electronic or hard copy form.
#*****************************************************************************
#+

# FindMaya
#   Find the Maya related libraries and executables
#
# Imported Targets
#   This module provides access to the following:
#      Maya::OpenMaya
#      Maya::Foundation
#
# Result Variable:
#   Bifrost_FOUND

if (NOT DEFINED MAYA_RUNTIME_LOCATION)
    message(FATAL_ERROR "Required variable MAYA_RUNTIME_LOCATION has not been defined.")
endif()


function(maya_skd_init)

    # Library names
    set(maya_libraries
        OpenMaya
        Foundation
        # OpenMayaAnim
        # OpenMayaFX
        # OpenMayaRender
        # OpenMayaUI
    )

    foreach( lib ${maya_libraries})
        list(APPEND all_targets "Maya::${lib}")
    endforeach()

    set(targets_defined)
    set(targets_not_defined)
    foreach( one_target ${all_targets} )
        if(TARGET ${one_target})
            list(APPEND targets_defined ${one_target})
        else()
            list(APPEND targets_not_defined ${one_target})
        endif()
    endforeach()

    # Already all defined - OK
    if("${targets_defined}" STREQUAL "${all_targets}")
        set(Maya_FOUND true PARENT_SCOPE)
        return()
    endif()

    # Some defined some not defined - Error
    if(NOT "${targets_defined}" STREQUAL "")
        set(Maya_FOUND false PARENT_SCOPE)
        message(FATAL_ERROR "Some (but not all) targets in this export set were already defined.\nTargets Defined: ${targets_defined}\nTargets not yet defined: ${targets_not_defined}\n")
        return()
    endif()


    # Nothing defined, then define all targets
    set( maya_inc_hints "${MAYA_RUNTIME_LOCATION}/include")
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(CMAKE_FIND_LIBRARY_PREFIXES "")
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib" )
        set(maya_bin_hints
            "${MAYA_RUNTIME_LOCATION}/bin/"
			"${MAYA_RUNTIME_LOCATION}/bin2/"
			"${MAYA_RUNTIME_LOCATION}/bin3/")
        set(maya_lib_hints
            "${MAYA_RUNTIME_LOCATION}/lib/"
			"${MAYA_RUNTIME_LOCATION}/lib2/"
			"${MAYA_RUNTIME_LOCATION}/lib3/")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".dylib")
        set(maya_lib_hints "${MAYA_RUNTIME_LOCATION}/Maya.app/Contents/MacOS/")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".so")
        set(maya_lib_hints
            "${MAYA_RUNTIME_LOCATION}/lib/"
			"${MAYA_RUNTIME_LOCATION}/lib2/"
			"${MAYA_RUNTIME_LOCATION}/lib3/")
    endif()

    set( Maya_SHARED_LIB_DIRS ${maya_lib_hints})
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(Maya_SHARED_LIB_DIRS ${maya_bin_hints})
    endif()

    if( CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(Maya_OSX_FWK_DIR ${MAYA_RUNTIME_LOCATION}/Maya.app/Contents/Frameworks)
    endif()

    find_path( maya_INCLUDE_DIR "maya/adskCommon.h" REQUIRED NO_DEFAULT_PATH HINTS ${MAYA_RUNTIME_LOCATION}/include)

    set( maya_found_libs )
    foreach( lib ${maya_libraries})
        find_library( ${lib}_LIBRARY
            NAMES ${lib}
            HINTS ${maya_lib_hints}
            NO_DEFAULT_PATH
        )
        list( APPEND maya_found_libs ${lib}_LIBRARY)
    endforeach()

    # Find all the necessary libs and files
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Maya
        "Maya Package not found..."
        maya_INCLUDE_DIR
        ${maya_found_libs}
    )

    if( Maya_FOUND)
        foreach(lib ${maya_libraries})
            set( target_name Maya::${lib})
            add_library(${target_name} UNKNOWN IMPORTED)
            set_property(TARGET ${target_name} PROPERTY IMPORTED_LOCATION "${${lib}_LIBRARY}")
            set_property(TARGET ${target_name} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${maya_INCLUDE_DIR}")
        endforeach()
        set(Maya_OSX_FWK_DIR ${Maya_OSX_FWK_DIR} PARENT_SCOPE)
        set( Maya_SHARED_LIB_DIRS ${Maya_SHARED_LIB_DIRS} PARENT_SCOPE)
    endif()

    set(Maya_FOUND ${Maya_FOUND} PARENT_SCOPE)

endfunction()

# Using a function to scope variables and avoid polluting the global namespace!
maya_skd_init()
