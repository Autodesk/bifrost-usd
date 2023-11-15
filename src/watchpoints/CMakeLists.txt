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

set(public_libs
    BifrostGraph::Executor
    usdHeadersInstall
    BifrostUSD
)

bifusd_set_extra_rpaths(extra_rpaths)

bifusd_create_shared_lib(BifrostUSDWatchpoint             "USD_WATCHPOINT_BUILD_DLL"
        PUBLIC_INSTALL
        PRIVATE_DEFINITIONS           $<${BIFUSD_IS_OSX}: OSMac_> $<${BIFUSD_IS_DEBUG}: TBB_USE_DEBUG BOOST_DEBUG_PYTHON BOOST_LINKING_PYTHON>

        PRIVATE_OPTIONS               $<${BIFUSD_IS_GCC}: -Wno-unused-macros>
                                      $<${BIFUSD_IS_CLANG}: -Wno-unused-macros>

        SRC_FILES                     usd_watchpoint.cpp
        PUBLIC_LINK_LIBS              ${public_libs}
        EXTRA_RPATH                   ${extra_rpaths})
