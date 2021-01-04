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

set(resources_dir "${BIFUSD_TEST_RESOURCES_LOCATION}/resources")
set(usd_test_dir "${CMAKE_CURRENT_LIST_DIR}")

message( STATUS "BifrostUSD test dir.:           ${usd_test_dir}")
message( STATUS "BifrostUSD test resources dir.: ${resources_dir}")

if(NOT DEFINED BIFUSD_BIFCMD_LOCATION)
    bifusd_required_variables(BIFROST_LOCATION)
    set(BIFUSD_BIFCMD_LOCATION "${BIFROST_LOCATION}/bin")
endif()

add_subdirectory(BifrostUsd)
add_subdirectory(nodedefs)
