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

#ifndef BIFROST_HD_ENGINE_JOB_TRANSLATION_DATA_H
#define BIFROST_HD_ENGINE_JOB_TRANSLATION_DATA_H

#include <BifrostHydra/Engine/Export.h>

#include <BifrostGraph/Executor/JobPreview.h>

#include <memory>

namespace BifrostHd {

class Parameters;

class BIFROST_HD_ENGINE_SHARED_DECL JobTranslationData final
    : public BifrostGraph::Executor::JobPreview::JobTranslationData {
public:
    struct Time {
        double currentTime;
        double currentFrame;
        double frameLength;
    };

    JobTranslationData(Parameters& params, bool logVerbose, Time const& time);

    ~JobTranslationData() override;

    Time getTime() const;

    Parameters& getParameters();

public:
    /// Disabled
    /// \{
    JobTranslationData(const JobTranslationData&)            = delete;
    JobTranslationData(JobTranslationData&&)                 = delete;
    JobTranslationData& operator=(const JobTranslationData&) = delete;
    JobTranslationData& operator=(JobTranslationData&&)      = delete;
    /// \}

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace BifrostHd

#endif // BIFROST_HD_ENGINE_JOB_TRANSLATION_DATA_H
