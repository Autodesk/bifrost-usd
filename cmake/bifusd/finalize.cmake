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

if(bifusd_finalize_included)
    return()
endif()
set(bifusd_finalize_included true)


#==============================================================================
# DOCUMENTATION TARGETS
#==============================================================================

# simplifies build as it will always be defined.
add_custom_target(bifusd_doc)

#==============================================================================
# Exporting a CMake package for downstream projects to use
#==============================================================================
if (BIFUSD_PACKAGE_NAME)
    # Config version
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        ${BIFUSD_OUTPUT_CMAKE_DIR}/${BIFUSD_PACKAGE_NAME}ConfigVersion.cmake
        VERSION ${BIFUSD_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    # Package for using the developper build
    export(
        EXPORT ${BIFUSD_PACKAGE_NAME}
        FILE ${BIFUSD_OUTPUT_CMAKE_DIR}/${BIFUSD_PACKAGE_NAME}Config.cmake
        )

    # Package for using the installed build
    install(
        EXPORT      ${BIFUSD_PACKAGE_NAME}
        DESTINATION ${BIFUSD_INSTALL_CMAKE_DIR}
        FILE        ${BIFUSD_PACKAGE_NAME}Config.cmake)
    install(
        FILES "${BIFUSD_OUTPUT_CMAKE_DIR}/${BIFUSD_PACKAGE_NAME}ConfigVersion.cmake"
        DESTINATION ${BIFUSD_INSTALL_CMAKE_DIR})
endif()