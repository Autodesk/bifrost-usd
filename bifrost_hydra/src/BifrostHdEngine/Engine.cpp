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

#include <BifrostHydra/Engine/Engine.h>

#include <BifrostHydra/Engine/Container.h>
#include <BifrostHydra/Engine/JobTranslationData.h>
#include <BifrostHydra/Engine/Parameters.h>

namespace BifrostHd {

class Engine::Impl {
public:
    Impl() = default;

    Amino::Job::State execute(const double frame) {
        auto qualifiedName = Amino::String{m_parameters.compoundName().c_str()};

        // Since for now we can't change the graph at runtime (no Bifrost Graph
        // Editor "attached" to our runtime), loading of the graph should happen
        // just once. Then, the user can only change input values so we only
        // need execution.
        if (m_container.graphExecutionCounter() == 0) {
            if (!m_container.loadGraph(
                    qualifiedName,
                    BifrostBoardContainer::GraphMode::kLoadAsReference)) {
                return Amino::Job::State::kErrors;
            }
            if (!m_container.updateJob()) {
                return Amino::Job::State::kErrors;
            }
        }


        const double currentTime = frame / m_fps;
        const double frameLength = 1.0 / m_fps;

        BifrostHd::JobTranslationData jobTranslationData{
            m_parameters,
            /*logVerbose*/ true,
            /*Time data*/ {currentTime, frame, frameLength}};
        return m_container.executeJob(jobTranslationData);
    }

    void setInputScene(PXR_NS::HdSceneIndexBaseRefPtr inputScene) {
        m_parameters.setInputScene(std::move(inputScene));
    }

    void setInputs(PXR_NS::HdSceneIndexPrim const& prim) {
        m_parameters.setInputs(prim);
    }

    const Output& output() { return m_parameters.output(); }

private:
    double               m_fps{24.0};
    Parameters           m_parameters;
    BifrostHd::Container m_container;
};

Engine::Engine() : m_impl(std::make_unique<Impl>()) {}

Engine::~Engine() = default;

void Engine::setInputScene(PXR_NS::HdSceneIndexBaseRefPtr inputScene) {
    m_impl->setInputScene(std::move(inputScene));
}

void Engine::setInputs(PXR_NS::HdSceneIndexPrim const& prim) {
    m_impl->setInputs(prim);
}

Amino::Job::State Engine::execute(const double frame) {
    return m_impl->execute(frame);
}

const Output& Engine::output() { return m_impl->output(); }

} // namespace BifrostHd
