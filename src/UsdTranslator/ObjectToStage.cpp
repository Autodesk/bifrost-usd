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

#include "include/ObjectToStage.h"

// Amino
#include <Amino/Core/StringView.h>
#include <Amino/Executor/ExecutableT.h>
#include <Amino/Executor/ExecutionInputs.h>
#include <Amino/Executor/ExecutionOutputs.h>
#include <Amino/Library/ConstLibrary.h>

// BifrostUsd
#include <BifrostUsd/GraphExecutor/GraphExecutorFactoryInternal.h>
#include <BifrostUsd/GraphExecutor/StreamObserver.h>

// Bifrost
#include <Bifrost/Geometry/GeometryTypes.h> // provides Bifrost::Simulation Time
#include <Bifrost/Object/Object.h>

// C++ Standard Library
#include <iostream>

using namespace Amino::ExecutableT_Types; // For "Port" and "Ports"
using namespace Amino::StringViewLiterals;

namespace BifrostUsd::DynamicPayload {

namespace {
constexpr auto kDefaultPrimPath = Amino::StringView{"/root"};

Amino::Ptr<BifrostUsd::Stage> get_stage_from_objects(
    const ObjectArrayPtr&        objects,
    Amino::ExecutionState&       translatorState,
    const Amino::String&         identifier,
    BifrostUsd::ImageablePurpose purpose,
    bool                         use_frame,
    float                        frame,
    bool                         varying_topology) {
    StringArray       errors;
    Amino::Executable executable =
        GraphExecutor::makeExecutable("USD::IO::objects_to_stage"_asv, errors);
    if (!executable) {
        // This is unexpected, since the compounds used in this file should have
        // been loaded through regular config files.
        assert(false);
        return nullptr; // Nevertheless, do a safe exit
    }

    auto callable = Amino::ExecutableT{
        std::move(executable), // untyped executable
        Ports{Port<const ObjectArrayPtr>{"objects"_asv},
              Port<Amino::String>{"identifier"},
              Port<Amino::String>{"default_prim_path"},
              Port<BifrostUsd::ImageablePurpose>{"purpose"},
              Port<bool>{"use_frame"}, Port<float>{"frame"},
              Port<bool>{"varying_topology"}}, // input names, types
        Ports{
            Port<Amino::Ptr<BifrostUsd::Stage>>{"stage"}} // output names, types
    };

    // Create an observer to monitor the executions.
    auto observer =
        Amino::TaskObserver::make<GraphExecutor::StreamObserver<std::ostream>>(
            std::cout, std::cerr);
    observer->setVerbosityLevel(GraphExecutor::VerbosityLevel::eErrorsOnly);
    observer->setPrintPrefix("[object_to_stage] ");

    Amino::Ptr<BifrostUsd::Stage> stage;

    std::tie(stage, translatorState) =
        callable(objects, identifier, kDefaultPrimPath.data(),
                 purpose, use_frame, frame, varying_topology,
                 std::move(translatorState), observer.getNotifier());

    return stage;
}

} // namespace

Amino::Ptr<BifrostUsd::Stage> objects_to_stage(
    const ObjectArrayPtr&              objects,
    const Amino::String&               identifier,
    const BifrostUsd::ImageablePurpose purpose) {
    Amino::ExecutionState unusedState;
    bool                  use_frame        = false;
    float                 frame            = 0.f;
    bool                  varying_topology = false;

    return get_stage_from_objects(objects, unusedState, identifier, purpose,
                                   use_frame, frame, varying_topology);
}

Amino::Ptr<BifrostUsd::Stage> objects_to_stage(
    const ObjectArrayPtr&        objects,
    Amino::ExecutionState&       translatorState,
    const Amino::String&         identifier,
    BifrostUsd::ImageablePurpose purpose,
    float                        frame,
    bool                         varying_topology) {
    bool use_frame = true;
    return get_stage_from_objects(objects, translatorState, identifier,
                                   purpose, use_frame, frame, varying_topology);
}

} // namespace BifrostUsd::DynamicPayload
