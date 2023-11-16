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

#include <BifrostHydra/Engine/JobTranslationData.h>
#include <BifrostHydra/Engine/Requirement.h>
#include <BifrostHydra/Engine/Runtime.h>

#include <BifrostGraph/Executor/BifrostBoardJob.h>

namespace BifrostHd {

class Container final : public BifrostBoardContainer {
public:
    explicit Container();
    ~Container() override;

    static Runtime& GetRuntime();

    BifrostBoardJob::Requirement* createEvaluationRequirement(
        const Amino::String&                               name,
        Amino::PortDescription::PortDirection              direction,
        const Amino::Type&                                 type,
        BifrostGraph::Executor::TypeTranslation::PortClass portClass)
        const override;

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
    BifrostBoardJob m_job;
};

} // namespace BifrostHd
#endif // BIFROST_USD_ENGINE_CONTAINER_H_
