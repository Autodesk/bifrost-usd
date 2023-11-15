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

function(bifrost_usd_clang_tidy)

    # Collect Bifrost USD source files
    file(GLOB_RECURSE usdSrcfiles
        ${PROJECT_SOURCE_DIR}/src/*.h
        ${PROJECT_SOURCE_DIR}/src/*.cpp
    )

    if( BIFUSD_BUILD_HYDRA )
        # Collect Bifrost Hydra source files
        file(GLOB_RECURSE hydraSrcFiles
            ${PROJECT_SOURCE_DIR}/bifrost_hydra/src/*.h
            ${PROJECT_SOURCE_DIR}/bifrost_hydra/src/*.cpp
        )
    endif()

    add_custom_target(bifrost_usd_clang_tidy
        COMMAND
            ${CLAND_TIDY_LOCATION} -p ${PROJECT_SOURCE_DIR} --quiet ${usdSrcfiles} ${hydraSrcFiles}
        VERBATIM
    )

endfunction()
