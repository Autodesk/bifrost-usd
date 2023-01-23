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
        testDictSerialization.cpp
        testGeomNodeDefs.cpp
        testLayerNodeDefs.cpp
        testMaterialBindingNodeDefs.cpp
        testPrimNodeDefs.cpp
        testStageNodeDefs.cpp
        testVariantSetNodeDefs.cpp
    )
    foreach(test_file ${test_files})
        configure_bifusd_unittest(
            ${test_file} "usd_nodedefs"
            LINK_LIBS ${libs}
            SHARED_LIB_DIRS ${BIFUSD_EXTRA_BUILD_AND_TEST_PATHS}
            ENV_VARS USD_TEST_RESOURCES_DIR=${resources_dir}
                     PXR_AR_DEFAULT_SEARCH_PATH=${resources_dir}
                     USD_TEST_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
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
    if(DEFINED BIFROST_LOCATION)
        # When running tests in standalone mode with BIFROST_LOCATION being defined,
        # we need to disable the default usd_pack that is shipped with bifrost,
        # and use the nodedefs from this current repo instead:
        list(APPEND env_vars BIFROST_DISABLE_PACKS=usd_pack)
    endif()

    foreach(test_file ${python_tests})
        bifusd_configure_python_unittest(
            "usd_nodedefs_runtime_${test_file}"

            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/${test_file}
                --bifcmd-location ${BIFUSD_BIFCMD_LOCATION}
                --bifrost-lib-config-files ${lib_config_files}
                --resource-dir ${resources_dir}
                --test-dir ${usd_test_dir}

            SHARED_LIB_DIRS ${BIFUSD_EXTRA_BUILD_AND_TEST_PATHS}
            CATEGORIES      ${BIFUSD_SANITY_TEST_CATEGORY}
            DEPENDS         bifcmd
            ENV_VARS        ${env_vars}
        )

    endforeach()
endfunction()

if(BIFUSD_IS_DEBUG OR (BIFUSD_IS_OSX AND NOT CMAKE_SYSTEM_PROCESSOR IN_LIST CMAKE_OSX_ARCHITECTURES) )
    # Testing graphs in DEBUG with bifcmd in release will fail.
    # See BIFROST-8039
    #
    # On OSX, testing when the processor is not part the target architectures
    # will not work because amino tools target the native processor.
    # See BIFROST-8149
else()
    configure_bifusd_runtime_unittests()
endif()
