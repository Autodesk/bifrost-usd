//-
// Copyright 2022 Autodesk, Inc.
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
#include "export.h"

#include <BifrostGraph/Executor/TypeTranslation.h>

#include <Amino/Core/Array.h>

namespace BifrostUsd{
class Stage;
}

class UsdTranslation : public BifrostGraph::Executor::TypeTranslation {
public:
    UsdTranslation();
    ~UsdTranslation() override;

    void deleteThis() override;

    USD_MODULE_API
    int64_t AddStageToCache(const Amino::Ptr<BifrostUsd::Stage>& stage,
                            const Amino::String& name);

    // Bifrost Executor TypeTranslation functions

    void getSupportedTypeNames(Amino::StringList& out_names) const override;

    bool convertValueFromHost(
        Amino::Type const&          type,
        Amino::Value&               value,
        ValueTranslationData const* translationData) const override;

    bool convertValueToHost(
        Amino::Value const&   value,
        ValueTranslationData* translationData) const override;

    bool portAdded(Amino::String const&   name,
                   PortDirection          direction,
                   Amino::Type const&     type,
                   Amino::Metadata const& metadata,
                   PortClass              portClass,
                   PortTranslationData*   translationData) const override;

    virtual bool portRemoved(Amino::String const& name,
                             Amino::String const& graphName) const override;

    virtual bool portRenamed(Amino::String const& prevName,
                             Amino::String const& name,
                             Amino::String const& graphName) const override;

    bool registerHostPlugins(const PluginHostData* hostData) const override;
    bool unregisterHostPlugins(const PluginHostData* hostData) const override;

    bool getDataTypeColorHint(Amino::Type const& dataType,
                              Amino::String&     colorHint) const override;

private:
    struct PortData {
        Amino::String m_portName;
        int64_t       m_cacheId;
        Amino::String m_mayaProxyShape;

        PortData() : m_portName(""), m_cacheId(-1), m_mayaProxyShape("") {}

        PortData(Amino::String portName,
                 int64_t       cacheId,
                 Amino::String mayaProxyShape)
            : m_portName(portName),
              m_cacheId(cacheId),
              m_mayaProxyShape(mayaProxyShape) {}
    };
    Amino::Array<PortData> m_portData;

    void addStageForPort(Amino::String const& portName, int64_t id);
    bool removeStageForPort(Amino::String const& portName);
    typename Amino::Array<UsdTranslation::PortData>::iterator getPortData(
        Amino::String const& portName);
    void removePortData(Amino::String const& portName);
};

extern "C" {
USD_MODULE_API BifrostGraph::Executor::TypeTranslation* createBifrostTypeTranslation(void);
}
