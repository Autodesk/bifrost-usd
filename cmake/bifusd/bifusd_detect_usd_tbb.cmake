#-
#*****************************************************************************
# Copyright 2026 Autodesk, Inc.
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
# Detect the TBB shared library that is shipped with Pixar USD, and creates
# an INTERFACE target BifusdTBB that links against it and exposes the proper
# include dirs, shared libs and compiler definitions.
#
# bifusd_detect_usd_tbb.cmake
#
# Usage:
#   Once you have a 'usd' CMake target to link against:
#       bifusd_detect_usd_tbb(usd [verbose])
#
# This module defines:
#   USD_TBB_VERSION_MAJOR
#   USD_TBB_VERSION_MINOR
#   USD_TBB_SHAREDLIB_DIR
#   USD_TBB_INCLUDE_DIR
#   USD_TBB_LIB_DIR         (for Windows platform)
#
# It creates:
#   add_library(BifusdTBB INTERFACE)  (only once)

include_guard(GLOBAL)

# Collect include directory paths from a CMake target and output them to a
# variable in the parent scope.
#
# Args:
#   out_list    - Variable name to set with the cleaned list of include dirs.
#   in_target   - USD target to inspect for include directories.
#   verbose     - If TRUE, prints detailed status messages during detection.
function(_bifusd_collect_target_include_dirs out_list in_target verbose)
    set(_incs "")

    # Read INTERFACE_INCLUDE_DIRECTORIES property.
    get_target_property(_inc "${in_target}" INTERFACE_INCLUDE_DIRECTORIES)
    if(_inc)
        list(APPEND _incs ${_inc})
    endif()

    # Read from INCLUDE_DIRECTORIES also (e.g. if given target is non-imported):
    get_target_property(_inc "${in_target}" INCLUDE_DIRECTORIES)
    if(_inc)
        list(APPEND _incs ${_inc})
    endif()

    # Filter out generator expressions ($<...>) to keep only plain paths:
    set(_clean "")
    foreach(_i IN LISTS _incs)
        if(_i MATCHES "^\\$<") # skip generator expressions
            continue()
        endif()
        if(_i)
            list(APPEND _clean "${_i}")
        endif()
    endforeach()

    # Removes duplicate entries:
    list(REMOVE_DUPLICATES _clean)

    if(NOT _clean)
        message(FATAL_ERROR "   Target '${in_target}' has no include dirs for searching TBB headers.")
    elseif(verbose)
        message(STATUS "   Collected include dirs from target '${in_target}':")
        foreach(_dir IN LISTS _clean)
            message(STATUS "      ${_dir}")
        endforeach()
    endif()
    set(${out_list} "${_clean}" PARENT_SCOPE)
endfunction()

# Find the TBB include dir and the TBB version header file by scanning a list
# of candidate TBB version header files.
# On first match, set 'out_tbb_inc_dir' to the tbb include dir and set
# 'out_tbb_header' to the full pathname of the found TBB version header file.
# For example, if the found candidate TBB header is `<inc>/oneapi/tbb/version.h`,
# then 'out_tbb_inc_dir' is set to `<inc>/oneapi/` and 'out_tbb_header' is set
# to `<inc>/oneapi/tbb/version.h`.
#
# Arg:
#   out_tbb_inc_dir - Variable name to store the directory where the /tbb/*.h
#                     header files are located (empty if header is not found).
#   out_tbb_header  - Variable name to store the pathname of the found existing
#                     TBB version header file (empty if header is not found).
#   include_dirs    - List of directories to search.
#   verbose         - If TRUE, prints detailed status messages during detection.
function(_bifusd_find_tbb_header_from_include_dirs out_tbb_inc_dir out_tbb_header include_dirs verbose)
    set(_tbb_inc_dir "")
    set(_tbb_header "")

    foreach(_d IN LISTS include_dirs)
        if(NOT IS_DIRECTORY "${_d}")
            continue()
        endif()
        # Look for the TBB version header file.
        # First check for OneTBB header file, then for legacy TBB header file:
        set(_h "${_d}/oneapi/tbb/version.h")
        if(EXISTS "${_h}")
            set(_tbb_inc_dir "${_d}")
            set(_tbb_header  "${_h}")
            break()
        endif()
        set(_h "${_d}/tbb/tbb_stddef.h")
        if(EXISTS "${_h}")
            set(_tbb_inc_dir "${_d}")
            set(_tbb_header  "${_h}")
            break()
        endif()
    endforeach()

    if(NOT _tbb_inc_dir OR NOT _tbb_header)
        message(FATAL_ERROR "   Could not locate TBB headers from USD target.")
    endif()
    if(verbose)
        message(STATUS "   Found TBB header file:  '${_tbb_header}'")
        message(STATUS "   TBB include dir to use: '${_tbb_inc_dir}'")
    endif()
    set(${out_tbb_inc_dir} "${_tbb_inc_dir}" PARENT_SCOPE)
    set(${out_tbb_header}  "${_tbb_header}"  PARENT_SCOPE)
endfunction()

# Parse the given TBB version header file, and return the major and minor
# version components to the parent scope.
# If header does not exist or if version macros are not found, outputs are set
# to empty strings.
#
# Args:
#   out_major   - Variable name to set the TBB major version in parent scope.
#   out_minor   - Variable name to set the TBB minor version in parent scope.
#   tbb_header  - The TBB version header file to scan for version components.
#   verbose     - If TRUE, prints detailed status messages during detection.
function(_bifusd_parse_tbb_version out_major out_minor tbb_header verbose)
    set(${out_major} "" PARENT_SCOPE)
    set(${out_minor} "" PARENT_SCOPE)

    if(NOT EXISTS "${tbb_header}")
        message(FATAL_ERROR "   TBB header file '${tbb_header}' does not exist.\n"
                            "   Cannot parse file for TBB version components.")
    endif()
    get_filename_component(_filename "${tbb_header}" NAME)
    if(verbose)
        message(STATUS "   Parsing TBB file '${_filename}' for version components...")
    endif()

    set(_major "")
    set(_minor "")
    file(READ "${tbb_header}" _txt)

    # Search for standard TBB version component macros:
    if(_txt MATCHES "#[ \t]*define[ \t]+TBB_VERSION_MAJOR[ \t]+([0-9]+)")
        set(_major "${CMAKE_MATCH_1}")
    endif()
    if(_txt MATCHES "#[ \t]*define[ \t]+TBB_VERSION_MINOR[ \t]+([0-9]+)")
        set(_minor "${CMAKE_MATCH_1}")
    endif()

    if(_major STREQUAL "" OR _minor STREQUAL "")
        message(FATAL_ERROR "   TBB_VERSION_MAJOR or TBB_VERSION_MINOR not found in file:\n"
                            "      '${tbb_header}'.\n"
                            "   Match results were: major='${_major}', minor='${_minor}'")
    endif()
    if(verbose)
        message(STATUS "      Found TBB version ${_major}.${_minor}")
    endif()
    set(${out_major} "${_major}" PARENT_SCOPE)
    set(${out_minor} "${_minor}" PARENT_SCOPE)
endfunction()

# Detect the library directory for the given USD target and current build type.
# If no location is found, out_var is set to an empty string.
#
# Args:
#   out_var     - Variable name to receive the detected library directory.
#   in_target   - USD target to inspect.
#   verbose     - If TRUE, prints detailed status messages during detection.
function(_bifusd_guess_usd_libdir out_var in_target verbose)
    # USD only has Debug and RelWithDebInfo configs:
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(_build_type "DEBUG")
    else()
        set(_build_type "RELWITHDEBINFO")
    endif()

    # Query the target's imported location for current build type
    set(_libdir "")
    get_target_property(_loc "${in_target}" IMPORTED_LOCATION_${_build_type})
    if(NOT _loc)
        # Fallback - try without config suffix
        if(verbose)
            message(STATUS "   Target does not have an IMPORTED_LOCATION_${_build_type} property.")
            message(STATUS "   Trying IMPORTED_LOCATION instead...")
        endif()
        get_target_property(_loc "${in_target}" IMPORTED_LOCATION)
    endif()
    if(_loc)
        get_filename_component(_libdir "${_loc}" DIRECTORY)
    endif()
    if(NOT _libdir)
        message(FATAL_ERROR "   Could not determine USD sharedlib directory.\n"
                            "   Target has no IMPORTED_LOCATION property with a valid path.")
    endif()
    if(verbose)
        message(STATUS "   USD sharedlib dir: '${_libdir}'")
    endif()
    set(${out_var} "${_libdir}" PARENT_SCOPE)
endfunction()

# Find the directory where the TBB shared libs are located, relative to the
# given USD shared lib directory.
# If TBB shared libs are not found, set out_var to an empty string.
#
# Args:
#   out_var     - Variable name to set with the found TBB shared lib directory.
#   usd_libdir  - Path to the USD shared library dir to search for TBB shared libs.
#   verbose     - If TRUE, prints detailed status messages during detection.
function(_bifusd_find_tbb_sharedlib_dir out_var usd_libdir verbose)
    set(${out_var} "" PARENT_SCOPE)
    if(NOT usd_libdir OR NOT IS_DIRECTORY "${usd_libdir}")
        message(FATAL_ERROR "   '${usd_libdir}' is not an existing directory.\n"
                            "   Cannot search for TBB shared libs.")
    endif()

    # USD typically ships its third-party dependencies (including TBB) in the
    # same lib/ directory as its own shared libraries. Check if we can match
    # any of the following TBB shared lib pathnames across all platforms:
    file(GLOB _tbb_candidates
        "${usd_libdir}/libtbb*.so*"
        "${usd_libdir}/libtbb*.dylib*"
        "${usd_libdir}/tbb*.dll"
        "${usd_libdir}/tbb*.lib"
        "${usd_libdir}/libtbb*.a"       # rare, but harmless
    )

    if(NOT _tbb_candidates)
        message(FATAL_ERROR "   No typical TBB shared libs found in '${usd_libdir}'.")
    elseif(verbose)
        message(STATUS "   Found the following TBB shared libs near USD libs:")
        foreach(_tbb_candidate IN LISTS _tbb_candidates)
            message(STATUS "      ${_tbb_candidate}")
        endforeach()
    endif()
    set(${out_var} "${usd_libdir}" PARENT_SCOPE)
endfunction()

# Append the first matching file from a directory by glob patterns to a list.
# It first GLOBs for files matching the given patterns in the specified dir,
# sorts the results alphabetically, and appends the first match to the list.
#
# Args:
#   out_list    - List to which the first matching file path will be appended
#                 (list is unchanged if no matches found).
#   src_dir     - Directory path to search for files.
#   patterns    - List of glob patterns (e.g., "libtbb.so*" "libtbb.dylib*").
#                 Patterns will be prefixed with src_dir automatically.
function(_bifusd_append_first_matching_file out_list src_dir)
    if(NOT src_dir OR NOT IS_DIRECTORY "${src_dir}")
        message(FATAL_ERROR "   '${src_dir}' is not an existing directory.\n"
                            "   Cannot search for file(s).")
    endif()

    # Find all files matching any of the given patterns in the source dir:
    set(_patterns "")
    foreach(_pattern IN LISTS ARGN)
        list(APPEND _patterns "${src_dir}/${_pattern}")
    endforeach()
    file(GLOB _matches ${_patterns})

    # Sort matches alphabetically and append the first one to the list, if any:
    if(_matches)
        list(SORT _matches)
        list(GET  _matches 0 _first)
        list(APPEND ${out_list} "${_first}")
        set(${out_list} "${${out_list}}" PARENT_SCOPE)
    else()
        message(STATUS "   Error: The TBB sharedlib dir has no file matching these pattern(s):")
        foreach(_pattern IN LISTS ARGN)
            message(STATUS "      ${_pattern}")
        endforeach()
        message(FATAL_ERROR "   Cannot find required TBB file(s) with given pattern(s).")
    endif()
endfunction()

# Detect and configure TBB as used by a given USD target.
#
# This function locates the TBB headers and shared libraries that are bundled
# with the specified USD target, parses the TBB version, and defines an
# INTERFACE target (BifusdTBB) for downstream consumers. It ensures that
# consumers use the same TBB headers and libraries as the USD target, avoiding
# ABI mismatches.
#
# Args:
#   usd_target: The CMake target representing the USD library to inspect for
#               TBB dependencies.
#   verbose:    If TRUE, prints detailed status messages during detection.
#
# This function exports the following variables to the parent scope:
#   USD_TBB_VERSION_MAJOR   - Detected TBB major version.
#   USD_TBB_VERSION_MINOR   - Detected TBB minor version.
#   USD_TBB_INCLUDE_DIR     - Path to TBB include directory.
#   USD_TBB_SHAREDLIB_DIR   - Path to TBB shared libs directory.
#   USD_TBB_LIB_DIR         - Path to TBB import libs directory (if Windows).
function(bifusd_detect_usd_tbb usd_target verbose)
    message(STATUS "Find TBB library bundled with USD target '${usd_target}'...")

    # Check prerequisites before doing any work:
    if(NOT TARGET "${usd_target}")
        message(FATAL_ERROR "   Target '${usd_target}' does not exist.")
    endif()
    if(NOT BIFUSD_PACKAGE_NAME)
        message(FATAL_ERROR "   The variable BIFUSD_PACKAGE_NAME must be set.")
    endif()
    if(NOT CMAKE_BUILD_TYPE)
        message(FATAL_ERROR "   CMake build type is not set.")
    endif()

    # 1) Gather include dirs from the USD target to find the TBB include root dir:
    _bifusd_collect_target_include_dirs(_usd_incs "${usd_target}" "${verbose}")
    _bifusd_find_tbb_header_from_include_dirs(_tbb_inc_dir _tbb_header "${_usd_incs}" "${verbose}")

    # 2) Parse TBB version
    _bifusd_parse_tbb_version(_version_major _version_minor "${_tbb_header}" "${verbose}")

    # 3) Determine USD shared lib dir, then locate TBB shared libs shipped with USD:
    _bifusd_guess_usd_libdir(_usd_libdir "${usd_target}" "${verbose}")
    _bifusd_find_tbb_sharedlib_dir(_tbb_sharedlib_dir "${_usd_libdir}" "${verbose}")

    # 4) Select tbb and tbbmalloc shared libs to link against:
    set(_suffix "")
    set(_tbb_link_libs "")
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        set(_suffix "_debug")
    endif()
    if(WIN32)
        set(_tbb_implib_dir "")
        # On Windows, link against import libs (.lib), not DLLs.
        _bifusd_append_first_matching_file(_tbb_link_libs "${_tbb_sharedlib_dir}" "tbb*${_suffix}.lib")
        _bifusd_append_first_matching_file(_tbb_link_libs "${_tbb_sharedlib_dir}" "tbbmalloc*${_suffix}.lib")
        if(_tbb_link_libs)
            set(_tbb_implib_dir "${_tbb_sharedlib_dir}")
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        _bifusd_append_first_matching_file(_tbb_link_libs "${_tbb_sharedlib_dir}" "libtbb${_suffix}.so*")
        _bifusd_append_first_matching_file(_tbb_link_libs "${_tbb_sharedlib_dir}" "libtbbmalloc${_suffix}.so*")
    else()
        _bifusd_append_first_matching_file(_tbb_link_libs "${_tbb_sharedlib_dir}" "libtbb${_suffix}.*dylib")
        _bifusd_append_first_matching_file(_tbb_link_libs "${_tbb_sharedlib_dir}" "libtbbmalloc${_suffix}.*dylib")
    endif()
    if(_tbb_link_libs)
        if(verbose)
            message(STATUS "   Selected TBB shared libs to link against on ${CMAKE_SYSTEM_NAME}:")
            foreach(_lib IN LISTS _tbb_link_libs)
                get_filename_component(_name "${_lib}" NAME)
                message(STATUS "      ${_name}")
            endforeach()
        endif()
    endif()

    # 5) Create BifusdTBB INTERFACE target for downstream consumers:
    if(NOT TARGET BifusdTBB)
        if(verbose)
            message(STATUS  "   Creating new BifusdTBB INTERFACE target...")
        endif()
        add_library(BifusdTBB INTERFACE)

        # Ensure consumers compile against the same TBB headers USD exposes.
        # Note: SYSTEM INTERFACE: to suppress reserved identifier warnings for TBB headers.
        #       BUILD_INTERFACE: during build, use the detected path to TBB headers from USD.
        #       INSTALL_INTERFACE: intentionally empty, consumers must find USD, which provides TBB.
        target_include_directories(BifusdTBB SYSTEM INTERFACE
            "$<BUILD_INTERFACE:${_tbb_inc_dir}>"
        )

        # Link against found TBB shared libs during build only.
        # At install time, consumers get TBB from their own USD installation.
        if(_tbb_link_libs)
            foreach(_lib IN LISTS _tbb_link_libs)
                target_link_libraries(BifusdTBB INTERFACE
                    "$<BUILD_INTERFACE:${_lib}>")
            endforeach()
            target_link_libraries(BifusdTBB INTERFACE
                "$<INSTALL_INTERFACE:usd>")
        endif()

        # Add C++ compile definitions:
        target_compile_definitions(BifusdTBB INTERFACE
            "USD_TBB_VERSION_MAJOR=${_version_major}"
            "USD_TBB_VERSION_MINOR=${_version_minor}"
        )

        # Add an RPATH for the TBB shared library directory so that downstream
        # consumers can find these TBB shared libs at runtime.
        if(NOT WIN32)
            target_link_options(BifusdTBB INTERFACE
                "$<BUILD_INTERFACE:LINKER:-rpath,${_tbb_sharedlib_dir}>")
        endif()

        # Add BifusdTBB to the same export set as BifusdTBBHelpers, so that it
        # gets properly exported with the package:
        install(TARGETS BifusdTBB EXPORT ${BIFUSD_PACKAGE_NAME})
    elseif(verbose)
        message(STATUS  "   BifusdTBB INTERFACE target already exists. Skipping target creation.")
    endif()

    # 6) Export variables to parent scope:
    set(USD_TBB_VERSION_MAJOR "${_version_major}" PARENT_SCOPE)
    set(USD_TBB_VERSION_MINOR "${_version_minor}" PARENT_SCOPE)
    set(USD_TBB_INCLUDE_DIR "${_tbb_inc_dir}" PARENT_SCOPE)
    set(USD_TBB_SHAREDLIB_DIR "${_tbb_sharedlib_dir}" PARENT_SCOPE)
    set(USD_TBB_LIB_DIR "${_tbb_implib_dir}" PARENT_SCOPE)

    message(STATUS "   Successfully found TBB library bundled with USD:")
    message(STATUS "      USD_TBB_VERSION_MAJOR   = ${_version_major}")
    message(STATUS "      USD_TBB_VERSION_MINOR   = ${_version_minor}")
    message(STATUS "      USD_TBB_INCLUDE_DIR     = ${_tbb_inc_dir}")
    message(STATUS "      USD_TBB_SHAREDLIB_DIR   = ${_tbb_sharedlib_dir}")
    message(STATUS "      USD_TBB_LIB_DIR (WIN32) = ${_tbb_implib_dir}")
endfunction()
