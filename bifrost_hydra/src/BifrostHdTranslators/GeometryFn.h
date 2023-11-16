//-
// Copyright 2023 Autodesk, Inc.
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
/// \file  GeometryFn.h
/// \brief Functions to access Bifrost objects data and create HdSceneIndexPrims.

#ifndef BIFROST_HD_GRAPH_GEOMETRY_FN_H
#define BIFROST_HD_GRAPH_GEOMETRY_FN_H

#include <Bifrost/Object/Object.h>

#include <pxr/imaging/hd/sceneIndex.h>

enum class BifrostHdGeoTypes {
    Empty,
    Mesh,
    Strands,
    PointCloud,
    Instances
};

namespace BifrostHd {

BifrostHdGeoTypes GetGeoType(const Bifrost::Object& object);

// Bifrost geometry object accesors
size_t GetPointCount(const Bifrost::Object& object);

PXR_NS::VtVec3fArray GetPoints(const Bifrost::Object& object);

PXR_NS::VtVec3fArray GetDisplayColor(const Bifrost::Object& object);

PXR_NS::VtIntArray GetFaceVertexIndices(const Bifrost::Object& object);

PXR_NS::VtFloatArray GetWidth(const Bifrost::Object& object);

PXR_NS::VtInt64Array GetPointIDs(const Bifrost::Object& object);

PXR_NS::VtInt64Array GetPointInstanceIDs(const Bifrost::Object& object);

Amino::Ptr<Bifrost::Object> GetInstanceShape(const Bifrost::Object& object);

Amino::Ptr<Bifrost::Object> GetShapeFromId(const Bifrost::Object& object,
                                           const Amino::long_t    id);

Amino::Ptr<Bifrost::Object> GetRenderGeometry(const Bifrost::Object& object);

// Bifrost to Hydra translators
PXR_NS::HdSceneIndexPrim CreateHdSceneIndexMesh(const Bifrost::Object& object);

PXR_NS::HdSceneIndexPrim CreateHdSceneIndexBasisCurves(
    const Bifrost::Object& object);

PXR_NS::HdSceneIndexPrim CreateHdSceneIndexExplicitInstancer(
    const Bifrost::Object& object);

// Hydra to Bifrost translators

Amino::Ptr<Bifrost::Object> CreateBifrostMesh(const PXR_NS::HdSceneIndexPrim& prim);

} // namespace BifrostHd

#endif // BIFROST_HD_GRAPH_GEOMETRY_FN_H
