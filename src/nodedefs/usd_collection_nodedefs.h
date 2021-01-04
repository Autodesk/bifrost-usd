//-
// Copyright 2022 Autodesk, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//+
/// \file  usd_collection_nodedefs.h
/// \brief Bifrost USD Collection nodes.

#ifndef USD_COLLECTION_NODEDEFS_H
#define USD_COLLECTION_NODEDEFS_H

// #include <Amino/Core/Array.h>

#include <BifrostUsd/Prim.h>
#include <BifrostUsd/Stage.h>

#include "header_parser_macros.h"
#include "nodedef_export.h"

// forward declarations
namespace Amino {
class String;
} // namespace Amino

/// \defgroup USD USD
namespace USD {

/// \ingroup USD
/// \defgroup Collection Collection
namespace Collection {

/// \ingroup Collection
/// \defgroup get_or_create_collection get_or_create_collection node
///
/// \brief Creates or returns a collection on the USD prim.
///
/// \param [in] stage The stage holding the prim.
/// \param [in] prim_path The path of the prim you want to add a collection to.
/// \param [in] collection_name The name of the prim collection to create.
/// \param [in] rule Specifies how the paths that are included in the collection
///             must be expanded to determine its members.
/// \param [in] include_paths Includes the given Targets' paths in the collection.
/// \param [in] exclude_paths Excludes the given Targets' paths from the collection.
/// \returns true if a collection is created or is found.
USD_NODEDEF_DECL
bool get_or_create_collection(BifrostUsd::Stage& stage
                                                   USDPORT_INOUT("out_stage"),
                              const Amino::String& prim_path,
                              const Amino::String& collection_name,
                              const BifrostUsd::ExpansionRule    rule,
                              const Amino::Array<Amino::String>& include_paths,
                              const Amino::Array<Amino::String>& exclude_paths)
    USDNODE_DOC_ICON_X("get_or_create_collection",
                       "get_or_create_collection",
                       "usd.svg",
                       "outName=success");

/// \ingroup Collection
/// \defgroup get_all_collection_names get_all_collection_names node
///
/// \brief Returns all collection names of the USD prim.
///
/// \param [in] prim The prim for which you want to get the collection names.
/// \param [out] names The collection names.
USD_NODEDEF_DECL
void get_all_collection_names(
    const BifrostUsd::Prim&                         prim,
    Amino::MutablePtr<Amino::Array<Amino::String>>& names)
    USDNODE_DOC_ICON("get_all_collection_names",
                     "get_all_collection_names",
                     "usd.svg");

/// \ingroup Collection
/// \defgroup get_includes_paths get_includes_paths node
///
/// \brief Returns the Includes Targets' paths of the USD collection.
///
/// \param [in] prim The prim for which you want to get the collection's Includes Targets' paths.
/// \param [in] collection_name The collection name.
/// \param [out] paths The collection's Includes Targets' paths.
USD_NODEDEF_DECL
void get_includes_paths(const BifrostUsd::Prim& prim,
                        const Amino::String&    collection_name,
                        Amino::MutablePtr<Amino::Array<Amino::String>>& paths)
    USDNODE_DOC_ICON("get_includes_paths", "get_includes_paths", "usd.svg");

/// \ingroup Collection
/// \defgroup get_excludes_paths get_excludes_paths node
///
/// \brief Returns the Excludes Targets' paths of the USD collection.
///
/// \param [in] prim The prim for which you want to get the collection's Excludes Targets' paths.
/// \param [in] collection_name The collection name.
/// \param [out] paths The Excludes Targets' paths of the collection.
USD_NODEDEF_DECL
void get_excludes_paths(const BifrostUsd::Prim& prim,
                        const Amino::String&    collection_name,
                        Amino::MutablePtr<Amino::Array<Amino::String>>& paths)
    USDNODE_DOC_ICON("get_excludes_paths", "get_excludes_paths", "usd.svg");

} // namespace Collection
} // namespace USD

#endif // USD_COLLECTION_NODEDEFS_H
