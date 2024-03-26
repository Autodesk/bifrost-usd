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

#ifndef BIFROST_HD_ENGINE_REQUIREMENT_H_
#define BIFROST_HD_ENGINE_REQUIREMENT_H_

#include <BifrostHydra/Engine/Export.h>

#include <BifrostGraph/Executor/BifrostBoardRuntime.h>

namespace BifrostHd {

class BIFROST_HD_ENGINE_SHARED_DECL Requirement final
    : public BifrostGraph::Executor::JobPreview::Requirement {
public:
    Requirement(const Amino::String&                  name,
                BifrostGraph::Executor::PortDirection direction,
                const Amino::Type&                    type,
                BifrostGraph::Executor::PortClass     portClass,
                Amino::Any                            defaultValue);

    ~Requirement() override = default;

    void translate(BifrostBoardRuntime const* runtime,
                   Amino::Job const&          job,
                   BifrostGraph::Executor::JobPreview::JobTranslationData*
                       translationData) const override;

public:
    /// Disabled
    /// \{
    Requirement(Requirement const&)            = delete;
    Requirement(Requirement&&)                 = delete;
    Requirement& operator=(const Requirement&) = delete;
    Requirement& operator=(Requirement&&)      = delete;
    /// \}

private:
    Amino::Any m_defaultVal; ///< Default value to use if none is set in input
};

} // namespace BifrostHd
#endif // BIFROST_HD_ENGINE_REQUIREMENT_H_
