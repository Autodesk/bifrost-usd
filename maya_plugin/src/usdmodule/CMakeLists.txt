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

#----------------- Generate lib file -----------------

bifusd_install_headers(
    usd_packMayaTranslationHeadersInstall
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
    FILES export.h usd_translator.h
    DESTINATION "maya_plugin/usd_pack"
)

set(public_libs
    BifrostGraph::Executor
    BifrostUSD
)

set(private_libs
    Maya::Foundation
    Maya::OpenMaya
    Amino::Core
    BifrostGraph::Maya
    usdHeadersInstall
    usdUtils
)

set(src_files
    usd_translator.cpp
)

set(public_include_dirs
    ${BIFUSD_OUTPUT_INCLUDE_DIR}
)

bifusd_set_extra_rpaths(extra_rpaths)

bifusd_create_shared_lib(BifrostUSDMayaTranslation      "USD_MODULE_API_IMPL"
        PUBLIC_INSTALL
        PRIVATE_DEFINITIONS           $<${BIFUSD_IS_OSX}: OSMac_> $<${BIFUSD_IS_DEBUG}: TBB_USE_DEBUG BOOST_DEBUG_PYTHON BOOST_LINKING_PYTHON>
        SRC_FILES                     ${src_files}
        PUBLIC_LINK_LIBS              ${public_libs}
        PRIVATE_LINK_LIBS             ${private_libs}
        EXTRA_RPATH                   ${extra_rpaths})
