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

add_subdirectory(testUtils)

#------------------------------------------------------------------------------
# Library dependencies
#------------------------------------------------------------------------------

set(libs
    bifusd_gtest
    Amino::Core
    Amino::SDK::Preview
    Bifrost::Geometry::Preview
    Bifrost::VTT::Preview
    BifrostGraph::Executor
    BifrostHdTranslators
    BifrostHdEngine
    BifrostUSDTestUtils
    BifrostHdTestUtils
)

set(pxr_libs
    arch
    garch
    glf
    hd
    sdf
    tf
    usd
    usdUtils
    usdImaging
    usdImagingGL

)

#------------------------------------------------------------------------------
# test files
#------------------------------------------------------------------------------
set(test_files
    testHdSceneIndexBifrostGraphPrim.cpp
    testHdSceneIndexQueries.cpp
)

set(lib_config_files
    ${BIFUSD_EXTRA_CONFIG_FILES}
    ${BIFUSD_OUTPUT_ROOT_DIR}/bifrost_hydra_translation.json
    ${PROJECT_SOURCE_DIR}/bifrost_hydra/test/test_bif_geo_compounds_config.json
)

if(IS_BIFUSD_STANDALONE)
    list(PREPEND lib_config_files ${BIFROST_LOCATION}/resources/standalone_config.json )
endif()

if(NOT BIFUSD_IS_WINDOWS)
    string(REPLACE ";" ":" lib_config_files "${lib_config_files}")
endif()

set(bifrost_env_config
    BIFROST_LOCATION=${BIFROST_LOCATION}
    BIFROST_LIB_CONFIG_FILES=${lib_config_files}
)

# To debug you can add : TF_DEBUG="PLUG_LOAD PLUG_REGISTRATION"
set(usd_env_config
     USD_TEST_RESOURCES_DIR=${CMAKE_CURRENT_SOURCE_DIR}/resources
     PXR_PLUGINPATH_NAME=${BIFROST_USD_OUTPUT_PROC_DIR}
     HD_ENABLE_SCENE_INDEX_EMULATION=1
     USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=1
     HDGP_INCLUDE_DEFAULT_RESOLVER=1
     USD_TEST_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}
)

foreach(test_file ${test_files})
    configure_bifusd_unittest(
        ${test_file} "bifrost_hydra"
        LINK_LIBS ${libs} ${pxr_libs}
        SHARED_LIB_DIRS ${Maya_SHARED_LIB_DIRS} ${BIFUSD_EXTRA_BUILD_AND_TEST_PATHS}
        ENV_VARS ${bifrost_env_config} ${usd_env_config}
        ASAN_OPTIONS ${ASAN_OPTIONS}
        OPTIONS      $<${BIFUSD_IS_MSC}:/wd4251 /wd4273>
    )
endforeach()
