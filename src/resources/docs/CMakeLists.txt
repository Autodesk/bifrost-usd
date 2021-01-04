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

file( GLOB ALL_MD_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.md )

set( usd_compound_docs )
foreach( doc_file ${ALL_MD_FILES})
   get_filename_component( basename ${doc_file} NAME )
   list( APPEND usd_compound_docs ${basename})
endforeach()

foreach(doc_file ${usd_compound_docs})
    configure_file(
        ${doc_file}
        ${PROJECT_BINARY_DIR}/doc/markdown/nodes/${doc_file}
        COPYONLY)
endforeach()

install(FILES ${usd_compound_docs}
    DESTINATION "${BIFROST_USD_PACK_INSTALL_RES_DOCS_DIR}/ENU")