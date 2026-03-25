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

set(python_tests
    testAssetTemplate.py
    testBifrostStageCmds.py
    testComponentCreator.py
    testComponentCreatorGraphOnly.py
    testComponentMaterialBinding.py
    testCreateComponentGraph.py
    testGetComponentMetadata.py
    testImportMayaModel.py
    testImportMayaModelInNewStage.py
    testImportMayaVariants.py
    testLookdevWorkflow.py
    testMaterialHintAttribute.py
    testModelExporter.py
    testPatternMatching.py
    testPrimPath.py
    testValidateGeo.py
)

# When running tests in standalone mode with BIFROST_LOCATION being defined,
# we disable the default `usd_pack` and `usdMayaTranslations` libraries that
# are shipped with bifrost, and use the nodedefs and translation table built
# from this current repo instead:
set(disabled_packs usd_pack usdMayaTranslations)

# Include the installed plugin_config.json in BIFROST_LIB_CONFIG_FILES:
set(bifrost_config_files
    "${CMAKE_INSTALL_PREFIX}/plugin_config.json"
    "${BIFROST_LIB_CONFIG_FILES}"
)

set(USD_PYTHON_PATH "${MAYA_USD_PLUGIN_DIR}/mayausd/USD${MULTI_USD_VERSION_SUFFIX}/lib/python")
set(MAYA_USD_PYTHON_PATH "${MAYA_USD_PLUGIN_DIR}/mayausd/MayaUSD${MULTI_USD_VERSION_SUFFIX}/lib/python")

option(IS_MAYA_USD_LOCAL_ECG_BUILD "OFF" "Option not needed from the public side.")

if (IS_MAYA_USD_LOCAL_ECG_BUILD)
    set(USD_PYTHON_PATH "${USD_LOCATION}/lib/python")
    set(MAYA_USD_PYTHON_PATH "${MAYA_USD_PLUGIN_DIR}/lib/python")
endif()

set(EXAMPLES_MAYA_PLUGIN ${CMAKE_INSTALL_PREFIX}/${BIFUSD_INSTALL_ROOT_DIR}/examples/maya_plugin)

# Add extra modules and Python search paths for local build case
set(maya_modules_paths
    "${BIFROST_LOCATION}/.."
    "${BIFROST_LOCATION}/../../modules"
    "${MAYA_USD_PLUGIN_DIR}"
    "${LOOKDEVX_PLUGIN_DIR}"
    "${EXAMPLES_MAYA_PLUGIN}"
)
set(python_paths
    "${USD_PYTHON_PATH}"
    "${MAYA_USD_PYTHON_PATH}"
    "${EXAMPLES_MAYA_PLUGIN}/python"
)

foreach(test_file ${python_tests})
    set(unittest_target examples_${test_file})
    add_test(NAME ${unittest_target}
             COMMAND "${MAYA_PY_EXECUTABLE}" ${CMAKE_CURRENT_SOURCE_DIR}/${test_file}
    )
    bifusd_set_test_shared_library_path(${unittest_target} ${BIFUSD_EXTRA_BUILD_AND_TEST_PATHS})

    # Note: bifusd_append_test_path_env_var handles the platform-specific path separator
    #       (':' on Unix, ';' on Windows) and ensures semicolons in the list are properly
    #       escaped so they are not consumed by CMake and can reach the Python script.
    if(IS_BIFUSD_STANDALONE)
        bifusd_append_test_path_env_var(${unittest_target} "BIFROST_DISABLE_PACKS" "${disabled_packs}")
        bifusd_append_test_path_env_var(${unittest_target} "BIFROST_LIB_CONFIG_FILES" "${bifrost_config_files}")
    endif()
    bifusd_append_test_path_env_var(${unittest_target} "MAYA_MODULE_PATH" "${maya_modules_paths}")
    bifusd_append_test_path_env_var(${unittest_target} "PYTHONPATH" "${python_paths}")
endforeach()
