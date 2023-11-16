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

#include <BifrostHydra/Engine/JobTranslationData.h>

#include <BifrostHydra/Engine/Parameters.h>

namespace BifrostHd {

class JobTranslationData::Impl {
public:
    Impl(bool logVerbose, Time const& time, Parameters& params)
        : m_logVerbose(logVerbose), m_time(time), m_params(params) {
        (void)m_logVerbose;
    }

    JobTranslationData::Time getTime() const { return m_time; }
    Parameters&              getParameters() { return m_params; }

private:
    bool const  m_logVerbose;
    Time        m_time;
    Parameters& m_params;
};

JobTranslationData::JobTranslationData(Parameters& params,
                                       bool        logVerbose,
                                       Time const& time)
    : BifrostBoardJob::JobTranslationData(),
      m_impl(std::make_unique<Impl>(logVerbose, time, params)) {}

JobTranslationData::~JobTranslationData() = default;

JobTranslationData::Time JobTranslationData::getTime() const {
    return m_impl->getTime();
}

Parameters& JobTranslationData::getParameters() {
    return m_impl->getParameters();
}
} // namespace BifrostHd
