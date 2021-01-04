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

if(bifusd_final_compiler_setup_included)
    message(WARNING "final_compiler_setup.cmake included twice!")
    return()
endif()
set(bifusd_final_compiler_setup_included true)

# Initialize variables related the release with assertions builds.
set(CMAKE_CXX_FLAGS_RELWITHASSERTS ${CMAKE_CXX_FLAGS_RELWITHASSERTS_INIT}
    CACHE STRING "Flags used by the compiler during RelWithAsserts builds.")
set(CMAKE_EXE_LINKER_FLAGS_RELWITHASSERTS ${CMAKE_EXE_LINKER_FLAGS_RELWITHASSERTS_INIT}
    CACHE STRING "Flags used by the linker during RelWithAsserts builds.")
set(CMAKE_MODULE_LINKER_FLAGS_RELWITHASSERTS ${CMAKE_MODULE_LINKER_FLAGS_RELWITHASSERTS_INIT}
    CACHE STRING "Flags used by the linker during RelWithAsserts builds.")
set(CMAKE_SHARED_LINKER_FLAGS_RELWITHASSERTS ${CMAKE_SHARED_LINKER_FLAGS_RELWITHASSERTS_INIT}
    CACHE STRING "Flags used by the linker during RelWithAsserts builds.")
set(CMAKE_STATIC_LINKER_FLAGS_RELWITHASSERTS ${CMAKE_STATIC_LINKER_FLAGS_RELWITHASSERTS_INIT}
    CACHE STRING "Flags used by the linker during RelWithAsserts builds.")
mark_as_advanced(
    CMAKE_CXX_FLAGS_RELWITHASSERTS
    CMAKE_EXE_LINKER_FLAGS_RELWITHASSERTS
    CMAKE_MODULE_LINKER_FLAGS_RELWITHASSERTS
    CMAKE_STATIC_LINKER_FLAGS_RELWITHASSERTS
    CMAKE_SHARED_LINKER_FLAGS_RELWITHASSERTS
    )

if(BIFUSD_IS_LINUX)
    # When linking executables or creating shared libraries, force the libraries
    # being linked to the executable or shared library to be treated as a group,
    # therefore making them order independent. This mimics, to some extend, the
    # linking behaviour of MSVC and Clang.
    string(REPLACE "<LINK_LIBRARIES>"
        "-Wl,--start-group <LINK_LIBRARIES> -Wl,--end-group"
        CMAKE_CXX_LINK_EXECUTABLE
        "${CMAKE_CXX_LINK_EXECUTABLE}")

    string(REPLACE "<LINK_LIBRARIES>"
        "-Wl,--start-group <LINK_LIBRARIES> -Wl,--end-group"
        CMAKE_CXX_CREATE_SHARED_LIBRARY
        "${CMAKE_CXX_CREATE_SHARED_LIBRARY}")
endif()
