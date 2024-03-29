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

#ifndef BIFROST_HD_GRAPH_MESH_H
#define BIFROST_HD_GRAPH_MESH_H

#include <Bifrost/Object/Object.h>
#include <BifrostHydra/Translators/Geometry.h>

namespace BifrostHd {

class BIFROST_HD_TRANSLATORS_SHARED_DECL Mesh : public Geometry {
public:
    explicit Mesh(const Bifrost::Object& object, const size_t index=0);

    const PXR_NS::TfToken& getSceneIndexPrimTypeName() const override;

    const PXR_NS::HdDataSourceLocator& TopologyLocator() const override;

    const ChildPrimMap& getChildren() const override;

private:
};

} // namespace BifrostHd

#endif // BIFROST_HD_GRAPH_MESH_H
