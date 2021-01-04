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
/// \file  usd_attribute_nodedefs.h
/// \brief Bifrost USD Attribute nodes.

#ifndef USD_ATTRIBUTE_NODEDEFS_H
#define USD_ATTRIBUTE_NODEDEFS_H

#include <Amino/Core/Array.h>
#include <Amino/Core/BuiltInTypes.h>
#include <Amino/Core/Ptr.h>
#include <Bifrost/Math/Types.h>
#include <Bifrost/Object/Object.h>
#include <BifrostUsd/Attribute.h>
#include <BifrostUsd/Prim.h>
#include <BifrostUsd/Stage.h>

#include "header_parser_macros.h"
#include "nodedef_export.h"

#define FRAME_ANNOTATION                                \
    AMINO_ANNOTATE(                                      \
        "Amino::Port value=1 metadata=[{quick_create, " \
        "string, Core::Time::time.frame}] ")

#define FOR_EACH_SUPPORTED_BUILTIN_ATTRIBUTE(MACRO) \
    MACRO(Amino::bool_t)                            \
    MACRO(Amino::uchar_t)                           \
    MACRO(Amino::int_t)                             \
    MACRO(Amino::uint_t)                            \
    MACRO(Amino::long_t)                            \
    MACRO(Amino::ulong_t)                           \
    MACRO(Amino::float_t)                           \
    MACRO(Amino::double_t)

// Note that Amino::String is not actually a struct, but as far as operator's
// passing conventions are concerned, Amino::String very often use the same
// passing conventions as struct, so it's convenient to put it there.
#define FOR_EACH_SUPPORTED_STRUCT_ATTRIBUTE(MACRO) \
    MACRO(Amino::String)                           \
    MACRO(Bifrost::Math::float2)                   \
    MACRO(Bifrost::Math::float3)                   \
    MACRO(Bifrost::Math::float4)                   \
    MACRO(Bifrost::Math::double2)                  \
    MACRO(Bifrost::Math::double3)                  \
    MACRO(Bifrost::Math::double4)                  \
    MACRO(Bifrost::Math::double4x4)

#define FOR_EACH_SUPPORTED_ARRAY_ATTRIBUTE(MACRO) \
    MACRO(Amino::bool_t)                          \
    MACRO(Amino::uchar_t)                         \
    MACRO(Amino::int_t)                           \
    MACRO(Amino::uint_t)                          \
    MACRO(Amino::long_t)                          \
    MACRO(Amino::ulong_t)                         \
    MACRO(Amino::float_t)                         \
    MACRO(Amino::double_t)                        \
    MACRO(Amino::String)                          \
    MACRO(Bifrost::Math::float2)                  \
    MACRO(Bifrost::Math::float3)                  \
    MACRO(Bifrost::Math::float4)                  \
    MACRO(Bifrost::Math::double2)                 \
    MACRO(Bifrost::Math::double3)                 \
    MACRO(Bifrost::Math::double4)                 \
    MACRO(Bifrost::Math::double4x4)

// forward declarations
namespace Amino {
class String;
} // namespace Amino

/// \defgroup USD USD
namespace USD {

/// \ingroup USD
/// \defgroup Attribute Attribute
namespace Attribute {

/// \ingroup Prim
/// \defgroup create_prim_attribute create_prim_attribute node
///
/// \brief This node creates a USD attribute on a USD prim.
///
/// \param [in] stage The USD stage in which to create the prim attribute.
/// \param [in] prim_path The prim path to add the attribute.
/// \param [in] name The attribute name.
/// \param [in] type_name The attribute type.
/// \param [in] custom Declares that the new attribute is user-defined.
/// \param [in] variability Indicates whether the attribute may vary over time
/// and value coordinates.
/// \returns true if the attribute was created successfully.
USD_NODEDEF_DECL
bool create_prim_attribute(BifrostUsd::Stage& stage
                                                USDPORT_INOUT("out_stage"),
                           const Amino::String& prim_path,
                           const Amino::String& name,
                           const BifrostUsd::SdfValueTypeName type_name,
                           const bool                           custom,
                           const BifrostUsd::SdfVariability   variablity)
    USDNODE_DOC_ICON_X("create_prim_attribute",
                       "create_prim_attribute",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup clear_attribute clear_attribute node
///
/// \brief Clears the authored default value and all time samples for an attribute.
///
/// \param [in] stage The USD stage.
/// \param [in] prim_path The path of the prim with the attribute.
/// \param [in] name The name of the attribute.
/// \returns true if the attribute was cleared successfully.
USD_NODEDEF_DECL
bool clear_attribute(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                     const Amino::String&       prim_path,
                     const Amino::String&       name) //
    USDNODE_DOC_ICON_X("clear_attribute",
                       "clear_attribute",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup block_attribute block_attribute node
///
/// \brief This node blocks a USD attribute on a USD prim. This causes the attribute
/// to resolve as if there were no authored value opinions in weaker layers.
///
/// \param [in] stage The USD stage on which to block the attribute on the prim.
/// \param [in] prim_path The path of the prim with the attribute.
/// \param [in] name The name of the attribute.
USD_NODEDEF_DECL
void block_attribute(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                     const Amino::String&       prim_path,
                     const Amino::String&       name)
    USDNODE_DOC_ICON("block_attribute", "block_attribute", "usd.svg");

/// \ingroup Prim
/// \defgroup create_primvar create_primvar node
///
/// \brief This node creates a USD UsdGeomPrimvar on a USD prim.
///
/// \param [in] stage The USD stage on which to create the primvar.
/// \param [in] prim_path The prim path on which to add the attribute.
/// \param [in] name The attribute name.
/// \param [in] type_name The attribute type.
/// \param [in] interpolation How the Primvar
///             interpolates over a geometric primitive.
/// \param [in] element_size Return the "element size" for this Primvar, which
///             is 1 if unauthored.
///
/// \returns true if the attribute was created successfully.
USD_NODEDEF_DECL
bool create_primvar(
    BifrostUsd::Stage& stage                      USDPORT_INOUT("out_stage"),
    const Amino::String&                            prim_path,
    const Amino::String&                            name,
    const BifrostUsd::SdfValueTypeName            type_name,
    const BifrostUsd::UsdGeomPrimvarInterpolation interpolation,
    const int element_size AMINO_ANNOTATE("Amino::Port value=-1"))
    USDNODE_DOC_ICON_X("create_primvar",
                       "create_primvar",
                       "usd.svg",
                       "outName=success");

/// \ingroup Attribute
/// \defgroup get_prim_attribute get_prim_attribute node
///
/// \brief This node returns the attribute with the specified name.
///
/// \param [in] prim The USD prim that you want to get the attribute for.
/// \param [in] attribute_name The attribute name.
/// \param [out] attribute The attribute (if it exists).
/// \param returns true if the attribute was returned successfully.
USD_NODEDEF_DECL
bool get_prim_attribute(Amino::Ptr<BifrostUsd::Prim> prim,
                        const Amino::String&            attribute_name,
                        Amino::MutablePtr<BifrostUsd::Attribute>& attribute)
    USDNODE_DOC_ICON_X("get_prim_attribute",
                       "get_prim_attribute",
                       "usd.svg",
                       "outName=success");

/// \ingroup Attribute
/// \defgroup get_prim_attribute_data get_prim_attribute_data node
///
/// \brief This node returns the data of the attribute with the specified name.
///
/// \param [in] attribute The attribute to get.
/// \param [in] type The type of the data.
/// \param [in] frame The frame at which to get the attribute data.
/// \param [out] value The returned value data.
/// \returns true if the data was succcessfully returned.

#define DECLARE_GET_PRIM_ATTRIBUTE_DATA(TYPE)                    \
    USD_NODEDEF_DECL bool get_prim_attribute_data(               \
        const BifrostUsd::Attribute& attribute, TYPE type,     \
        const float frame FRAME_ANNOTATION, TYPE& value)         \
        USDNODE_DOC_ICON_X("get_prim_attribute_data",            \
                           "get_prim_attribute_data", "usd.svg", \
                           "outName=success");
FOR_EACH_SUPPORTED_BUILTIN_ATTRIBUTE(DECLARE_GET_PRIM_ATTRIBUTE_DATA)
#undef DECLARE_GET_PRIM_ATTRIBUTE_DATA

#define DECLARE_GET_PRIM_ATTRIBUTE_DATA(TYPE)                       \
    USD_NODEDEF_DECL bool get_prim_attribute_data(                  \
        const BifrostUsd::Attribute& attribute, const TYPE& type, \
        const float frame FRAME_ANNOTATION, TYPE& value)            \
        USDNODE_DOC_ICON_X("get_prim_attribute_data",               \
                           "get_prim_attribute_data", "usd.svg",    \
                           "outName=success");
FOR_EACH_SUPPORTED_STRUCT_ATTRIBUTE(DECLARE_GET_PRIM_ATTRIBUTE_DATA)
#undef DECLARE_GET_PRIM_ATTRIBUTE_DATA

#define DECLARE_GET_PRIM_ATTRIBUTE_DATA(TYPE)                               \
    USD_NODEDEF_DECL bool get_prim_attribute_data(                          \
        const BifrostUsd::Attribute& attribute,                           \
        const Amino::Array<TYPE>& type, const float frame FRAME_ANNOTATION, \
        Amino::MutablePtr<Amino::Array<TYPE>>& value)                       \
        USDNODE_DOC_ICON_X("get_prim_attribute_data",                       \
                           "get_prim_attribute_data", "usd.svg",            \
                           "outName=success");
FOR_EACH_SUPPORTED_ARRAY_ATTRIBUTE(DECLARE_GET_PRIM_ATTRIBUTE_DATA)
#undef DECLARE_GET_PRIM_ATTRIBUTE_DATA

/// \ingroup Attribute
/// \defgroup set_prim_attribute set_prim_attribute node
///
/// \brief This node sets the attribute value with the specified data.
///
/// \param [in] attribute The attribute you want to get.
/// \param [in] value The data.
/// \param [in] use_frame If enabled, sets the attribute at the given frame.
/// \param [in] frame The frame at which to set the attribute data.
/// \param [out] new_attribute The new attribute with the set data.
/// \param [out] new_stage The new stage with the modified attribute.
/// \returns true if the data was succcessfully set.
#define DECLARE_SET_PRIM_ATTRIBUTE_DATA(TYPE)                                  \
    USD_NODEDEF_DECL bool set_prim_attribute(                                  \
        BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),                 \
        const Amino::String& prim_path, const Amino::String& name, TYPE value, \
        const bool use_frame, const float frame FRAME_ANNOTATION)              \
        USDNODE_DOC_ICON_X("set_prim_attribute", "set_prim_attribute",         \
                           "usd.svg", "outName=success");
FOR_EACH_SUPPORTED_BUILTIN_ATTRIBUTE(DECLARE_SET_PRIM_ATTRIBUTE_DATA)
#undef DECLARE_SET_PRIM_ATTRIBUTE_DATA

#define DECLARE_SET_PRIM_ATTRIBUTE_DATA(TYPE)                          \
    USD_NODEDEF_DECL bool set_prim_attribute(                          \
        BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),         \
        const Amino::String& prim_path, const Amino::String& name,     \
        const TYPE& value, const bool use_frame,                       \
        const float frame FRAME_ANNOTATION)                            \
        USDNODE_DOC_ICON_X("set_prim_attribute", "set_prim_attribute", \
                           "usd.svg", "outName=success");
FOR_EACH_SUPPORTED_STRUCT_ATTRIBUTE(DECLARE_SET_PRIM_ATTRIBUTE_DATA)
#undef DECLARE_SET_PRIM_ATTRIBUTE_DATA

#define DECLARE_SET_PRIM_ATTRIBUTE_DATA(TYPE)                          \
    USD_NODEDEF_DECL bool set_prim_attribute(                          \
        BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),         \
        const Amino::String& prim_path, const Amino::String& name,     \
        const Amino::Array<TYPE>& value, const bool use_frame,         \
        const float frame FRAME_ANNOTATION)                            \
        USDNODE_DOC_ICON_X("set_prim_attribute", "set_prim_attribute", \
                           "usd.svg", "outName=success");
FOR_EACH_SUPPORTED_ARRAY_ATTRIBUTE(DECLARE_SET_PRIM_ATTRIBUTE_DATA)
#undef DECLARE_SET_PRIM_ATTRIBUTE_DATA

/// \ingroup Attribute
/// \defgroup add_attribute_connection add_attribute_connection node
///
/// \brief Adds \p source to the list of connections in the
/// specified \p position.
///
/// \param [in] stage The USD stage in which to add an attribute connection.
/// \param [in] prim_path The path to the prim holding the attribute.
/// \param [in] attribute_name The name of the attribute.
/// \param [in] source Full path of to the source to connect to.
/// \returns true if the attribute connection was added successfully.
USD_NODEDEF_DECL bool add_attribute_connection(
    BifrostUsd::Stage& stage          USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const Amino::String&                attribute_name,
    const Amino::String&                source,
    const BifrostUsd::UsdListPosition position)
    USDNODE_DOC_ICON_X("add_attribute_connection",
                       "add_attribute_connection",
                       "usd.svg",
                       "outName=success");

/// \ingroup Attribute
/// \defgroup remove_attribute_connection remove_attribute_connection node
///
/// \brief Removes \p source from the list of connections.
///
/// \param [in] stage The USD stage in which to remove an attribute connection.
/// \param [in] prim_path The path to the prim holding the attribute.
/// \param [in] attribute_name The name of the attribute.
/// \param [in] source Full path of to the source to remove.
/// \returns true if the attribute connection was removed successfully.
USD_NODEDEF_DECL bool remove_attribute_connection(
    BifrostUsd::Stage& stage            USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const Amino::String&                attribute_name,
    const Amino::String&                source)
    USDNODE_DOC_ICON_X("remove_attribute_connection",
                       "remove_attribute_connection",
                       "usd.svg",
                       "outName=success");

/// \ingroup Attribute
/// \defgroup clear_attribute_connections clear_attribute_connections node
///
/// \brief Clears every attribute connection.
///
/// \param [in] stage The USD stage in which to remove attribute connections.
/// \param [in] prim_path The path to the prim holding the attribute.
/// \param [in] attribute_name The name of the attribute.
/// \returns true if the attribute connections were removed successfully.
USD_NODEDEF_DECL bool clear_attribute_connections(
    BifrostUsd::Stage& stage            USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const Amino::String&                attribute_name)
    USDNODE_DOC_ICON_X("clear_attribute_connections",
                       "clear_attribute_connections",
                       "usd.svg",
                       "outName=success");

/// \ingroup Attribute
/// \defgroup set_attribute_metadata set_attribute_metadata node
///
/// \brief This node set the value of an attribute's metadata.
///
/// \param [in] prim_path The path to the prim holding the attribute.
/// \param [in] attribute_name The name of the attribute.
/// \param [in] key The metadatum key.
/// \param [in] value The metadatum value.
/// \param [in] stage The stage in which to set the metadatum.
#define SET_ATTRIBUTE_METADATA(VALUE_TYPE)                                     \
    USD_NODEDEF_DECL                                                           \
    bool set_attribute_metadata(                                               \
        BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),                 \
        const Amino::String& prim_path, const Amino::String& attribute_name,   \
        const Amino::String& key, const VALUE_TYPE& value)                     \
        USDNODE_INTERNAL_X("set_attribute_metadata", "set_attribute_metadata", \
                           "outName=success");

SET_ATTRIBUTE_METADATA(Amino::String)
SET_ATTRIBUTE_METADATA(Amino::bool_t)
SET_ATTRIBUTE_METADATA(Amino::float_t)
SET_ATTRIBUTE_METADATA(Amino::double_t)
SET_ATTRIBUTE_METADATA(Amino::int_t)
SET_ATTRIBUTE_METADATA(Amino::long_t)
SET_ATTRIBUTE_METADATA(Bifrost::Object)

/// \ingroup Attribute
/// \defgroup get_attribute_metadata get_attribute_metadata node
///
/// \brief This node gets the value of an attribute's metadata.
///
/// \param [in] prim_path The path to the prim holding the attribute.
/// \param [in] attribute_name The name of the attribute.
/// \param [in] key The metadatum key.
/// \param [in] stage The USD stage.
/// \param [out] value The metadatum value.
#define GET_ATTRIBUTE_METADATA(VALUE_TYPE)                                     \
    USD_NODEDEF_DECL                                                           \
    bool get_attribute_metadata(                                               \
        const BifrostUsd::Stage& stage, const Amino::String& prim_path,      \
        const Amino::String& attribute_name, const Amino::String& key,         \
        const VALUE_TYPE& default_and_type, VALUE_TYPE& value)                 \
        USDNODE_INTERNAL_X("get_attribute_metadata", "get_attribute_metadata", \
                           "outName=success");

GET_ATTRIBUTE_METADATA(Amino::String)
GET_ATTRIBUTE_METADATA(Amino::bool_t)
GET_ATTRIBUTE_METADATA(Amino::float_t)
GET_ATTRIBUTE_METADATA(Amino::double_t)
GET_ATTRIBUTE_METADATA(Amino::int_t)
GET_ATTRIBUTE_METADATA(Amino::long_t)
GET_ATTRIBUTE_METADATA(Amino::Ptr<Bifrost::Object>)

} // namespace Attribute
} // namespace USD

#endif // USD_ATTRIBUTE_NODEDEFS_H
