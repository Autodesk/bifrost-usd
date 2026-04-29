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

set(private_libs
    Amino::Core
    Amino::Executor
    Amino::RTTI
    Bifrost::Object
    BifrostUSD
    BifrostUsdGraphExecutor
    BifrostUsdTranslator
    usd
)

set(src_files
    fileFormat.cpp
)

bifusd_set_extra_rpaths(extra_rpaths)

# On OSX, the default file extension for a CMake MODULE library is ".so",
# but USD plugins on OSX conventionnally use ".dylib". Set the suffix explicitly
# to ensure the generated plugin has the correct extension:
if(BIFUSD_IS_OSX)
    set(CMAKE_SHARED_MODULE_SUFFIX ".dylib")
endif()

# BifrostUsdDynamicFileFormat is a USD plugin that is dynamically loaded,
# not linked against. Use bifusd_create_module_lib instead of
# bifusd_create_shared_lib to create a MODULE library that doesn't generate
# import libraries on Windows and isn't added to CMake exports.
bifusd_create_module_lib(
    BifrostUsdDynamicFileFormat "DYNAMIC_PAYLOAD_LIBRARY_BUILD_DLL"
    INSTALL
    INSTALL_TO_BIN
    PUBLIC_DEFINITIONS     $<${BIFUSD_IS_MSC}: BOOST_LIB_TOOLSET="vc142">
    PRIVATE_DEFINITIONS    $<${BIFUSD_IS_DEBUG}: TBB_USE_DEBUG BOOST_DEBUG_PYTHON BOOST_LINKING_PYTHON>

    PRIVATE_OPTIONS        $<${BIFUSD_IS_MSC}:/wd4251 /wd4273 /wd4003>
                           $<${BIFUSD_IS_GCC}: -Wno-deprecated -Wno-unused-macros>
                           $<${BIFUSD_IS_CLANG}: -Wno-deprecated -Wno-reserved-identifier -Wno-unused-macros>

    SRC_FILES              ${src_files}
    PRIVATE_LINK_LIBS      ${private_libs}
    EXTRA_RPATH            ${extra_rpaths}
)

set(plugin_info_json ${BIFROST_USD_OUTPUT_DFF_DIR}/plugInfo.json)

# NOTE: To keep on the radar... For some reason, for tests to work, the relative path to add in the plugInfo.json needs
# to be computed from the directory that contains the directory that contains plugInfo.json.
if(BIFUSD_IS_WINDOWS)
    file(RELATIVE_PATH BIFROST_DFF_REL_SHARED_LIB_DIR ${BIFROST_USD_OUTPUT_DFF_DIR}/.. ${BIFUSD_OUTPUT_BIN_DIR})
else()
    file(RELATIVE_PATH BIFROST_DFF_REL_SHARED_LIB_DIR ${BIFROST_USD_OUTPUT_DFF_DIR}/.. ${BIFUSD_OUTPUT_LIB_DIR})
endif()

configure_file(plugInfo.json.in ${plugin_info_json} @ONLY)

install(
    FILES ${plugin_info_json}
    DESTINATION "${BIFROST_USD_PACK_INSTALL_RES_DFF_DIR}")
