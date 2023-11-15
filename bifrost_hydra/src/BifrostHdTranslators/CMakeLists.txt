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

set(hd_GraphTranslator_headers
    Geometry.h
    GeometryFn.h
    Mesh.h
    Instances.h
    Strands.h
)

bifusd_install_headers(
    hdGraphTranslatorsHeadersInstall
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${hd_GraphTranslator_headers}
    DESTINATION "BifrostHydra/Translators"
)

set(private_libs
    Amino::Core
    Bifrost::Object::Preview
    Amino::SDK::Preview
    Bifrost::Geometry::Preview
    Bifrost::VTT::Preview
    hd
    hdGraphTranslatorsHeadersInstall
)

set(src_files
    Geometry.cpp
    GeometryFn.cpp
    Mesh.cpp
    Instances.cpp
    Strands.cpp
)

bifusd_set_extra_rpaths(extra_rpaths)

bifusd_create_shared_lib(
    BifrostHdTranslators "USD_BUILD_DLL"
    PUBLIC_INSTALL
    PUBLIC_DEFINITIONS      $<${BIFUSD_IS_MSC}: BOOST_LIB_TOOLSET="vc141">
    PRIVATE_DEFINITIONS     $<${BIFUSD_IS_DEBUG}: TBB_USE_DEBUG BOOST_DEBUG_PYTHON BOOST_LINKING_PYTHON>

    PRIVATE_OPTIONS         $<${BIFUSD_IS_MSC}:/wd4251 /wd4273>
                            $<${BIFUSD_IS_GCC}: -Wno-deprecated -Wno-unused-macros>
                            $<${BIFUSD_IS_CLANG}: 
                                # 'GetValue' overrides a member function but is not marked 'override' 
                                # in pxr/imaging/hd/dataSource.h:176:21                                                 
                                -Wno-suggest-override -Wno-inconsistent-missing-override
                                # There are some unused member function in pxr/imaging/hd/dataSource.h:65:5
                                 -Wno-unused-member-function
                                 -Wno-unused-macros
                            >

    SRC_FILES              ${src_files}
    PRIVATE_LINK_LIBS      ${private_libs}
    EXTRA_RPATH            ${extra_rpaths}
)

