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
#include <BifrostHydra/Translators/Mesh.h>

#include <pxr/imaging/hd/meshSchema.h>
#include <pxr/imaging/hd/tokens.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {} // namespace

namespace BifrostHd {

Mesh::Mesh(const Bifrost::Object& object, const size_t index) {
    const auto name = std::string{"mesh"} + std::to_string(index);
    m_children_map[PXR_NS::SdfPath{name}] = CreateHdSceneIndexMesh(object);
}

const PXR_NS::TfToken& Mesh::getSceneIndexPrimTypeName() const {
    return HdPrimTypeTokens->mesh;
}

const PXR_NS::HdDataSourceLocator& Mesh::TopologyLocator() const {
    return HdMeshSchema::GetTopologyLocator();
}

const ChildPrimMap& Mesh::getChildren() const { return m_children_map; }

} // namespace BifrostHd
