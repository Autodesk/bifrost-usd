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

add_subdirectory(BifrostUsd)
add_subdirectory(compounds)
add_subdirectory(config)
add_subdirectory(docs)

add_subdirectory(graphs)
add_subdirectory(icons)
add_subdirectory(nodedefs)
add_subdirectory(watchpoints)

add_subdirectory(GraphExecutor)
add_subdirectory(UsdTranslator)
add_subdirectory(DynamicFileFormat)

# Temporarily exclude GraphExecutor from the sdk doxygen pass:
set_property(GLOBAL APPEND PROPERTY BIFUSD_DOXYGEN_EXCLUDE
    "${BIFUSD_OUTPUT_INCLUDE_DIR}/BifrostUsd/GraphExecutor")
