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

add_subdirectory(include)

set(usd_public_libs
    Bifrost::FileUtils
    Bifrost::Object
    Amino::Core
    Amino::Cpp
    usdHeadersInstall
    usd
)

set(usd_private_libs
        usdUtils
)

set(usd_src_files
    Attribute.cpp
    Layer.cpp
    Prim.cpp
    Stage.cpp
    StageCache.cpp
)

bifusd_set_extra_rpaths(extra_rpaths)

bifusd_create_shared_lib(
    BifrostUSD "USD_BUILD_DLL"
    PUBLIC_INSTALL
    PUBLIC_DEFINITIONS     $<${BIFUSD_IS_MSC}: BOOST_LIB_TOOLSET="vc142">
    PRIVATE_DEFINITIONS    $<${BIFUSD_IS_DEBUG}: TBB_USE_DEBUG BOOST_DEBUG_PYTHON BOOST_LINKING_PYTHON>
    PUBLIC_OPTIONS         $<${BIFUSD_IS_MSC}:/wd4251 /wd4273 /wd4003>
                           $<${BIFUSD_IS_CLANG}: -Wno-weak-vtables -Wno-unused-template -Wno-unneeded-internal-declaration>
                           $<${BIFUSD_IS_GCC}: -Wno-deprecated>

    PRIVATE_OPTIONS        $<${BIFUSD_IS_GCC}: -Wno-unused-macros>
                           $<${BIFUSD_IS_CLANG}: -Wno-unused-macros>

    SRC_FILES              ${usd_src_files}
    PUBLIC_LINK_LIBS       ${usd_public_libs}
    PRIVATE_LINK_LIBS      ${usd_private_libs}
    EXTRA_RPATH            ${extra_rpaths}
)
