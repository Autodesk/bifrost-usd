#-
#*****************************************************************************
# Copyright 2025 Autodesk, Inc.
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

#===============================================================================
# Nodedef tests with direct function calls
#===============================================================================

function(configure_bifusd_nodedefs_unittests)

    # Library dependencies
    set(libs
        bifusd_gtest
        BifrostGraph::Executor
        BifrostUSD
        BifrostUSDNodedefs
        usd
        usdGeom
        BifrostUSDTestUtils
    )

    # test files
    set(test_files
        testAttributeNodeDefs.cpp
        testCollectionNodeDefs.cpp
        testGeomNodeDefs.cpp
        testLayerNodeDefs.cpp
        testMaterialBindingNodeDefs.cpp
        testPrimNodeDefs.cpp
        testStageNodeDefs.cpp
        testVariantSetNodeDefs.cpp
    )

    if (USD_PXR_VERSION LESS 2408)
        # BIFROST-11249 Adding testDictSerialization only when building older version of USD
        # as since USD 24.08, it can be built using oneTBB and it is causing a crash.
        # The code to serialize uses "function tables" in USD.
        # If the code was built against USD with no python, for example then
        # it may not run correctly against a USD with python because the
        # "function tables" changed.
        # This issue is being investigated under BIFROST-11249.
        list(APPEND test_files testDictSerialization.cpp)
    endif()

    foreach(test_file ${test_files})
        configure_bifusd_unittest(
            ${test_file} "usd_nodedefs"
            LINK_LIBS ${libs}
            SHARED_LIB_DIRS ${BIFUSD_EXTRA_BUILD_AND_TEST_PATHS}
            ENV_VARS USD_TEST_RESOURCES_DIR=${resources_dir}
                     PXR_AR_DEFAULT_SEARCH_PATH=${resources_dir}
                     USD_TEST_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
                     ${asan_options}
        )
    endforeach()

endfunction()
configure_bifusd_nodedefs_unittests()

#===============================================================================
# Nodedef python tests loaded through bifrost runtime.
#===============================================================================

function(configure_bifusd_runtime_unittests)

    set(python_tests
        test_graphs.py
    )

    set(lib_config_files
        ${BIFUSD_EXTRA_CONFIG_FILES}
        ${usd_test_dir}/usd_pack_test_config.json
        ${BIFUSD_OUTPUT_ROOT_DIR}/usd_pack_config.json
    )
    set(env_vars)
    if(IS_BIFUSD_STANDALONE)
        # When running tests in standalone mode with BIFROST_LOCATION being defined,
        # we need to disable the default usd_pack that is shipped with bifrost,
        # and use the nodedefs from this current repo instead:
        list(APPEND env_vars BIFROST_DISABLE_PACKS=usd_pack)
    endif()

    if( BIFUSD_ENABLE_ADDRESS_SANITIZER OR BIFUSD_ENABLE_UNDEFINED_SANITIZER)
        set(running_sanitizers
            --running-sanitizers
        )
    endif()

    foreach(test_file ${python_tests})
        bifusd_configure_python_unittest(
            "usd_nodedefs_runtime_${test_file}"

            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/${test_file}
                --bifcmd-location ${BIFUSD_BIFCMD_LOCATION}
                --bifrost-lib-config-files ${lib_config_files}
                --resource-dir ${resources_dir}
                --test-dir ${usd_test_dir}
                --debug
                ${running_sanitizers}

            SHARED_LIB_DIRS ${BIFUSD_EXTRA_BUILD_AND_TEST_PATHS}
            CATEGORIES      ${BIFUSD_SANITY_TEST_CATEGORY}
            DEPENDS         bifcmd
            ENV_VARS        ${env_vars}
        )

    endforeach()
endfunction()

# Bifrost USD maps coverage to BIFUSD_IS_RELEASE but
# we may be asked to run Coverage from other tool chains.
# We found that the python test test_graphs.py hangs in that case.
bifusd_set_if_matches(coverage_demanded  CMAKE_BUILD_TYPE "^(Coverage)$")

if( BIFUSD_TEST_GRAPHS AND NOT BIFUSD_IS_DEBUG AND NOT coverage_demanded )
    # Testing graphs in DEBUG with bifcmd in release will fail.
    # When Coverage is demanded, the test hangs
    # See BIFROST-8039
    configure_bifusd_runtime_unittests()
endif()
