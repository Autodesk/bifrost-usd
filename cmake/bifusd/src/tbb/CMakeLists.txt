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

#==============================================================================
# TBB Helpers
#==============================================================================

# Set the name of Bifusd's TBB helpers target
set(BIFUSD_TBB_HELPERS_TARGET BifusdTBBHelpers
    CACHE INTERNAL "Name of BifusdTBBHelpers target")

if(TARGET ${BIFUSD_TBB_HELPERS_TARGET})
    return()
endif()

set(header_only_srcfiles
    TBBInitGuard.h
)

bifusd_install_headers(
    ${BIFUSD_TBB_HELPERS_TARGET}
    SOURCE              ${CMAKE_CURRENT_SOURCE_DIR}
    FILES               ${header_only_srcfiles}
    HEADER_ONLY_FILES   ${header_only_srcfiles}
    DESTINATION         "bifusd/tbb"
    PUBLIC_LINK_LIBS    BifusdTBB
)
