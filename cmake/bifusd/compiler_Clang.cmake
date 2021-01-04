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

if(bifusd_compiler_clang_included)
    message(WARNING "compiler_Clang.cmake included twice!")
    return()
endif()
set(bifusd_compiler_clang_included true)

if ( NOT ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "" ))
    if (NOT ((CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang") OR
             (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")))
        message(FATAL_ERROR "compiler_Clang.cmake included for wrong C++ compiler!")
    endif()
elseif ( NOT ( "${CMAKE_C_COMPILER_ID}" STREQUAL "" ))
    if (NOT ((CMAKE_C_COMPILER_ID STREQUAL "AppleClang") OR
             (CMAKE_C_COMPILER_ID STREQUAL "Clang")))
        message(FATAL_ERROR "compiler_Clang.cmake included for wrong C compiler!")
    endif()
else()
    message(FATAL_ERROR "compiler_Clang.cmake included for wrong compiler!")
endif()

#------------------------------------------------------------------------------
# Default values for CMake cache variables that can be overridden using
# an equivalent variable with an _INIT suffix. See the documentation
# for CMAKE_USER_MAKE_RULES_OVERRIDE.
# ------------------------------------------------------------------------------

# We want squeaky clean builds, but warning as error flags should never be put in
# CMAKE_CXX_FLAGS_INIT since they will interfere with feature detection. They will
# instead be added to both CMAKE_CXX_FLAGS_DEBUG_INIT and its RELEASE
# counterpart.
set(cxx_warning_flags
    # Enable every single possible compiler warning by default, and trigger a
    # compilation error unless explicitly white listed below... This strategy
    # will ensure that warnings available in future version of the Clang
    # compiler will be automatically enabled for us!
    -Weverything -Werror

    # Allow C99 extensions. Necessary for hexadecimal floats among other
    # things...
    -Wno-c99-extensions

    # We don't care about C++98 compatibility.
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic

    #############################################
    # We will likely never be interested in addressing the following warnings.

    # Turn-off: 10 enumeration values not explicitly handled in switch:
    # 'eBuiltInTypeSymbolKind', 'eConnectionSymbolKind',
    # 'eStructMemberSymbolKind'...
    #
    # The only difference between -Wswitch and this option is that this option
    # gives a warning about an omitted enumeration code even if there is a
    # default label!
    -Wno-switch-enum

    # Turn off: declaration requires a global destructor
    -Wno-global-constructors

    # Turn off: declaration requires an exit-time destructor
    -Wno-exit-time-destructors

    # Turn off: padding class 'Amino::BaRuntimeLog' with 7 bytes to align 'm_messages'
    -Wno-padded

    # Turn off: function 'foo' could be declared with attribute 'noreturn'
    -Wno-missing-noreturn

    # Clang isn't aware of all newest Doxygen commands!
    -Wno-documentation-unknown-command

    # Turn off: instantiation of function 'X' required here, but no definition
    #           is available
    #           note: add an explicit instantiation declaration to suppress
    #           this warning if 'X' is explicitly instantiated in another
    #           translation unit
    -Wno-undefined-func-template

    #############################################
    # FIXME: The following warnings have been disabled because the Amino code
    # base triggers many of them. The plan is to re-enable them one by one...

    # Turn off: implicit conversion changes signedness: 'size_t' (aka 'unsigned
    # long') to 'difference_type' (aka 'long')
    -Wno-sign-conversion

    # Turn off: explicit template instantiation 'DmBuiltInType<bool>' will emit
    # a vtable in every translation unit
    -Wno-weak-template-vtables

    # Clang is even more pedantic than Doxygen is! Turn-off for now...
    -Wno-documentation

    #############################################
)

# Add default flags for Clang 5 or later
if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "5.0.0")
    list(APPEND cxx_warning_flags
        # Turn off: zero as null pointer constant
        -Wno-zero-as-null-pointer-constant
    )
endif()

# Add default flags for Clang 8
if (CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "8.0.0")
    list(APPEND cxx_warning_flags
         # Clang8 introduces this warning which is actually a suggestion for older compilers
         -Wno-return-std-move-in-c++11
         # we are still not aiming for c++ 2a compliance
	 -Wno-c++2a-compat
    )
endif()

if(BIFUSD_SUPPRESS_WARNINGS_FOR_MOC)
    # Add default flags for Clang 8
    if (CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "8.0.0")
       list(APPEND cxx_warning_flags
	 -Wno-shadow -Wno-redundant-parens -Wno-extra-semi-stmt
       )
    endif()
endif()

if (BIFUSD_IGNORE_DEPRECATED_DECLARATIONS)
    list(APPEND cxx_warning_flags -Wno-deprecated)
else()
    # Warn about deprecated functions
    list(APPEND cxx_warning_flags -Wno-error=deprecated-declarations)
endif()

# Use -O3 instead of -O2 to match the release build default.
set(cxx_flags_release
    -g
    -O3
    -msse4.2
    ${cxx_warning_flags}
)

# Setting target architecture on non Universal Binary 2 build.
if (NOT BIFUSD_OSX_BUILD_UB2)
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
#   - Force the usage of stack protectors for all functions
set(cxx_flags_debug
    -g
    -O0
    -ftrapv
    -fstack-protector-all
    ${cxx_warning_flags}
)
string(REPLACE ";" " " cxx_flags_debug "${cxx_flags_debug}")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "${cxx_flags_debug}")

# compiler flags
set(cxx_flags -std=c++14 -fPIC)
