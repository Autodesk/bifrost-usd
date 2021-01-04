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

if(bifusd_tests_included)
    return()
endif()
set(bifusd_tests_included true)

#
# Labels used to place tests in specific test suites.
#
# Used as:
#    bifusd_configure_unitest(
#        MyTest
#        ...
#        LABELS ${BIFUSD_SMOKE_TEST}
#        ... )
#

set(BIFUSD_SMOKE_TEST       Smoke Sanity Daily Release)
set(BIFUSD_SANITY           Sanity Daily Release)
set(BIFUSD_DAILY_TEST       Daily Release)
set(BIFUSD_RELEASE_TEST     Release)
set(BIFUSD_PERFORMANCE_TEST Performance Daily Release)

set(BIFUSD_MAYA_TEST        Maya)
set(BIFUSD_MAYA_SMOKE_TEST  Maya MayaSmoke MayaDaily)
set(BIFUSD_MAYA_DAILY_TEST  Maya MayaDaily)

set(BIFUSD_IMAGE_COMPARE_TEST ImageCompare)


# Constants for categories
set(BIFUSD_CATEGORY_PREFIX "category:")

# Labels for test categories
set(BIFUSD_SANITY_TEST_CATEGORY      "sanity")
set(BIFUSD_PERFORMANCE_TEST_CATEGORY "performance")
set(BIFUSD_BENCHMARK_TEST_CATEGORY   "benchmark")

set(BIFUSD_REQUIRE_EXTERNAL_TOOLS_TEST_CATEGORY "require-external-tools")

set(BIFUSD_PYTHON_TEST_CATEGORY "python")

set(BIFUSD_MAYA_TEST_CATEGORY  "maya")
set(BIFUSD_IMAGE_TEST_CATEGORY "image")

# Initial scope categories
set(BIFUSD_SCOPE_TEST_CATEGORIES)
