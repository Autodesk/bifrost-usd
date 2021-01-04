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

add_subdirectory( docs )

set(usd_icons
    usd.svg
)

foreach(icon_file ${usd_icons})
    configure_file(
        ${icon_file}
        ${BIFROST_USD_OUTPUT_ICONS_DIR}/${icon_file}
        COPYONLY)
endforeach()

install(FILES ${usd_icons}
    DESTINATION "${BIFROST_USD_PACK_INSTALL_RES_ICONS_DIR}")
