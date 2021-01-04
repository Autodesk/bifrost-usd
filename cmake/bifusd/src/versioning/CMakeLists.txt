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

#==============================================================================
# TARGET LIBRARIES
#==============================================================================

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/VerInfo_src.cpp
               ${CMAKE_CURRENT_BINARY_DIR}/VerInfo.cpp)

set(src_files
    ${CMAKE_CURRENT_BINARY_DIR}/VerInfo.cpp
)

set(header_files
    VerFormat.h
)

set(private_libs
    ${BIFUSD_CONFIG_TARGET}
)

bifusd_install_headers(
    ${BIFUSD_PACKAGE_NAME}VersioningHeaders
    SOURCE              ${CMAKE_CURRENT_SOURCE_DIR}
    FILES               VerFormat.h VerInfo.h
    DESTINATION         "bifusd/versioning"
    )

# Main target.
bifusd_create_static_lib(
    ${BIFUSD_PACKAGE_NAME}Versioning
    PUBLIC_INSTALL
    SRC_FILES            ${src_files}
    HEADER_ONLY_FILES    ${HEADER_ONLY_FILES}
    PUBLIC_INCLUDE_DIRS  ${BIFUSD_OUTPUT_INCLUDE_DIR}
    PUBLIC_LINK_LIBS     ${BIFUSD_PACKAGE_NAME}VersioningHeaders
    PRIVATE_LINK_LIBS    ${private_libs}
    )
