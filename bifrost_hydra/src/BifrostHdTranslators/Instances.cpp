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

#include <BifrostHydra/Translators/GeometryFn.h>
#include <BifrostHydra/Translators/Instances.h>

#include <Bifrost/Geometry/bifrost_geometry.h>
#include <Bifrost/Object/bifrost_what_is.h>

#include <pxr/imaging/hd/instancerTopologySchema.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {} // namespace

namespace BifrostHd {

Instances::Instances(const Bifrost::Object& object) {
    m_children_map[PXR_NS::SdfPath{"instancer"}] =
        CreateHdSceneIndexExplicitInstancer(object);

    auto instanceShape = BifrostHd::GetInstanceShape(object);
    if (instanceShape) {
        const auto vtPointInstanceIDs = BifrostHd::GetPointInstanceIDs(object);
        if (!vtPointInstanceIDs.empty()) {
            auto shapeID = vtPointInstanceIDs[0];
            auto shape   = BifrostHd::GetShapeFromId(*instanceShape, shapeID);
            if (shape) {
                auto renderGeometry = BifrostHd::GetRenderGeometry(*shape);
                if (renderGeometry) {
                    m_children_map[PXR_NS::SdfPath{"proto0_mesh_id0"}] =
                        CreateHdSceneIndexMesh(*renderGeometry);
                }
            }
        }
    }
}

const TfToken& Instances::getSceneIndexPrimTypeName() const {
    return HdInstancerTopologySchemaTokens->instancerTopology;
}

const HdDataSourceLocator& Instances::TopologyLocator() const {
    return HdInstancerTopologySchema::GetDefaultLocator();
}

const ChildPrimMap& Instances::getChildren() const { return m_children_map; }

} // namespace BifrostHd
