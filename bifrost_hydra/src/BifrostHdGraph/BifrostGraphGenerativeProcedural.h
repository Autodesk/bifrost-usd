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

#ifndef BIFROST_HD_GRAPH_PROCEDURAL_H
#define BIFROST_HD_GRAPH_PROCEDURAL_H

#include <pxr/imaging/hdGp/generativeProcedural.h>

#include <BifrostHydra/Engine/Engine.h>
#include <BifrostHydra/Translators/Geometry.h>

PXR_NAMESPACE_OPEN_SCOPE


class BifrostGraphGenerativeProcedural : public HdGpGenerativeProcedural {
public:
    explicit BifrostGraphGenerativeProcedural(const SdfPath& proceduralPrimPath);

    DependencyMap UpdateDependencies(
        const HdSceneIndexBaseRefPtr& /*inputScene*/) override;

    HdGpGenerativeProcedural::ChildPrimTypeMap Update(
        const HdSceneIndexBaseRefPtr&                     inputScene,
        const HdGpGenerativeProcedural::ChildPrimTypeMap& previousResult,
        const HdGpGenerativeProcedural::DependencyMap&    dirtiedDependencies,
        HdSceneIndexObserver::DirtiedPrimEntries* outputDirtiedPrims) override;

    HdSceneIndexPrim GetChildPrim(const HdSceneIndexBaseRefPtr& inputScene,
                                  const SdfPath& childPrimPath) override;

private:
    using BifrostTranslatorsMap = std::unordered_map<SdfPath, std::shared_ptr<BifrostHd::Geometry>, TfHash>;

    BifrostHd::Engine                    m_engine;
    BifrostTranslatorsMap                m_geomTranslators; 
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // BIFROST_HD_GRAPH_PROCEDURAL_H
