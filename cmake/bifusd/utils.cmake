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
if(bifusd_utils_included)
    return()
endif()
set(bifusd_utils_included true)

option(BIFUSD_USE_DEBUGGER "Launch unit tests using the platform-specific debugger." OFF)

# Export the given list of variables from the local scope to the parent scope.
#
#   bifusd_export_vars(<var1> <var2> ...)
macro(bifusd_export_vars)
    set(varnames "${ARGN}")

    foreach(varname IN LISTS varnames)
        set(${varname} "${${varname}}" PARENT_SCOPE)
    endforeach()
endmacro(bifusd_export_vars)

# Export the given list of path variables from the local scope to the parent
# scope.
#
# This function behaves in the same way as `bifusd_export_vars`, with the
# exception that path variables are always normalized before being returned to
# the parent scope.
#
#   bifusd_export_path_vars(<var1> <var2> ...)
macro(bifusd_export_path_vars)
    bifusd_normalize_paths(${ARGN})
    bifusd_export_vars(${ARGN})
endmacro(bifusd_export_path_vars)

# Add a test category label to the list of labels.
#
#   bifusd_add_test_categories(labels_list_var <cat1> <cat2> ...)
#
#   labels_list_var -  The name of the variable containing the list of labels.
macro(bifusd_add_test_categories labels_var)
    set(categories "${ARGN}")

    foreach(category IN LISTS categories)
        list(APPEND ${labels_var} "${BIFUSD_CATEGORY_PREFIX}${category}")
    endforeach()
endmacro(bifusd_add_test_categories)


# Split the given string using the provided separator into a list of substrings.
# The separator is not included in the resulting list.
#
#   bifusd_string_split(out_list_var  in_string  separator)
#
#   out_list_var - The name of variable to store the resulting list.
#   in_string    - The input string to split.
#   separator    - The separator used to split the input string.
function(bifusd_string_split out_list_var in_string separator)
    string(REPLACE "${separator}" ";" ${out_list_var} "${in_string}")
    bifusd_export_vars(${out_list_var})
endfunction(bifusd_string_split)

# Evaluate the condition and store either 0 or 1 in the given variable name
# depending on whether the condition is false or true, respectively.
#
#   bifusd_eval_condition(varname
#                          CONDITION <condition>)
#
#   varname   - The name of the variable to store the boolean result.
#   CONDITION - The boolean condition to be evaluated.
function(bifusd_eval_condition varname)
    bifusd_extract_options("CONDITION" ${ARGN})

    if (${CONDITION})
        set(${varname} 1 PARENT_SCOPE)
    else()
        set(${varname} 0 PARENT_SCOPE)
    endif()
endfunction(bifusd_eval_condition)

# Set a given variable based on whether the source string matches
# a regular expression or not.
#
#   varname - name of variable to set
#   src_varname - source variable name from which contents will be compared for a match.
#   regex - regular expression used to match the source string contents.
function(bifusd_set_if_matches varname src_varname regex)
    bifusd_eval_condition(${varname}
                           CONDITION ${src_varname} MATCHES "${regex}")
    bifusd_export_vars(${varname})
endfunction(bifusd_set_if_matches)

# Select the value associated with the currently running platform and store it
# in the provided variable.
#
# If the LINUX or OSX modes are provided together with the UNIX mode, the
# evaluation of LINUX and OSX takes precedence over the UNIX one.
#
#   bifusd_platform_select(varname
#                           LINUX   <val1> <val2> ...
#                           OSX     <val1> <val2> ...
#                           WINDOWS <val1> <val2> ...
#                           UNIX    <val1> <val2> ...)
#
#   varname - The variable to store the selected values.
#   LINUX   - The values to be selected if the running platform is Linux.
#   OSX     - The values to be selected if the running platform is OSX.
#   WINDOWS - The values to be selected if the running platform is Windows.
#   UNIX    - The values to be selected if the running platform is Unix,
#             provided that neither LINUX nor OSX were selected.
function(bifusd_platform_select varname)
    set(modes
        "LINUX"
        "OSX"
        "WINDOWS"
        "UNIX"
    )

    bifusd_extract_options("${modes}" ${ARGN})

    set(value)

    if(BIFUSD_IS_LINUX AND LINUX-FOUND)
        set(value ${LINUX})
    elseif(BIFUSD_IS_OSX AND OSX-FOUND)
        set(value ${OSX})
    elseif(BIFUSD_IS_WINDOWS AND WINDOWS-FOUND)
        set(value ${WINDOWS})
    elseif(BIFUSD_IS_UNIX AND UNIX-FOUND)
        set(value ${UNIX})
    endif()

    set(${varname} ${value} PARENT_SCOPE)
endfunction(bifusd_platform_select)

# Load the values from a list into the given variable names.
#
# If the list of values is smaller than the list of variable names to be
# assigned, the provided default value is used for the unassigned variables.
#
#   bifusd_load_vars(LIST <val1> <var2> ...
#                     VARS <var1> <var2> ...
#                     [DEFAULT <val>])
#
#   LIST    - The list of values to be assigned to the variables.
#   VARS    - The name of the variables to have the values assigned.
#   DEFAULT - The default value to use for unassigned variables.
function(bifusd_load_vars)
    set(modes
        "LIST"
        "VARS"
        "DEFAULT"
    )

    bifusd_extract_options("${modes}" ${ARGN})

    if (${DEFAULT-FOUND})
        set(default_elem ${DEFAULT})
    else()
        set(default_elem)
    endif()

    list(LENGTH LIST num_elems)
    set(index 0)
    foreach(varname ${VARS})
        if (index LESS num_elems)
            list(GET LIST ${index} elem_value)
            MATH(EXPR index "${index}+1")
        else()
            set(elem_value ${default_elem})
        endif()

        set(${varname} "${elem_value}" PARENT_SCOPE)
    endforeach()
endfunction(bifusd_load_vars)

# Extract the version elements from the version string.
#
# The element variables passed as arguments are assigned in the order as they
# are specified in the given version string. If the number of version element
# variables is larger than the actual number of elements in the version string
# they unassigned element variables are filled with 0 (zero).
#
#   bifusd_extract_version_elements(version <elem1> [<elem2> <elem3> ...])
#
#   version - The version string.
#   elems   - The variable names to set with the version element values.
function(bifusd_extract_version_elements version)
    bifusd_string_split(version_elems "${version}" ".")
    bifusd_load_vars(LIST ${version_elems} VARS ${ARGN} DEFAULT "0")
    bifusd_export_vars(${ARGN})
endfunction(bifusd_extract_version_elements)

# Perform the intersection of the values in the given list variable and the
# sequence of values provided in the function.
function(bifusd_list_intersect list_var)
    set(temp_list ${${list_var}})
    list(REMOVE_ITEM temp_list ${ARGN})

    if (temp_list)
        list(REMOVE_ITEM ${list_var} ${temp_list})
    endif()

    set(${list_var} ${${list_var}} PARENT_SCOPE)
endfunction(bifusd_list_intersect)

# Reset the value of a cache entry without changing its type or doc string.
#
#   name - name of the cache entry to modify
#   value - new value to be set.

function(bifusd_reset_cache_entry name value)
  get_property(type CACHE ${name} PROPERTY TYPE)
  get_property(doc CACHE ${name} PROPERTY HELPSTRING)
  set(${name} ${value} CACHE ${type} "${doc}" FORCE)
endfunction(bifusd_reset_cache_entry)

# Append a target property to a list variable
#
# Gets the property 'propname' from the specified target. If the property isn't
# empty, it is appended to specified variable.
#
#   var      - name of the variable to which the property is appended
#   target   - the target from which the property is fetched
#   propname - the name of the property to fetch
function(bifusd_append_target_property var target propname)
    get_target_property(propval ${target} ${propname})
    if(propval)
        set(result ${${var}})
        list(APPEND result ${propval})
        set(${var} ${result} PARENT_SCOPE)
    endif()
endfunction()

# Sort a list variable and remove duplicates
#
#   var - name of the variable to sort
function(bifusd_sort_unique var)
    if(${var})
        list(SORT              ${var})
        list(REMOVE_DUPLICATES ${var})
        set(${var} ${${var}} PARENT_SCOPE)
    endif()
endfunction()

# Remove items from the list
#
# Like list(REMOVE_ITEM) except that it also works when no items are
# present.
#
#   var - name of the variable to filter
function(bifusd_list_remove_items var)
    list(LENGTH ARGN   count1)
    list(LENGTH ${var} count2)
    if((${count1} GREATER 0) AND (${count2} GREATER 0))
        list(REMOVE_ITEM ${var} ${ARGN})
        set(${var} ${${var}} PARENT_SCOPE)
    endif()
endfunction()

# Sort a list of include directories
#
# Sort a list of include directories. Duplicated entries are eliminated. In
# addition, the function makes sure that BIFUSD_OUTPUT_INCLUDE_DIR is always
# the first entry in the list if present. This is to ensure that the installed
# header files are always included instead of the headers from the git source
# tree.
#
# var - name of the variable to sort
function(bifusd_sort_include_dirs var)
    if(${var})

        set(bifusd_include-FOUND FALSE)

        set(result "")
        foreach(dir ${${var}})
            if (dir STREQUAL "$<BUILD_INTERFACE:${BIFUSD_OUTPUT_INCLUDE_DIR}>" OR
                dir STREQUAL "$<INSTALL_INTERFACE:${BIFUSD_INSTALL_INCLUDE_DIR}>" OR
                dir STREQUAL "${BIFUSD_OUTPUT_INCLUDE_DIR}")
                set(bifusd_include-FOUND TRUE)
                continue()
            endif()

            if (NOT dir MATCHES "^[$]<(BUILD|INSTALL)_INTERFACE:.*>$")
                # By default, regular includes are the for the local build. We
                # simply wrap them into an explicit BUILD_INTERFACE annotation
                # to ensure stable sorting below.
                set(dir "$<BUILD_INTERFACE:${dir}>")
            endif()

            list(APPEND result "${dir}")
        endforeach()

        list(SORT              result)
        list(REMOVE_DUPLICATES result)

        # Make sure that BIFUSD_OUTPUT_INCLUDE_DIR is always first. We want to
        # include installed headers in priority.
        if(bifusd_include-FOUND)
            set(result
                "$<INSTALL_INTERFACE:${BIFUSD_INSTALL_INCLUDE_DIR}>"
                "$<BUILD_INTERFACE:${BIFUSD_OUTPUT_INCLUDE_DIR}>" ${result}
                )
        endif()

        set(${var} ${result} PARENT_SCOPE)
    endif()
endfunction()

# Check whether the variable names given as arguments are defined or not.
# A fatal error is emitted if any of the variables is not defined.
function(bifusd_required_variables)
    foreach(var ${ARGN})
        if (NOT DEFINED ${var})
            message(FATAL_ERROR "Required variable ${var} has not been defined.")
        endif()
    endforeach()
endfunction(bifusd_required_variables)


# Separate the list of arguments according to a list of valid modes.
# The list of valid modes is given as the first argument as a single
# list string, and the remaining arguments are then slit into separate
# lists for each of the modes. After all the arguments have been processed
# the list are exported to the parent scope with the same name as the
# mode they are associated with.
#
# In addition, the variable ${mode}-FOUND is set to either TRUE or
# FALSE to indicate whether a given mode was found while scanning the
# arguments. This can be used to support modes that are merely flags
# and that do not require to be followed by additional arguments.
#
# Finally, if the mode is present in ${ARGN}, the variable ${mode}-ARGS will be
# set to a list containing the mode name and the mode arguments. Otherwise, the
# variable ${mode}-ARGS won't be set. This can be used to easily forward a mode
# from a caller to a callee.
#
#   modes - list of modes used separate the following list of arguments.
#   ARGN  - list of arguments to be split into separate lists according
#           to the provided modes.

function(bifusd_extract_options modes)
    foreach(mode ${modes})
        set("${mode}-FOUND" "FALSE")
        unset("${mode}")
    endforeach()

    foreach(arg ${ARGN})
        list(FIND modes ${arg} mode_pos)
        if (mode_pos GREATER -1)
            set(mode_list "${arg}")
            set("${mode_list}-FOUND" "TRUE")
        else()
            if (NOT mode_list)
                message(WARNING "\"${arg}\" is not a valid option. Ignoring...")
            endif()
            list(APPEND ${mode_list} ${arg})
        endif()
    endforeach()

    # Export option tags lists to the parent scope
    foreach(mode ${modes})
        set(${mode} "${${mode}}" PARENT_SCOPE)
        set("${mode}-FOUND" "${${mode}-FOUND}" PARENT_SCOPE)
        if(${mode}-FOUND)
            set(${mode}-ARGS "${mode};${${mode}}" PARENT_SCOPE)
        else()
            unset("${mode}-ARGS" PARENT_SCOPE)
        endif()
    endforeach()
endfunction(bifusd_extract_options)

# Utility function to append arguments to a mode option
#
#   bifusd_append_option(<option> <arg1> [<arg2> <arg3> ...])
#
#   option - the option to which the arguments will be appended.
#   args   - list of arguments to be appended.
#
# bifusd_append_option() option correctly appends an argument to a mode
# option. bifusd_append_option() assumes the option is a list. This behavior
# differs from bifusd_concatenate_option() which assumes that the option is a
# single string. bifusd_append_option() ensures that the <option> -FOUND and
# <option>-ARGS variable are also being properly updated.
macro(bifusd_append_option option arg1)
    list(APPEND ${option} ${arg1} ${ARGN})
    set(${option}-FOUND "TRUE")
    set(${option}-ARGS ${option} ${${option}})
endmacro(bifusd_append_option)

# Utility function to concatenate an argument to a mode option
#
#   bifusd_concatenate_option(<option> <arg>)
#
#   option - the option to which the argument will be concatenated.
#   arg    - argument to be concatenated.
#
# bifusd_concatenate_option() option concatenates an argument to a mode
# option. bifusd_concatenate_option() assumes the option is a single
# string. The argument is concatenated with no space or separator. This
# behavior differs from bifusd_append_option() which assumes that the option
# is a list. bifusd_concatenate_option() ensures that the <option>-FOUND and
# <option>-ARGS variable are also being properly updated.
macro(bifusd_concatenate_option option arg)
    set(${option} "${${option}}${arg}")
    set(${option}-FOUND "TRUE")
    set(${option}-ARGS ${option} ${${option}})
endmacro(bifusd_concatenate_option)

# Returns a list options
#
#   bifusd_get_options(<output_var> <modes>)
#
#   output_var - the output_variable.
#   modes      - list modes to include in the output variable.
#
# Concatenate the content of all currently set <mode>-ARGS variables and store
# the result in <output_var>. This is useful to propagate a list of mode
# options so that they can be forwarded appropriately to another bifusd
# function.
function(bifusd_get_options output_var)
    set(output)
    foreach(mode ${ARGN})
        if (${mode}-FOUND)
            list(APPEND output ${${mode}-ARGS})
        endif()
    endforeach()
    set(${output_var} "${output}" PARENT_SCOPE)
endfunction(bifusd_get_options)

# Check if a string ends with a slash
#
# Returns 1 in ${ends_with_slash} if the string s ends with a slash,
# 0 otherwise.
#
#   s               - the string to check.
#   ends_with_slash - return value
#
function(bifusd_ends_with_slash s ends_with_slash)
    bifusd_set_if_matches(res "${s}" "/$")
    set(${ends_with_slash} ${res} PARENT_SCOPE)
endfunction()

# Concatenates path elements
#
# Creates a path resulting from concatenating all additional parameters
# Duplicate slashes are removed, but any slash in the beginning or end
# is kept.
#
#   ret_path     - variable to receive the resulting path
function(bifusd_concat_path ret_path)
    set(result "")
    foreach(item ${ARGN})
        if (result)
            string(REGEX REPLACE "/+$" "" first "${result}")
            string(REGEX REPLACE "^/+" "" second "${item}")

            set (result ${first}/${second})
        else()
            set (result ${item})
        endif()
    endforeach()
    set (${ret_path} ${result} PARENT_SCOPE)
endfunction()

# Get the absolute cmake path of a source file
#
#   bifusd_get_absolute_cmake_path(<return_path> <src_file>)
#
# return_path  - Name of variable in which the generated target name will be
#                returned.
# src_file - Relative or absolute path to the source file. Relative paths are
#            assumed to be relative to the CMAKE_CURRENT_SOURCE_DIR
#            directory. Absolute path can reside in either the source or binary
#            tree.
function(bifusd_get_absolute_cmake_path return_path src_file)
    # First build absolute paths
    get_filename_component(src_file ${src_file} ABSOLUTE)

    # Make sure that we are always dealing with internal CMake style paths
    file(TO_CMAKE_PATH "${src_file}" src_file)

    set (${return_path} ${src_file} PARENT_SCOPE)
endfunction()

# Split the path into its directory and name components.
#
# Combining/joining the produced directory and name components results on the
# original path value.
#
#   bifusd_split_path(<dirname_var> <basename_var> <path>)
#
#   dirname_var  - The variable where the directory component is to be stored.
#   basename_var - The variable where the name component is to be stored.
#   path         - The path to be split.
function(bifusd_split_path dirname_var basename_var path)
    get_filename_component(dn ${path} DIRECTORY)
    get_filename_component(bn ${path} NAME)

    set(${dirname_var} "${dn}" PARENT_SCOPE)
    set(${basename_var} "${bn}" PARENT_SCOPE)
endfunction(bifusd_split_path)

# Split the path into a list containing all its individual path elements.
#
#   bifusd_split_path(<dirname_var> <basename_var> <path>)
#
#   elements_var - The variable where the list of path elements is to be stored.
#   path         - The path to be split.
function(bifusd_split_path_elements elements_var path)
    set(elems)

    while(path)
        bifusd_split_path(dirname basename ${path})

        if (NOT basename)
            set(elems ${dirname} ${elems})
            set(path)
        else()
            set(elems ${basename} ${elems})
            set(path ${dirname})
        endif()
    endwhile()

    set(${elements_var} "${elems}" PARENT_SCOPE)
endfunction(bifusd_split_path_elements)

# Given a list containing variable names, filter all variables that are empty,
# that is variables that contain an empty string, empty list, or are undefined.
#
#   bifusd_filter_empty_vars(<out_lst_var> <lst>)
#
#   out_lst_var - The variable to store the filtered list of variables.
#   lst         - The list of variables to filter.
function(bifusd_filter_empty_vars out_lst_var lst)
    set(out_lst)
    foreach(elem_var ${lst})
        if (${elem_var})
            list(APPEND out_lst ${elem_var})
        endif()
    endforeach()
    set(${out_lst_var} ${out_lst} PARENT_SCOPE)
endfunction()

# Select and rank a list of paths.
#
# The list of paths is filtered so that only paths containing elements belonging
# to all element ranking variables are included. That is, each selected path
# must have at least one path element in each of the ranking element list
# variables. Only path elements past the given root directory are considered for
# selection and ranking.
#
# Additionally, the selected paths are recursively ranked according to the order
# of the elements in each ranking element variables. That is, the selected paths
# are first ranked according to the ordering of the first element ranking list
# variable, then within paths of the same rank, they are ranked according to the
# second element ranking list variable, and so on for all element ranking
# variables.
#
#   bifusd_select_paths(<sel_paths_var>
#                        PATHS <path1> <path2> ...
#                        ROOT <path>
#                        [ELEMENT_RANK_VARS <var1> <var2> ...])
#
#   sel_paths_var     - The variable where the filtered and ranked paths are to
#                       be stored.
#   PATHS             - The list of path to be selected and ranked.
#   ROOT              - The root directory to be used as reference. Only path
#                       elements after the root directory are considered for
#                       selecting and ranking.
#   ELEMENT_RANK_VARS - The list of variables containing the list of path
#                       elements to be used for selecting and ranking. Each
#                       variable define one level of selection and ranking.
function(bifusd_select_paths sel_paths_var)
    set(options
        PATHS
        ROOT
        ELEMENT_RANK_VARS
    )
    bifusd_extract_options("${options}" ${ARGN})

    bifusd_filter_empty_vars(elem_rank_vars "${ELEMENT_RANK_VARS}")

    set(sel_paths)

    list(LENGTH elem_rank_vars num_rank_vars)
    if (num_rank_vars EQUAL 0)
        set(sel_paths ${PATHS})
    else()
        list(GET elem_rank_vars 0 rank_elems_var)
        foreach(rank_elem ${${rank_elems_var}})
            # Get paths from the input paths that have the given rank element
            set(rank_elem_sel_paths)
            foreach(path ${PATHS})
                file(RELATIVE_PATH rel_path ${ROOT} ${path})
                bifusd_split_path_elements(path_elems ${rel_path})

                if (rank_elem IN_LIST path_elems)
                    list(APPEND rank_elem_sel_paths ${path})
                endif()
            endforeach()

            if (num_rank_vars GREATER 1)
                list(SUBLIST elem_rank_vars 1 -1 sub_elem_rank_vars_list)
                bifusd_select_paths(rank_elem_sel_paths
                                     PATHS ${rank_elem_sel_paths}
                                     ROOT ${BIFUSD_TBB_DIR}
                                     ELEMENT_RANK_VARS ${sub_elem_rank_vars_list})
            endif()

            list(APPEND sel_paths ${rank_elem_sel_paths})
        endforeach()
    endif()

    set(${sel_paths_var} "${sel_paths}" PARENT_SCOPE)
endfunction(bifusd_select_paths)

# Rank a list of paths and select the path with the highest rank.
#
# Path are ranked using the variables in the element ranking list of variables,
# according to the description given in `bifusd_select_paths`.
#
#   bifusd_select_single_path(<sel_path_var>
#                              PATHS <path1> <path2> ...
#                              ROOT <path>
#                              [ELEMENT_RANK_VARS <var1> <var2> ...])
#
#   sel_path_var      - The variable where the path with the highest rank is to
#                       be stored.
#   PATHS             - The list of path to be ranked.
#   ROOT              - The root directory to be used as reference. Only path
#                       elements after the root directory are considered for
#                       ranking.
#   ELEMENT_RANK_VARS - The list of variables containing the list of path
#                       elements to be used for ranking. Each variable define
#                       one level of ranking.
function(bifusd_select_single_path sel_path_var)
    set(options
        PATHS
        ROOT
        ELEMENT_RANK_VARS
    )
    bifusd_extract_options("${options}" ${ARGN})

    set(sel_path)

    bifusd_select_paths(sel_paths
                         ${PATHS-ARGS}
                         ${ROOT-ARGS}
                         ${ELEMENT_RANK_VARS-ARGS})

    list(LENGTH sel_paths sel_paths_length)
    if (sel_paths_length GREATER 0)
        list(GET sel_paths 0 sel_path)
    endif()

    set(${sel_path_var} ${sel_path} PARENT_SCOPE)
endfunction(bifusd_select_single_path)

# Select a library with a given basename from the provided directory.
#
# Versioned libraries are also considered and versioned libraries with the
# largest version number will take precedence over libraries with lower version
# numbers or unversioned libraries.
#
#   bifusd_select_lib(<libpath_var>
#                      DIR <path>
#                      LIBNAME <name>
#                      [REQUIRED])
#
#   libpath_var - The variable where the found/selected library path must be
#                 stored.
#   DIR         - The directory to search for a library.
#   LIBNAME     - The library name to search.
#   REQUIRED    - Flag indicating whether a library must be found or not.
function(bifusd_select_lib libpath_var)
    set(options
        DIR
        LIBNAME
        REQUIRED
    )
    bifusd_extract_options("${options}" ${ARGN})

    file(GLOB versioned_3_libpaths ${DIR}/${LIBNAME}.???)
    file(GLOB versioned_2_libpaths ${DIR}/${LIBNAME}.??)
    file(GLOB versioned_1_libpaths ${DIR}/${LIBNAME}.?)
    file(GLOB libpaths             ${DIR}/${LIBNAME})

    list(SORT versioned_3_libpaths)
    list(SORT versioned_2_libpaths)
    list(SORT versioned_1_libpaths)

    list(REVERSE versioned_3_libpaths)
    list(REVERSE versioned_2_libpaths)
    list(REVERSE versioned_1_libpaths)

    set(all_libpaths ${versioned_3_libpaths}
                     ${versioned_2_libpaths}
                     ${versioned_1_libpaths}
                     ${libpaths})

    list(LENGTH all_libpaths all_libpaths_length)
    if (all_libpaths_length GREATER 0)
        list(GET all_libpaths 0 libpath)
    endif()

    if (REQUIRED-FOUND AND NOT libpath)
        message(FATAL_ERROR "Unable to locate library ${LIBNAME}, or any suitable version variant, in ${DIR}")
    endif()

    set(${libpath_var} ${libpath} PARENT_SCOPE)
endfunction(bifusd_select_lib)

# Select a shared library, versioned or unversioned, with a given basename from
# the provided directory.
#
#   bifusd_select_shared_lib(<sharedlib_var>
#                             DIR <path>
#                             LIBNAME <name>
#                             [REQUIRED])
#
#   sharedlib_var - The variable where the found shared library path must be
#                   stored.
#   DIR           - The directory to search for a shared library.
#   LIBNAME       - The shared library name to search.
#   REQUIRED      - Flag indicating whether a shared library must be found or
#                   not.
function(bifusd_select_shared_lib sharedlib_var)
    set(options
        DIR
        BASENAME
        REQUIRED
    )
    bifusd_extract_options("${options}" ${ARGN})

    set(shared_libname ${CMAKE_SHARED_LIBRARY_PREFIX}${BASENAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

    bifusd_select_lib(sharedlib
                       DIR ${DIR}
                       LIBNAME ${shared_libname}
                       ${REQUIRED-ARGS})

    set(${sharedlib_var} ${sharedlib} PARENT_SCOPE)
endfunction(bifusd_select_shared_lib)

# Select an import library, versioned or unversioned, with a given basename from
# the provided directory.
#
#   bifusd_select_import_lib(<importlib_var>
#                             DIR <path>
#                             LIBNAME <name>
#                             [REQUIRED])
#
#   importlib_var - The variable where the found import library path must be
#                   stored.
#   DIR           - The directory to search for a import library.
#   LIBNAME       - The import library name to search.
#   REQUIRED      - Flag indicating whether an import library must be found or
#                   not.
function(bifusd_select_import_lib importlib_var)
    set(options
        DIR
        BASENAME
        REQUIRED
    )
    bifusd_extract_options("${options}" ${ARGN})

    set(import_libname ${CMAKE_IMPORT_LIBRARY_PREFIX}${BASENAME}${CMAKE_IMPORT_LIBRARY_SUFFIX})

    bifusd_select_lib(importlib
                       DIR ${DIR}
                       LIBNAME ${import_libname}
                       ${REQUIRED-ARGS})

    set(${importlib_var} ${importlib} PARENT_SCOPE)
endfunction(bifusd_select_import_lib)

# Configure the imported shared library associated with the given target,
# optionally copying the shared library to the default location in the build
# directory.
#
#   bifusd_configure_target_imported_sharedlib(<target>
#                                               LIB <path>
#                                               [COPY])
#
#   <target> - The target to configure the imported shared library.
#   LIB      - The imported shared library to associated with the target.
#   COPY     - Flag indicating whether the shared library should be copied to
#              the default location in the build directory or not.
function(bifusd_configure_target_imported_sharedlib target)
    set(options
        LIB
        COPY
    )
    bifusd_extract_options("${options}" ${ARGN})

    # Resolve symlink
    get_filename_component(libpath "${LIB}" REALPATH)

    if (COPY-FOUND)
        get_filename_component(libname "${libpath}" NAME)

        if(BIFUSD_IS_WINDOWS)
            set(sharedlib "${BIFUSD_OUTPUT_BIN_DIR}/${libname}")
        else()
            set(sharedlib "${BIFUSD_OUTPUT_LIB_DIR}/${libname}")
        endif()

        # FIXME: We can't use `bifusd_copy_file` or `bifusd_configure_file`
        #        here. CMake doesn't set the RPATH of dependent test programs
        #        correctly if the file isn't copied as part of the CMake
        #        configuration process. This issue happens at least on OSX
        #        and we should figure out why.
        configure_file("${libpath}" "${sharedlib}" COPYONLY)
    else()
        set(sharedlib "${libpath}")
    endif()

    set_property(TARGET ${target} PROPERTY IMPORTED_LOCATION "${sharedlib}")
endfunction(bifusd_configure_target_imported_sharedlib)

# Configure the import library associated with the given target, optionally
# copying the import library to the default location in the build directory.
#
#   bifusd_configure_target_imported_implib(<target>
#                                            LIB <path>
#                                            [COPY])
#
#   <target> - The target to configure the import library.
#   LIB      - The import library to associated with the target.
#   COPY     - Flag indicating whether the import library should be copied to
#              the default location in the build directory or not.
function(bifusd_configure_target_imported_implib target)
    set(options
        LIB
        COPY
    )
    bifusd_extract_options("${options}" ${ARGN})

    # Resolve symlink
    get_filename_component(libpath "${LIB}" REALPATH)

    if (BIFUSD_IS_WINDOWS)
        if (COPY-FOUND)
            get_filename_component(libname "${libpath}" NAME)
            set(importlib "${BIFUSD_OUTPUT_LIB_DIR}/${libname}")

            # FIXME: We can't use `bifusd_copy_file` or `bifusd_configure_file`
            #        here. CMake doesn't set the RPATH of dependent test programs
            #        correctly if the file isn't copied as part of the CMake
            #        configuration process. This issue happens at least on OSX
            #        and we should figure out why.
            configure_file("${libpath}" "${importlib}" COPYONLY)
        else()
            set(importlib "${libpath}")
        endif()

        set_property(TARGET ${target} PROPERTY IMPORTED_IMPLIB "${importlib}")
    endif()
endfunction(bifusd_configure_target_imported_implib)

# Generate a shortened hash
#
# value - input string to hash
#
# returh_hash - the shortened hash
function(bifusd_short_string_hash return_hash value)
    string(SHA1 hash_value ${value})

    # A SHA1 hash is 40 hex digits. Assuming 64k unique possible values, using
    # just the first 11 digits should yield a reasonably low collision
    # probability of approximately p(n) = n^2/2m
    # = (2^16)^2 / 2(16^11) = 2^32 / 2^(4*11+1) = 2^32/2^45 = 2^-13 = 0.00012
    # (Where n=64k and m=16^11 - an HEX character has 2^4=16 possible values)
    # Ref.: https://en.wikipedia.org/wiki/Birthday_problem
    string(SUBSTRING ${hash_value} 0 11 short_hash)

    set (${return_hash} ${short_hash} PARENT_SCOPE)
endfunction(bifusd_short_string_hash)

# Generate a target name from a file name or path
#
# This function generate a unique target name from a given file name or path.
# The generated target name will be of the form:
#
#   <prefix>-<filename>-<hash_of_path>
#
# where the directories are built from the file path relative to either the
# source or binary directory.
#
#   bifusd_target_from_file(<return_target> <prefix> <file>)
#
# return_target  - Name of variable in which the generated target name will be
#                  returned.
# prefix         - Prefix prepended to the generate target name
# file           - File name or path for which to generate a target name
function(bifusd_target_from_file return_target prefix file)
    get_filename_component(filename ${file} NAME)

    # Using hash suffix to avoid file paths that are too long on Windows.
    bifusd_short_string_hash( suffix ${file})

    set(target "${prefix}-${filename}-${suffix}")

    set (${return_target} ${target} PARENT_SCOPE)
endfunction()

# Generate a unique file name
#
# This function generates a unique filename in the form
# <dir>/<prefix><random><ext>
# Note: This function only generates a filename, but does not create any file.
# It relies on CMake not being invoked in parallel.
#
#   return_filename - name of the variable in which the generated filename
#                     should be returned
#   DIR <dir>       - The directory where the file should be located.
#                     Defaults to ${CMAKE_CURRENT_BINARY_DIR}
#   EXT <ext>       - Suffix that should be used for the filename.
#                     Defaults to an empty string
#   PREFIX <prefix> - Prefix that should be used for the filename.
#                     Defaults to "tmp_"
#
function(bifusd_generate_unique_filename return_filename)
    set (options
        EXT
        DIR
        PREFIX
    )
    bifusd_extract_options("${options}" ${ARGN})

    if (NOT DIR-FOUND)
        set (DIR "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    if (NOT PREFIX-FOUND)
        set(PREFIX "tmp_")
    endif()

    bifusd_concat_path(base ${DIR} ${PREFIX})

    function(generate_candidate length res)
        string( RANDOM ALPHABET "abcdefghijklmnopqrstuvwxyz0123456789"
                LENGTH ${length} uniqueness)
        set(${res} ${base}${uniqueness}${EXT} PARENT_SCOPE)
    endfunction()

    set(uniqueness_length 5)
    generate_candidate(${uniqueness_length} candidate)
    while (EXISTS "${candidate}")
        # If there was a collision, make the names longer to make subsequent
        # collisions less likely. Not much point in making the string longer
        # than 10.
        if (${uniqueness_length} LESS 10)
            math(EXPR uniqueness_length "${uniqueness_length} + 1")
        endif()

        generate_candidate(${uniqueness_length} candidate)
    endwhile()

    set (${return_filename} "${candidate}" PARENT_SCOPE)
endfunction()

# Validates that list1 is included in list2
#
#   list1  list to be validated against list2
#   list2  the bigger list that should contain all the elements of list1
function(bifusd_validate_list1_is_included_in_list2 list1 list2)
    FOREACH(element ${${list1}})
        if(NOT ${element} IN_LIST ${list2})
            message(FATAL_ERROR
                "error: element ${element} of list1 is not included in list2:\n"
                "  list1: ${${list1}}\n"
                "  list2: ${${list2}}\n"
                )
        endif()
    ENDFOREACH(element)
endfunction()

# Add a target to the Bifusd internal targets folder
#
# Places the specified target in the folder given by ${BIFUSD_INTERNAL_TARGETS_FOLDER}
#
function(bifusd_add_to_internal_targets_folder target)
    if (BIFUSD_INTERNAL_TARGETS_FOLDER)
        set_property(TARGET ${target} PROPERTY FOLDER "${BIFUSD_INTERNAL_TARGETS_FOLDER}")
    endif()
endfunction()

# Copy a file at compile time
#
# This function creates a compile time target that will copy the source file to
# its destination at compile time. It is assumed that the destination file will
# be required by the dependent target.
#
#   bifusd_copy_file(<dependent_target> <src_file> <dst_file>)
#
# dependent_target - The target which requires the copied file
# src_file         - The path of the file to be copied
# dst_file         - The path where the file should be copied
function(bifusd_copy_file dependent_target src_file dst_file)
    if (NOT IS_ABSOLUTE ${src_file})
        set(src_file "${CMAKE_CURRENT_SOURCE_DIR}/${src_file}")
    endif()
    if (NOT IS_ABSOLUTE ${dst_file})
        set(dst_file "${CMAKE_CURRENT_BINARY_DIR}/${dst_file}")
    endif()

    # Get destination directory
    # If dst_file ends with a forward slash, that means it is a directory
    bifusd_ends_with_slash(${dst_file} ends_with_slash)
    if (ends_with_slash)
        # By adding the filename to the end, cmake will create the directory if needed
        get_filename_component(filename ${src_file} NAME)
        set(dst_file "${dst_file}${filename}")
    endif()

    # Build target name
    bifusd_target_from_file(file_target "cp" ${dst_file})

    if (TARGET ${file_target})
        message(STATUS "Target ${file_target} already exists. Skipping.")
    else()
        add_custom_command(
            COMMENT "Copy ${src_file} to ${dst_file}"
            OUTPUT  "${dst_file}" DEPENDS "${src_file}"
            COMMAND ${CMAKE_COMMAND} -E copy "${src_file}" "${dst_file}"
            VERBATIM)

        add_custom_target(${file_target} DEPENDS "${dst_file}")
        bifusd_add_to_internal_targets_folder(${file_target})

        add_dependencies(${dependent_target} ${file_target})
    endif()
endfunction()

# Normalize the paths in the given variable names.
#
#   The path contained in each one of the variable names passed as arguments is
#   resolved to its real path (symlinks are resolved) and converted to the
#   path representation of the native platform.
#
#   bifusd_normalize_paths(<var1> <var2> ...)
function(bifusd_normalize_paths)
    foreach(path_varname ${ARGN})
        get_filename_component(normalized_path "${${path_varname}}" REALPATH)
        file(TO_NATIVE_PATH "${normalized_path}" normalized_path)
        set(${path_varname} "${normalized_path}" PARENT_SCOPE)
    endforeach()
endfunction(bifusd_normalize_paths)

# Copy a file to another location at compile time while modifying its contents.
#
# Copies a file <src_file> to file <dst_file> and substitutes variable values
# referenced in the file content. This command replaces any variables in the
# input file referenced as ${VAR}.
#
# The variable definitions in the parameter VARS also accepts variable
# assignments in the format var=value, to temporarily override the current value
# of the variable.
#
#   bifusd_configure_file(<src_file> <dst_file>
#                        [DEPENDENT_TARGET dependent_target]
#                        [PREPROCESSOR_LINE]
#                        [VARS <var1> <var2> ...])
#
# src_file          - The path of the file to be copied
# dst_file          - The path where the file should be copied
# ALL               - Indicate that this target should be added to the
#                     default build target so that it will be run every time.
# DEPENDENT_TARGET  - The target which requires the configured file
# PREPROCESSOR_LINE - Add a #line directive pointing at the original source file
# VARS              - Variables to be replaced
function(bifusd_configure_file src_file dst_file)
    set(modes
        "ALL"
        "DEPENDENT_TARGET"
        "PREPROCESSOR_LINE"
        "VARS"
    )

    bifusd_extract_options("${modes}" ${ARGN})

    if (NOT IS_ABSOLUTE ${src_file})
        set(src_file "${CMAKE_CURRENT_SOURCE_DIR}/${src_file}")
    endif()
    if (NOT IS_ABSOLUTE ${dst_file})
        set(dst_file "${CMAKE_CURRENT_BINARY_DIR}/${dst_file}")
    endif()

    set(args "")
    if (PREPROCESSOR_LINE-FOUND)
        list(APPEND args "--preprocessor-line")
    endif()
    foreach(v ${VARS})
        string(FIND ${v} "=" pos)
        if (pos EQUAL -1)
            list(APPEND args "-D${v}=${${v}}")
        else()
            string(SUBSTRING "${v}" 0 ${pos} key)
            string(STRIP "${key}" key)

            math(EXPR start_value "${pos} + 1")
            string(SUBSTRING "${v}" ${start_value} -1 value)

            list(APPEND args "-D${key}=${value}")
        endif()
    endforeach()

    set(script "${BIFUSD_TOOLS_DIR}/configure-file.py")
    add_custom_command(
        COMMENT "Configuring ${dst_file}"
        OUTPUT  ${dst_file}
        DEPENDS ${src_file} ${script}
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMAND ${BIFUSD_PYTHON_EXECUTABLE} ${script} ${args} ${src_file} ${dst_file}
        VERBATIM)

    bifusd_target_from_file(file_target "cp" ${dst_file})

    add_custom_target(${file_target} ${ALL-ARGS} DEPENDS "${dst_file}")
    bifusd_add_to_internal_targets_folder(${file_target})

    if (DEPENDENT_TARGET-FOUND)
        add_dependencies(${DEPENDENT_TARGET} ${file_target})
    endif()

endfunction()

# Create an object library containing the version information for a target.
#
#   bifusd_version_object_lib(<target> <libname>
#                           [IS_SHARED_LIB "TRUE"/"FALSE"]
#
# target        - the target to be versioned.
# libname       - the name of the object library being defined.
# IS_SHARED_LIB - to indicate if the target is dynamic library or an executable
#                 (required for binary versioning).
function(bifusd_version_object_lib target libname)
    set(modes
        "IS_SHARED_LIB"
    )

    bifusd_extract_options("${modes}" ${ARGN})

    # Create a source file containing the version information.
    set(dumper_dst_file "${CMAKE_CURRENT_BINARY_DIR}/${target}VersionInfo.cpp")
    set(dumper_src_file "${BIFUSD_TOOLS_DIR}/src/versioning/VerDumper_src.cpp")

    set(BIFUSD_COMPONENT_NAME ${target})

    bifusd_configure_file(
        ${dumper_src_file} ${dumper_dst_file}
        PREPROCESSOR_LINE
        VARS BIFUSD_MAJOR_VERSION
             BIFUSD_MINOR_VERSION
             BIFUSD_PATCH_LEVEL
             BIFUSD_GIT_COMMIT
             BIFUSD_GIT_BRANCH
             BIFUSD_JENKINS_BUILD
             BIFUSD_PRODUCT_DATE
             BIFUSD_PRODUCT_NAME
             BIFUSD_COMPONENT_NAME
    )

    set(src_files "${dumper_dst_file}")

    if(BIFUSD_IS_WINDOWS)
        set(rc_file         "${CMAKE_CURRENT_BINARY_DIR}/${target}VersionResource.rc")
        set(version_rc_file "${BIFUSD_TOOLS_DIR}/src/versioning/VersionResource.rc")

        if(IS_SHARED_LIB)
            set(LIB_EXTENSION "dll")
            set(BIFUSD_FILE_TYPE "VFT_DLL // dynamic library")
        else()
            set(LIB_EXTENSION "exe")
            set(BIFUSD_FILE_TYPE "VFT_APP // executable")
        endif()

        if(BIFUSD_IS_DEBUG)
            set(BIFUSD_FILE_FLAGS "(VS_FF_DEBUG)")
        else()
            set(BIFUSD_FILE_FLAGS "0x0L")
        endif()

        set(BIFUSD_FILE_DESCRIPTION "\"${BIFUSD_COMPONENT_NAME}\"")
        set(BIFUSD_INTERNAL_NAME "\"${BIFUSD_COMPONENT_NAME}.${LIB_EXTENSION}\"")
        set(BIFUSD_ORIGINAL_FILENAME "\"${BIFUSD_COMPONENT_NAME}.${LIB_EXTENSION}\"")

        bifusd_configure_file(
            ${version_rc_file} ${rc_file}
            VARS BIFUSD_BUILD_ID_0
                 BIFUSD_BUILD_ID_1
                 BIFUSD_BUILD_VERSION
                 BIFUSD_COMPANY_NAME
                 BIFUSD_FILE_DESCRIPTION
                 BIFUSD_FILE_FLAGS
                 BIFUSD_FILE_TYPE
                 BIFUSD_INTERNAL_NAME
                 BIFUSD_LEGAL_COPYRIGHT
                 BIFUSD_LEGAL_TRADEMARKS1
                 BIFUSD_LEGAL_TRADEMARKS2
                 BIFUSD_MAJOR_VERSION
                 BIFUSD_MINOR_VERSION
                 BIFUSD_ORIGINAL_FILENAME
                 BIFUSD_PATCH_LEVEL
                 BIFUSD_PRODUCT_DATE
                 BIFUSD_PRODUCT_NAME
            )
        list(APPEND src_files "${rc_file}")
    endif()

    # Put the compile source files in an object library.
    add_library(${libname} OBJECT ${src_files})

    # For config/CfgWarningMacros.h and versioning/*.h
    target_include_directories(
        ${libname}
        # Make sure that the installed headers are included in priority.
        PRIVATE $<BUILD_INTERFACE:${BIFUSD_OUTPUT_INCLUDE_DIR}>
                $<BUILD_INTERFACE:${BIFUSD_TOOLS_DIR}>
        )

    if (NOT BIFUSD_CONFIG_TARGET OR  NOT BIFUSD_PACKAGE_NAME)
        message(FATAL_ERROR
            "The variable BIFUSD_PACKAGE_NAME must be set and the variable BIFUSD_CONFIG_TARGET must be set to the name of a valid BifusdConfig target. Either add the \${BIFUSD_TOOLS_DIR}/config subdirectory or import a package already containing BifusdConfig and manually set BIFUSD_CONFIG_TARGET to the name of the imported BifusdConfigtarget.
            ")
    elseif( NOT TARGET "${BIFUSD_CONFIG_TARGET}" OR NOT TARGET "${BIFUSD_PACKAGE_NAME}Versioning")
        if( NOT TARGET "${BIFUSD_CONFIG_TARGET}" )
            message( STATUS "Variable ${BIFUSD_CONFIG_TARGET} must be a target")
        endif()
        if( NOT TARGET "${BIFUSD_PACKAGE_NAME}Versioning" )
            message( STATUS "Variable ${BIFUSD_PACKAGE_NAME}Versioning must be a target")
        endif()
        message(FATAL_ERROR "Missing target(s)" )      
    endif()
    
    bifusd_propagate_target_requirements(
        ${libname} PRIVATE_LINK_LIBS ${BIFUSD_CONFIG_TARGET} ${BIFUSD_PACKAGE_NAME}Versioning)

    if (BIFUSD_IS_GCC_52)
        # This compile flag doesn't seem to disabled from code, so we need
        # to set the proper flag when compiling the object target.
        target_compile_options(${libname} PRIVATE "-Wno-unused-variable")
    endif()

endfunction(bifusd_version_object_lib)


# Propagate target usage requirements
#
# According to CMake documentation, target usage requirements should be
# automatically transitively propagated. See:
#
# http://www.cmake.org/cmake/help/v3.1/manual/cmake-buildsystem.7.html#transitive-usage-requirements
#
# This seems to work well for static libraries, shared libraries and
# executables. Unfortunately, it seems to fail miserably when interface
# libraries, imported libraries and object libraries are thrown into the
# mix. To work around these problems, this function re-implements the
# transitive propagation of target requirements.
#
#
#   bifusd_propagate_target_requirements(
#                           <target>
#                           [PUBLIC_INCLUDE_DIRS  dir1 dir2 ...]
#                           [PRIVATE_INCLUDE_DIRS dir1 dir2 ...]
#                           [PUBLIC_LINK_LIBS     lib1 lib2 ...]
#                           [PRIVATE_LINK_LIBS    lib1 lib2 ...]
#                           [PUBLIC_OBJECT_LIBS   lib1 lib2 ...]
#                           [PRIVATE_OBJECT_LIBS  lib1 lib2 ...])
#
#   target               - name of the target for target usage requirements are
#                          being propagated.
#   PUBLIC_INCLUDE_DIRS  - set the mode to append to the list of public include
#                          directories used by the target source files.
#   PRIVATE_INCLUDE_DIRS - set the mode to append to the list of private include
#                          directories used by the target source files.
#   PUBLIC_LINK_LIBS     - set the mode to append to the list of public libraries
#                          to be linked with the target.
#   PRIVATE_LINK_LIBS    - set the mode to append to the list of private
#                          libraries to be linked with the target.
#   PUBLIC_OBJECT_LIBS   - set the mode to append to the list of object libraries
#                          to be included publicly with the target.
#   PRIVATE_OBJECT_LIBS  - set the mode to append to the list of object libraries
#                          to be included privately with the target.
function(bifusd_propagate_target_requirements target)

    set(modes
        "PUBLIC_INCLUDE_DIRS"
        "PRIVATE_INCLUDE_DIRS"
        "PUBLIC_LINK_LIBS"
        "PRIVATE_LINK_LIBS"
        "PUBLIC_OBJECT_LIBS"
        "PRIVATE_OBJECT_LIBS"
    )

    bifusd_extract_options("${modes}" ${ARGN})

    #
    # Compute include directories
    #
    set(public_incdirs  ${PUBLIC_INCLUDE_DIRS})
    set(private_incdirs ${PRIVATE_INCLUDE_DIRS})

    foreach(lib ${PUBLIC_LINK_LIBS} ${PUBLIC_OBJECT_LIBS})
        get_target_property(imported ${lib} IMPORTED)
        bifusd_append_target_property(public_system_incdirs ${lib} INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
        if(imported)
            # Imported libraries are always system headers
            bifusd_append_target_property(public_system_incdirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)
        else()
            bifusd_append_target_property(public_incdirs        ${lib} INTERFACE_INCLUDE_DIRECTORIES)
        endif()
    endforeach(lib)

    foreach(lib ${PRIVATE_LINK_LIBS} ${PRIVATE_OBJECT_LIBS})
        get_target_property(imported ${lib} IMPORTED)
        bifusd_append_target_property(private_system_incdirs ${lib} INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
        if(imported)
            # Imported libraries are always system headers
            bifusd_append_target_property(private_system_incdirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)
        else()
            bifusd_append_target_property(private_incdirs        ${lib} INTERFACE_INCLUDE_DIRECTORIES)
        endif()
    endforeach(lib)

    bifusd_sort_include_dirs(public_system_incdirs)
    bifusd_sort_include_dirs(public_incdirs)
    bifusd_sort_include_dirs(private_system_incdirs)
    bifusd_sort_include_dirs(private_incdirs)

    bifusd_list_remove_items(public_incdirs         ${public_system_incdirs})
    bifusd_list_remove_items(private_system_incdirs ${public_system_incdirs}
                                                     ${public_incdirs})
    bifusd_list_remove_items(private_incdirs        ${public_system_incdirs}
                                                     ${public_incdirs}
                                                     ${private_system_incdirs})

    #
    # Compute library dependencies
    #
    set(public_libs)  # Create an empty local variable!
    set(private_libs) # Create an empty local variable!

    foreach(lib ${PUBLIC_LINK_LIBS})
        list(APPEND public_libs ${lib})

        get_target_property(type ${lib} TYPE)
        if(type STREQUAL INTERFACE_LIBRARY)
            # Interface library target, also use its dependencies instead.
            bifusd_append_target_property(public_libs  ${lib} INTERFACE_LINK_LIBRARIES)
        endif()
    endforeach(lib)

    foreach(olib ${PUBLIC_OBJECT_LIBS})
        bifusd_append_target_property(public_libs  ${olib} INTERFACE_LINK_LIBRARIES)
        bifusd_append_target_property(private_libs ${olib} LINK_LIBRARIES)
    endforeach(olib)

    foreach(lib ${PRIVATE_LINK_LIBS})
        list(APPEND private_libs ${lib})

        get_target_property(type ${lib} TYPE)
        if(type STREQUAL INTERFACE_LIBRARY)
            # Interface library target, use its dependencies instead.
            bifusd_append_target_property(private_libs  ${lib} INTERFACE_LINK_LIBRARIES)
        endif()
    endforeach(lib)

    foreach(olib ${PRIVATE_OBJECT_LIBS})
        bifusd_append_target_property(private_libs ${olib} INTERFACE_LINK_LIBRARIES)
        bifusd_append_target_property(private_libs ${olib} LINK_LIBRARIES)
    endforeach(olib)

    bifusd_sort_unique(public_libs)
    bifusd_sort_unique(private_libs)

    bifusd_list_remove_items(private_libs ${public_libs})

    #
    # Set target properties
    #
    get_target_property(type ${target} TYPE)
    get_target_property(imported ${target} IMPORTED)

    # message(STATUS "${target} TYPE                   = ${type}")
    # message(STATUS "${target} IMPORTED               = ${imported}")
    # message(STATUS "${target} PUBLIC_LINK_LIBS       = ${PUBLIC_LINK_LIBS}")
    # message(STATUS "${target} PRIVATE_LINK_LIBS      = ${PRIVATE_LINK_LIBS}")
    # message(STATUS "${target} PUBLIC_OBJECT_LIBS     = ${PUBLIC_OBJECT_LIBS}")
    # message(STATUS "${target} PRIVATE_LINK_LIBS      = ${PRIVATE_OBJECT_LIBS}")

    # message(STATUS "${target} public_system_incdirs  = ${public_system_incdirs}")
    # message(STATUS "${target} private_system_incdirs = ${private_system_incdirs}")
    # message(STATUS "${target} public_incdirs         = ${public_incdirs}")
    # message(STATUS "${target} private_incdirs        = ${private_incdirs}")
    # message(STATUS "${target} public_libs            = ${public_libs}")
    # message(STATUS "${target} private_libs           = ${private_libs}")

    if(type STREQUAL INTERFACE_LIBRARY)
        if(public_system_incdirs)
            target_include_directories(${target} SYSTEM INTERFACE ${public_system_incdirs})
        endif()
        if(public_incdirs)
            target_include_directories(${target}        INTERFACE ${public_incdirs})
        endif()

        # CMake does not allow us to set target link libraries on interface
        # libraries. We therefore manually set the LINK_LIBRARIES
        # properties.
        if(public_libs)
            set_target_properties(${target} PROPERTIES INTERFACE_LINK_LIBRARIES
                "${public_libs}")
        endif()
    elseif (NOT ${imported})
        if(public_system_incdirs)
            target_include_directories(${target} SYSTEM PUBLIC    ${public_system_incdirs})
        endif()
        if(private_system_incdirs)
            target_include_directories(${target} SYSTEM PRIVATE   ${private_system_incdirs})
        endif()
        if(public_incdirs)
            target_include_directories(${target}        PUBLIC    ${public_incdirs})
        endif()
        if(private_incdirs)
            target_include_directories(${target}        PRIVATE   ${private_incdirs})
        endif()

        if(type STREQUAL OBJECT_LIBRARY)
            # CMake does not allow us to set target link libraries on object
            # libraries. We therefore manually set the LINK_LIBRARIES
            # properties.
            if(public_libs)
                set_target_properties(${target} PROPERTIES INTERFACE_LINK_LIBRARIES
                    "${public_libs}")
            endif()
            set(all_libs ${public_libs} ${private_libs})
            bifusd_sort_unique(all_libs)
            if(all_libs)
                set_target_properties(${target} PROPERTIES LINK_LIBRARIES
                    "${all_libs}")
            endif()
        else()
            if(public_libs)
                target_link_libraries(${target} PUBLIC    ${public_libs})
            endif()
            if(private_libs)
                target_link_libraries(${target} PRIVATE   ${private_libs})
            endif()
        endif()
    else()
        # CMake does not allow us to set target link libraries on imported
        # libraries. We therefore manually set the INTERFACE properties...
        # properties.
        if(public_system_incdirs)
            set_target_properties(${target}
                PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${public_system_incdirs}")
        endif()
        if(public_incdirs)
            set_target_properties(${target}
                PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${public_incdirs}")
        endif()
        if(public_libs)
            set_target_properties(${target}
                PROPERTIES INTERFACE_LINK_LIBRARIES "${public_libs}")
        endif()
    endif()

endfunction(bifusd_propagate_target_requirements)


# Configure and create an interface library.
#
#   bifusd_create_interface_lib(<interface_lib_target>
#                           [HEADER_ONLY_FILES    hdr1 hdr2 ...]
#                           [PUBLIC_DEFINITIONS   def1 def2 ...]
#                           [PUBLIC_OPTIONS       opt1 opt2 ...]
#                           [PUBLIC_INCLUDE_DIRS  dir1 dir2 ...]
#                           [PUBLIC_LINK_LIBS     lib1 lib2 ...])
#
#   interface_lib_target  - name of the interface library target.
#   HEADER_ONLY_FILES    - list of headers to compile.
#   PUBLIC_DEFINITIONS   - Additional definitions to be set when compiling the
#                          source files.
#   PUBLIC_OPTIONS       - Additional compiler options to be set when compiling
#                          the source files.
#   PUBLIC_INCLUDE_DIRS  - set the mode to append to the list of public include
#                          directories used by the interface library source files.
#   PUBLIC_LINK_LIBS     - set the mode to append to the list of public libraries
#                          to be linked with the unit test.

function(bifusd_create_interface_lib  interface_lib_target)
    set(modes
        "HEADER_ONLY_FILES"
        "PUBLIC_DEFINITIONS"
        "PUBLIC_OPTIONS"
        "PUBLIC_INCLUDE_DIRS"
        "PUBLIC_LINK_LIBS"
        "NOEXPORT"
    )

    bifusd_extract_options("${modes}" ${ARGN})

    # Header only files are using the same options as the library
    bifusd_compile_header_only_files(
        SRC_FILES    ${HEADER_ONLY_FILES}
        DEFINITIONS  ${PUBLIC_DEFINITIONS}
        OPTIONS      ${PUBLIC_OPTIONS}
        INCLUDE_DIRS ${PUBLIC_INCLUDE_DIRS}
        LINK_LIBS    ${PUBLIC_LINK_LIBS})

    add_library(${interface_lib_target} INTERFACE)
    target_compile_definitions(${interface_lib_target} INTERFACE ${PUBLIC_DEFINITIONS})
    target_compile_options(    ${interface_lib_target} INTERFACE ${PUBLIC_OPTIONS})

    # Export interface libraries for other projects to use.
    # (Unless specified noexport).
    if (NOT NOEXPORT-FOUND)
        install(
            TARGETS ${interface_lib_target}
            EXPORT  ${BIFUSD_PACKAGE_NAME})
    endif()

    # CMake does not allow us to set target link libraries on interface
    # libraries. We therefore have to manually compute the include directories
    # that are transitively required.
    bifusd_propagate_target_requirements(
        ${interface_lib_target}
        PUBLIC_INCLUDE_DIRS  ${PUBLIC_INCLUDE_DIRS}
        PUBLIC_LINK_LIBS     ${PUBLIC_LINK_LIBS})

endfunction(bifusd_create_interface_lib)


# Configure and create an object library.
#
#   bifusd_create_object_lib(<object_lib_target>
#                           [SRC_FILES            file1 file2 ...]
#                           [HEADER_ONLY_FILES    hdr1 hdr2 ...]
#                           [PUBLIC_DEFINITIONS   def1 def2 ...]
#                           [PRIVATE_DEFINITIONS  def1 def2 ...]
#                           [PUBLIC_OPTIONS       opt1 opt2 ...]
#                           [PRIVATE_OPTIONS      opt1 opt2 ...]
#                           [PUBLIC_INCLUDE_DIRS  dir1 dir2 ...]
#                           [PRIVATE_INCLUDE_DIRS dir1 dir2 ...]
#                           [PUBLIC_LINK_LIBS     lib1 lib2 ...]
#                           [PRIVATE_LINK_LIBS    lib1 lib2 ...]
#
#   object_lib_target    - name of the object library target.
#   SRC_FILES            - set the mode to append to the list of source files to
#                          be added to the object library.
#   HEADER_ONLY_FILES    - list of headers to compile.
#   PUBLIC_DEFINITIONS   - Additional definitions to be set when compiling the
#                          source files.
#   PRIVATE_DEFINITIONS  - Additional definitions to be set when compiling the
#                          source files.
#   PUBLIC_OPTIONS       - Additional compiler options to be set when compiling the
#                          source files.
#   PRIVATE_OPTIONS      - Additional compiler options to be set when compiling the
#                          source files.
#   PUBLIC_INCLUDE_DIRS  - set the mode to append to the list of public include
#                          directories used by the object library source files.
#   PRIVATE_INCLUDE_DIRS - set the mode to append to the list of private include
#                          directories used by the object library source files.
#   PUBLIC_LINK_LIBS     - set the mode to append to the list of public libraries
#                          to be linked with the unit test.
#   PRIVATE_LINK_LIBS    - set the mode to append to the list of private
#                          libraries to be linked with the unit test.

function(bifusd_create_object_lib  object_lib_target)
    set(modes
        "SRC_FILES"
        "HEADER_ONLY_FILES"
        "PUBLIC_DEFINITIONS"
        "PRIVATE_DEFINITIONS"
        "PUBLIC_OPTIONS"
        "PRIVATE_OPTIONS"
        "PRIVATE_INCLUDE_DIRS"
        "PUBLIC_INCLUDE_DIRS"
        "PRIVATE_LINK_LIBS"
        "PUBLIC_LINK_LIBS"
    )

    bifusd_extract_options("${modes}" ${ARGN})

    # Header only files are using the same options as the library
    bifusd_compile_header_only_files(
        SRC_FILES    ${HEADER_ONLY_FILES}
        DEFINITIONS  ${PUBLIC_DEFINITIONS}  ${PRIVATE_DEFINITIONS}
        OPTIONS      ${PUBLIC_OPTIONS}      ${PRIVATE_OPTIONS}
        INCLUDE_DIRS ${PUBLIC_INCLUDE_DIRS} ${PRIVATE_INCLUDE_DIRS}
        LINK_LIBS    ${PUBLIC_LINK_LIBS}    ${PRIVATE_LINK_LIBS})

    add_library(${object_lib_target} OBJECT ${SRC_FILES})
    target_compile_definitions(${object_lib_target} PUBLIC  ${PUBLIC_DEFINITIONS})
    target_compile_definitions(${object_lib_target} PRIVATE ${PRIVATE_DEFINITIONS})
    target_compile_options(    ${object_lib_target} PUBLIC  ${PUBLIC_OPTIONS})
    target_compile_options(    ${object_lib_target} PRIVATE ${PRIVATE_OPTIONS})

    # CMake does not allow us to set target link libraries on object
    # libraries. We therefore have to manually compute the include directories
    # that are transitively required.
    bifusd_propagate_target_requirements(
        ${object_lib_target}
        PUBLIC_INCLUDE_DIRS  ${PUBLIC_INCLUDE_DIRS}
        PRIVATE_INCLUDE_DIRS ${PRIVATE_INCLUDE_DIRS}
        PUBLIC_LINK_LIBS     ${PUBLIC_LINK_LIBS}
        PRIVATE_LINK_LIBS    ${PRIVATE_LINK_LIBS})

endfunction(bifusd_create_object_lib)


# Configure the rpaths of the given target.
#
# Assume that the default dependencies are located in the same directory as the
# target. The platform specific variable will be automatically prepended to each
# one of the given paths: `$ORIGIN` for linux and `@loader_path` for OSX.
#
#   bifusd_configure_rpath(target <dir1> <dir2> ...)
#
#   target - The target to configure the rpaths.
#   <dirs> - The list of directories to add to the target's rpath.
function(bifusd_configure_rpath target)
    # Set an explicit rpath on Linux and OSX.
    # Assume that the default dependencies are located in the same directory.
    bifusd_platform_select(rpath LINUX "$ORIGIN"
                                  OSX   "@loader_path/.")

    # Append the extra rpaths
    foreach(path ${ARGN})
        bifusd_platform_select(extra_rpath LINUX "$ORIGIN/${path}"
                                            OSX   "@loader_path/${path}")

        list(APPEND rpath "${extra_rpath}")
    endforeach()

    set_target_properties(${target} PROPERTIES INSTALL_RPATH "${rpath}")
endfunction(bifusd_configure_rpath)


# Configure and create a static library.
#
#   bifusd_create_static_lib(<static_lib_target>
#                           [SRC_FILES            file1 file2 ...]
#                           [HEADER_ONLY_FILES    hdr1 hdr2 ...]
#                           [PUBLIC_DEFINITIONS   def1 def2 ...]
#                           [PRIVATE_DEFINITIONS  def1 def2 ...]
#                           [PUBLIC_OPTIONS       opt1 opt2 ...]
#                           [PRIVATE_OPTIONS      opt1 opt2 ...]
#                           [PUBLIC_INCLUDE_DIRS  dir1 dir2 ...]
#                           [PRIVATE_INCLUDE_DIRS dir1 dir2 ...]
#                           [PUBLIC_LINK_LIBS     lib1 lib2 ...]
#                           [PRIVATE_LINK_LIBS    lib1 lib2 ...]
#                           [PUBLIC_OBJECT_LIBS   lib1 lib2 ...]
#                           [PRIVATE_OBJECT_LIBS  lib1 lib2 ...]
#
#   static_lib_target      - name of the static library target.
#   HEADER_ONLY_FILES      - list of headers to compile.
#   SRC_FILES              - set the mode to append to the list of source files to
#                            be added to the static library.
#   PUBLIC_DEFINITIONS     - Additional definitions to be set when compiling the
#                            source files.
#   PRIVATE_DEFINITIONS    - Additional definitions to be set when compiling the
#                            source files.
#   PUBLIC_OPTIONS         - Additional compiler options to be set when compiling the
#                            source files.
#   PRIVATE_OPTIONS        - Additional compiler options to be set when compiling the
#                            source files.
#   PRIVATE_INCLUDE_DIRS   - set the mode to append to the list of private include
#                            directories used by the static library source files.
#   PUBLIC_INCLUDE_DIRS    - set the mode to append to the list of public include
#                            directories used by the static library source files.
#   PRIVATE_LINK_LIBS      - set the mode to append to the list of private
#                            libraries to be linked with the static library.
#   PUBLIC_LINK_LIBS       - set the mode to append to the list of public libraries
#                            to be linked with the static library.
#   PRIVATE_OBJECT_LIBS    - set the mode to append to the list of object libraries
#                            to be included privately with library.
#   PUBLIC_OBJECT_LIBS     - set the mode to append to the list of object libraries
#                            to be included publicly with the library.
#   PUBLIC_INSTALL         - set the mode to install the library in the product
#                            distribution.
#   CUSTOM_OUTPUT_BIN_DIR  - set when a custom output bin directory is needed other
#                            than the ones provided for a public install.
#   CUSTOM_OUTPUT_LIB_DIR  - set when a custom output lib directory is needed other
#                            than the ones provided for a public install.
#   CUSTOM_INSTALL_BIN_DIR - set when a custom install bin directory is needed other
#                            than the ones provided for a public install.
#   CUSTOM_INSTALL_LIB_DIR - set when a custom install lib directory is needed other
#                            than the ones provided for a public install.
function(bifusd_create_static_lib  static_lib_target)
    set(modes
        "SRC_FILES"
        "HEADER_ONLY_FILES"
        "PUBLIC_DEFINITIONS"
        "PRIVATE_DEFINITIONS"
        "PUBLIC_OPTIONS"
        "PRIVATE_OPTIONS"
        "PRIVATE_INCLUDE_DIRS"
        "PUBLIC_INCLUDE_DIRS"
        "PRIVATE_LINK_LIBS"
        "PUBLIC_LINK_LIBS"
        "PRIVATE_OBJECT_LIBS"
        "PUBLIC_OBJECT_LIBS"
        "PUBLIC_INSTALL"
        "CUSTOM_OUTPUT_BIN_DIR"
        "CUSTOM_OUTPUT_LIB_DIR"
        "CUSTOM_INSTALL_BIN_DIR"
        "CUSTOM_INSTALL_LIB_DIR"
    )

    bifusd_extract_options("${modes}" ${ARGN})

    # Header only files are using the same options as the library
    bifusd_compile_header_only_files(
        SRC_FILES    ${HEADER_ONLY_FILES}
        DEFINITIONS  ${PUBLIC_DEFINITIONS}  ${PRIVATE_DEFINITIONS}
        OPTIONS      ${PUBLIC_OPTIONS}      ${PRIVATE_OPTIONS}
        INCLUDE_DIRS ${PUBLIC_INCLUDE_DIRS} ${PRIVATE_INCLUDE_DIRS}
        LINK_LIBS    ${PUBLIC_LINK_LIBS}    ${PRIVATE_LINK_LIBS})

    # Add the objects of object libraries to the list of our sources.
    foreach(olib ${PRIVATE_OBJECT_LIBS} ${PUBLIC_OBJECT_LIBS})
        list(APPEND obj_files $<TARGET_OBJECTS:${olib}>)
    endforeach()
    set(src_files ${SRC_FILES} ${obj_files})

    add_library(${static_lib_target} STATIC ${src_files})
    target_compile_definitions(${static_lib_target} PUBLIC  ${PUBLIC_DEFINITIONS})
    target_compile_definitions(${static_lib_target} PRIVATE ${PRIVATE_DEFINITIONS})
    target_compile_options(    ${static_lib_target} PUBLIC  ${PUBLIC_OPTIONS})
    target_compile_options(    ${static_lib_target} PRIVATE ${PRIVATE_OPTIONS})

    # CMake does not allow us to set target link libraries using object
    # libraries. We therefore have to manually compute the include directories
    # that are transitively required.
    bifusd_propagate_target_requirements(
        ${static_lib_target}
        PUBLIC_INCLUDE_DIRS  ${PUBLIC_INCLUDE_DIRS}
        PRIVATE_INCLUDE_DIRS ${PRIVATE_INCLUDE_DIRS}
        PUBLIC_LINK_LIBS     ${PUBLIC_LINK_LIBS}
        PRIVATE_LINK_LIBS    ${PRIVATE_LINK_LIBS}
        PUBLIC_OBJECT_LIBS   ${PUBLIC_OBJECT_LIBS}
        PRIVATE_OBJECT_LIBS  ${PRIVATE_OBJECT_LIBS})

    if(${PUBLIC_INSTALL-FOUND} OR
            ${CUSTOM_OUTPUT_BIN_DIR-FOUND} OR
            ${CUSTOM_OUTPUT_LIB_DIR-FOUND} OR
            ${CUSTOM_INSTALL_BIN_DIR-FOUND} OR
            ${CUSTOM_INSTALL_LIB_DIR-FOUND})

        if(${PUBLIC_INSTALL-FOUND})
            set(output_bin_dir ${BIFUSD_OUTPUT_BIN_DIR})
            set(output_lib_dir ${BIFUSD_OUTPUT_LIB_DIR})

            set(install_bin_dir ${BIFUSD_INSTALL_BIN_DIR})
            set(install_lib_dir ${BIFUSD_INSTALL_LIB_DIR})
        endif()

        # Set with custom paths if any

        if(${CUSTOM_OUTPUT_BIN_DIR-FOUND})
            set(output_bin_dir ${CUSTOM_OUTPUT_BIN_DIR})
        endif()

        if(${CUSTOM_OUTPUT_LIB_DIR-FOUND})
            set(output_lib_dir ${CUSTOM_OUTPUT_LIB_DIR})
        endif()

        if(${CUSTOM_INSTALL_BIN_DIR-FOUND})
            set(install_bin_dir ${CUSTOM_INSTALL_BIN_DIR})
        endif()

        if(${CUSTOM_INSTALL_LIB_DIR-FOUND})
            set(install_lib_dir ${CUSTOM_INSTALL_LIB_DIR})
        endif()

        bifusd_configure_rpath(${static_lib_target})

        # MSDev visualizers will not work if there is any symbol in the
        # library name that could be interpreted as a mathematical operator
        # sadly this includes the "-" we like to use to separate the
        # library name from the version. To help developers write and use
        # visualizers, we will use an underscore instead on Windows.
        # See: https://connect.microsoft.com/VisualStudio/feedback/details/3074995
        #
        # NOTE: We might want to enable this to version our static libraries...
        #
        # set_target_properties(${static_lib_target}
        #                       PROPERTIES VERSION   "${BIFUSD_VERSION}")
        set(static_lib_output_name
            "${static_lib_target}_${BIFUSD_MAJOR_VERSION}_${BIFUSD_MINOR_VERSION}")
        set_target_properties(${static_lib_target}
            PROPERTIES OUTPUT_NAME ${static_lib_output_name})

        #
        # Put installable static library targets in the common output
        # directory.
        #
        set_target_properties(${static_lib_target} PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${output_bin_dir})

        # Export static libraries for other projects to use.
        install(
            TARGETS ${static_lib_target}
            EXPORT  ${BIFUSD_PACKAGE_NAME}
            ARCHIVE DESTINATION ${install_lib_dir})
    endif()

endfunction(bifusd_create_static_lib)

# Configure and create a shared library.
#
#   bifusd_create_shared_lib(<shared_lib_target>
#                             <export_definition>
#                             [SRC_FILES            file1 file2 ...]
#                             [LIB_PREFIX           string       ]
#                             [LIB_SUFFIX           string       ]
#                             [HEADER_ONLY_FILES    hdr1 hdr2 ...]
#                             [PUBLIC_DEFINITIONS   def1 def2 ...]
#                             [PRIVATE_DEFINITIONS  def1 def2 ...]
#                             [PUBLIC_OPTIONS       opt1 opt2 ...]
#                             [PRIVATE_OPTIONS      opt1 opt2 ...]
#                             [PUBLIC_INCLUDE_DIRS  dir1 dir2 ...]
#                             [PRIVATE_INCLUDE_DIRS dir1 dir2 ...]
#                             [PUBLIC_LINK_LIBS     lib1 lib2 ...]
#                             [PRIVATE_LINK_LIBS    lib1 lib2 ...]
#                             [PUBLIC_OBJECT_LIBS   lib1 lib2 ...]
#                             [PRIVATE_OBJECT_LIBS  lib1 lib2 ...]
#                             [EXTRA_RPATH          dir1 dir2 ...]
#                             [PUBLIC_INSTALL]
#                             [PRIVATE_INSTALL]
#                             [CUSTOM_OUTPUT_BIN_DIR]
#                             [CUSTOM_OUTPUT_LIB_DIR]
#                             [CUSTOM_INSTALL_BIN_DIR]
#                             [CUSTOM_INSTALL_LIB_DIR]
#                             [NO_VERSION_SUFFIX])
#                             [LIB_VERSION_SUFFIX   string]
#                             )
#
#   shared_lib_target      - name of the shared library target.
#   export_definition      - Define used to indicate the exported symbols in the
#                            shared library.
#   SRC_FILES              - set the mode to append to the list of source files to
#                            be added to the shared library.
#   LIB_PREFIX             - Prefix of the library name (default "lib" on *nix).
#                            The default value is correct. Change it only for special
#                            occasions, like when building a Python module.
#   LIB_SUFFIX             - Extension of the library name. The default value is correct.
#                            Change it only for special occasions, like when building a
#                            Python module.
#   HEADER_ONLY_FILES      - list of headers to compile.
#   PUBLIC_DEFINITIONS     - Additional definitions to be set when compiling the
#                            source files.
#   PRIVATE_DEFINITIONS    - Additional definitions to be set when compiling the
#                            source files.
#   PUBLIC_OPTIONS         - Additional compiler options to be set when compiling the
#                            source files.
#   PRIVATE_OPTIONS        - Additional compiler options to be set when compiling the
#                            source files.
#   PUBLIC_INCLUDE_DIRS    - set the mode to append to the list of public include
#                            directories used by the shared library source files.
#   PRIVATE_INCLUDE_DIRS   - set the mode to append to the list of private include
#                            directories used by the shared library source files.
#   PUBLIC_LINK_LIBS       - set the mode to append to the list of public libraries
#                            to be linked with the shared library.
#   PRIVATE_LINK_LIBS      - set the mode to append to the list of private
#                            libraries to be linked with the shared library.
#   PUBLIC_OBJECT_LIBS     - set the mode to append to the list of object libraries
#                            to be included publicly with the library.
#   PRIVATE_OBJECT_LIBS    - set the mode to append to the list of object libraries
#                            to be included privately with library.
#   EXTRA_RPATH            - Additional directories to add the rpath search.
#                            The platform specific variable will be
#                            automatically prepended to each paths. $ORIGIN/
#                            for linux and @loader_path/ for OSX.
#   PUBLIC_INSTALL         - set the mode to install the library in the product
#                            distribution.
#   PRIVATE_INSTALL        - set the mode to install the library in the product
#                            distribution. The .lib file is not installed on
#                            Windows to indicate clearly that customers should
#                            not attempt to link directly against these private
#                            shared libraries.
#   CUSTOM_OUTPUT_BIN_DIR  - set when a custom output bin directory is needed other
#                            than the ones provided for a public or private install.
#   CUSTOM_OUTPUT_LIB_DIR  - set when a custom output lib directory is needed other
#                            than the ones provided for a public or private install.
#   CUSTOM_INSTALL_BIN_DIR - set when a custom install bin directory is needed other
#                            than the ones provided for a public or private install.
#   CUSTOM_INSTALL_LIB_DIR - set when a custom install lib directory is needed other
#                            than the ones provided for a public or private install.
#   NO_VERSION_SUFFIX      - Do not add the version information to the shared library
#                            name.
#   LIB_VERSION_SUFFIX     - Override the internal shared library version suffix
#                            with the input one. The default value is:
#                            "_${BIFUSD_MAJOR_VERSION}_${BIFUSD_MINOR_VERSION}"
function(bifusd_create_shared_lib  shared_lib_target export_definition)
    set(modes
        "SRC_FILES"
        "LIB_PREFIX"
        "LIB_SUFFIX"
        "HEADER_ONLY_FILES"
        "PUBLIC_DEFINITIONS"
        "PRIVATE_DEFINITIONS"
        "PUBLIC_OPTIONS"
        "PRIVATE_OPTIONS"
        "PUBLIC_INCLUDE_DIRS"
        "PRIVATE_INCLUDE_DIRS"
        "PUBLIC_LINK_LIBS"
        "PRIVATE_LINK_LIBS"
        "PUBLIC_OBJECT_LIBS"
        "PRIVATE_OBJECT_LIBS"
        "EXTRA_RPATH"
        "PUBLIC_INSTALL"
        "PRIVATE_INSTALL"
        "CUSTOM_OUTPUT_BIN_DIR"
        "CUSTOM_OUTPUT_LIB_DIR"
        "CUSTOM_INSTALL_BIN_DIR"
        "CUSTOM_INSTALL_LIB_DIR"
        "NO_VERSION_SUFFIX"
        "LIB_VERSION_SUFFIX"
    )

    bifusd_extract_options("${modes}" ${ARGN})

    # Header only files are using the same options as the library
    bifusd_compile_header_only_files(
        SRC_FILES    ${HEADER_ONLY_FILES}
        DEFINITIONS  ${PUBLIC_DEFINITIONS}  ${PRIVATE_DEFINITIONS}
        OPTIONS      ${PUBLIC_OPTIONS}      ${PRIVATE_OPTIONS}
        INCLUDE_DIRS ${PUBLIC_INCLUDE_DIRS} ${PRIVATE_INCLUDE_DIRS}
        LINK_LIBS    ${PUBLIC_LINK_LIBS}    ${PRIVATE_LINK_LIBS})

    # Shared libraries are versioned.
    bifusd_version_object_lib(${shared_lib_target} ${shared_lib_target}VersionInfo
                             IS_SHARED_LIB "TRUE")

    # Add the objects of object libraries to the list of our sources.
    set(src_files ${SRC_FILES} $<TARGET_OBJECTS:${shared_lib_target}VersionInfo>)
    foreach(olib ${PRIVATE_OBJECT_LIBS} ${PUBLIC_OBJECT_LIBS})
        list(APPEND src_files $<TARGET_OBJECTS:${olib}>)
    endforeach()
    add_library(${shared_lib_target} SHARED ${src_files})

    # The code path in USD's pxr/base/vt/hashmap.h (and hashset.h) is not the
    # same on Linux when Clang is used instead of GCC.
    # To use the same code path for Clang and GCC (the GCC code path) it is
    # suggested to set the following compiler definitions flags
    #
    # Ref.: https://groups.google.com/g/usd-interest/c/_fRLYwkt328
    #
    # https://github.com/PixarAnimationStudios/USD/issues/1057 and
    # https://github.com/PixarAnimationStudios/USD/issues/1291 are issues related to this.
    # I've used a workaround of defining ARCH_HAS_GNU_STL_EXTENSIONS,
    # along with _GLIBCXX_PERMIT_BACKWARD_HASH to silence the deprecation warnings produced.
    # I haven't encountered any other issues, but our official release builds are all GCC
    # so this combination has only been tested locally by a few developers in our case.

    set(public_defs ${PUBLIC_DEFINITIONS})
    if(UNIX AND NOT APPLE AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
        list( APPEND public_defs ARCH_HAS_GNU_STL_EXTENSIONS _GLIBCXX_PERMIT_BACKWARD_HASH)
    endif()

    target_compile_definitions(${shared_lib_target} PRIVATE ${export_definition})
    target_compile_definitions(${shared_lib_target} PUBLIC  ${public_defs})
    target_compile_definitions(${shared_lib_target} PRIVATE ${PRIVATE_DEFINITIONS})
    target_compile_options(    ${shared_lib_target} PUBLIC  ${PUBLIC_OPTIONS})
    target_compile_options(    ${shared_lib_target} PRIVATE ${PRIVATE_OPTIONS})

    bifusd_propagate_target_requirements(
        ${shared_lib_target}
        PUBLIC_INCLUDE_DIRS  ${PUBLIC_INCLUDE_DIRS}
        PRIVATE_INCLUDE_DIRS ${PRIVATE_INCLUDE_DIRS}
        PUBLIC_LINK_LIBS     ${PUBLIC_LINK_LIBS}
        PRIVATE_LINK_LIBS    ${PRIVATE_LINK_LIBS}
        PUBLIC_OBJECT_LIBS   ${PUBLIC_OBJECT_LIBS}
        PRIVATE_OBJECT_LIBS  ${PRIVATE_OBJECT_LIBS} ${shared_lib_target}VersionInfo)

    if(${LIB_PREFIX-FOUND})
        set_target_properties(${shared_lib_target} PROPERTIES PREFIX "${LIB_PREFIX}")
    endif()

    if(${LIB_SUFFIX-FOUND})
        set_target_properties(${shared_lib_target} PROPERTIES SUFFIX "${LIB_SUFFIX}")
    endif()

    if(${PUBLIC_INSTALL-FOUND} OR ${PRIVATE_INSTALL-FOUND} OR
            ${CUSTOM_OUTPUT_BIN_DIR-FOUND} OR
            ${CUSTOM_OUTPUT_LIB_DIR-FOUND} OR
            ${CUSTOM_INSTALL_BIN_DIR-FOUND} OR
            ${CUSTOM_INSTALL_LIB_DIR-FOUND})

        if(${PUBLIC_INSTALL-FOUND} OR ${PRIVATE_INSTALL-FOUND})
            set(output_bin_dir ${BIFUSD_OUTPUT_BIN_DIR})
            set(output_lib_dir ${BIFUSD_OUTPUT_LIB_DIR})

            set(install_bin_dir ${BIFUSD_INSTALL_BIN_DIR})
            set(install_lib_dir ${BIFUSD_INSTALL_LIB_DIR})
        endif()

        # Set with custom paths if any

        if(${CUSTOM_OUTPUT_BIN_DIR-FOUND})
            set(output_bin_dir ${CUSTOM_OUTPUT_BIN_DIR})
        endif()

        if(${CUSTOM_OUTPUT_LIB_DIR-FOUND})
            set(output_lib_dir ${CUSTOM_OUTPUT_LIB_DIR})
        endif()

        if(${CUSTOM_INSTALL_BIN_DIR-FOUND})
            set(install_bin_dir ${CUSTOM_INSTALL_BIN_DIR})
        endif()

        if(${CUSTOM_INSTALL_LIB_DIR-FOUND})
            set(install_lib_dir ${CUSTOM_INSTALL_LIB_DIR})
        endif()

        bifusd_configure_rpath(${shared_lib_target} ${EXTRA_RPATH})

        # MSDev visualizers will not work if there is any symbol in the
        # library name that could be interpreted as a mathematical operator
        # sadly this includes the "-" we like to use to separate the
        # library name from the version. To help developers write and use
        # visualizers, we will use an underscore instead on Windows.
        # See: https://connect.microsoft.com/VisualStudio/feedback/details/3074995
        #
        # NOTE: We might want to enable this to version our shared libraries...
        #
        # set_target_properties(${shared_lib_target}
        #                       PROPERTIES VERSION   "${BIFUSD_VERSION}")
        if (${NO_VERSION_SUFFIX-FOUND})
            set(shared_lib_version_suffix "")
        elseif (${LIB_VERSION_SUFFIX-FOUND})
            set(shared_lib_version_suffix ${LIB_VERSION_SUFFIX})
        else()
            set(shared_lib_version_suffix "_${BIFUSD_MAJOR_VERSION}_${BIFUSD_MINOR_VERSION}")
        endif()

        set(shared_lib_output_name "${shared_lib_target}${shared_lib_version_suffix}")
        set_target_properties(${shared_lib_target}
            PROPERTIES OUTPUT_NAME ${shared_lib_output_name})

        #
        # Put installable shared library targets in the common output
        # directory.
        #
        if(BIFUSD_IS_WINDOWS)
            # .dll files
            set_target_properties(${shared_lib_target} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${output_bin_dir})
            #.pdb files
            set_target_properties(${shared_lib_target} PROPERTIES
                PDB_OUTPUT_DIRECTORY     ${output_bin_dir})
            # .lib files
            set_target_properties(${shared_lib_target} PROPERTIES
                ARCHIVE_OUTPUT_DIRECTORY ${output_lib_dir})
        else()
            # .dylib or .so files...
            set_target_properties(${shared_lib_target} PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY ${output_lib_dir})
        endif()

        # Install the shared libraries as requested
        install(
            TARGETS  ${shared_lib_target}
            EXPORT   ${BIFUSD_PACKAGE_NAME}
            RUNTIME DESTINATION ${install_bin_dir}
            LIBRARY DESTINATION ${install_lib_dir}
            ARCHIVE DESTINATION ${install_lib_dir})

        if(BIFUSD_HAS_DEBUG_FILES)
            # Set variables to take custom LIB_PREFIX/LIB_SUFFIX into account
            set(shared_lib_prefix ${CMAKE_SHARED_LIBRARY_PREFIX})
            if(${LIB_PREFIX-FOUND})
                set(shared_lib_prefix ${LIB_PREFIX})
            endif()

            set(shared_lib_suffix ${CMAKE_SHARED_LIBRARY_SUFFIX})
            if(${LIB_SUFFIX-FOUND})
                set(shared_lib_suffix ${LIB_SUFFIX})
            endif()

            if (BIFUSD_IS_WINDOWS)
                install(FILES ${output_bin_dir}/${shared_lib_prefix}${shared_lib_output_name}.pdb
                    DESTINATION ${install_bin_dir})
                if (${BIFUSD_USE_DEBUG_FASTLINK})
                    set(pdb_file "${CMAKE_INSTALL_PREFIX}/${install_bin_dir}/${shared_lib_prefix}${shared_lib_output_name}.pdb")
                    install(CODE "execute_process(COMMAND cmd /c \"${BIFUSD_WINDOWS_MSPDBCMF}\" /nologo ${pdb_file})")
                endif()
            endif()

            if(BIFUSD_IS_OSX)
                # Debugging symbols
                set(shared_lib_file "${shared_lib_prefix}${shared_lib_output_name}${shared_lib_suffix}")
                set(dsymfile        "${shared_lib_file}.dSYM")

                set(dsymtarget      "${output_lib_dir}/${dsymfile}")

                # List the full content so that everything can be cleaned
                # properly. The listing is in reverse directory order so that
                # the directory is empty when erased (else an error is
                # emitted).
                set(dsymfiles-list
                    "Contents/Resources/DWARF/${shared_lib_file}"
                    "Contents/Info.plist"
                    )
                set(dsymdirs-list
                    "Contents/Resources/DWARF"
                    "Contents/Resources"
                    "Contents"
                    )
                foreach(f ${dsymfiles-list} ${dsymdirs-list})
                    list(APPEND dsymcontent "${dsymtarget}/${f}")
                endforeach()

                add_custom_command(
                    OUTPUT ${dsymcontent} ${dsymtarget}
                    DEPENDS ${shared_lib_target}
                    COMMAND ${BIFUSD_OSX_DSYMUTIL} $<TARGET_FILE:${shared_lib_target}> -o ${dsymtarget}
                    COMMENT "Generating debugging symbols ${dsymfile} for ${shared_lib_target}"
                    VERBATIM)
                add_custom_target(${shared_lib_target}_dsym DEPENDS ${dsymtarget})
                bifusd_add_to_internal_targets_folder(${shared_lib_target}_dsym)

                add_dependencies(bifusd-dsym ${shared_lib_target}_dsym)

                # Installation of the debugging symbols
                foreach(f ${dsymfiles-list})
                    set(src "${dsymtarget}/${f}")
                    set(dst "${install_lib_dir}/${dsymfile}/${f}")
                    get_filename_component(dstdir ${dst} DIRECTORY)
                    install(FILES ${src} DESTINATION ${dstdir})
                endforeach()
            endif()
        endif()
    endif()

endfunction(bifusd_create_shared_lib)

# Configure and create an executable.
#
#   bifusd_create_executable(<exec_target>
#                             [SRC_FILES     file1 file2 ...]
#                             [DEFINITIONS   def1 def2 ...]
#                             [OPTIONS       opt1 opt2 ...]
#                             [INCLUDE_DIRS  dir1 dir2 ...]
#                             [LINK_LIBS     lib1 lib2 ...]
#                             [OBJECT_LIBS   obj1 obj2 ...]
#                             [INSTALL]
#                             [CUSTOM_OUTPUT_BIN_DIR dir]
#                             [CUSTOM_INSTALL_BIN_DIR dir]
#                             [CUSTOM_EXECUTABLE_NAME name])
#
#   exec_target            - name of the executable target.
#   SRC_FILES              - set the mode to append to the list of source files to
#                            be added to the executable.
#   DEFINITIONS            - Additional definitions to be set when compiling the
#                            source files.
#   OPTIONS                - Additional compiler options to be set when compiling
#                            the source files.
#   INCLUDE_DIRS           - set the mode to append to the list of include
#                            directories used by the executable source file.
#   LINK_LIBS              - set the mode to append to the list of libraries to be
#                            linked with the executable.
#   OBJECT_LIBS            - set the mode to append to the list of object libraries
#                            to be linked with the executable.
#   INSTALL                - set the mode to install the executable in the product
#                            distribution.
#   EXTRA_RPATH            - Additional directories to add the rpath search.
#                            The platform specific variable will be
#                            automatically prepended to each paths. $ORIGIN/
#                            for linux and @loader_path/ for OSX.
#   CUSTOM_OUTPUT_BIN_DIR  - set when a custom output bin directory is needed other
#                            than the ones provided.
#   CUSTOM_INSTALL_BIN_DIR - set when a custom install bin directory is needed other
#                            than the ones provided.
#   CUSTOM_EXECUTABLE_NAME - set when a executable name is needed other
#                            than the ones provided.
function(bifusd_create_executable exec_target)
    set(modes
        "SRC_FILES"
        "DEFINITIONS"
        "OPTIONS"
        "INCLUDE_DIRS"
        "LINK_LIBS"
        "OBJECT_LIBS"
        "INSTALL"
        "EXTRA_RPATH"
        "CUSTOM_OUTPUT_BIN_DIR"
        "CUSTOM_INSTALL_BIN_DIR"
        "CUSTOM_EXECUTABLE_NAME"
    )

    bifusd_extract_options("${modes}" ${ARGN})

    bifusd_version_object_lib(${exec_target} ${exec_target}VersionInfo
                             IS_SHARED_LIB "FALSE")

    set(src_files ${SRC_FILES} $<TARGET_OBJECTS:${exec_target}VersionInfo>)
    foreach(olib ${OBJECT_LIBS})
        list(APPEND srcfiles $<TARGET_OBJECTS:${olib}>)
    endforeach()
    add_executable(${exec_target} ${src_files})
    target_compile_definitions(${exec_target} PRIVATE ${DEFINITIONS})
    target_compile_options(${exec_target} PRIVATE ${OPTIONS})

    bifusd_propagate_target_requirements(
        ${exec_target}
        PRIVATE_INCLUDE_DIRS ${INCLUDE_DIRS}
        PRIVATE_LINK_LIBS    ${LINK_LIBS})

    if(${INSTALL-FOUND})
        bifusd_install_executable(
            ${exec_target}
            ${EXTRA_RPATH-ARGS}
            ${CUSTOM_OUTPUT_BIN_DIR-ARGS}
            ${CUSTOM_INSTALL_BIN_DIR-ARGS}
            ${CUSTOM_EXECUTABLE_NAME-ARGS}
        )
    endif()
endfunction(bifusd_create_executable)

# Install an executable.
#
#   bifusd_install_executable(<exec_target>
#                              [CUSTOM_OUTPUT_BIN_DIR dir]
#                              [CUSTOM_INSTALL_BIN_DIR dir]
#                              [CUSTOM_EXECUTABLE_NAME name])
#
#   exec_target            - name of the executable target.
#   EXTRA_RPATH            - Additional directories to add the rpath search.
#                            The platform specific variable will be
#                            automatically prepended to each paths. $ORIGIN/
#                            for linux and @loader_path/ for OSX.
#   CUSTOM_OUTPUT_BIN_DIR  - set when a custom output bin directory is needed other
#                            than the ones provided.
#   CUSTOM_INSTALL_BIN_DIR - set when a custom install bin directory is needed other
#                            than the ones provided.
#   CUSTOM_EXECUTABLE_NAME - set when a executable name is needed other
#                            than the ones provided.
function(bifusd_install_executable exec_target)
    set(modes
        "EXTRA_RPATH"
        "CUSTOM_OUTPUT_BIN_DIR"
        "CUSTOM_INSTALL_BIN_DIR"
        "CUSTOM_EXECUTABLE_NAME"
    )

    bifusd_extract_options("${modes}" ${ARGN})

    # Set with custom paths if any

    if(${CUSTOM_OUTPUT_BIN_DIR-FOUND})
        set(output_bin_dir ${CUSTOM_OUTPUT_BIN_DIR})
    else()
        set(output_bin_dir ${BIFUSD_OUTPUT_BIN_DIR})
    endif()

    if(${CUSTOM_INSTALL_BIN_DIR-FOUND})
        set(install_bin_dir ${CUSTOM_INSTALL_BIN_DIR})
    else()
        set(install_bin_dir ${BIFUSD_INSTALL_BIN_DIR})
    endif()

    if(${CUSTOM_EXECUTABLE_NAME-FOUND})
        set(exec_name ${CUSTOM_EXECUTABLE_NAME})
    else()
        set(exec_name ${exec_target})
    endif()

    # Add an explicit extra rpath.
    # Assume that shared library dependencies are located in a directory named
    # `lib` located beside the `bin` directory in which the executable resides.
    set(extra_rpath "../lib"
                    ${EXTRA_RPATH})

    bifusd_configure_rpath(${exec_target} ${extra_rpath})

    # NOTE: We might want to enable this to version our executables...
    #
    # set_target_properties(${exec_target}
    #                       PROPERTIES VERSION   "${BIFUSD_VERSION}")

    #
    # Put installable shared library targets in the common output
    # directory.
    #
    set_target_properties(${exec_target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${output_bin_dir})
    if(BIFUSD_IS_WINDOWS)
        set_target_properties(${exec_target} PROPERTIES
            PDB_OUTPUT_DIRECTORY ${output_bin_dir})
    endif()

    # Export the executables for other projects to use
    install(
        TARGETS  ${exec_target}
        EXPORT   ${BIFUSD_PACKAGE_NAME}
        RUNTIME DESTINATION ${install_bin_dir})

    if (BIFUSD_HAS_DEBUG_FILES)
        if (BIFUSD_IS_WINDOWS)
            install(FILES ${output_bin_dir}/${exec_name}.pdb
                    DESTINATION ${install_bin_dir})

            if (${BIFUSD_USE_DEBUG_FASTLINK})
                set(pdb_file "${CMAKE_INSTALL_PREFIX}/${install_bin_dir}/${exec_name}.pdb")
                install(CODE "execute_process(COMMAND cmd /c \"${BIFUSD_WINDOWS_MSPDBCMF}\" /nologo ${pdb_file})")
            endif()
        endif()

        if (BIFUSD_IS_OSX)
            # Debugging symbols
            set(exec_file  "${exec_name}${CMAKE_EXECUTABLE_SUFFIX}")
            set(dsymfile   "${exec_file}.dSYM")
            set(dsymtarget "${output_bin_dir}/${dsymfile}")

            # List the full content so that everything can be cleaned
            # properly. The listing is in reverse directory order so that
            # the directory is empty when erased (else an error is
            # emitted).
            set(dsymfiles_list
                "Contents/Resources/DWARF/${exec_file}"
                "Contents/Info.plist"
                )
            set(dsymdirs_list
                "Contents/Resources/DWARF"
                "Contents/Resources"
                "Contents"
                )
            foreach(f ${dsymfiles_list} ${dsymdirs_list})
                list(APPEND dsymcontent "${dsymtarget}/${f}")
            endforeach()

            add_custom_command(
                OUTPUT ${dsymcontent} ${dsymtarget}
                DEPENDS ${exec_target}
                COMMAND ${BIFUSD_OSX_DSYMUTIL} $<TARGET_FILE:${exec_target}> -o ${dsymtarget}
                COMMENT "Generating debugging symbols ${dsymfile} for ${exec_target}"
                VERBATIM)
            add_custom_target(${exec_target}_dsym DEPENDS ${dsymtarget})
            bifusd_add_to_internal_targets_folder(${exec_target}_dsym)

            add_dependencies(bifusd-dsym ${exec_target}_dsym)

            # Installation of the debugging symbols
            foreach(f ${dsymfiles_list})
                set(src "${dsymtarget}/${f}")
                set(dst "${install_bin_dir}/${dsymfile}/${f}")
                get_filename_component(dstdir ${dst} DIRECTORY)

                install(FILES ${src} DESTINATION ${dstdir})
            endforeach()
        endif()
    endif()
endfunction(bifusd_install_executable)


define_property(GLOBAL PROPERTY BIFUSD_DOXYGEN_EXCLUDE
    BRIEF_DOCS "List of extra doxygen input files to exclude"
    FULL_DOCS "List of extra doxygen input files to exclude")

define_property(GLOBAL PROPERTY BIFUSD_DOXYGEN_INCLUDE
    BRIEF_DOCS "List of extra doxygen input files to include"
    FULL_DOCS "List of extra doxygen input files to include")

# Configure a list of header files to be installed in the distributable package,
# as well as copy these files to the common include directory.
#
#   bifusd_install_headers(target
#                           FILES                file1 file2 ...
#                           [HEADER_ONLY_FILES   hdr1 hdr2 ...]
#                           [SOURCE              dir]
#                           [DESTINATION         dir]
#                           [PUBLIC_LINK_LIBS    lib1 lib2 ...])
#
#   target            - Name of the interface library for these headers
#   FILES             - The list of header files to be installed.
#   HEADER_ONLY_FILES - List of installed headers to compile.
#   SOURCE            - The subdirectory where the list of files are being copied from.
#   DESTINATION       - The subdirectory where the files are going to be copied to.
#   PUBLIC_LINK_LIBS  - set the mode to append to the list of public libraries

function(bifusd_install_headers target)

    set(modes
        "FILES"
        "HEADER_ONLY_FILES"
        "SOURCE"
        "DESTINATION"
        "PUBLIC_LINK_LIBS"
    )

    bifusd_extract_options("${modes}" ${ARGN})

    set(dest_dir "")
    if (DESTINATION-FOUND)
        set(dest_dir "${DESTINATION}/")
    endif()

    set(source_dir "")
    if (SOURCE-FOUND)
        set(source_dir "${SOURCE}/")
    endif()

    bifusd_create_interface_lib(${target}
        PUBLIC_INCLUDE_DIRS $<BUILD_INTERFACE:${BIFUSD_OUTPUT_INCLUDE_DIR}>
                            $<INSTALL_INTERFACE:${BIFUSD_INSTALL_INCLUDE_DIR}>
        PUBLIC_LINK_LIBS    ${PUBLIC_LINK_LIBS})

    foreach(header_file ${FILES})
        set(src_file "${source_dir}${header_file}")
        set(dst_output_file  "${BIFUSD_OUTPUT_INCLUDE_DIR}/${dest_dir}${header_file}")
        set(dst_install_file "${BIFUSD_INSTALL_INCLUDE_DIR}/${dest_dir}${header_file}")
        get_filename_component(dst_install_dir "${dst_install_file}" DIRECTORY)

        bifusd_configure_file(
            "${src_file}" "${dst_output_file}"
            DEPENDENT_TARGET ${target}
            PREPROCESSOR_LINE)

        install(FILES "${src_file}" DESTINATION "${dst_install_dir}")

        # Explicitly add a dependency. That sesms to be only necessary to support
        # custom commands which depends on the installed headers. Most likely a
        # limitation/bug of CMake.
        add_dependencies(${target} ${dst_output_file})

        # Append the source to the list of files that doxygen should skip. It
        # will be processed from the BIFUSD_OUTPUT_INCLUDE_DIR instead.
        if(NOT IS_ABSOLUTE "${src_file}")
            get_filename_component(src_file "${src_file}" ABSOLUTE)
        endif()
        set_property(GLOBAL APPEND PROPERTY BIFUSD_DOXYGEN_EXCLUDE "${src_file}")
        set_property(GLOBAL APPEND PROPERTY BIFUSD_DOXYGEN_INCLUDE "${dst_output_file}")
    endforeach()

    #
    # Compile all installed header files. Installed headers must compile
    # without any other include than BIFUSD_OUTPUT_INCLUDE_DIR.
    #
    unset(hdr_only_files)
    foreach(hdr_only_file ${HEADER_ONLY_FILES})
        list(APPEND hdr_only_files
            ${BIFUSD_OUTPUT_INCLUDE_DIR}/${DESTINATION}/${hdr_only_file})
    endforeach()

    bifusd_compile_header_only_files(
        SRC_FILES    ${hdr_only_files}
        INCLUDE_DIRS ${BIFUSD_OUTPUT_INCLUDE_DIR}
        LINK_LIBS    ${target} ${PUBLIC_LINK_LIBS})

endfunction(bifusd_install_headers)

#==============================================================================
# Unit test helper functions
#==============================================================================

# Generate the unit test target name based on a given prefix and a basename.
#
#   get_unittest_target(<unittest_target>
#                       <unittest_prefix>
#                       <unittest_basename>)
#
# <unittest_target>   - The variable name where the unit test target names should
#                       be returned.
# <unittest_prefix>   - The component prefix to be added to the unit test target
#                       name.
# <unittest_basename> - The basename for the unit test. If the basename is a
#                       filename, only the filename without extension is used
#                       as the unit test basename.

function(get_unittest_target unittest_target  unittest_prefix  unittest_basename)
    get_filename_component(unittest_name ${unittest_basename} NAME_WE)

    if (unittest_prefix)
        set(unittest_name "${unittest_prefix}-${unittest_name}")
    endif()

    set(${unittest_target} "${unittest_name}" PARENT_SCOPE)
endfunction(get_unittest_target)

# Append new environment variables to a unit test target.
#
#   bifusd_append_test_env_vars(<target> <env_vars>)
#
#       <target>   - Unit test target to append new environment variables.
#       <env_vars> - The new environment variables to append.

function(bifusd_append_test_env_vars  target env_vars)
    get_test_property(${target} ENVIRONMENT curr_env_vars)

    # Prepend the environment variables already set for the target
    if (curr_env_vars)
        set(env_vars "${curr_env_vars};${env_vars}")
    endif()

    set_tests_properties(${target} PROPERTIES ENVIRONMENT "${env_vars}")
endfunction(bifusd_append_test_env_vars)

# Append new path environment variable to a unit test target.
#
#   bifusd_append_test_path_env_var(<target> <env_var_name> dir1 dir2 ...)
#
#       <target>       - Unit test target to append new path environment
#                        variable.
#       <env_var_name> - The new path environment variable to append.
#       dir1 dir2 ...  - The directories that constitute the new path.

function(bifusd_append_test_path_env_var unittest_target env_var_name)
    if (BIFUSD_IS_WINDOWS)
        # On windows we have to escape the semicolons, otherwise only
        # the first path entry will be passed to the test executable
        STRING(REPLACE ";" "\\;" env_var_path "${ARGN}")
    elseif (BIFUSD_IS_UNIX)
        STRING(REPLACE ";" ":" env_var_path "${ARGN}")
    else()
        message(FATAL_ERROR "Unsupported platform.")
    endif()

    bifusd_append_test_env_vars(${unittest_target} "${env_var_name}=${env_var_path}")
endfunction(bifusd_append_test_path_env_var)

# Set the shared library paths for a given test
#
#   bifusd_set_test_shared_library_path(test_target dir1 dir2 ...)
#
#       test_target   - The test target for which the shared library path is
#                       being specified.
#       dir1 dir2 ... - The directories where shared libraries can be found.

function(bifusd_set_test_shared_library_path unittest_target)
    if (BIFUSD_IS_WINDOWS)
        # On Windows, use PATH
        bifusd_append_test_path_env_var(${unittest_target} "PATH" ${ARGN})
    elseif (BIFUSD_IS_OSX)
        # On OSX, use DYLD_LIBRARY_PATH
        bifusd_append_test_path_env_var(${unittest_target} "DYLD_LIBRARY_PATH" ${ARGN})
    elseif (BIFUSD_IS_LINUX)
        # On Linux, use LD_LIBRARY_PATH
        bifusd_append_test_path_env_var(${unittest_target} "LD_LIBRARY_PATH" ${ARGN})
    else()
        message(FATAL_ERROR "Unsupported platform.")
    endif()
endfunction(bifusd_set_test_shared_library_path)


# Set the shared library paths for a given test
#
#   bifusd_configure_test_shared_library_path(target dir1 dir2 ...)
#
#   target        - The test target for which the shared library path is
#                   being specified.
#   dir1 dir2 ... - Directories where shared libraries can be found.

function(bifusd_configure_test_shared_library_path test_target)
    if(BIFUSD_IS_WINDOWS)
        # The directory containing the Amino libraries is always
        # included in the search path on Windows. It is not necessary
        # on UNIX; rpaths are taking care of this. shared_lib_dirs
        # should only contain extra library directory that are
        # specific to the given test.
        bifusd_set_test_shared_library_path(
            ${test_target}
            ${ARGN} ${BIFUSD_OUTPUT_BIN_DIR})
    elseif(BIFUSD_IS_LINUX)
        # On Linux, we have to forward the LD_LIBRARY_PATH so that Python can
        # run correctly under scl enable shells...
        string(REPLACE ":" ";" python_shared_lib_dirs "$ENV{LD_LIBRARY_PATH}")
        bifusd_set_test_shared_library_path(
            ${test_target}
            ${ARGN} ${python_shared_lib_dirs})
    else()
        bifusd_set_test_shared_library_path(${test_target} ${ARGN})
    endif()
endfunction(bifusd_configure_test_shared_library_path)


# Generic function to configure a unit test.
#
#   bifusd_configure_unitest(<unitest_target>
#                             [SRC_FILES       src1 src2 ...]
#                             [COMMAND         executable arg1 arg2 ...]
#                             [WORKING_DIRECTORY dir]
#                             [INCLUDE_DIRS    dir1 dir2 ...]
#                             [LINK_LIBS       lib1 lib2 ...]
#                             [OBJECT_LIBS     obj1 obj2 ...]
#                             [SHARED_LIB_DIRS dir1 dir2 ...]
#                             [DEFINITIONS     def1 def2 ...]
#                             [ENV_VARS        env1 env2 ...]
#                             [EXPECTED_FILE   expected_file]
#                             [EXPECTED_VARS   var1 var2 ...]
#                             [FILECHECK       expected_file]
#                             [FILECHECK_ARGS  arg1 arg2 ...]
#                             [STDERR]
#                             [WILL_FAIL]
#                             [SERIAL]
#                             [PASS_REGEX      regex]
#                             [FAIL_REGEX      regex]
#                             [LABELS          lbl1 lbl2 ...]
#                             [CATEGORIES      cat1 cat2 ...]
#                             [REQUIRE_EXTERNAL_TOOL]
#                             [DEPENDS         dep1 dep2 ...]
#
#   unittest_target        - The unit test target name.
#   SRC_FILES              - Set the mode to append to the list of source files
#                            compiled into the unit test. (Optional)
#   COMMAND                - Set the mode to the command used to run the unit test.
#                            Useful for tests that requires command line arguments.
#                            (Optional)
#   WORKING_DIRECTORY      - Execute the command with the given current working
#                            directory. If it is a relative path it will be
#                            interpreted relative to the build tree directory
#                            corresponding to the current source directory.
#   INCLUDE_DIRS           - Set the mode to append to the list of include directories
#                            used by the unit test.
#   LINK_LIBS              - Set the mode to append to the list of libraries to be linked
#                            with the unit test.
#   OBJECT_LIBS            - Set the mode to append to the list of object libraries to be
#                            linked with the executable.
#   SHARED_LIB_DIRS        - Set the mode to append to the list of directories where
#                            the shared libraries used by unit test are located. The
#                            directory containing the Amino libraries is always
#                            included. SHARED_LIB_DIRS should only contain extra
#                            library directory that are specific to the given test.
#   DEFINITIONS            - Additional definitions to be set when compiling the source files.
#   OPTIONS                - Additional compiler options to be set when compiling
#                            the unit test source files.
#   ENV_VARS               - Environment variables to set when running the unit test.
#   EXPECTED_FILE          - The output of the test (stdout, and optionally
#                            stderr, or an EXPECTED_OUTFILE) must match exactly
#                            the content of the expected file for the test to
#                            pass.
#   EXPECTED_OUTFILE       - Specify the name of an output file generated by
#                            the test program which should be used as the
#                            source of the comparison against the expected
#                            file. This output file is used instead of stdout
#                            which is used by default.
#   EXPECTED_VARS          - Variables to be replaced in the expected file before
#                            performing the comparison.
#   EXPECTED_IGNORE_PATTERNS - Regular expressions describing sub-strings which
#                            should be ignored when comparing the expected file
#                            with the output of the test command. Any
#                            sub-string matching the patterns are erased from
#                            both the expected file and the output before
#                            making the comparison. The rest of a line
#                            containing an ignore pattern must still match for
#                            the comparison to succeed.
#   FILECHECK              - The expected output that will be verified using the LLVM
#                            FileCheck utility.
#   FILECHECK_ARGS         - Extra command-line argument to pass to filecheck.
#   STDERR                 - Include stderr in the expected output.
#   WILL_FAIL              - Invert the pass/fail flag of the test. This property can be
#                            used for tests that are expected to fail and return a non zero
#                            return code.
#   SERIAL                 - Force the unit test to be run sequentially.
#                            Note: This option should be used mainly when unit tests are
#                                  *designed* to be run sequentially, regardless of the
#                                  number of parallel jobs specified in CTest.
#   PASS_REGEX             - The expected regular expression from the test in case of
#                            success.
#   FAIL_REGEX             - The expected regular expression from the test in case of
#                            failure.
#   LABELS                 - The labels for the unit tests. Uses BIFUSD_SMOKE_TEST if
#                            unset.
#   CATEGORIES             - The categories for the unit tests.
#   DEPENDS                - The tests that must be run before this one.
#   REQUIRE_EXTERNAL_TOOLS - The unit test makes use of some external tool in order
#                            to successfully run.

function(bifusd_configure_unittest  unittest_target)
    set(modes
        "SRC_FILES"
        "COMMAND"
        "WORKING_DIRECTORY"
        "INCLUDE_DIRS"
        "LINK_LIBS"
        "OBJECT_LIBS"
        "SHARED_LIB_DIRS"
        "DEFINITIONS"
        "OPTIONS"
        "ENV_VARS"
        "EXPECTED_FILE"
        "EXPECTED_OUTFILE"
        "EXPECTED_VARS"
        "EXPECTED_IGNORE_PATTERNS"
        "FILECHECK"
        "FILECHECK_ARGS"
        "STDERR"
        "WILL_FAIL"
        "SERIAL"
        "PASS_REGEX"
        "FAIL_REGEX"
        "LABELS"
        "REQUIRE_EXTERNAL_TOOLS"
        "CATEGORIES"
        "DEPENDS"
    )

    bifusd_extract_options("${modes}" ${ARGN})

    if(SRC_FILES-FOUND)
        set(srcfiles ${SRC_FILES})
        foreach(olib ${OBJECT_LIBS})
            list(APPEND srcfiles $<TARGET_OBJECTS:${olib}>)
        endforeach()
        add_executable(${unittest_target} ${srcfiles})
        target_compile_definitions(${unittest_target} PRIVATE ${DEFINITIONS})
        if(OPTIONS-FOUND)
            target_compile_options(${unittest_target} PRIVATE ${OPTIONS})
        endif()
        bifusd_propagate_target_requirements(
            ${unittest_target}
            PRIVATE_INCLUDE_DIRS ${INCLUDE_DIRS}
            PRIVATE_LINK_LIBS    ${LINK_LIBS})
    endif()

    if(COMMAND-FOUND)
        set(test_command ${COMMAND})
    else()
        set(test_command $<TARGET_FILE:${unittest_target}>)
    endif()

    if ( BIFUSD_USE_DEBUGGER )
        if ( BIFUSD_IS_WINDOWS )
            # Open the source files in Visual Studio, except when they are targets.
            foreach(src_file ${SRC_FILES})
                if ( NOT ${src_file} MATCHES "<.*>" )
                    get_filename_component(abs_src_file "${src_file}" ABSOLUTE)
                    list(APPEND edit_files ${abs_src_file})
                endif()
            endforeach()

            if ( edit_files )
                set(debugger_command_edit_option /edit ${edit_files})
            endif()

            set(debugger_command devenv ${debugger_command_edit_option} /debugexe )
            set(test_command ${debugger_command} ${test_command})
        elseif( BIFUSD_IS_LINUX )
            set(test_command xterm -e gdb --args ${test_command})
        else()
            string(REPLACE ";" " " test_command2 "${test_command}")
            string(REPLACE ";" " " env_vars "${ENV_VARS}")
            set(env_vars "${env_vars} AMINO_DETECT_LEAKS=1")
            set(test_command
                osascript -e "tell application \"Terminal\""
                          -e activate
                          -e "set newTab to do script(\"cd ${CMAKE_CURRENT_BINARY_DIR}\")"
                          -e "do script(\"env ${env_vars} lldb -- ${test_command2}\") in front window"
                          -e "end tell")
        endif()
    elseif (EXPECTED_FILE-FOUND OR FILECHECK-FOUND)
        if (EXPECTED_OUTFILE-FOUND)
            set(unittest_output_file ${EXPECTED_OUTFILE})
        else()
            # Reuse the same filename extension as the expected file for
            # file holding the output of the program. This achieves
            # multiple goals. First, 3rd-party applications will correctly
            # recognize the content of the output file. Secondly, for the
            # cases where the EXPECTED_FILE basename is the same as the
            # unittest_target name, both files will end-up with the same
            # name, which allows a developer to simply copy all the files
            # in a directory from the ${CMAKE_CURRENT_BINARY_DIR} to
            # ${CMAKE_CURRENT_BINARY_DIR} to update the expected files with
            # a new baseline.
            if (EXPECTED_FILE)
                get_filename_component(unittest_output_ext ${EXPECTED_FILE} EXT)
            else()
                get_filename_component(unittest_output_ext ${FILECHECK} EXT)
            endif()
            # Put all output files in a sub-directory for convenience and
            # to ensure that they are not clashing with anything else (for
            # example, with unittest_target if the expected file had no
            # file extension).
            set(unittest_output_file
                ${CMAKE_CURRENT_BINARY_DIR}/test_output/${unittest_target}${unittest_output_ext})

            if(STDERR-FOUND)
                set(errflag --stderr)
            else()
                set(errflag)
            endif()
            set(test_command
                ${BIFUSD_PYTHON_EXECUTABLE} ${BIFUSD_TOOLS_DIR}/exec_tee.py ${errflag}
                ${unittest_output_file} -- ${test_command})
        endif()
    endif()

    add_test(NAME ${unittest_target} COMMAND ${test_command})

    if(WORKING_DIRECTORY-FOUND)
        set_tests_properties(
            ${unittest_target} PROPERTIES
            WORKING_DIRECTORY "${WORKING_DIRECTORY}")
    endif()

    if(WILL_FAIL-FOUND)
        set_tests_properties(${unittest_target} PROPERTIES WILL_FAIL TRUE)
    endif()

    set_tests_properties(
        ${unittest_target} PROPERTIES
        PASS_REGULAR_EXPRESSION "${PASS_REGEX}"
        FAIL_REGULAR_EXPRESSION "${FAIL_REGEX}"
        DEPENDS "${DEPENDS}")

    if(SERIAL-FOUND)
        set_tests_properties(${unittest_target} PROPERTIES RUN_SERIAL TRUE)
    endif()

    set(labels)
    if(LABELS-FOUND)
        list(APPEND labels ${LABELS})
    else()
        list(APPEND labels ${BIFUSD_SMOKE_TEST})
    endif()

    bifusd_add_test_categories(labels ${BIFUSD_SCOPE_TEST_CATEGORIES})
    if (CATEGORIES-FOUND)
        bifusd_add_test_categories(labels ${CATEGORIES})
    endif()

    if(REQUIRE_EXTERNAL_TOOLS-FOUND)
        bifusd_add_test_categories(labels ${BIFUSD_REQUIRE_EXTERNAL_TOOLS_TEST_CATEGORY})
    endif()

    list(REMOVE_DUPLICATES labels)

    set_tests_properties(${unittest_target} PROPERTIES LABELS "${labels}")

    bifusd_append_test_env_vars(${unittest_target} "${ENV_VARS}")
    bifusd_append_test_env_vars(${unittest_target} "AMINO_DETECT_LEAKS=1")

    bifusd_configure_test_shared_library_path(${unittest_target} ${SHARED_LIB_DIRS})

    if(BIFUSD_IS_OSX AND SRC_FILES-FOUND)
        # Debugging symbols
        set(unittest_file "${unittest_target}${CMAKE_EXECUTABLE_SUFFIX}")
        set(dsymfile      "${unittest_file}.dSYM")
        set(dsymtarget    "${CMAKE_CURRENT_BINARY_DIR}/${dsymfile}")

        # List the full content so that everything can be cleaned
        # properly. The listing is in reverse directory order so that
        # the directory is empty when erased (else an error is
        # emitted).
        set(dsymfiles-list
            "Contents/Resources/DWARF/${unittest_file}"
            "Contents/Info.plist"
            )
        set(dsymdirs-list
            "Contents/Resources/DWARF"
            "Contents/Resources"
            "Contents"
            )
        foreach(f ${dsymfiles-list} ${dsymdirs-list})
            list(APPEND dsymcontent "${dsymtarget}/${f}")
        endforeach()

        add_custom_command(
            OUTPUT ${dsymcontent} ${dsymtarget}
            DEPENDS ${unittest_target}
            COMMAND ${BIFUSD_OSX_DSYMUTIL} $<TARGET_FILE:${unittest_target}> -o ${dsymtarget}
            COMMENT "Generating debugging symbols ${dsymfile} for ${unittest_target}"
            VERBATIM)
        add_custom_target(${unittest_target}_dsym DEPENDS ${dsymtarget})
        bifusd_add_to_internal_targets_folder(${unittest_target}_dsym)

        add_dependencies(bifusd-dsymtest ${unittest_target}_dsym)
    endif()

    set(dependent_options ${LABELS-ARGS}
                            ${CATEGORIES-ARGS}
                            ${REQUIRE_EXTERNAL_TOOLS-ARGS})

    if(EXPECTED_FILE-FOUND)
        set(defines "")
        foreach(v ${EXPECTED_VARS})
            list(APPEND defines "-D${v}=${${v}}")
        endforeach()

        foreach(f ${EXPECTED_IGNORE_PATTERNS})
            list(APPEND from_substs "--from_subst" "${f}" "$<0:EMPTY>")
            list(APPEND to_substs   "--to_subst"   "${f}" "$<0:EMPTY>")
        endforeach()

        bifusd_configure_unittest(
            ${unittest_target}-compare
            COMMAND
                ${BIFUSD_PYTHON_EXECUTABLE} ${BIFUSD_TOOLS_DIR}/diff.py -u
                ${defines} "${from_substs}" "${to_substs}"
                ${unittest_output_file}
                ${CMAKE_CURRENT_SOURCE_DIR}/${EXPECTED_FILE}
            DEPENDS
                ${unittest_target}
            ${dependent_options}
        )
    endif()

    if(FILECHECK-FOUND)
        bifusd_configure_unittest(
            ${unittest_target}-filecheck
            COMMAND
                $<TARGET_FILE:FileCheck> ${FILECHECK_ARGS}
                --input-file=${unittest_output_file}
                ${CMAKE_CURRENT_SOURCE_DIR}/${FILECHECK}
            DEPENDS
                ${unittest_target}
            ${dependent_options}
        )
    endif()
endfunction(bifusd_configure_unittest)

# Generic function to configure a Python unit test.
#
#   bifusd_configure_python_unittest(<unitest_target>
#                                     [COMMAND         test.py opt1 opt2 ...]
#                                     [WORKING_DIRECTORY dir]
#                                     [ENV_VARS        env1 env2 ...]
#                                     [EXPECTED_FILE   expected_file]
#                                     [FILECHECK       expected_file]
#                                     [FILECHECK_ARGS  arg1 arg2 ...]
#                                     [STDERR]
#                                     [WILL_FAIL]
#                                     [SERIAL]
#                                     [PASS_REGEX      regex]
#                                     [FAIL_REGEX      regex]
#                                     [LABELS          lbl1 lbl2 ...]
#                                     [CATEGORIES      cat1 cat2 ...]
#                                     [DEPENDS         dep1 dep2 ...]
#
#   unittest_target   - The unit test target name.
#   COMMAND           - Set the mode to the Python script used to run the unit
#                       test along with all command line arguments. (Optional)
#   WORKING_DIRECTORY - Execute the command with the given current working
#                       directory. If it is a relative path it will be
#                       interpreted relative to the build tree directory
#                       corresponding to the current source directory.
#   ENV_VARS          - Environment variables to set when running the unit test.
#   EXPECTED_FILE     - The output of the test (stdout and optionally stderr) must
#                       match exactly the content of the expected file for the
#                       test to pass.
#   FILECHECK         - The expected output that will be verified using the LLVM
#                       FileCheck utility.
#   FILECHECK_ARGS    - Extra command-line argument to pass to filecheck.
#   STDERR            - Include stderr in the expected output.
#   WILL_FAIL         - Invert the pass/fail flag of the test. This property can be
#                       used for tests that are expected to fail and return a non zero
#                       return code.
#   SERIAL            - Force the unit test to be run sequentially.
#                       Note: This option should be used mainly when unit tests are
#                             *designed* to be run sequentially, regardless of the
#                             number of parallel jobs specified in CTest.
#   PASS_REGEX        - The expected regular expression from the test in case of
#                       success.
#   FAIL_REGEX        - The expected regular expression from the test in case of
#                       failure.
#   LABELS            - The labels for the unit tests. Uses BIFUSD_SMOKE_TEST if
#                       unset.
#   CATEGORIES        - The categories for the unit tests.
#   DEPENDS           - The tests that must be run before this one.
function(bifusd_configure_python_unittest  unittest_target)

    set(forwarded_modes
        "WORKING_DIRECTORY"
        "ENV_VARS"
        "EXPECTED_FILE"
        "FILECHECK"
        "FILECHECK_ARGS"
        "STDERR"
        "WILL_FAIL"
        "PASS_REGEX"
        "FAIL_REGEX"
        "SERIAL"
        "LABELS"
        "DEPENDS"
        "SHARED_LIB_DIRS"
    )

    set(modes
        "COMMAND"
        "CATEGORIES"
        ${forwarded_modes}
    )

    bifusd_extract_options("${modes}" ${ARGN})
    bifusd_get_options(forwarded_options ${forwarded_modes})

    set(categories)
    if (CATEGORIES-FOUND)
        list(APPEND categories ${CATEGORIES})
    endif()

    list(APPEND categories ${BIFUSD_PYTHON_TEST_CATEGORY})

    if (BIFUSD_HAS_PYTHON)
        set(python_exe ${BIFUSD_PYTHON_EXECUTABLE})
    else()
        message(FATAL_ERROR "Python unit test ${unittest_target} requires Python, but no Python executable has been configured.")
    endif()

    bifusd_configure_unittest(
        ${unittest_target}
        COMMAND ${python_exe} ${COMMAND}
        CATEGORIES ${categories}
        ${forwarded_options}
    )
endfunction(bifusd_configure_python_unittest)


#==============================================================================
# HEADER FILE VERIFICATION
#==============================================================================

# Check that each header file compiles correctly.
#
#  bifusd_compile_header_only_files(SRC_FILES <header_file1> <header_file2> ...
#                                  [INCLUDE_DIRS <dir1> <dir2> ...])
#                                  [LINK_LIBS    <lib1> <lib2> ...])
#
#  This function can be used to verify that header files can be parsed
#  correctly by the C++ compiler. For example, it will check that each
#  header file can be compiled on his own without having to include
#  any prerequisite header files before.
#
#   SRC_FILES     - List of headers to compile.
#   DEFINITIONS   - Additional definitions to be set when compiling the header
#                   files.
#   OPTIONS       - Additional compiler options to be set when compiling the
#                   header files.
#   INCLUDE_DIRS  - Include directories necessary to compile the header
#                   files.
#   LINK_LIBS     - Appends the public include directories of the given
#                   libraries
#
#  NOTE: Prefer LINK_LIBS to using explicit INCLUDE_DIRS. It will be
#        easier to move libraries around this way.
#
#  WARNING: Do not invoke bifusd_compile_header_only_files() for header
#  files that have an associated source file (like foo.h and
#  foo.cpp). In that case, foo.h can simply be the first include of
#  foo.cpp and that will check that foo.h can be included on his own
#  without having to include any other header file before.
function(bifusd_compile_header_only_files)
    set(modes
        "SRC_FILES"
        "DEFINITIONS"
        "OPTIONS"
        "INCLUDE_DIRS"
        "LINK_LIBS"
        )
    bifusd_extract_options("${modes}" ${ARGN})

    foreach(header_only_srcfile ${SRC_FILES})
        # First build absolute paths
        bifusd_get_absolute_cmake_path(hdrfullpath ${header_only_srcfile})

        # Compute the path that should be used to include header
        set(all_inc_dirs ${INCLUDE_DIRS})
        foreach(lib ${LINK_LIBS})
            get_target_property(inc_dirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)
            if(inc_dirs)
                list(APPEND all_inc_dirs ${inc_dirs})
            endif()
        endforeach()

        set(hdrpath_to_include ${hdrfullpath})
        foreach(inc_dir ${all_inc_dirs})
            # Ignore install paths..
            if (inc_dir MATCHES "^[$]<INSTALL_INTERFACE:.*>$")
                continue()
            endif()

            # Expand build paths
            string(REGEX REPLACE "^[$]<BUILD_INTERFACE:(.*)>$" "\\1"
                inc_dir ${inc_dir})

            if (IS_ABSOLUTE ${inc_dir})
                string(REPLACE "${inc_dir}/" ""
                    hdrpath_to_include "${hdrpath_to_include}")
            else()
                string(REPLACE "${PROJECT_BINARY_DIR}/${inc_dir}/" ""
                    hdrpath_to_include "${hdrpath_to_include}")
            endif()

            # Break as soon as a header replacement succeeds.
            if (NOT IS_ABSOLUTE ${hdrpath_to_include})
                break()
            endif()
        endforeach()

        if (IS_ABSOLUTE ${hdrpath_to_include})
            message(WARNING "  Including absolute path ${hdrpath_to_include} "
                "for header only file ${header_only_srcfile}")
        endif()

        # Compute the name of the temporary .cpp file.
        get_filename_component(dirname  ${hdrpath_to_include} DIRECTORY)
        get_filename_component(basename ${hdrpath_to_include} NAME_WE)
        set(tmp_cpp_file "${CMAKE_CURRENT_BINARY_DIR}/${dirname}/${basename}.cpp")

        add_custom_command(
            COMMENT "Generating .cpp for header only file: ${tmp_cpp_file}"
            OUTPUT  ${tmp_cpp_file}
            DEPENDS ${BIFUSD_TOOLS_DIR}/header_only_check.src.h
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            COMMAND ${BIFUSD_PYTHON_EXECUTABLE}
                    ${BIFUSD_TOOLS_DIR}/configure-file.py
                    "-DHEADER_ONLY_TO_INCLUDE=${hdrpath_to_include}"
                    ${BIFUSD_TOOLS_DIR}/header_only_check.src.h ${tmp_cpp_file}
            VERBATIM)

        bifusd_target_from_file(cpp_target "cg" "${header_only_srcfile}")
        bifusd_add_to_internal_targets_folder(${cpp_target})
        add_custom_target(${cpp_target} DEPENDS ${tmp_cpp_file})

        bifusd_target_from_file(target "hc" "${header_only_srcfile}")
        bifusd_add_to_internal_targets_folder(${target})
        add_library(${target} OBJECT ${tmp_cpp_file})
        target_compile_definitions(${target} PRIVATE ${DEFINITIONS})
        target_compile_options(    ${target} PRIVATE ${OPTIONS})

        bifusd_propagate_target_requirements(
            ${target}
            PRIVATE_INCLUDE_DIRS ${INCLUDE_DIRS}
            PRIVATE_LINK_LIBS    ${LINK_LIBS})
    endforeach()
endfunction(bifusd_compile_header_only_files)


#==============================================================================
# Debugging functions
#==============================================================================

function(bifusd_print_target_property target prop)
    get_target_property(foo ${target} ${prop})
    message(STATUS "    ${prop}:")
    message(STATUS "        ${foo}")
endfunction()

function(bifusd_print_target_properties target)
    get_target_property(type ${target} TYPE)
    get_target_property(imported ${target} IMPORTED)

    message(STATUS "${target} ${type}")
    bifusd_print_target_property(${target} IMPORTED)
    bifusd_print_target_property(${target} INTERFACE_COMPILE_OPTIONS)
    bifusd_print_target_property(${target} INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
    bifusd_print_target_property(${target} INTERFACE_INCLUDE_DIRECTORIES)
    bifusd_print_target_property(${target} INTERFACE_LINK_LIBRARIES)

    if(NOT type STREQUAL INTERFACE_LIBRARY)
        message(STATUS)
        bifusd_print_target_property(${target} COMPILE_OPTIONS)
        bifusd_print_target_property(${target} INCLUDE_DIRECTORIES)
        bifusd_print_target_property(${target} LINK_LIBRARIES)
        bifusd_print_target_property(${target} C_VISIBILITY_PRESET)
        bifusd_print_target_property(${target} CXX_VISIBILITY_PRESET)
    endif()

    if(type STREQUAL SHARED_LIBRARY OR type STREQUAL EXECUTABLE)
        bifusd_print_target_property(${target} INSTALL_RPATH)
    endif()

    if(imported AND NOT type STREQUAL INTERFACE_LIBRARY)
        bifusd_print_target_property(${target} IMPORTED_LOCATION)
        bifusd_print_target_property(${target} IMPORTED_IMPLIB)
    endif()

    message(STATUS)
endfunction()

#==============================================================================
# Settings functions
#==============================================================================

# Print the number of processors and memory size for debugging purposes.
function(print_settings)
    cmake_host_system_information(RESULT NUMBER_OF_LOGICAL_CORES   QUERY NUMBER_OF_LOGICAL_CORES)
    cmake_host_system_information(RESULT NUMBER_OF_PHYSICAL_CORES  QUERY NUMBER_OF_PHYSICAL_CORES)
    cmake_host_system_information(RESULT TOTAL_PHYSICAL_MEMORY     QUERY TOTAL_PHYSICAL_MEMORY)
    cmake_host_system_information(RESULT AVAILABLE_PHYSICAL_MEMORY QUERY AVAILABLE_PHYSICAL_MEMORY)

    message(STATUS "CMAKE_HOST_SYSTEM_NAME:       ${CMAKE_HOST_SYSTEM_NAME}")
    message(STATUS "CMAKE_HOST_SYSTEM_PROCESSOR:  ${CMAKE_HOST_SYSTEM_PROCESSOR}")
    message(STATUS "CMAKE_HOST_SYSTEM:            ${CMAKE_HOST_SYSTEM}")
    message(STATUS "CMAKE_HOST_SYSTEM_VERSION:    ${CMAKE_HOST_SYSTEM_VERSION}")

    message(STATUS "NUMBER_OF_LOGICAL_CORES:      ${NUMBER_OF_LOGICAL_CORES}")
    message(STATUS "NUMBER_OF_PHYSICAL_CORES      ${NUMBER_OF_PHYSICAL_CORES}")
    message(STATUS "TOTAL_PHYSICAL_MEMORY:        ${TOTAL_PHYSICAL_MEMORY} MB")
    message(STATUS "AVAILABLE_PHYSICAL_MEMORY:    ${AVAILABLE_PHYSICAL_MEMORY} MB")

    message(STATUS "Make generator:               ${CMAKE_GENERATOR}")
    message(STATUS "Make program:                 ${CMAKE_MAKE_PROGRAM}")

    if(CMAKE_GENERATOR STREQUAL "Ninja")
        execute_process(COMMAND "${CMAKE_MAKE_PROGRAM}" --version OUTPUT_VARIABLE NINJA_VERSION)
        string(STRIP "${NINJA_VERSION}" NINJA_VERSION)
        message(STATUS "Ninja version:                ${NINJA_VERSION}")
    endif()

    message(STATUS "gtest      location:          ${BIFUSD_GTEST_LOCATION}")

    if(BIFUSD_TBB_LOCATION)
        bifusd_print_tbb_locations()
    endif()

    if(BIFUSD_PYTHON_INCLUDED)
        bifusd_print_python_settings()
    endif()
endfunction()
