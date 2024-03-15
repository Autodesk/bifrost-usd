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

#ifndef BIFROST_USD_ENGINE_CONTAINER_H_
#define BIFROST_USD_ENGINE_CONTAINER_H_

#include <BifrostHydra/Engine/Export.h>

#include <BifrostGraph/Executor/GraphContainerPreview.h>
#include <BifrostGraph/Executor/JobPreview.h>

namespace BifrostHd {
class Runtime;
class JobTranslationData;

class BIFROST_HD_ENGINE_SHARED_DECL Container final : public BifrostGraph::Executor::GraphContainerPreview {
public:
    explicit Container(Runtime& runtime);
    ~Container() override;

    BifrostGraph::Executor::JobPreview::Requirement*
    createEvaluationRequirement(
        const Amino::String&                  name,
        BifrostGraph::Executor::PortDirection direction,
        const Amino::Type&                    type,
        BifrostGraph::Executor::PortClass     portClass) const override;

    bool initialize();

    /// \brief Init the evaluation job
    /// \return True if initialization was successful, false otherwise
    bool updateJob();

    Amino::Job::State executeJob(JobTranslationData& translationData);

public:
    /// Disabled
    /// \{
    Container(const Container&)            = delete;
    Container(Container&&)                 = delete;
    Container& operator=(const Container&) = delete;
    Container& operator=(Container&&)      = delete;
    /// \}

private:
    BifrostGraph::Executor::JobPreview m_job;
};

} // namespace BifrostHd
#endif // BIFROST_USD_ENGINE_CONTAINER_H_
