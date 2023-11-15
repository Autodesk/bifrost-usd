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

set(usd_headers
    Attribute.h
    Layer.h
    Prim.h
    Stage.h
    Enum.h
)


set(usdHeaders
    StageCache.h
    BifrostUsdExport.h
    ${usd_headers}
)

bifusd_install_headers(
    usdHeadersInstall
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC_LINK_LIBS
        Amino::Cpp
	FILES ${usdHeaders}
	DESTINATION "BifrostUsd"
)

bifusd_header_parser(generate_usd_type_json
    HEADER_FILES    ${usd_headers}
    LINK_LIBS       usdHeadersInstall
)

add_dependencies(${BIFUSD_PACKAGE_NAME}_config_info
    generate_usd_type_json)
