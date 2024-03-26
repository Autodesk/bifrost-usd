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

#ifndef BIFROST_HD_ENGINE_TYPE_TRANSLATION_H_
#define BIFROST_HD_ENGINE_TYPE_TRANSLATION_H_

#include "export.h"

#include <BifrostGraph/Executor/TypeTranslation.h>

namespace BifrostHd {

class BIFROST_HD_TRANSLATION_SHARED_DECL TypeTranslation final : public BifrostGraph::Executor::TypeTranslation {
public:
    TypeTranslation() noexcept;

    ~TypeTranslation() noexcept override;

    void deleteThis() noexcept override;

    void getSupportedTypeNames(StringArray& out_names) const noexcept override;

    bool convertValueFromHost(
        Amino::Type const& type,
        Amino::Any&        value,
        BifrostGraph::Executor::TypeTranslation::ValueData const*
            valueTranslationData) const noexcept override;

    bool convertValueToHost(Amino::Any const& value,
                            BifrostGraph::Executor::TypeTranslation::ValueData*
                                valueTranslationData) const noexcept override;
};

} // namespace BifrostHd

extern "C" {
BIFROST_HD_TRANSLATION_SHARED_DECL BifrostGraph::Executor::TypeTranslation* createBifrostTypeTranslation(void);
}

#endif // BIFROST_HD_ENGINE_TYPE_TRANSLATION_H_
