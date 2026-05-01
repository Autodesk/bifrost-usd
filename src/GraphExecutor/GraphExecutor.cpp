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

#include "include/GraphExecutor.h"
#include "include/GraphExecutorConstants.h"
#include "include/StreamObserver.h"

// Amino
#include <Amino/Core/TaskObserver.h>
#include <Amino/Executor/ExecutionState.h>

// Bifrost
#include <Bifrost/Geometry/GeometryTypes.h> // Bifrost::Simulation Time and TimelineInfo

// C++ Standard Library
#include <iostream>
#include <ostream>
#include <utility>

namespace BifrostUsd::GraphExecutor {

constexpr Amino::StringView terminalTypeToString(TerminalType type) {
    switch (type) {
        case TerminalType::eFinal: return kTerminalFinal;
        case TerminalType::eProxy: return kTerminalProxy;
        case TerminalType::eDiagnostic: return kTerminalDiagnostic;
    }
    // All enum values handled. Unreachable code to satisfy MSVC C4715 warning:
    return "";
}

GraphExecutor::GraphExecutor(Amino::Executable executable) noexcept {
    assert(executable.isValid());
    m_executable = std::move(executable);
}

static bool setGraphInputs_impl(Amino::ExecutionInputs& executionInputs,
                                Amino::Executable&      executable,
                                GraphArgs const&        args,
                                StringArray*            messages) {
    executionInputs = executable.createInputs();
    if(args.empty()) {
        return executionInputs.isValid();
    }
    const auto& graph = executable.getGraph();
    assert(graph.isValid());
    for (auto const& input : graph.getInputs()) {
        auto        nameRef = input.getName();
        const auto& it      = args.find(Amino::String{nameRef});
        if (it != args.end()) {
            // the arg name matches the input name, now check if
            // the arg type matches the input type.
            const auto& argAny = it->second;
            if (argAny.type() != input.getTypeId()) {
                if (messages) {
                    Amino::String msg = "Type mismatch for input param " +
                                        Amino::String{nameRef};
                    messages->push_back(std::move(msg));
                }
                continue;
            }
            executionInputs.setInput(input, argAny);
        }
    }
    return executionInputs.isValid();
}

bool GraphExecutor::setGraphInputs(GraphArgs const& args) {
    return setGraphInputs_impl(m_executionInputs, m_executable, args, nullptr);
}

bool GraphExecutor::setGraphInputs(GraphArgs const& args,
                                   StringArray&     messages) {
    messages.clear();
    return setGraphInputs_impl(m_executionInputs, m_executable, args,
                               &messages);
}

void GraphExecutor::setTimelineSettings(const TimelineSettings& settings) {
    const auto& graph = m_executable.getGraph();
    assert(graph.isValid());

    Amino::Graph::GlobalVariableRef globalVar =
        graph.findGlobalVariable("Simulation::timeline_info");
    if (!globalVar) {
        return;
    }

    auto timeLineInfo = Bifrost::Simulation::TimelineInfo{
        settings.startFrame, settings.endFrame, settings.minFrame,
        settings.maxFrame, settings.frameStep};
    m_executable.setGlobalVariable(globalVar, timeLineInfo);
}

void GraphExecutor::setFps(double fps) {
    assert(fps > 0.0);
    if (fps > 0.0) {
        m_fps = fps;
    }
}

void GraphExecutor::setFrame(double frame) {
    const auto& graph = m_executable.getGraph();
    assert(graph.isValid());

    Amino::Graph::GlobalVariableRef globalVar =
        graph.findGlobalVariable("Simulation::time");
    if (!globalVar) {
        return;
    }

    Bifrost::Simulation::Time time;
    time.ticks       = 0;
    time.time        = frame / m_fps;
    time.frameLength = 1.0 / m_fps;
    time.frame       = frame;
    m_executable.setGlobalVariable(globalVar, time);
}

static bool execute_impl(Amino::ExecutionInputs&  executionInputs,
                         Amino::Executable&       executable,
                         Amino::ExecutionOutputs& executionOutputs,
                         Amino::ExecutionState&   executionState,
                         Amino::TaskNotifier      notifier) {
    if(!executionInputs) {
        return false;
    }
    // Set execution state from previous execution (if any):
    executionInputs.setExecutionState(std::move(executionState));

    executionOutputs =
        executable.execute(std::move(executionInputs), std::move(notifier));

    // Preserve the execution state for next execution:
    if (executionOutputs) {
        executionState = executionOutputs.extractExecutionState();
    } else {
        executionState = Amino::ExecutionState();
    }

    return executionOutputs.isValid();
}

bool GraphExecutor::execute(VerbosityLevel verbosityLevel) {
    using StreamObserver = StreamObserver<std::ostream>;

    Amino::TaskObserverT<StreamObserver> observer =
        Amino::TaskObserver::make<StreamObserver>(std::cout, std::cerr);
    observer->setVerbosityLevel(verbosityLevel);
    observer->setPrintPrefix(kCtxGExecExecute);

    return execute_impl(m_executionInputs, m_executable, m_executionOutputs,
                        m_executionState, observer.getNotifier());
}

bool GraphExecutor::execute(StringArray&   messages,
                            VerbosityLevel verbosityLevel) {
    using StreamObserver = StreamObserver<std::ostream>;

    Amino::TaskObserverT<StreamObserver> observer =
        Amino::TaskObserver::make<StreamObserver>(std::cout, std::cerr,
                                                  messages);
    observer->setVerbosityLevel(verbosityLevel);
    observer->setPrintPrefix(kCtxGExecExecute);

    return execute_impl(m_executionInputs, m_executable, m_executionOutputs,
                        m_executionState, observer.getNotifier());
}

Amino::Closure GraphExecutor::extractOutputClosure(Amino::StringView name) {
    auto graph = m_executable.getGraph();
    assert(graph.isValid());
    auto outputRef = graph.findOutput(name);
    if (outputRef.isValid()) {
        return m_executionOutputs.extractOutput(outputRef);
    }
    return Amino::Closure{};
}

bool GraphExecutor::hasTerminal() const {
    return !m_executable.getGraph().getTerminals().empty();
}

bool GraphExecutor::hasTerminalPort(TerminalType type) const {
    auto graph = m_executable.getGraph();
    assert(graph.isValid());
    Amino::StringView name = terminalTypeToString(type);
    return graph.findTerminal(name).isValid();
}

Amino::TerminalOutput GraphExecutor::extractTerminalOutput(TerminalType type) {
    auto graph = m_executable.getGraph();
    assert(graph.isValid());
    Amino::StringView name = terminalTypeToString(type);
    auto terminalRef = graph.findTerminal(name);
    if (terminalRef.isValid()) {
        return m_executionOutputs.extractTerminalOutput(terminalRef);
    }
    return Amino::TerminalOutput{};
}

} // namespace BifrostUsd::GraphExecutor
