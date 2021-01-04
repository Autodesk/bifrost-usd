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
if(TARGET bifusd_gtest)
    return()
endif()

# Download google test when not provided.
if(NOT BIFUSD_GTEST_LOCATION)
    # Fetch google tests
    # Will go in the build tree...
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )

    # Populate
    FetchContent_Populate( googletest )

    # Where the googletest files can be found
    set( BIFUSD_GTEST_LOCATION ${googletest_SOURCE_DIR}/googletest)
endif()


#==============================================================================
# Google C++ Testing Framework
#==============================================================================

# gtest is composed of 2 simple libraries. In order to simplify our
# build system, we replicate the instructions to build these
# here. That way, we don't have to check-in pre-built binaries for
# these.

function(bifusd_init_gtest)
    # Defining an imported library for gtest headers causes CMake to handle
    # them as system headers and turn-off the reporting of warnings from these
    # include directories.
    add_library(bifusd_gtest_includes INTERFACE IMPORTED)
    set_property(TARGET bifusd_gtest_includes PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 ${BIFUSD_GTEST_LOCATION}/include)

    # Defines the bifusd_gtest & bifusd_gtest_main libraries.  User tests should link
    # with one of them.
    add_library(bifusd_gtest STATIC ${BIFUSD_GTEST_LOCATION}/src/gtest-all.cc)
    target_include_directories(bifusd_gtest PRIVATE ${BIFUSD_GTEST_LOCATION})
    target_link_libraries(bifusd_gtest PUBLIC bifusd_gtest_includes)

    if(BIFUSD_IS_LINUX)
        target_link_libraries(bifusd_gtest PRIVATE pthread)
    endif()

    add_library(bifusd_gtest_main STATIC ${BIFUSD_TOOLS_DIR}/src/gtest/bifusd_gtest_main.cpp)
    target_link_libraries(bifusd_gtest_main PUBLIC bifusd_gtest_includes PRIVATE bifusd_gtest)

    if(BIFUSD_TBB_LOCATION)
        # The client project is using TBB. Ensure that gtest is correctly
        # initializing and terminating the TBB task scheduler.
        target_compile_definitions(
            bifusd_gtest_main PRIVATE BIFUSD_USING_TBB)
        target_link_libraries(
            bifusd_gtest_main PRIVATE ${BIFUSD_TBB_HELPERS_TARGET} BifusdTBB)
        target_include_directories(bifusd_gtest_main PRIVATE ${BIFUSD_OUTPUT_INCLUDE_DIR})
        add_dependencies(bifusd_gtest_main ${BIFUSD_TBB_HELPERS_TARGET})
    endif()

    if(NOT BIFUSD_IS_WINDOWS)
        # Suppress all warnings since this isn't our source code!
        target_compile_options(bifusd_gtest      PRIVATE "-w")
        target_compile_options(bifusd_gtest_main PRIVATE "-w")
    endif()
endfunction(bifusd_init_gtest)

# Using a function to scope variables and avoid poluting the global namespace!
bifusd_init_gtest()
