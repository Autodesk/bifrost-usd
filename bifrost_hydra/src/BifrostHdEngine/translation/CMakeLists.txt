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

set(private_libs
    Amino::Core
    BifrostGraph::Executor
    Amino::SDK::Preview
    BifrostHdEngine
)

set(src_files
    TypeTranslation.cpp
)

bifusd_set_extra_rpaths(extra_rpaths)

bifusd_create_shared_lib(
    BifrostHdTypeTranslation "BIFROST_HD_TRANSLATION_BUILD_DLL"
    PUBLIC_INSTALL
    PUBLIC_DEFINITIONS     $<${BIFUSD_IS_MSC}: BOOST_LIB_TOOLSET="vc142">
    PRIVATE_DEFINITIONS    $<${BIFUSD_IS_DEBUG}: TBB_USE_DEBUG BOOST_DEBUG_PYTHON BOOST_LINKING_PYTHON>

    PRIVATE_OPTIONS        $<${BIFUSD_IS_MSC}:/wd4251 /wd4273>
                           $<${BIFUSD_IS_GCC}: -Wno-deprecated -Wno-unused-macros>
                           $<${BIFUSD_IS_CLANG}: -Wno-deprecated -Wno-unused-macros>

    PUBLIC_OPTIONS         $<${BIFUSD_IS_MSC}:/wd4251 /wd4273>
    SRC_FILES              ${src_files}
    PRIVATE_LINK_LIBS      ${private_libs}
    EXTRA_RPATH            ${extra_rpaths}
)
