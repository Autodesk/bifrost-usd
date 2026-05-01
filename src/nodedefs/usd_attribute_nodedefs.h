//-
// Copyright 2024 Autodesk, Inc.
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
    AMINO_ANNOTATE(                                     \
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
    MACRO(Bifrost::Math::double2x2)                \
    MACRO(Bifrost::Math::double3x3)                \
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
    MACRO(Bifrost::Math::double2x2)               \
    MACRO(Bifrost::Math::double3x3)               \
    MACRO(Bifrost::Math::double4x4)

// forward declarations
namespace Amino {
class String;
} // namespace Amino

namespace USD {

namespace Attribute {

USD_NODEDEF_DECL
bool create_prim_attribute(BifrostUsd::Stage& stage
                                                USDPORT_INOUT("out_stage"),
                           const Amino::String& prim_path,
                           const Amino::String& name,
                           const BifrostUsd::SdfValueTypeName type_name,
                           const bool                           custom,
                           const BifrostUsd::SdfVariability   variablity)
    USDNODE_DOC_ICON_X("create_prim_attribute",
                       "USD_Attribute_create_prim_attribute.md",
                       "prim_att.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool clear_attribute(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                     const Amino::String&       prim_path,
                     const Amino::String&       name) //
    USDNODE_DOC_ICON_X("clear_attribute",
                       "USD_Attribute_clear_attribute.md",
                       "prim_att.svg",
                       "outName=success");

USD_NODEDEF_DECL
void block_attribute(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                     const Amino::String&       prim_path,
                     const Amino::String&       name)
    USDNODE_DOC_ICON("block_attribute",
                     "USD_Attribute_block_attribute.md",
                     "prim_att.svg");

USD_NODEDEF_DECL
bool create_primvar(
    BifrostUsd::Stage& stage                      USDPORT_INOUT("out_stage"),
    const Amino::String&                            prim_path,
    const Amino::String&                            name,
    const BifrostUsd::SdfValueTypeName            type_name,
    const BifrostUsd::UsdGeomPrimvarInterpolation interpolation,
    const int element_size AMINO_ANNOTATE("Amino::Port value=-1"))
    USDNODE_DOC_ICON_X("create_primvar",
                       "USD_Attribute_create_primvar.md",
                       "prim_att.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool get_prim_attribute(Amino::Ptr<BifrostUsd::Prim> prim,
                        const Amino::String&            attribute_name,
                        Amino::MutablePtr<BifrostUsd::Attribute>& attribute)
    USDNODE_DOC_ICON_X("get_prim_attribute",
                       "USD_Attribute_get_prim_attribute.md",
                       "prim_att.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool get_prim_attribute_type(const BifrostUsd::Attribute&  attribute,
                             BifrostUsd::SdfValueTypeName& type_name)
    USDNODE_DOC_ICON_X("get_prim_attribute_type",
                       "USD_Attribute_get_prim_attribute_type.md",
                       "prim_att.svg",
                       "outName=success");

#define DECLARE_GET_PRIM_ATTRIBUTE_DATA(TYPE)                          \
    USD_NODEDEF_DECL bool get_prim_attribute_data(                     \
        const BifrostUsd::Attribute& attribute, TYPE type,             \
        const float frame FRAME_ANNOTATION, TYPE& value)               \
        USDNODE_DOC_ICON_X("get_prim_attribute_data",                  \
                           "USD_Attribute_get_prim_attribute_data.md", \
                           "usd_get.svg", "outName=success");
FOR_EACH_SUPPORTED_BUILTIN_ATTRIBUTE(DECLARE_GET_PRIM_ATTRIBUTE_DATA)
#undef DECLARE_GET_PRIM_ATTRIBUTE_DATA

#define DECLARE_GET_PRIM_ATTRIBUTE_DATA(TYPE)                          \
    USD_NODEDEF_DECL bool get_prim_attribute_data(                     \
        const BifrostUsd::Attribute& attribute, const TYPE& type,      \
        const float frame FRAME_ANNOTATION, TYPE& value)               \
        USDNODE_DOC_ICON_X("get_prim_attribute_data",                  \
                           "USD_Attribute_get_prim_attribute_data.md", \
                           "usd_get.svg", "outName=success");
FOR_EACH_SUPPORTED_STRUCT_ATTRIBUTE(DECLARE_GET_PRIM_ATTRIBUTE_DATA)
#undef DECLARE_GET_PRIM_ATTRIBUTE_DATA

#define DECLARE_GET_PRIM_ATTRIBUTE_DATA(TYPE)                               \
    USD_NODEDEF_DECL bool get_prim_attribute_data(                          \
        const BifrostUsd::Attribute& attribute,                             \
        const Amino::Array<TYPE>& type, const float frame FRAME_ANNOTATION, \
        Amino::MutablePtr<Amino::Array<TYPE>>& value)                       \
        USDNODE_DOC_ICON_X("get_prim_attribute_data",                       \
                           "USD_Attribute_get_prim_attribute_data.md",      \
                           "usd_get.svg", "outName=success");
FOR_EACH_SUPPORTED_ARRAY_ATTRIBUTE(DECLARE_GET_PRIM_ATTRIBUTE_DATA)
#undef DECLARE_GET_PRIM_ATTRIBUTE_DATA

#define DECLARE_SET_PRIM_ATTRIBUTE_DATA(TYPE)                                  \
    USD_NODEDEF_DECL bool set_prim_attribute(                                  \
        BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),                   \
        const Amino::String& prim_path, const Amino::String& name, TYPE value, \
        const bool use_frame, const float frame FRAME_ANNOTATION)              \
        USDNODE_DOC_ICON_X("set_prim_attribute",                               \
                           "USD_Attribute_set_prim_attribute.md",              \
                           "usd_set.svg", "outName=success");
FOR_EACH_SUPPORTED_BUILTIN_ATTRIBUTE(DECLARE_SET_PRIM_ATTRIBUTE_DATA)
#undef DECLARE_SET_PRIM_ATTRIBUTE_DATA

#define DECLARE_SET_PRIM_ATTRIBUTE_DATA(TYPE)                          \
    USD_NODEDEF_DECL bool set_prim_attribute(                          \
        BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),           \
        const Amino::String& prim_path, const Amino::String& name,     \
        const TYPE& value, const bool use_frame,                       \
        const float frame FRAME_ANNOTATION)                            \
        USDNODE_DOC_ICON_X("set_prim_attribute",                       \
                           "USD_Attribute_set_prim_attribute.md",      \
                           "usd_set.svg", "outName=success");
FOR_EACH_SUPPORTED_STRUCT_ATTRIBUTE(DECLARE_SET_PRIM_ATTRIBUTE_DATA)
#undef DECLARE_SET_PRIM_ATTRIBUTE_DATA

#define DECLARE_SET_PRIM_ATTRIBUTE_DATA(TYPE)                          \
    USD_NODEDEF_DECL bool set_prim_attribute(                          \
        BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),           \
        const Amino::String& prim_path, const Amino::String& name,     \
        const Amino::Array<TYPE>& value, const bool use_frame,         \
        const float frame FRAME_ANNOTATION)                            \
        USDNODE_DOC_ICON_X("set_prim_attribute",                       \
                           "USD_Attribute_set_prim_attribute.md",      \
                           "usd_set.svg", "outName=success");
FOR_EACH_SUPPORTED_ARRAY_ATTRIBUTE(DECLARE_SET_PRIM_ATTRIBUTE_DATA)
#undef DECLARE_SET_PRIM_ATTRIBUTE_DATA

USD_NODEDEF_DECL bool add_attribute_connection(
    BifrostUsd::Stage& stage          USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const Amino::String&                attribute_name,
    const Amino::String&                source,
    const BifrostUsd::UsdListPosition position)
    USDNODE_DOC_ICON_X("add_attribute_connection",
                       "USD_Attribute_add_attribute_connection.md",
                       "usd_set.svg",
                       "outName=success");

USD_NODEDEF_DECL bool remove_attribute_connection(
    BifrostUsd::Stage& stage            USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const Amino::String&                attribute_name,
    const Amino::String&                source)
    USDNODE_DOC_ICON_X("remove_attribute_connection",
                       "USD_Attribute_remove_attribute_connection.md",
                       "prim_att.svg",
                       "outName=success");

USD_NODEDEF_DECL bool clear_attribute_connections(
    BifrostUsd::Stage& stage            USDPORT_INOUT("out_stage"),
    const Amino::String&                prim_path,
    const Amino::String&                attribute_name)
    USDNODE_DOC_ICON_X("clear_attribute_connections",
                       "USD_Attribute_clear_attribute_connections.md",
                       "prim_att.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool get_prim_attribute_connections(const BifrostUsd::Attribute&  attribute,
                             Amino::MutablePtr<Amino::Array<Amino::String>>& connections)
    USDNODE_DOC_ICON_X("get_prim_attribute_connections",
                       "USD_Attribute_get_prim_attribute_connections.md",
                       "prim_att.svg",
                       "outName=success");

USD_NODEDEF_DECL
bool get_time_samples(
    const BifrostUsd::Attribute&              attribute,
    Amino::MutablePtr<Amino::Array<double>>& time_samples)
    USDNODE_DOC_ICON_X("get_time_samples",
                       "USD_Attribute_get_time_samples.md",
                       "usd_get.svg",
                       "outName=success");

#define SET_ATTRIBUTE_METADATA(VALUE_TYPE)                                   \
    USD_NODEDEF_DECL                                                         \
    bool set_attribute_metadata(                                             \
        BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),                 \
        const Amino::String& prim_path, const Amino::String& attribute_name, \
        const Amino::String& key, const VALUE_TYPE& value)                   \
        USDNODE_INTERNAL_X("set_attribute_metadata",                         \
                           "USD_Attribute_set_attribute_metadata.md",        \
                           "outName=success");

SET_ATTRIBUTE_METADATA(Amino::String)
SET_ATTRIBUTE_METADATA(Amino::bool_t)
SET_ATTRIBUTE_METADATA(Amino::float_t)
SET_ATTRIBUTE_METADATA(Amino::double_t)
SET_ATTRIBUTE_METADATA(Amino::int_t)
SET_ATTRIBUTE_METADATA(Amino::long_t)
SET_ATTRIBUTE_METADATA(Bifrost::Object)

#define GET_ATTRIBUTE_METADATA(VALUE_TYPE)                              \
    USD_NODEDEF_DECL                                                    \
    bool get_attribute_metadata(                                        \
        const BifrostUsd::Stage& stage, const Amino::String& prim_path, \
        const Amino::String& attribute_name, const Amino::String& key,  \
        const VALUE_TYPE& default_and_type, VALUE_TYPE& value)          \
        USDNODE_INTERNAL_X("get_attribute_metadata",                    \
                           "USD_Attribute_get_attribute_metadata.md",   \
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
