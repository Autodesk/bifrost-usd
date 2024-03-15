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

set(hd_Engine_headers
    Container.h
    Engine.h
    Export.h
    JobTranslationData.h
    Parameters.h
    Requirement.h
    Runtime.h
    ValueTranslationData.h
    Workspace.h
)

bifusd_install_headers(
    hdEngineHeadersInstall
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${hd_Engine_headers}
    DESTINATION "BifrostHydra/Engine"
)

set(public_libs
    Amino::SDK::Preview
    BifrostGraph::Executor::Preview
    hdEngineHeadersInstall
)

set(private_libs
    Amino::Core
    Bifrost::Object
    hd
    BifrostHdTranslators
)

set(src_files
    Container.cpp
    Engine.cpp
    JobTranslationData.cpp
    Parameters.cpp
    Requirement.cpp
    Runtime.cpp
    ValueTranslationData.cpp
    Workspace.cpp
)

bifusd_set_extra_rpaths(extra_rpaths)

bifusd_create_shared_lib(
    BifrostHdEngine        "BIFROST_HD_ENGINE_BUILD_DLL" # TODO: add export file
    PUBLIC_INSTALL
    PUBLIC_DEFINITIONS     $<${BIFUSD_IS_MSC}: BOOST_LIB_TOOLSET="vc142">
    PRIVATE_DEFINITIONS    $<${BIFUSD_IS_DEBUG}: TBB_USE_DEBUG BOOST_DEBUG_PYTHON BOOST_LINKING_PYTHON>

    PRIVATE_OPTIONS        $<${BIFUSD_IS_MSC}:/wd4251 /wd4273 /wd4003>
                           $<${BIFUSD_IS_GCC}: -Wno-deprecated -Wno-unused-macros>
                           $<${BIFUSD_IS_CLANG}: -Wno-deprecated -Wno-unused-macros>

    SRC_FILES              ${src_files}
    PUBLIC_LINK_LIBS       ${public_libs}
    PRIVATE_LINK_LIBS      ${private_libs}
    EXTRA_RPATH            ${extra_rpaths}
)

add_subdirectory(translation)
add_subdirectory(config)
