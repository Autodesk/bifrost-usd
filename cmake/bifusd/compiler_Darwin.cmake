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

if(bifusd_compiler_darwin_included)
    message(WARNING "compiler_Darwin.cmake included twice!")
    return()
endif()
set(bifusd_compiler_darwin_included true)

# This file is actually included multiple times through the
# CMAKE_USER_MAKE_RULES_OVERRIDE mechanism. It is included as part of CMake's
# make processing and again each time try_compile() is invoked. When invoked
# through try_compile(), none of CMake variable are available unless the value
# is explcitly listed below!
#
# See: https://cmake.org/cmake/help/v3.6/variable/CMAKE_TRY_COMPILE_PLATFORM_VARIABLES.html
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
    BIFUSD_TOOLS_DIR
    BIFUSD_SYMBOLS_FILE
    BIFUSD_CLANG_COMPILER_BINDIR
    )

# Setup bifusd variables containing information about compiler versions
include(${BIFUSD_TOOLS_DIR}/compiler_config.cmake)

#------------------------------------------------------------------------------
# Configure the compiler setting the first time cmake/ccmake is ran. This
# allows the user to modify them afterward.
#------------------------------------------------------------------------------

if (${BIFUSD_COMPILER_CONFIGURED})
  # empty
else()
  set(BIFUSD_COMPILER_CONFIGURED 1
    CACHE BOOL "Set to false to reset the compiler to default settings." FORCE)

  # For bifusd_reset_cache_entry. This might seem like a redundant entry but it
  # necessary to when compiler_Darwin.cmake is used by try_compile().
  include(${BIFUSD_TOOLS_DIR}/utils.cmake)

  function(bifusd_clang_compiler_bindir_init)
	  # Since both CMAKE_CXX_COMPILER AND CMAKE_C_COMPILER can be set
	  # but only one of the _ID variant can be set, check the latter
	  # to get the correct compiler path.
	  #
      if ( NOT ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "" ))
          get_filename_component(bindir ${CMAKE_CXX_COMPILER} DIRECTORY)
      elseif ( NOT ( "${CMAKE_C_COMPILER_ID}" STREQUAL "" ))
          get_filename_component(bindir ${CMAKE_C_COMPILER} DIRECTORY)
      else()
          message(WARNING "compiler_Darwin.cmake - Unknown compiler")
		  return()
      endif()
      set(BIFUSD_CLANG_COMPILER_BINDIR ${bindir} CACHE PATH
          "Path to the 'bin' directory of the clang installation")
  endfunction()
  bifusd_clang_compiler_bindir_init()

  function(bifusd_compiler_bindir_init)
	  # Since both CMAKE_CXX_COMPILER AND CMAKE_C_COMPILER can be set
	  # but only one of the _ID variant can be set, check the latter
	  # to get the correct compiler path.
	  #
      if ( NOT ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "" ))
		  get_filename_component(bindir ${CMAKE_CXX_COMPILER} DIRECTORY)
      elseif ( NOT ( "${CMAKE_C_COMPILER_ID}" STREQUAL "" ))
		  get_filename_component(bindir ${CMAKE_C_COMPILER} DIRECTORY)
      else()
          message(WARNING "compiler_Darwin.cmake - Unknown compiler")
		  return()
      endif()
      set(BIFUSD_COMPILER_BINDIR ${bindir} CACHE PATH
          "Path to the 'bin' directory of the C/C++ compiler installation")
  endfunction()
  bifusd_compiler_bindir_init()

  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      function(bifusd_gcc_compiler_incdir_init)
          file(GLOB_RECURSE xmmintrin_path "${BIFUSD_COMPILER_BINDIR}/../lib/gcc/*/xmmintrin.h")
          get_filename_component(incdir ${xmmintrin_path} DIRECTORY)
          set(BIFUSD_GCC_COMPILER_INCDIR ${incdir} CACHE PATH
              "Path to the 'include' directory of the GCC compiler installation")
      endfunction()
      bifusd_gcc_compiler_incdir_init()
  endif()

  unset(BIFUSD_OSX_LIBCXX_INCLUDEDIR CACHE)
  find_path(BIFUSD_OSX_LIBCXX_INCLUDEDIR
    NAMES atomic ratio regex unordered_map vector
    PATHS ${BIFUSD_CLANG_COMPILER_BINDIR}/../include/c++/v1
          ${CMAKE_OSX_SYSROOT}/../../../../../Toolchains/XcodeDefault.xctoolchain/usr/lib/c++/v1
          ${CMAKE_OSX_SYSROOT}/../../../../../Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1
          /Library/Developer/CommandLineTools/usr/lib/c++/v1/
          /Library/Developer/CommandLineTools/usr/include/c++/v1/
    NO_DEFAULT_PATH
    DOC "Path to the libc++ header files.")

    message(STATUS "Found OSX SDK:                ${CMAKE_OSX_SYSROOT}")
    message(STATUS "Minimum macOS vesion:         ${CMAKE_OSX_DEPLOYMENT_TARGET}")
    message(STATUS "Found C++ std headers:        ${BIFUSD_OSX_LIBCXX_INCLUDEDIR}")
endif()

# include common compiler options
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    include(${BIFUSD_TOOLS_DIR}/compiler_GNU.cmake)
else()
    # Clang or AppleClang !
    include(${BIFUSD_TOOLS_DIR}/compiler_Clang.cmake)

    list(APPEND cxx_flags
        -stdlib=libc++
        )
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # We can't place these flags into CMAKE_CXX_FLAGS. CMake forwards the C++
    # flags to the link command line. The end result is that the '-Werror' and
    # '-nostdinc++' flags will be passed to the link command line. This will
    # trigger a "error: argument unused during compilation" which will stop the
    # compilation. So, we instead make use of add_compile_options().
    add_compile_options(-nostdinc++ -cxx-isystem "${BIFUSD_OSX_LIBCXX_INCLUDEDIR}")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # We have to replace all standard include paths to compiled using:
    #   a) Clang's libc++
    #   b) The proper OS X SDK version
    list(APPEND cxx_flags -nostdinc++ -nostdinc)

    # We disable the use of C++14 sized deallocations because we otherwise get
    # on undefined symbol for ::operator delete(void*, size_t)!
    list(APPEND cxx_flags -fno-sized-deallocation)

    # GCC 6.1 emits bogus warnings that we can only silence-up using
    # -fpermissive... That isn't so great, but there isn't much can do about
    # that!
    #
    # Here's the case :
    #
    #    auto foo(auto* obj, Func f) { return f(obj); }
    #
    # If Func is a lambda returnin 'void', gcc 6.1 gives the warning:
    #
    #    return-statement with a value, in function returning 'void' [-fpermissive]
    #
    # which is completely bogus as far as I can tell.. Note that neither Clang
    # 3.8 nor MSVC 2015 complains about this!
    list(APPEND cxx_flags -fpermissive)

    # List of system include paths!
    include_directories(AFTER SYSTEM
        # Clang's libc++
        "${BIFUSD_OSX_LIBCXX_INCLUDEDIR}"

        # Ignore Clang's system headers (stdarg.h, etc...).
        # "${BIFUSD_CLANG_COMPILER_INCDIR}"

        # GCC headers for intrisincs (xmmintrin.h and others..) and system headers.
        "${BIFUSD_GCC_COMPILER_INCDIR}"

        # OS X SDK header files!
        #
        # BTW, the '/.' is added here because CMake will otherwise ignore any
        # paths ending with '/usr/include' when it is re-run on an existing build
        # area. Go figure out why!!!
        "${CMAKE_OSX_SYSROOT}/usr/include/."
        "${CMAKE_OSX_SYSROOT}/System/Library/Frameworks"
        )
endif()

# Set the default compiler flags
string(REPLACE ";" " " cxx_flags "${cxx_flags}")
set(CMAKE_CXX_FLAGS_INIT "${cxx_flags}")


#------------------------------------------------------------------------------
# Shared linker flags
#------------------------------------------------------------------------------

# No special flags for internally used shared libraries.
set(BIFUSD_INTERNAL_SHARED_LINKER_FLAGS "")
set(BIFUSD_EXTERNAL_SHARED_LINKER_FLAGS "")

# Externally available (deliverable) shared libraries should filter the
# exported symbols. These are simply any symbol in the product namespace or
# involving a type in the product namespace.
if (BIFUSD_SYMBOLS_FILE)
    list(APPEND BIFUSD_EXTERNAL_SHARED_LINKER_FLAGS
        -Wl,-w
        -Wl,-exported_symbols_list
        -Wl,${BIFUSD_SYMBOLS_FILE}
        )
endif()

set(BIFUSD_EXE_LINKER_FLAGS "")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(gnu_extra_linker_flags -Wl,-no_compact_unwind -nostdlib -L ${CMAKE_OSX_SYSROOT}/usr/lib -lc++ -lc)
    list(APPEND BIFUSD_INTERNAL_SHARED_LINKER_FLAGS ${gnu_extra_linker_flags})
    list(APPEND BIFUSD_EXTERNAL_SHARED_LINKER_FLAGS ${gnu_extra_linker_flags})
    list(APPEND BIFUSD_EXE_LINKER_FLAGS             ${gnu_extra_linker_flags})
endif()

string(REPLACE ";" " " BIFUSD_INTERNAL_SHARED_LINKER_FLAGS "${BIFUSD_INTERNAL_SHARED_LINKER_FLAGS}")
string(REPLACE ";" " " BIFUSD_EXTERNAL_SHARED_LINKER_FLAGS "${BIFUSD_EXTERNAL_SHARED_LINKER_FLAGS}")
string(REPLACE ";" " " BIFUSD_EXE_LINKER_FLAGS             "${BIFUSD_EXE_LINKER_FLAGS}")

# Set the default linker flags
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${BIFUSD_EXTERNAL_SHARED_LINKER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT    "${BIFUSD_EXE_LINKER_FLAGS}")


#------------------------------------------------------------------------------
# Compile definitions must be set outside of BIFUSD_COMPILER_CONFIGURED
# as these are not stored in CMake cache variables.
#------------------------------------------------------------------------------

add_definitions(
  -D__STDC_CONSTANT_MACROS
  -D__STDC_FORMAT_MACROS
  -D__STDC_LIMIT_MACROS
)

# Setup an rpath into our libraries and executable so that they can be
# run directly without having to set a DYLD_LIBRARY_PATH.
#
# See: http://www.kitware.com/blog/home/post/510
#      Upcoming in CMake 2.8.12: OSX RPath Support
set(CMAKE_MACOSX_RPATH ON)
set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
