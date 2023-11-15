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

#include <BifrostHydra/Engine/Requirement.h>

#include <BifrostHydra/Engine/JobTranslationData.h>
#include <BifrostHydra/Engine/ValueTranslationData.h>

namespace BifrostHd {

Requirement::Requirement(
    const Amino::String&                               name,
    Amino::PortDescription::PortDirection              direction,
    const Amino::Type&                                 type,
    BifrostGraph::Executor::TypeTranslation::PortClass portClass,
    Amino::Value                                       defaultValue)
    : BifrostBoardJob::Requirement(name, direction, type, portClass),
      m_defaultVal(std::move(defaultValue)) {}

void Requirement::translate(
    BifrostBoardRuntime const*           runtime,
    Amino::Job const&                    job,
    BifrostBoardJob::JobTranslationData* translationData) const {
    // Override so we can create the value translation data on the stack
    assert(dynamic_cast<JobTranslationData*>(translationData));

    ValueTranslationData valueTranslationData(
        *static_cast<JobTranslationData*>(translationData), m_defaultVal,
        m_name.c_str());

    BifrostBoardJob::Requirement::translate(runtime, job,
                                            &valueTranslationData);
}

} // namespace BifrostHd
