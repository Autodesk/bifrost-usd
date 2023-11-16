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

#include "BifrostGraphGenerativeProcedural.h"

#include <BifrostHydra/Translators/GeometryFn.h>
#include <BifrostHydra/Translators/Instances.h>
#include <BifrostHydra/Translators/Mesh.h>
#include <BifrostHydra/Translators/Strands.h>

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

BifrostGraphGenerativeProcedural::BifrostGraphGenerativeProcedural(
    const SdfPath& proceduralPrimPath)
    : HdGpGenerativeProcedural(proceduralPrimPath) {}

HdGpGenerativeProcedural::DependencyMap
BifrostGraphGenerativeProcedural::UpdateDependencies(
    const HdSceneIndexBaseRefPtr& /*inputScene*/) {
    HdGpGenerativeProcedural::DependencyMap result;

    return result;
}

// Cooks/Recooks and returns the current state of child paths and their
// types. At the moment we create a new Bifrost Graph when any of the parameters
// on the procedural are changed.
HdGpGenerativeProcedural::ChildPrimTypeMap
BifrostGraphGenerativeProcedural::Update(
    const HdSceneIndexBaseRefPtr& inputScene,
    const ChildPrimTypeMap& /*previousResult*/,
    const DependencyMap& /*dirtiedDependencies*/,
    HdSceneIndexObserver::DirtiedPrimEntries* outputDirtiedPrims) {
    ChildPrimTypeMap result;
    auto             graphPath = _GetProceduralPrimPath();
    m_engine.setInputScene(inputScene);
    m_engine.setInputs(inputScene->GetPrim(graphPath));

    if (m_engine.execute(/*frame*/ 0.0) == Amino::Job::State::kSuccess) {
        const auto& output      = m_engine.output();
        const auto& objectArray = output.second;

        for (size_t i = 0; i < objectArray.size(); ++i) {
            // We need to cache if the geo is an instance as it is
            // costly to re-do it for each path in the sub-loop later on

            std::shared_ptr<BifrostHd::Geometry> geo;
            const auto&                          obj = *(objectArray[i]);

            auto geoType = BifrostHd::GetGeoType(obj);

            switch (geoType) {
                case BifrostHdGeoTypes::Empty: break;
                case BifrostHdGeoTypes::Mesh:
                    geo = std::make_shared<BifrostHd::Mesh>(obj, i);
                    break;
                case BifrostHdGeoTypes::Strands:
                    geo = std::make_shared<BifrostHd::Strands>(obj, i);
                    break;
                case BifrostHdGeoTypes::PointCloud: break;
                case BifrostHdGeoTypes::Instances:
                    geo = std::make_shared<BifrostHd::Instances>(obj);
                    break;
            }

            if (geo) {
                auto   path = graphPath;
                size_t j    = 0;
                for (const auto& child : geo->getChildren()) {
                    // TODO(laforgg): Instances support is not working yet
                    if (geoType == BifrostHdGeoTypes::Instances && j > 0) {
                        path = path.AppendChild(TfToken{"prototypes"});
                        path = path.AppendChild(TfToken{"mesh"});
                    } else {
                        const auto& childName = child.first;
                        path = path.AppendChild(childName.GetToken());
                    }

                    result[path] = geo->getSceneIndexPrimTypeName();
                    outputDirtiedPrims->emplace_back(path,
                                                     geo->TopologyLocator());
                    m_geomTranslators[path] = geo;
                    j++;
                }
            }
        }
    }

    return result;
}

HdSceneIndexPrim BifrostGraphGenerativeProcedural::GetChildPrim(
    const HdSceneIndexBaseRefPtr& /*inputScene*/,
    const SdfPath& childPrimPath) {
    HdSceneIndexPrim result;

    const auto searchGeo = m_geomTranslators.find(childPrimPath);
    if (searchGeo == m_geomTranslators.end()) {
        return result;
    }
    auto geo = searchGeo->second;
    if (geo) {
        const auto& pathToPrim = geo->getChildren();
        const auto  search =
            pathToPrim.find(PXR_NS::SdfPath{childPrimPath.GetNameToken()});
        if (search != pathToPrim.end()) {
            result = search->second;
        }
    }
    return result;
}
