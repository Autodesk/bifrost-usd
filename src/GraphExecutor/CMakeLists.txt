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

add_subdirectory(include)

set(public_libs
    BifrostUsdGraphExecutorHeadersInstall
)

set(private_libs
    Amino::Core
    Amino::Executor
    Amino::SDK
    Bifrost::Geometry
    BifrostGraph::Executor # For BifrostGraph::Executor::Utility::ConfigEnv
)

set(src_files
    GraphExecutor.cpp
    GraphExecutorFactory.cpp
)

bifusd_set_extra_rpaths(extra_rpaths)

bifusd_create_shared_lib(
    BifrostUsdGraphExecutor "BIFROSTUSD_GRAPH_EXECUTOR_BUILD_SHARED"
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
