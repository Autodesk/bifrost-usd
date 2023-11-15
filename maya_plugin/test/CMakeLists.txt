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

#------------------------------------------------------------------------------
# Library dependencies
#------------------------------------------------------------------------------

set(libs
    bifusd_gtest
    usd_packMayaTranslationHeadersInstall
    BifrostUSDMayaTranslation
    BifrostUSD
    BifrostUSDTestUtils
    usdUtils
)

#------------------------------------------------------------------------------
# test files
#- Tests that need maya libraries but do not run in Maya.
#------------------------------------------------------------------------------

# First the list of tests that do not depend on Maya 
# None for now...
set(test_files)

# Second the test that depend on maya unless
# they need to be skipped if the environment for 
# Maya is not "complete"
if( NOT BIFUSD_MAYA_SKIP_TESTS )
    list( APPEND test_files
        testUsdTranslation.cpp
    )
endif()

set( env_vars 
     "USD_TEST_RESOURCES_DIR=${resources_dir}"
     "USD_TEST_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}" )

# On OSX you need to set the Maya Framework
if( BIFUSD_IS_OSX)
    list( APPEND env_vars 
          "DYLD_FRAMEWORK_PATH=${Maya_OSX_FWK_DIR}")
endif()

# KLUDGE_LINK_OPTIONS:
# It is there to make sure that Maya's dependency on TBB is satisfied by 
# Maya's tbb lib and not another tbb (like the one from some USD)
# 
# How it popped-up 
# a) You have Maya 2022 runtime with a tbb of version X.
# b) You have recompiled USD 21.05, it has a tbb of version Y. Not same as X ?
# c) You have a dependency on USD (usdUtils).  It brings in linking with -ltbb 
# d) You have an indirect dependency on OpenMaya via the UsdTranslationMayaPlugin code. 
#    It needs OpenMaya in its cpp not its .h ?    So it is a PRIVATE dependency?
# e) At link time it will complain that a given Maya lib cannot find an entry in tbb? 
#    ex.: bb::interface7::internal::isolate_within_arena 
#    That is because the tbb from USD is picked-up ? I have a direct dependency on usdUtils, not on Maya
#
# Below, one of the two following kludges is used: 
# a) -Wl,-allow-shlib-undefined 
#     This delays things. (From the docs) "The reason is that the shared library being 
#     specified at link time may not be  the  same  as the one that 
#     is available at load time, so the symbols might actually
#     be resolvable at load time."  This is the case for our tbb libs... 
#
# b) -L${MAYA_RUNTIME_LOCATION}/lib    // Adds Maya in search path, at start... see where KLUDGE is used.

# Note: It is important to have the Maya_SHARED_LIB_DIRS before
# To make sure that when Maya is used, its python, its tbb, its ... 
# are used.
foreach(test_file ${test_files})
    configure_bifusd_unittest(
        ${test_file} "maya_plugin"
        LINK_LIBS ${libs}
        SHARED_LIB_DIRS ${Maya_SHARED_LIB_DIRS} ${BIFUSD_EXTRA_BUILD_AND_TEST_PATHS}
        ENV_VARS ${env_vars}
        KLUDGE_LINK_OPTIONS $<${BIFUSD_IS_LINUX}: -Wl,-allow-shlib-undefined>
    )
endforeach()
