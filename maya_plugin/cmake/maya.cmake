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

# TODO: BIFROST-8046 - Create a module to find Maya.

#------------------------------------------------------------------------------
#
# Creates an SHARED library named Maya.
#
function(bifusd_init_maya)
    bifusd_required_variables(MAYA_RUNTIME_LOCATION)
    message( STATUS "BifrostUSD Maya location:  ${MAYA_RUNTIME_LOCATION}")


    if(BIFUSD_IS_WINDOWS)
        set(maya_shared_lib "${MAYA_RUNTIME_LOCATION}/bin/"
						  "${MAYA_RUNTIME_LOCATION}/bin2/"
						  "${MAYA_RUNTIME_LOCATION}/bin3/")

        set(maya_import_lib "${MAYA_RUNTIME_LOCATION}/lib/"
						  "${MAYA_RUNTIME_LOCATION}/lib2/"
						  "${MAYA_RUNTIME_LOCATION}/lib3/")
    elseif(BIFUSD_IS_OSX)
        set(maya_shared_lib "${MAYA_RUNTIME_LOCATION}/Maya.app/Contents/MacOS/")
    else()
        set(maya_shared_lib "${MAYA_RUNTIME_LOCATION}/lib/"
						  "${MAYA_RUNTIME_LOCATION}/lib2/"
						  "${MAYA_RUNTIME_LOCATION}/lib3/")
    endif()

    # MAYA LOCATION VARS
    set(BIFUSD_MAYA_DEVKIT_INCLUDE_DIRS "${MAYA_RUNTIME_LOCATION}/include")
    set(BIFUSD_MAYA_DEVKIT_INCLUDE_DIRS "${BIFUSD_MAYA_DEVKIT_INCLUDE_DIRS}" PARENT_SCOPE)

    set( BIFUSD_MAYA_SHARED_LIB_DIR "${maya_shared_lib}" PARENT_SCOPE )

    if( BIFUSD_IS_OSX)
        set( BIFUSD_MAYA_OSX_FWK_DIR "${maya_shared_lib}/../Frameworks" PARENT_SCOPE)
    endif()

    # Do not skip tests unless demanded
    # Some Maya environement may not be complete
    # Ex.: On Linux with older Maya's you need
    # libXp.so.6 and it may not be installed...
    if( NOT DEFINED BIFUSD_MAYA_SKIP_TESTS )
        set(BIFUSD_MAYA_SKIP_TESTS 0 PARENT_SCOPE)
    endif()
    
    # Targets
    set(maya_libraries OpenMaya
                       Foundation
                       OpenMayaAnim
                       OpenMayaFX
                       OpenMayaRender
                       OpenMayaUI
                       clew)

    foreach( libname ${maya_libraries} )
        
        set(maya_shared_lib_name "${CMAKE_SHARED_LIBRARY_PREFIX}${libname}${CMAKE_SHARED_LIBRARY_SUFFIX}")
        
        unset( MAYASHAREDLIB_FULLNAME CACHE )
        find_file(
            MAYASHAREDLIB_FULLNAME ${maya_shared_lib_name}
            PATHS ${maya_shared_lib}
            DOC "Path to the maya library to use."
            NO_DEFAULT_PATH)

        if (NOT MAYASHAREDLIB_FULLNAME)
            message(FATAL_ERROR "Can't find maya library ${maya_shared_lib_name} in ${maya_shared_lib}")
        endif()

        # Create the imported library target
        set( target_name Maya::${libname})
        add_library(${target_name} SHARED IMPORTED)
        set_property(TARGET ${target_name} PROPERTY IMPORTED_LOCATION "${MAYASHAREDLIB_FULLNAME}")
        set_property(TARGET ${target_name} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${BIFUSD_MAYA_DEVKIT_INCLUDE_DIRS}")

        if(BIFUSD_IS_WINDOWS)
            set( MAYAIMPORTLIB_NAME "${libname}${CMAKE_IMPORT_LIBRARY_SUFFIX}" )
            
        unset( MAYAIMPORTLIB_FULLNAME CACHE )
            find_file(
                MAYAIMPORTLIB_FULLNAME ${MAYAIMPORTLIB_NAME}
                PATHS ${maya_import_lib}
                DOC "Path to the maya library to use."
                NO_DEFAULT_PATH)
            if (NOT MAYAIMPORTLIB_FULLNAME)
                message(FATAL_ERROR "Can't fin maya library ${MAYAIMPORTLIB_NAME} in ${maya_import_lib}")
            endif()
            
            set_property(TARGET ${target_name} PROPERTY IMPORTED_IMPLIB "${MAYAIMPORTLIB_FULLNAME}")
        endif()

    endforeach()

    if(BIFUSD_IS_WINDOWS)
        set_property(TARGET Maya::clew PROPERTY INTERFACE_LINK_LIBRARIES "opengl32.lib")
    endif()

endfunction(bifusd_init_maya)

# Using a function to scope variables and avoid polluting the global namespace
bifusd_init_maya()
