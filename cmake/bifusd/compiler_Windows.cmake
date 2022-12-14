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

if(bifusd_compiler_windows_included)
    return()
endif()
set(bifusd_compiler_windows_included true)

# This file is actually included multiple times through the
# CMAKE_USER_MAKE_RULES_OVERRIDE mechanism. It is included as part of CMake's
# make processing and again each time try_compile() is invoked. When invoked
# through try_compile(), none of CMake variable are available unless the value
# is explcitly listed below!
#
# See: https://cmake.org/cmake/help/v3.6/variable/CMAKE_TRY_COMPILE_PLATFORM_VARIABLES.html
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
    BIFUSD_TOOLS_DIR
    BIFUSD_IS_WINDOWS_7
    )

# Setup bifusd variables containing information about compiler versions
include(${BIFUSD_TOOLS_DIR}/compiler_config.cmake)

#==============================================================================
# COMPILER CONFIGURATION ON WINDOWS
#==============================================================================

# Use /Ob2 instead of /Ob1 to match the release build default.

# Note regarding SSE4.2 instructions. It seems that the auto-vectorizer and the
# SSE4.2 instructions are always on in MSVC 2015. There isn't any command-line
# options to enable or disable the vectorizer, only a per-loop pragma exists.
#
#     See https://msdn.microsoft.com/en-us/library/hh872235.aspx:
#
#     The Auto-Vectorizer analyzes loops in your code, and uses the vector
#     registers and instructions on the target computer to execute them, if it
#     can. This can improve the performance of your code. The compiler targets
#     the SSE2, AVX, and AVX2 instructions in Intel or AMD processors, or the
#     NEON instructions on ARM processors, according to the /arch switch.
#
#     The Auto-Vectorizer may generate different instructions than specified by
#     the /arch switch. These instructions are guarded by a runtime check to
#     make sure that code still runs correctly. For example, when you compile
#     /arch:SSE2, SSE4.2 instructions may be emitted. A runtime check verifies
#     that SSE4.2 is available on the target processor and jumps to a
#     non-SSE4.2 version of the loop if the processor does not support those
#     instructions.
#
#     By default, the Auto-Vectorizer is enabled. If you want to compare the
#     performance of your code under vectorization, you can use #pragma
#     loop(no_vector) to disable vectorization of any given loop.
#
# In addition, on x64, the minimum supported architecture is SSE4.2. The /arch
# switch can only be used to enable AVX and AVX2!
#
#     See: https://msdn.microsoft.com/en-us/library/jj620901.aspx
set(cxx_flags_release /MD /Zi /O2 /Ob2)

string(REPLACE ";" " " cxx_flags_relwithasserts "${cxx_flags_release}")
set(CMAKE_CXX_FLAGS_RELWITHASSERTS_INIT "${cxx_flags_relwithasserts}")

list(APPEND cxx_flags_release "/D NDEBUG")
string(REPLACE ";" " " cxx_flags_release "${cxx_flags_release}")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "${cxx_flags_release}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "${cxx_flags_release}")


#------------------------------------------------------------------------------
# Default values for CMake cache variables that can be overriden using
# an equivalent variable with an _INIT suffix. See the documentation
# for CMAKE_USER_MAKE_RULES_OVERRIDE.
# ------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# The fastlink option produces PDB files that are not exportable unless
# run thru mspdbcmf.exe to move the debug info into a self contained
# PDB file.
#
# WARNINGS:
#   1- The PDB files generated by mspdbcmf.exe are incomplete. Use
#       the dia2dump utility to check if later versions are fixed
#       /Microsoft Visual Studio 14.0/DIA SDK/Samples/DIA2Dump
#   2 - Visual Studio 2017 is said to use fastlink by default. Need to
#        find out if the switch needs to be flipped to off on build machines.
#------------------------------------------------------------------------------
set(BIFUSD_USE_DEBUG_FASTLINK 0
    CACHE BOOL "Enable fast linking of debug information by leaving debug info in .obj files and building an index-only .pdb file.")
if (${BIFUSD_USE_DEBUG_FASTLINK})
foreach(bin_type EXE SHARED MODULE)
foreach(bin_config DEBUG RELWITHDEBINFO)
    string(REPLACE "/debug " "/debug:fastlink "
           CMAKE_${bin_type}_LINKER_FLAGS_${bin_config}_INIT
           "${CMAKE_${bin_type}_LINKER_FLAGS_${bin_config}_INIT}")
endforeach()
endforeach()
endif()

# Disable linker warning LNK4221:
#     This object file does not define any previously undefined public symbols,
#     so it will not be used by any link operation that consumes this library.
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} /ignore:4221")
set(CMAKE_STATIC_LINKER_FLAGS_INIT "${CMAKE_STATIC_LINKER_FLAGS_INIT} /ignore:4221")

# Treat warnings as errors
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} /WX")
set(CMAKE_STATIC_LINKER_FLAGS_INIT "${CMAKE_STATIC_LINKER_FLAGS_INIT} /WX")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} /WX")

# /GR is redundant because RTTI is on by default. At times, we need to disable
# RTTI. We remove the /GR flags to avoid the MSVC warning complaining that both
# /GR and /GR- was specified.
string(REGEX REPLACE "/GR " "" CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT}")

# Disable runtime checks for performance reasons (build and/or execution).
string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_DEBUG_INIT "${CMAKE_CXX_FLAGS_DEBUG_INIT}")

# Adding the /bigobj compilation flag to avoid the following error:
#
#   fatal error C1128: number of sections exceeded object file format limit: compile with /bigobj
#
# Apparently, the only drawback is loosing backward compatibility with pre-2005
# linkers which we absolutely don't care about. For more information see:
#
#   https://msdn.microsoft.com/en-us/library/8578y171.aspx
#   http://stackoverflow.com/questions/15110580/penalty-of-the-msvs-compiler-flag-bigobj
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} /bigobj")

# Suppress copyright messages when compiling resources
set(CMAKE_RC_FLAGS "/nologo")

#------------------------------------------------------------------------------
# Shared linker flags
#------------------------------------------------------------------------------

# No special flags for internally used or externally available (deliverable) shared libraries.
set(BIFUSD_INTERNAL_SHARED_LINKER_FLAGS "")
set(BIFUSD_EXTERNAL_SHARED_LINKER_FLAGS "")

string(REPLACE ";" " " BIFUSD_INTERNAL_SHARED_LINKER_FLAGS "${BIFUSD_INTERNAL_SHARED_LINKER_FLAGS}")
string(REPLACE ";" " " BIFUSD_EXTERNAL_SHARED_LINKER_FLAGS "${BIFUSD_EXTERNAL_SHARED_LINKER_FLAGS}")


#------------------------------------------------------------------------------
# Compile definitions that are not stored in CMake cache variables.
#------------------------------------------------------------------------------

if(BIFUSD_IS_DEBUG)
    # For all versions of Visual Studio, the default value is 2 in debug.
    # However we do not want our clients to suffer from this issue:
    # https://ofekshilon.com/2014/10/14/accelerating-debug-runs-part-2-_iterator_debug_level/

    # This issue was fixed in Visual Studio 2015 Update 3:
    # https://blogs.msdn.microsoft.com/vcblog/2016/08/12/stl-fixes-in-vs-2015-update-3/

    # Bifusd libraries are currently compiled with debug level 1, so use the same
    # debug level by default.
    set(STL_DEBUG_LEVEL "1")
else()
    set(STL_DEBUG_LEVEL "0")
endif()

if (BIFUSD_USE_STL_DEBUG_LEVEL_OVERRIDE)
    # The default iterator debug level makes sure any Windows Debug build is
    # unusable as soon as container sizes are greater than 100 items.
    #
    # To give you an idea of the checks
    #
    # LEVEL 1:
    #  - Every time an iterator is accessed or incremented there will be a
    #     check that the iterator is valid, in a valid container, inside the
    #     container bounds.
    # LEVEL 2:
    # - For STL maps, the predicate is tested so that Pred(a,b) == !Pred(b,a)
    #     every time the predicate is used.
    #
    #  At level 0 no tests are done, and STL containers will not be
    #     compatible with level 1 and 2 since the debug structures are left out.
    #  At level 1 the code is usable.
    #  At level 2 (the default in Debug on Windows) be ready for severe slowdowns.
    #
    # You can use code compiled at level 1 in a program compiled at level 2. The
    # data structures will have the same size.
    set(STL_DEBUG_LEVEL "${BIFUSD_STL_DEBUG_LEVEL}")
endif()

add_definitions(
    -D_VARIADIC_MAX=10 # mainly for gtest tuples
    -DNOMINMAX=1       # To avoid clashes with Windef.h macros and STL/LLVM

    # Avoid MSVC complaining about construct that it can't prove
    # whether they are safe or not. For example, it complains about any
    # use of std::copy on raw pointers.
    #
    # This causes major trouble when using LLVM as llvm::SmallVector
    # uses std::copy in its implementation.
    #
    # Moreover, there does not seem to be any mean of selectively
    # shutting up these warning for particular uage sites.
    -D_SCL_SECURE_NO_WARNINGS

    -D_ITERATOR_DEBUG_LEVEL=${STL_DEBUG_LEVEL}
)

# Define the flag identifying the Windows version
if (BIFUSD_IS_WINDOWS_7)
  add_definitions(-DBIFUSD_WINDOWS_7)
elseif (BIFUSD_IS_WINDOWS_8)
  add_definitions(-DBIFUSD_WINDOWS_8)
endif()
