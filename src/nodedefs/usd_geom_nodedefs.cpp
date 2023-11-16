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

#include "usd_geom_nodedefs.h"

#include <Amino/Core/String.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/inherits.h>
#include <pxr/usd/usd/references.h>

#include <iostream>

#include "logger.h"
#include "return_guard.h"
#include "usd_utils.h"

// Note: To silence warnings coming from USD library
BIFUSD_WARNING_PUSH
BIFUSD_WARNING_DISABLE_MSC(4003)
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/payloads.h>
#include <pxr/usd/usdGeom/boundable.h>
#include <pxr/usd/usdGeom/imageable.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <pxr/usd/usdVol/field3DAsset.h>
#include <pxr/usd/usdVol/openVDBAsset.h>
#include <pxr/usd/usdVol/volume.h>
BIFUSD_WARNING_POP

using namespace USDUtils;

bool USD::Prim::get_usd_geom_xform_vectors(
    const BifrostUsd::Prim&      prim,
    const float                    frame,
    Bifrost::Math::float3&         translation,
    Bifrost::Math::float3&         rotation,
    Bifrost::Math::float3&         scale,
    Bifrost::Math::float3&         pivot,
    Bifrost::Math::rotation_order& rotation_order) {
    bool success = false;

    try {
        if (prim) {
            PXR_NS::UsdGeomXformCommonAPI xform_api =
                PXR_NS::UsdGeomXformCommonAPI(prim.getPxrPrim());
            if (xform_api) {
                PXR_NS::GfVec3d pxr_translation;
                PXR_NS::GfVec3f pxr_rotation, pxr_scale, pxr_pivot;
                PXR_NS::UsdGeomXformCommonAPI::RotationOrder pxr_rotOrder;
                auto time = PXR_NS::UsdTimeCode(static_cast<double>(frame));

                success = xform_api.GetXformVectors(
                    &pxr_translation, &pxr_rotation, &pxr_scale, &pxr_pivot,
                    &pxr_rotOrder, time);

                if (success) {
                    translation.x = static_cast<float>(pxr_translation[0]);
                    translation.y = static_cast<float>(pxr_translation[1]);
                    translation.z = static_cast<float>(pxr_translation[2]);

                    rotation.x = pxr_rotation[0];
                    rotation.y = pxr_rotation[1];
                    rotation.z = pxr_rotation[2];

                    scale.x = pxr_scale[0];
                    scale.y = pxr_scale[1];
                    scale.z = pxr_scale[2];

                    pivot.x = pxr_pivot[0];
                    pivot.y = pxr_pivot[1];
                    pivot.z = pxr_pivot[2];

                    rotation_order = GetRotationOrder(pxr_rotOrder);
                }
            }
        }

    } catch (std::exception& e) {
        log_exception("compute_usdgeom_extent", e);
    }
    return success;
}

bool USD::Prim::compute_usdgeom_extent(
    const BifrostUsd::Prim&                               prim,
    const float                                             frame,
    const bool                                              local,
    Amino::MutablePtr<Amino::Array<Bifrost::Math::float3>>& extent) {
    extent = Amino::newMutablePtr<Amino::Array<Bifrost::Math::float3>>(2);
    if (!prim) return false;

    bool success = false;
    try {
        auto boundable = PXR_NS::UsdGeomBoundable(prim.getPxrPrim());
        if (!boundable) return false;

        PXR_NS::VtVec3fArray pxr_extent;
        auto              time = PXR_NS::UsdTimeCode(static_cast<double>(frame));

        if (local) {
            success = PXR_NS::UsdGeomBoundable::ComputeExtentFromPlugins(
                boundable, time, &pxr_extent);
        } else {
            auto xform_api = PXR_NS::UsdGeomXformCommonAPI(prim.getPxrPrim());
            if (xform_api) {
                auto xformCache = PXR_NS::UsdGeomXformCache(time);
                auto xform =
                    xformCache.GetLocalToWorldTransform(prim.getPxrPrim());

                success = PXR_NS::UsdGeomBoundable::ComputeExtentFromPlugins(
                    boundable, time, xform, &pxr_extent);
            } else {
                success = PXR_NS::UsdGeomBoundable::ComputeExtentFromPlugins(
                    boundable, time, &pxr_extent);
            }
        }
        if (!success) return false;

        (*extent)[0].x = pxr_extent[0][0];
        (*extent)[0].y = pxr_extent[0][1];
        (*extent)[0].z = pxr_extent[0][2];

        (*extent)[1].x = pxr_extent[1][0];
        (*extent)[1].y = pxr_extent[1][1];
        (*extent)[1].z = pxr_extent[1][2];
    }

    catch (std::exception& e) {
        log_exception("compute_usdgeom_extent", e);
    }
    return success;
}

bool USD::Prim::usd_point_instancer(
    BifrostUsd::Stage&                       stage,
    const Amino::String&                       prim_path,
    const Amino::Array<Amino::String>&         prototypes,
    const Amino::Array<int>&                   protoindices,
    const Amino::Array<Bifrost::Math::float3>& positions,
    const Amino::Array<Bifrost::Math::float4>& orientations,
    const Amino::Array<Bifrost::Math::float3>& scales,
    const Amino::Array<Bifrost::Math::float3>& velocities,
    const Amino::Array<Bifrost::Math::float3>& accelerations,
    const Amino::Array<Bifrost::Math::float3>& angular_velocities,
    const Amino::Array<Amino::long_t>&             invisible_ids) {
    if (!stage) return false;

    bool success = false;
    try {
        auto instancer = PXR_NS::UsdGeomPointInstancer::Define(
            stage.getStagePtr(), PXR_NS::SdfPath(prim_path.c_str()));
        if (instancer) {
            success = true;
            // Add the prototypes
            if (!prototypes.empty()) {
                PXR_NS::SdfPathVector targets = {};
                for (size_t i = 0; i < prototypes.size(); ++i) {
                    targets.push_back(PXR_NS::SdfPath(prototypes[i].c_str()));
                }
                auto prototypes_rel = instancer.CreatePrototypesRel();
                success = success && prototypes_rel.SetTargets(targets);
            }
            // Add the protoindices
            if (!protoindices.empty()) {
                PXR_NS::VtIntArray pxr_protoindices;
                copy_array<Amino::Array<int>, PXR_NS::VtIntArray>(
                    protoindices, pxr_protoindices);
                auto protoindices_attr = instancer.CreateProtoIndicesAttr();
                success = success && protoindices_attr.Set(pxr_protoindices);
            }
            // Add the positions
            if (!positions.empty()) {
                PXR_NS::VtVec3fArray pxr_positions;
                copy_array(positions, pxr_positions);
                auto positions_attr = instancer.CreatePositionsAttr();
                success = success && positions_attr.Set(pxr_positions);
            }
            // Add the orientations
            if (!orientations.empty()) {
                PXR_NS::VtQuathArray pxr_orientations;
                copy_array(orientations, pxr_orientations);
                auto orientations_attr = instancer.CreateOrientationsAttr();
                success = success && orientations_attr.Set(pxr_orientations);
            } else if (!positions.empty()) { // Add default orientations
                PXR_NS::VtQuathArray pxr_orientations;
                pxr_orientations.resize(positions.size());
                for (size_t i = 0; i < pxr_orientations.size(); ++i) {
                    pxr_orientations[i].SetReal(PXR_NS::pxr_half::half(0.f));
                    pxr_orientations[i].SetImaginary(PXR_NS::pxr_half::half(0.f),
                                                     PXR_NS::pxr_half::half(0.f),
                                                     PXR_NS::pxr_half::half(0.f));
                }
                auto orientations_attr = instancer.CreateOrientationsAttr();
                success = success && orientations_attr.Set(pxr_orientations);
            }
            // Add the scales
            if (!scales.empty()) {
                PXR_NS::VtVec3fArray pxr_scales;
                pxr_scales.resize(scales.size());
                copy_array(scales, pxr_scales);
                auto scales_attr = instancer.CreateScalesAttr();
                success          = success && scales_attr.Set(pxr_scales);
            } else if (!positions.empty()) { // Add default scales
                PXR_NS::VtVec3fArray pxr_scales;
                pxr_scales.resize(positions.size());
                for (size_t i = 0; i < pxr_scales.size(); ++i) {
                    pxr_scales[i][0] = 1.0f;
                    pxr_scales[i][1] = 1.0f;
                    pxr_scales[i][2] = 1.0f;
                }
                auto scales_attr = instancer.CreateScalesAttr();
                success          = success && scales_attr.Set(pxr_scales);
            }
            // Add the velocities
            if (!velocities.empty()) {
                PXR_NS::VtVec3fArray pxr_velocities;
                copy_array(velocities, pxr_velocities);
                auto velocities_attr = instancer.CreateVelocitiesAttr();
                success = success && velocities_attr.Set(pxr_velocities);
            }
            // Add the accelerations
            if (!accelerations.empty()) {
                PXR_NS::VtVec3fArray pxr_accelerations;
                copy_array(accelerations, pxr_accelerations);
                auto accelerations_attr = instancer.CreateAccelerationsAttr();
                success = success && accelerations_attr.Set(pxr_accelerations);
            }
            // Add the angular velocities
            if (!angular_velocities.empty()) {
                PXR_NS::VtVec3fArray pxr_angular_velocities;
                copy_array(angular_velocities, pxr_angular_velocities);
                auto angular_velocities_attr =
                    instancer.CreateAngularVelocitiesAttr();
                success = success &&
                          angular_velocities_attr.Set(pxr_angular_velocities);
            }
            // Add the invisible ids
            if (!invisible_ids.empty()) {
                PXR_NS::VtInt64Array pxr_invisible_ids;
                copy_array<Amino::Array<long long>, PXR_NS::VtInt64Array>(
                    invisible_ids, pxr_invisible_ids);
                auto invisible_ids_attr = instancer.CreateInvisibleIdsAttr();
                success = success && invisible_ids_attr.Set(pxr_invisible_ids);
            }
        }

    } catch (std::exception& e) {
        log_exception("usd_point_instancer", e);
        success = false;
    }
    return success;
}

bool USD::Prim::usd_volume(
    BifrostUsd::Stage&                  stage,
    const Amino::String&                  prim_path,
    const BifrostUsd::VolumeFieldFormat file_format,
    const Amino::Array<Amino::String>&    field_names,
    const Amino::Array<Amino::String>&    file_paths,
    const Amino::Array<Amino::String>&    relationship_names,
    const float                           frame) {
    if (!stage) return false;

    bool success = false;

    try {
        if (field_names.size() != file_paths.size()) {
            throw std::runtime_error(
                "field_names size must be equal to file_paths");
        }
        if (!relationship_names.empty() &&
            relationship_names.size() != field_names.size()) {
            throw std::runtime_error(
                "relationship_names should be empty or same size than "
                "field_names");
        }

        PXR_NS::SdfPath volumePath(PXR_NS::SdfPath(prim_path.c_str()));
        auto         volume =
            PXR_NS::UsdVolVolume::Define(stage.getStagePtr(), volumePath);
        if (volume) {
            for (size_t i = 0; i < field_names.size(); ++i) {
                PXR_NS::TfToken fieldPathName(field_names[i].c_str());
                PXR_NS::SdfPath fieldPath = volumePath.AppendChild(fieldPathName);

                PXR_NS::TfToken fieldName(field_names[i].c_str());
                PXR_NS::TfToken relationshipName(fieldName);
                if (!relationship_names.empty()) {
                    relationshipName =
                        PXR_NS::TfToken(relationship_names[i].c_str());
                }

                PXR_NS::SdfAssetPath file_path(file_paths[i].c_str());

                if (file_format == BifrostUsd::VolumeFieldFormat::OpenVDB) {
                    auto fieldPrim = PXR_NS::UsdVolOpenVDBAsset::Define(
                        stage.getStagePtr(), fieldPath);

                    set_volume_field_relationship(
                        fieldPrim, fieldName, relationshipName, file_path,
                        volume, static_cast<double>(frame));
                } else if (file_format == BifrostUsd::VolumeFieldFormat::Field3D) {
                    auto fieldPrim = PXR_NS::UsdVolField3DAsset::Define(
                        stage.getStagePtr(), fieldPath);

                    set_volume_field_relationship(
                        fieldPrim, fieldName, relationshipName, file_path,
                        volume, static_cast<double>(frame));
                }
            }
            success = true;
        }

    } catch (std::exception& e) {
        log_exception("usd_volume", e);
        success = false;
    }
    return success;
}

bool USD::Prim::translate_prim(BifrostUsd::Stage&          stage,
                               const Amino::String&          prim_path,
                               const Bifrost::Math::double3& position,
                               const bool                    enable_time,
                               const float                   frame) {
    if (!stage) return false;

    bool success = false;
    try {
        VariantEditContext ctx(stage);

        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        if (!pxr_prim) {
            throw std::runtime_error("Can't find prim at path " +
                                     std::string(prim_path.c_str()));
        }

        auto time = PXR_NS::UsdTimeCode::Default();
        if (enable_time) {
            time = PXR_NS::UsdTimeCode(static_cast<double>(frame));
        }

        auto xform_api = PXR_NS::UsdGeomXformCommonAPI(pxr_prim);
        if (xform_api) {
            success = xform_api.SetTranslate(GetVec3d(position), time);
        } else {
            auto                xformable = PXR_NS::UsdGeomXformable(pxr_prim);
            PXR_NS::UsdGeomXformOp op;
            if (xformable) {
                op = xformable.AddTranslateOp();
            } else {
                throw std::runtime_error("Can't translate prim at path " +
                                         std::string(prim_path.c_str()) +
                                         ". Invalid UsdGeomXformable");
            }
            if (op) {
                success = op.Set(GetVec3d(position), time);
            } else {
                throw std::runtime_error("Can't translate prim at path " +
                                         std::string(prim_path.c_str()) +
                                         ". Invalid UsdGeomXformOp");
            }
        }

    } catch (std::exception& e) {
        log_exception("translate_prim", e);
        success = false;
    }
    return success;
}

bool USD::Prim::rotate_prim(BifrostUsd::Stage&                 stage,
                            const Amino::String&                 prim_path,
                            const Bifrost::Math::rotation_order& rotation_order,
                            const Bifrost::Math::float3&         rotation,
                            const bool                           enable_time,
                            const float                          frame) {
    if (!stage) return false;

    bool success = false;
    try {
        VariantEditContext ctx(stage);

        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        if (!pxr_prim) {
            throw std::runtime_error("Can't find prim at path " +
                                     std::string(prim_path.c_str()));
        }

        auto time = PXR_NS::UsdTimeCode::Default();
        if (enable_time) {
            time = PXR_NS::UsdTimeCode(static_cast<double>(frame));
        }

        PXR_NS::UsdGeomXformCommonAPI xform_api =
            PXR_NS::UsdGeomXformCommonAPI(pxr_prim);
        if (xform_api) {
            success = xform_api.SetRotate(
                GetVec3f(rotation), GetUsdRotationOrder(rotation_order), time);
        } else {
            auto                xformable = PXR_NS::UsdGeomXformable(pxr_prim);
            PXR_NS::UsdGeomXformOp op;
            if (xformable) {
                switch (rotation_order) {
                    case Bifrost::Math::rotation_order::XYZ:
                        op = xformable.AddRotateXYZOp();
                        break;
                    case Bifrost::Math::rotation_order::XZY:
                        op = xformable.AddRotateXZYOp();
                        break;
                    case Bifrost::Math::rotation_order::YXZ:
                        op = xformable.AddRotateYXZOp();
                        break;
                    case Bifrost::Math::rotation_order::YZX:
                        op = xformable.AddRotateYZXOp();
                        break;
                    case Bifrost::Math::rotation_order::ZXY:
                        op = xformable.AddRotateZXYOp();
                        break;
                    case Bifrost::Math::rotation_order::ZYX:
                        op = xformable.AddRotateZYXOp();
                        break;
                }
            }
            if (op) {
                success = op.Set(GetVec3f(rotation), time);
            }
        }

    } catch (std::exception& e) {
        log_exception("rotate_prim", e);
        success = false;
    }
    return success;
}

bool USD::Prim::scale_prim(BifrostUsd::Stage&         stage,
                           const Amino::String&         prim_path,
                           const Bifrost::Math::float3& scale,
                           const bool                   enable_time,
                           const float                  frame) {
    if (!stage) return false;

    bool success = false;
    try {
        VariantEditContext ctx(stage);

        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        if (!pxr_prim) return false;

        auto time = PXR_NS::UsdTimeCode::Default();
        if (enable_time) {
            time = PXR_NS::UsdTimeCode(static_cast<double>(frame));
        }

        PXR_NS::UsdGeomXformCommonAPI xform_api =
            PXR_NS::UsdGeomXformCommonAPI(pxr_prim);
        if (xform_api) {
            success = xform_api.SetScale(GetVec3f(scale), time);
        } else {
            auto                xformable = PXR_NS::UsdGeomXformable(pxr_prim);
            PXR_NS::UsdGeomXformOp op;
            if (xformable) op = xformable.AddScaleOp();
            if (op) {
                success = op.Set(GetVec3f(scale), time);
            }
        }

    } catch (std::exception& e) {
        log_exception("scale_prim", e);
        success = false;
    }
    return success;
}

bool USD::Prim::set_prim_pivot(BifrostUsd::Stage&         stage,
                               const Amino::String&         prim_path,
                               const Bifrost::Math::float3& pivot) {
    if (!stage) return false;

    bool success = false;
    try {
        auto pxr_prim = USDUtils::get_prim_at_path(prim_path, stage);
        if (!pxr_prim) return false;

        VariantEditContext ctx(stage);

        PXR_NS::UsdGeomXformCommonAPI xform_api =
            PXR_NS::UsdGeomXformCommonAPI(pxr_prim);
        if (xform_api) {
            success = xform_api.SetPivot(GetVec3f(pivot));
        }

    } catch (std::exception& e) {
        log_exception("set_prim_pivot", e);
        success = false;
    }
    return success;
}

bool USD::Prim::get_usd_geom_points(
    const BifrostUsd::Prim&                               prim,
    const bool                                              local_space,
    const float                                             frame,
    Amino::MutablePtr<Amino::Array<Bifrost::Math::float3>>& points) {
    points = Amino::newMutablePtr<Amino::Array<Bifrost::Math::float3>>();
    if (!prim) return false;

    bool success = false;
    try {
        auto points_attribute =
            prim.getPxrPrim().GetAttribute(PXR_NS::UsdGeomTokens->points);
        if (!points_attribute) {
            return false;
        }

        PXR_NS::VtArray<PXR_NS::GfVec3f> pxr_points;
        success = points_attribute.Get(&pxr_points, static_cast<double>(frame));
        if (!success) return false;

        if (!local_space) {
            auto xform_api = PXR_NS::UsdGeomXformCommonAPI(prim.getPxrPrim());
            if (xform_api) {
                PXR_NS::GfMatrix4d xform;
                auto            xformCache =
                    PXR_NS::UsdGeomXformCache(static_cast<double>(frame));
                xform = xformCache.GetLocalToWorldTransform(prim.getPxrPrim());

                for (size_t i = 0; i < pxr_points.size(); ++i) {
                    PXR_NS::GfVec4f global_point(pxr_points[i][0],
                                              pxr_points[i][1],
                                              pxr_points[i][2], 1.f);
                    global_point     = global_point * xform;
                    pxr_points[i][0] = global_point[0];
                    pxr_points[i][1] = global_point[1];
                    pxr_points[i][2] = global_point[2];
                }
            }
        }

        points->resize(pxr_points.size());
        // copy into our array
        for (size_t i = 0; i < pxr_points.size(); ++i) {
            (*points)[i].x = pxr_points[i][0];
            (*points)[i].y = pxr_points[i][1];
            (*points)[i].z = pxr_points[i][2];
        }

    } catch (std::exception& e) {
        log_exception("get_usd_geom_points", e);
        success = false;
    }
    return success;
}
