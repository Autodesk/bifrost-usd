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
#include <BifrostHydra/Translators/Strands.h>

#include <pxr/imaging/hd/basisCurvesSchema.h>
#include <pxr/imaging/hd/tokens.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {} // namespace

namespace BifrostHd {

Strands::Strands(const Bifrost::Object& object, const size_t index) {
    const auto name = std::string{"curves"} + std::to_string(index);
    m_children_map[PXR_NS::SdfPath{name}] =
        CreateHdSceneIndexBasisCurves(object);
}

const PXR_NS::TfToken& Strands::getSceneIndexPrimTypeName() const {
    return HdPrimTypeTokens->basisCurves;
}

const PXR_NS::HdDataSourceLocator& Strands::TopologyLocator() const {
    return HdBasisCurvesSchema::GetTopologyLocator();
}

const ChildPrimMap& Strands::getChildren() const { return m_children_map; }

} // namespace BifrostHd
