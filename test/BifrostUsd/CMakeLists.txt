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

#------------------------------------------------------------------------------
# Library dependencies
#------------------------------------------------------------------------------

set(libs
    bifusd_gtest
    BifrostUSD
    BifrostUSDNodedefs
    Amino::Core
    BifrostGraph::Executor
    BifrostUSDTestUtils
)

#------------------------------------------------------------------------------
# test files
#------------------------------------------------------------------------------
set(test_files
    testBifrostUsdLayer.cpp
    testBifrostUsdStage.cpp
)

foreach(test_file ${test_files})
    configure_bifusd_unittest(
        ${test_file} "BifrostUsd"
        LINK_LIBS ${libs}
        SHARED_LIB_DIRS ${BIFUSD_EXTRA_BUILD_AND_TEST_PATHS}
        ENV_VARS USD_TEST_RESOURCES_DIR=${resources_dir}
                 PXR_AR_DEFAULT_SEARCH_PATH=${resources_dir}
                 USD_TEST_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
    )
endforeach()
