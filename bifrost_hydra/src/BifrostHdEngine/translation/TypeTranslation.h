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

class TypeTranslation final : public BifrostGraph::Executor::TypeTranslation {
public:
    TypeTranslation();

    ~TypeTranslation() override;

    void deleteThis() override;

    void getSupportedTypeNames(Amino::StringList& out_names) const override;

    bool convertValueFromHost(
        Amino::Type const& type,
        Amino::Value&      value,
        BifrostGraph::Executor::TypeTranslation::ValueTranslationData const*
            valueTranslationData) const override;

    bool convertValueToHost(
        Amino::Value const& value,
        BifrostGraph::Executor::TypeTranslation::ValueTranslationData*
            valueTranslationData) const override;

    bool portAdded(Amino::String const&   name,
                   PortDirection          direction,
                   Amino::Type const&     type,
                   Amino::Metadata const& metadata,
                   PortClass              portClass,
                   PortTranslationData*   valueTranslationData) const override;

    bool registerHostPlugins(const PluginHostData* hostData) const override;
    bool unregisterHostPlugins(const PluginHostData* hostData) const override;

    bool getDataTypeColorHint(Amino::Type const& dataType,
                              Amino::String&     colorHint) const override;
};

} // namespace BifrostHd

extern "C" {
HD_MODULE_API BifrostGraph::Executor::TypeTranslation*
              createBifrostTypeTranslation(void);
}

#endif // BIFROST_HD_ENGINE_TYPE_TRANSLATION_H_
