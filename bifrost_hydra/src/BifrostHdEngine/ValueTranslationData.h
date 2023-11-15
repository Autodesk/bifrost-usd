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

#ifndef BIFROST_HD_ENGINE_TRANSLATION_DATA_H
#define BIFROST_HD_ENGINE_TRANSLATION_DATA_H

#include <BifrostGraph/Executor/BifrostBoardJob.h>

namespace BifrostHd {

class JobTranslationData;

class ValueTranslationData final
    : public BifrostBoardJob::ValueTranslationData {
public:
    ValueTranslationData(JobTranslationData& jobTranslationData,
                         Amino::Value        defaultVal,
                         std::string         name);
    ~ValueTranslationData() override;

    Amino::Value getInput(Amino::Type const& type) const;
    bool         setOutput(const Amino::Value& value);

    const JobTranslationData& jobTranslationData() const;

public:
    /// Disabled
    /// \{
    ValueTranslationData(const ValueTranslationData&)            = delete;
    ValueTranslationData(ValueTranslationData&&)                 = delete;
    ValueTranslationData& operator=(const ValueTranslationData&) = delete;
    ValueTranslationData& operator=(ValueTranslationData&&)      = delete;
    /// \}

private:
    Amino::Value        m_defaultVal;
    JobTranslationData& m_jobTranslationData;
    std::string         m_name;
};

} // namespace BifrostHd

#endif // BIFROST_HD_ENGINE_TRANSLATION_DATA_H
