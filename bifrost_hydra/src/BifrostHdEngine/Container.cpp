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

#include <BifrostHydra/Engine/Container.h>

#include <iostream>

namespace BifrostHd {

Container::Container() : BifrostBoardContainer(&Runtime::Get()), m_job(this) {}

Container::~Container() = default;

/* static */
Runtime& Container::GetRuntime() { return Runtime::Get(); }

BifrostBoardJob::Requirement* Container::createEvaluationRequirement(
    const Amino::String&                               name,
    Amino::PortDescription::PortDirection              direction,
    const Amino::Type&                                 type,
    BifrostGraph::Executor::TypeTranslation::PortClass portClass) const {
    Amino::Value           defaultVal;
    Amino::PortDescription portDesc;
    if (getGraph().getPorts().findByName(name, portDesc)) {
        defaultVal = portDesc.value();
    }

    return new Requirement(name, direction, type, portClass, defaultVal);
}

bool Container::initialize() {
    // The nullptr pointer used for update() will get passed to portAdded()
    // in the translation table, which is only called for each Job Port.
    // Job Ports are global state ports, such as for time information.
    // For now TypeTranslation::portAdded() does nothing, so the
    // pointer can be nullptr.
    auto status =
        m_job.update(nullptr, BifrostBoardJob::CompileSwitch::kCompile);

    // See BIFROST-3651.
    if (status == Amino::SetGraphStatus::Value::kFailure) {
        std::cout << "Failed initializing job.\n";
        std::cout
            << "This is most likely due to an input or output port being\n";
        std::cout << "set to type 'auto'.  In order to run Bifrost compounds\n";
        std::cout
            << "outside of Maya, all ports must have a set type such as\n";
        std::cout << "float or Object or double3 etc.  If using Bifrost\n";
        std::cout
            << "for Maya, you can easily check this by visually inspecting\n";
        std::cout
            << "the ports on the compound you are attempting to run here.\n";
        std::cout
            << "Any port with a vertical bar indicates an 'auto' port, and\n";
        std::cout
            << "you can fix the issue by right-clicking and setting it to \n";
        std::cout << "an explicit type." << std::endl;

        return false;
    }

    // populateRequirements() needs the list of exposed Terminals,
    // those at root level or those that are wrapped within compounds.
    // Update this list of exposed Terminals by retrieving the Terminals
    // from the job to execute:
#if 1
    // workaround until BIFROST-6157 gets done: use a temporary
    // GraphResolver to retrieve the Terminals:
    Amino::GraphResolver resolver;
    resolver.setGraph(m_job.aminoJob().getGraph());
    const Amino::GraphResolver::CompoundResolution::Terminals& resolvedTerms =
        resolver.getCompoundResolution().getTerminals();
    updateExposedTerminals(&resolvedTerms);
#else
    // When BIFROST-6157 will be done, we expect being able to query the
    // terminals from the Amino::Job, and it would return the same type
    // as GraphResolver::CompoundResolution::getTerminals() returns.
    updateExposedTerminals(m_job.aminoJob().getTerminals());
#endif

    // Fill the EvaluationRequirements structures in the job. This allows us
    // to examine the job inputs and outputs.
    m_job.populateRequirements();

    return true;
}

bool Container::updateJob() {
    return m_job.update(nullptr, BifrostBoardJob::CompileSwitch::kCompile) !=
           BifrostBoardJob::EvaluationStatus::kFailure;
}

Amino::Job::State Container::executeJob(JobTranslationData& translationData) {
    return m_job.execute(&translationData,
                         BifrostBoardJob::ExecuteFlags::kDefault);
}

} // namespace BifrostHd
