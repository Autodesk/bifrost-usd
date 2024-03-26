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

if(bifusd_compiler_gnu_included)
    message(WARNING "compiler_GNU.cmake included twice!")
    return()
endif()
set(bifusd_compiler_gnu_included true)

# Since both CMAKE_CXX_COMPILER AND CMAKE_C_COMPILER can be set
# but only one of the _ID variant can be set, check the latter
# to figure out if we're called for the C compiler, or the CXX one.
#
if ( NOT ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "" ))
    if (NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU") )
        message(FATAL_ERROR "compiler_GNU.cmake included for wrong C++ compiler!")
    endif()
elseif ( NOT ( "${CMAKE_C_COMPILER_ID}" STREQUAL "" ))
    if (NOT (CMAKE_C_COMPILER_ID STREQUAL "GNU") )
        message(FATAL_ERROR "compiler_GNU.cmake included for wrong C compiler!")
    endif()
else()
    message(FATAL_ERROR "compiler_GNU.cmake included for wrong compiler!")
endif()


#------------------------------------------------------------------------------
# Default values for CMake cache variables that can be overriden using
# an equivalent variable with an _INIT suffix. See the documentation
# for CMAKE_USER_MAKE_RULES_OVERRIDE.
# ------------------------------------------------------------------------------

# We want squeaky clean builds, but warning as error flags should never be put in
# CMAKE_CXX_FLAGS_INIT since they will interfere with feature detection. They will
# instead be added to both CMAKE_CXX_FLAGS_DEBUG_INIT and its RELEASE
# counterpart.
set(cxx_warning_flags
    -pedantic
    -Wall
    -W
    -Werror
    -Wwrite-strings
    -Wmissing-field-initializers
    -Wno-long-long
    -Wnon-virtual-dtor
    -Wno-unused-local-typedefs
)

if (BIFUSD_IGNORE_DEPRECATED_DECLARATIONS)
    list(APPEND cxx_warning_flags -Wno-deprecated)
else()
    # Warn about deprecated functions
    list(APPEND cxx_warning_flags -Wno-error=deprecated-declarations)
endif()


# Use -O3 instead of -O2 to match the release build default.
if (NOT "${BIFUSD_OSX_BINARY_ARCH}" STREQUAL "arm64")
    set(x86_64_only_options "-msse4.2")
endif()
set(cxx_flags_release
    -g
    -O3
    ${x86_64_only_options}
    ${cxx_warning_flags}
)

# On OSX, we pass this only if we are building for X64.
# on Other platforms we always generate for westmere.
if ((NOT BIFUSD_IS_OSX) OR ("${BIFUSD_OSX_BINARY_ARCH}" STREQUAL "x64"))
     list(APPEND cxx_flags_release -march=westmere)
endif()

string(REPLACE ";" " " cxx_flags_relwithasserts "${cxx_flags_release}")
set(CMAKE_CXX_FLAGS_RELWITHASSERTS_INIT "${cxx_flags_relwithasserts}")

list(APPEND cxx_flags_release -DNDEBUG)
string(REPLACE ";" " " cxx_flags_release "${cxx_flags_release}")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "${cxx_flags_release}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "${cxx_flags_release}")

# In debug:
#   - Generates traps for signed overflow on addition, subtraction,
#     multiplication operations.
#   - Generate code to verify that you do not go beyond the boundary of the
#     stack.
set(cxx_flags_debug
    -g
    -O0
    -ftrapv
    -fstack-check
    ${cxx_warning_flags}
)
string(REPLACE ";" " " cxx_flags_debug "${cxx_flags_debug}")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "${cxx_flags_debug}")

#
# NOTE: We explicitly disable C99 strict-aliasing
# (-fno-strict-aliasing).
#
# By default, GCC enables C99 strict-aliasing rules when compiling
# with -O3. But, these rules are very dangerous. They turn valid C++11
# uses of placement new into code that has an undefined behavior.
#
#     See:
#     http://blog.qt.digia.com/blog/2011/06/10/type-punning-and-strict-aliasing
#     http://cellperformance.beyond3d.com/articles/2006/06/understanding-strict-aliasing.html
#     Etc.
#
# We thus prefer to disable this optimization. Optimizations based on
# strict aliasing are useful in particular numerical type of
# computations. But, in other general context, there is generally no
# gain. In summary, we feel that using strict aliasing rules should be a
# conscious decision from the programmer. Not something that should be
# turned-on by default. Moreover, in most situations, proper usage of
# the C99 restrict keyword is a more appropriate choice for enabling
# optimization based on specifying that pointers are not aliasing.

# As of GCC4.8, the behavior of -Wall has changed and now includes the new warning flag
# -Wunused-local-typedefs. This stops the build due to the -Werror flag. In order to,
# resolve this issue, we use the -Wno-unused-local-typedefs compile flag to ignore the
# warning.


if ( NOT ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "" ))
	get_filename_component(gcc_bin_dir ${CMAKE_CXX_COMPILER} DIRECTORY) # .../usr/bin
elseif ( NOT ( "${CMAKE_C_COMPILER_ID}" STREQUAL "" ))
	get_filename_component(gcc_bin_dir ${CMAKE_C_COMPILER} DIRECTORY) # .../usr/bin
else()
	message(FATAL_ERROR "compiler_GNU.cmake - Unknown compiler")
	return()
endif()

set( cxx_flags
    -fPIC
    -fno-strict-aliasing

    -B${gcc_bin_dir}
)
