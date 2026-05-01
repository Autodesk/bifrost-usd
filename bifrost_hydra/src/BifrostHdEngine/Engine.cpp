//-
// Copyright 2026 Autodesk, Inc.
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

#include <BifrostHydra/Engine/JobTranslationData.h>
#include <BifrostHydra/Engine/Parameters.h>
#include <BifrostHydra/Engine/ValueTranslationData.h>
#include <BifrostHydra/Engine/Workspace.h>

#include <BifrostGraph/Executor/Factory.h>
#include <BifrostGraph/Executor/Library.h>
#include <BifrostGraph/Executor/Owner.h>
#include <BifrostGraph/Executor/Utility.h>

#include <Amino/Core/String.h>
#include <Amino/Core/TaskObserver.h>
#include <Amino/Executor/Executable.h>
#include <Amino/Executor/ExecutionInputs.h>
#include <Amino/Executor/ExecutionOutputs.h>
#include <Amino/Executor/Graph.h>

using namespace BifrostGraph::Executor;

namespace BifrostHd {

class Engine::Impl {
public:
    Impl() = default;
    ~Impl() noexcept {
        // Let the Workspace delete its GraphContainer
    }

    bool initWorkspace() {
        if (!m_workspace) {
            m_workspace =
                BifrostGraph::Executor::makeOwner<BifrostHd::Workspace>(
                    "BifrostHd");
            if (!m_workspace) {
                return false;
            }
            // Use the environment variables BIFROST_LIB_CONFIG_FILES and
            // BIFROST_DISABLE_PACKS to determine the required Bifrost resources
            // to be loaded.
            auto configEnv = BifrostGraph::Executor::makeOwner<
                BifrostGraph::Executor::Utility::ConfigEnv>();
            if (!configEnv) {
                return false;
            }
            if (!m_workspace->loadConfigFiles(configEnv->values("bifrost_pack_config_files"),
                                              configEnv->values("bifrost_disable_packs"))) {
                return false;
            }
        }
        return true;
    }

    BifrostHd::Workspace* getWorkspace() {
        if (!initWorkspace()) {
            return nullptr;
        }
        return m_workspace.get();
    }

    bool initExecutable() {
        assert(m_workspace);

        if (!m_executable.isValid()) {
            Amino::String name{m_parameters.compoundName().c_str()};
            BifrostGraph::Executor::Library& library =
                m_workspace->getLibrary();

            // Note 1: Since for now we can't change the 'graph topology' at
            // runtime (like connect/disconnect/add node) because there is no
            // Bifrost Graph Editor "attached" to our engine, then we can get
            // the executable just once.

            // Note 2: Note that in order to later compile/run the graph, none
            // of its inputs/outputs can have its type set to 'auto'. See
            // BIFROST-3651.
            m_executable =
                library.createExecutableGraph(name);

            if (!m_executable.isValid()) {
                return false;
            }
            auto theGraph = m_executable.getGraph();
            if (!theGraph.isValid() || theGraph.hasErrors()) {
                return false;
            }
        }
        return true;
    }

    void setInputScene(PXR_NS::HdSceneIndexBaseRefPtr inputScene) {
        m_parameters.setInputScene(std::move(inputScene));
    }

    void setInputs(const PXR_NS::HdSceneIndexPrim& prim) {
        m_parameters.setInputs(prim);
    }

    bool execute(const double frame) {
        if (!initWorkspace() || !initExecutable()) {
            return false;
        }


        // Prepare input values
        const double                  currentTime = frame / m_fps;
        const double                  frameLength = 1.0 / m_fps;
        BifrostHd::JobTranslationData jobData{
            m_parameters,
            /*Time data*/ {currentTime, frame, frameLength}};

        // Set all inputs: Graph inputs and global variables.
        // For each input, the Executor will call convertValueFromHost() on
        // our TypeTranslation class.
        auto theGraph = m_executable.getGraph();
        assert(theGraph.isValid());

        auto theGraphInputs = m_executable.createInputs();
        assert(theGraphInputs.isValid());

        // Inputs
        for (auto const& input : theGraph.getInputs()) {
            // Get default value for the translation data`
            /// \todo BIFROST-TBD This is probably unecessary?
            /// (Since default values are already set when doing createInputs().
            auto           defaultVal = theGraphInputs.getInput(input);
            InputValueData inputData(
                jobData,
                std::string{Amino::StringView(input.getFullyQualifiedName())},
                defaultVal ? defaultVal.getAny() : Amino::Any{});

            Amino::Closure closure;
            auto const typeTrans =
                m_workspace->getTypeTranslation(input.getTypeId());
            BifrostGraph::Executor::Utility::convertToClosureFromHost(
                typeTrans, input, &inputData, closure );

            // Set the input port value
            theGraphInputs.setInput(input, std::move(closure));
        }

        // Globals
        for (auto const& globalVariable : theGraph.getGlobalVariables()) {
            InputValueData inputData(
                jobData,
                std::string{
                    Amino::StringView(globalVariable.getFullyQualifiedName())},
                Amino::Any{});

            Amino::Closure closure;
            auto const typeTrans =
                m_workspace->getTypeTranslation(globalVariable.getTypeId());
            BifrostGraph::Executor::Utility::convertToClosureFromHost(
                typeTrans, globalVariable, &inputData, closure );

            // Set the global variable port value
            m_executable.setGlobalVariable(globalVariable, std::move(closure));
        }

        // Execute the graph
        if (!theGraphInputs.isReady()) {
            return false;
        }
        auto theGraphOutputs = m_executable.execute(std::move(theGraphInputs),
                                                    Amino::TaskNotifier());

        if (!theGraphOutputs.isValid()) {
            return false;
        }
        // Get all outputs: Graph outputs and terminals.
        // For each output, the Executor will call convertValueToHost()
        // on our TypeTranslation class. Outputs
        for (const auto& output : theGraph.getOutputs()) {
            OutputValueData outputData(
                jobData,
                std::string{Amino::StringView(output.getFullyQualifiedName())});

            Amino::Closure outputClosure = theGraphOutputs.getOutput(output);
            auto const typeTrans =
                m_workspace->getTypeTranslation(output.getTypeId());
            BifrostGraph::Executor::Utility::convertFromClosureToHost(
                typeTrans, output, outputClosure, &outputData);
        }

        // Terminals
        for (const auto& terminal : theGraph.getTerminals()) {
            OutputValueData outputData(jobData,
                                       std::string{Amino::StringView(
                                           terminal.getFullyQualifiedName())});
            Amino::Closure  terminalClosure =
                theGraphOutputs.getTerminalOutput(terminal).getFlattened();
            auto const typeTrans =
                m_workspace->getTypeTranslation(terminal.getFlattenedTypeId());
            BifrostGraph::Executor::Utility::convertFromClosureToHost(
                typeTrans, terminal, terminalClosure, &outputData);
        }

        return true;
    }

    const Output& getOutput() const { return m_parameters.output(); }

    const Inputs& getInputs() const { return m_parameters.inputs(); }

private:
    BifrostGraph::Executor::Owner<BifrostHd::Workspace> m_workspace{};
    Amino::Executable                                   m_executable{};

    double     m_fps{24.0};
    Parameters m_parameters;
};

Engine::Engine() : m_impl(std::make_unique<Impl>()) {}

Engine::~Engine() = default;

BifrostHd::Workspace* Engine::getWorkspace() { return m_impl->getWorkspace(); }

void Engine::setInputScene(PXR_NS::HdSceneIndexBaseRefPtr inputScene) {
    m_impl->setInputScene(std::move(inputScene));
}

void Engine::setInputs(const PXR_NS::HdSceneIndexPrim& prim) {
    m_impl->setInputs(prim);
}

bool Engine::execute(const double frame) { return m_impl->execute(frame); }

const Output& Engine::getOutput() const { return m_impl->getOutput(); }

const Inputs& Engine::getInputs() const { return m_impl->getInputs(); }


} // namespace BifrostHd
