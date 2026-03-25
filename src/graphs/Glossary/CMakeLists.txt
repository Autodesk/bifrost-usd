#-
#*****************************************************************************
# Copyright 2025 Autodesk, Inc.
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

#==============================================================================
# Graph directories to install
#==============================================================================

# The glossary examples are showing up under the "USD/Glossary" section of the Bifrost Browser.
set(glossary_content
	attribute_block
	edit_target
	inherits
	prim
	references
	specializes
	sublayers
	variant_set
)

install(DIRECTORY ${glossary_content} DESTINATION ${BIFROST_USD_PACK_INSTALL_RES_GRAPHS_DIR}/Glossary )
