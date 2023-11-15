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
/// \file  usd_geom_nodedefs.h
/// \brief Bifrost USD Geometry nodes.

#ifndef USD_GEOM_NODEDEFS_H
#define USD_GEOM_NODEDEFS_H

#include <Amino/Core/Array.h>
#include <Amino/Core/Ptr.h>
#include <Bifrost/Math/Types.h>
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
/// \defgroup Prim Prim
namespace Prim {

/// \ingroup Prim
/// \defgroup get_usd_geom_xform_vectors get_usd_geom_xform_vectors node
///
/// \brief This node returns the translation, rotation, scale, pivot and rotation order
/// of a prim.
///
/// \param [in] prim The Xform prim.
/// \param [in] frame The frame at which you want to get the values.
/// \param [out] translation The prim translation.
/// \param [out] rotation The prim rotation.
/// \param [out] scale The prim scale.
/// \param [out] pivot The prim pivot.
/// \param [out] rotation_order The prim rotation order.
/// \returns true if the vectors are found.
USD_NODEDEF_DECL
bool get_usd_geom_xform_vectors(
    const BifrostUsd::Prim& prim,
    const float frame
        AMINO_ANNOTATE("Amino::Port value=1 metadata=[{quick_create, "
                      "string, Core::Time::time.frame}] "),
    Bifrost::Math::float3&         translation,
    Bifrost::Math::float3&         rotation,
    Bifrost::Math::float3&         scale,
    Bifrost::Math::float3&         pivot,
    Bifrost::Math::rotation_order& rotation_order)
    USDNODE_DOC_ICON_X("get_usd_geom_xform_vectors",
                       "get_usd_geom_xform_vectors",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup compute_usdgeom_extent compute_usdgeom_extent node
///
/// \brief This node computes the extent of a boundable prim.
///
/// \param [in] prim The boundable prim.
/// \param [in] frame The frame at which you want to get the extent.
/// \param [in] local Computes in local space.
/// \param [out] extent The min and max postions of the extent.
/// \returns true if the extent was computed.
USD_NODEDEF_DECL
bool compute_usdgeom_extent(
    const BifrostUsd::Prim& prim,
    const float frame
        AMINO_ANNOTATE("Amino::Port value=1 metadata=[{quick_create, "
                      "string, Core::Time::time.frame}] "),
    const bool local AMINO_ANNOTATE("Amino::Port value=true"),
    Amino::MutablePtr<Amino::Array<Bifrost::Math::float3>>& extent)
    USDNODE_DOC_ICON_X("compute_usdgeom_extent",
                       "compute_usdgeom_extent",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup usd_point_instancer usd_point_instancer node
///
/// \brief This node creates a point instancer.
///
/// \param [in] stage The stage in whict to create a point instancer.
/// \param [in] prim_path The path to the point instancer.
/// \param [in] prototypes Array containing the paths for all instance
///                        prototypes. These are the prims that that you are instancing.
/// \param [in] protoindices Array containing all instance prototype indices.
/// \param [in] positions Array containing all instance positions. This array
///                       must be the same size as protoindices.
/// \param [in] orientations Array containing all instance orientations.
///                          This array must be either the same size as
///                          protoindices or empty.
///                          WARNING: USD Array uses halfs instead of floats.
///                          Bifrost  uses floats
///                          so there may be some loss of precision.
/// \param [in] scales Array containing all instance scales. This array must be
///                    either the same size as protoindices or empty.
/// \param [in] velocities Array containing all instance velocities. This array
///                        must be either the same size as protoindices or
///                        empty.
/// \param [in] accelerations Array containing all instance accelerations.
///                           This array must be either the same size as
///                           protoindices or empty.
/// \param [in] angular_velocities Array containing all instance angular
///                                velocities. This array must be either the
///                                same size as protoindices or empty.
/// \param [in] invisible_ids A list of IDs to be made invisible
///                           at evaluation time.
/// \returns true if the point instancer was created successfully.
USD_NODEDEF_DECL
bool usd_point_instancer(
    BifrostUsd::Stage& stage                 USDPORT_INOUT("out_stage"),
    const Amino::String&                       prim_path,
    const Amino::Array<Amino::String>&         prototypes,
    const Amino::Array<int>&                   protoindices,
    const Amino::Array<Bifrost::Math::float3>& positions,
    const Amino::Array<Bifrost::Math::float4>& orientations,
    const Amino::Array<Bifrost::Math::float3>& scales,
    const Amino::Array<Bifrost::Math::float3>& velocities,
    const Amino::Array<Bifrost::Math::float3>& accelerations,
    const Amino::Array<Bifrost::Math::float3>& angular_velocities,
    const Amino::Array<Amino::long_t>&             invisible_ids)
    USDNODE_DOC_ICON_X("usd_point_instancer",
                       "usd_point_instancer",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup usd_volume usd_volume node
///
/// \brief This node creates a USD volume prim.
///
/// \param [in] stage The stage in which to create a volume prim.
/// \param [in] prim_path The path to the volume prim.
/// \param [in] file_format The volume file format.
/// \param [in] field_names A list of field names.
/// \param [in] file_paths A list of file paths pointing to a supported volume
///             file format on disk.
/// \param [in] relationship_names A list of names used by the renderer to
///             associate individual fields with the named input parameters on
///             the volume.
/// \param [in] frame The frame at which to evaluate the files.
/// \returns true if the volume prim was created successfully.
USD_NODEDEF_DECL
bool usd_volume(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                const Amino::String&       prim_path,
                const BifrostUsd::VolumeFieldFormat file_format,
                const Amino::Array<Amino::String>&    field_names,
                const Amino::Array<Amino::String>&    file_paths,
                const Amino::Array<Amino::String>&    relationship_names,
                const float frame                     AMINO_ANNOTATE(
                    "Amino::Port value=1 metadata=[{quick_create, "
                    "string, Core::Time::time.frame}] "))
    USDNODE_DOC_ICON("usd_volume", "usd_volume", "usd.svg");

/// \ingroup Prim
/// \defgroup translate_prim translate_prim node
///
/// \brief This node translates a USD prim.
///
/// \param [in] stage The stage in which to translate the prim.
/// \param [in] prim_path The prim that you wish to translate.
/// \param [in] translation The translation for that prim .
/// \param [in] enable_time Enables time sampling.
/// \param [in] frame the frame at which you wish to translate.
/// \returns true if the prim was translated successfully.
USD_NODEDEF_DECL
bool translate_prim(BifrostUsd::Stage& stage    USDPORT_INOUT("out_stage"),
                    const Amino::String&          prim_path,
                    const Bifrost::Math::double3& translation,
                    const bool                    enable_time,
                    const float                   frame) //
    USDNODE_DOC_ICON_X("translate_prim",
                       "translate_prim",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup rotate_prim rotate_prim node
///
/// \brief This node rotates a USD prim.
///
/// \param [in] stage The stage in which to rotate the prim.
/// \param [in] prim_path The prim that you wish to rotate.
/// \param [in] rotation_order The rotation order.
/// \param [in] rotation The rotation for that prim .
/// \param [in] enable_time Enables time sampling.
/// \param [in] frame The frame at which you wish to set the rotation.
/// \returns true if the prim was rotated successfully.
USD_NODEDEF_DECL
bool rotate_prim(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                 const Amino::String&       prim_path,
                 const Bifrost::Math::rotation_order& rotation_order,
                 const Bifrost::Math::float3&         rotation AMINO_ANNOTATE("Amino::Port metadata=[{UiSoftMin, string, -180}, {UiSoftMax, string, 180}]"),
                 const bool                           enable_time,
                 const float                          frame) //
    USDNODE_DOC_ICON_X("rotate_prim",
                       "rotate_prim",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup scale_prim scale_prim node
///
/// \brief This node scales a USD prim.
///
/// \param [in] stage The stage in which to scale the prim.
/// \param [in] prim_path The prim that you wish to scale.
/// \param [in] scale The scale for that prim.
/// \param [in] enable_time Enables time sampling.
/// \param [in] frame The frame at which you wish to set the scale.
/// \returns true if the prim was scaled successfully.
USD_NODEDEF_DECL
bool scale_prim(BifrostUsd::Stage& stage USDPORT_INOUT("out_stage"),
                const Amino::String&       prim_path,
                const Bifrost::Math::float3& scale
                    AMINO_ANNOTATE("Amino::Port value={x:1.0f, y:1.0f, z:1.0f}"),
                const bool  enable_time,
                const float frame) //
    USDNODE_DOC_ICON_X("scale_prim",
                       "scale_prim",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup set_prim_pivot set_prim_pivot node
///
/// \brief This node sets the prim pivot.
///
/// \param [in] stage The stage in which to set the prim pivot.
/// \param [in] prim_path The prim that you wish to set the pivot for.
/// \param [in] pivot The pivot for that prim.
/// \returns true if the prim pivot was set successfully.
USD_NODEDEF_DECL
bool set_prim_pivot(BifrostUsd::Stage& stage   USDPORT_INOUT("out_stage"),
                    const Amino::String&         prim_path,
                    const Bifrost::Math::float3& pivot) //
    USDNODE_DOC_ICON_X("set_prim_pivot",
                       "set_prim_pivot",
                       "usd.svg",
                       "outName=success");

/// \ingroup Prim
/// \defgroup get_usd_geom_points get_usd_geom_points node
///
/// \brief This node outputs the points data of a point-based USD prim at the requested
/// frame.
///
/// \param [in] prim The USD Prim storing the points.
/// \param [in] local_space Gets the points positions in local space.
///             When off, the positions are returned in world space.
/// \param [in] frame The requested frame.
/// \param [out] points Array containing all the points.
/// \returns true if the prim returned points attribute data.
USD_NODEDEF_DECL
bool get_usd_geom_points(
    const BifrostUsd::Prim& prim,
    const bool                local_space,
    const float frame
        AMINO_ANNOTATE("Amino::Port value=1 metadata=[{quick_create, "
                      "string, Core::Time::time.frame}] "),
    Amino::MutablePtr<Amino::Array<Bifrost::Math::float3>>& points)
    USDNODE_DOC_ICON_X("get_usd_geom_points",
                       "get_usd_geom_points",
                       "usd.svg",
                       "outName=success");

} // namespace Prim
} // namespace USD

#endif // USD_GEOM_NODEDEFS_H
