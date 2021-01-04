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

set(src_files
    testUtils.cpp
)

set(header_files
    testUtils_export.h
    testUtils.h
)

set(public_libs
    Amino::Core
    BifrostGraph::Executor
    Bifrost::FileUtils
    BifrostUSD
    usd
)

set(private_libs
    bifrostUsdTestUtilsHeadersInstall
)

bifusd_install_headers(
    bifrostUsdTestUtilsHeadersInstall
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
	FILES ${header_files}
	DESTINATION "utils/test"
)

bifusd_create_shared_lib(
    BifrostUSDTestUtils "USD_BUILD_TESTUTILS_DLL"
    PUBLIC_INSTALL
    SRC_FILES               ${src_files}
    PUBLIC_LINK_LIBS        ${public_libs}
    PRIVATE_LINK_LIBS       ${private_libs}
)
