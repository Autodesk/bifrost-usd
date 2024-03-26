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
#include "export.h"

#include <BifrostGraph/Executor/TypeTranslation.h>

#include <Amino/Core/Array.h>

namespace BifrostUsd{
class Stage;
}

class UsdTranslation : public BifrostGraph::Executor::TypeTranslation {
public:
    UsdTranslation() noexcept;
    ~UsdTranslation() noexcept override;

    void deleteThis() noexcept override;

    USD_MODULE_API
    int64_t AddStageToCache(const Amino::Ptr<BifrostUsd::Stage>& stage,
                            const Amino::String& name);

    // Bifrost Executor TypeTranslation functions

    void getSupportedTypeNames(StringArray& out_names) const noexcept override;

    bool convertValueFromHost(
        Amino::Type const& type,
        Amino::Any&        value,
        ValueData const*   translationData) const noexcept override;

    bool convertValueToHost(Amino::Any const& value,
                            ValueData* translationData) const noexcept override;

    bool portAdded(Amino::String const&                  name,
                   BifrostGraph::Executor::PortDirection direction,
                   Amino::Type const&                    type,
                   Amino::Metadata const&                metadata,
                   BifrostGraph::Executor::PortClass     portClass,
                   PortData* translationData) const noexcept override;

    bool portRemoved(Amino::String const& name,
                     Amino::String const& graphName) const noexcept override;

    bool portRenamed(Amino::String const& prevName,
                     Amino::String const& name,
                     Amino::String const& graphName) const noexcept override;

private:
    struct UsdPortData {
        Amino::String m_portName;
        int64_t       m_cacheId;
        Amino::String m_mayaProxyShape;

        UsdPortData() : m_portName(""), m_cacheId(-1), m_mayaProxyShape("") {}

        UsdPortData(Amino::String portName,
                    int64_t       cacheId,
                    Amino::String mayaProxyShape)
            : m_portName(portName),
              m_cacheId(cacheId),
              m_mayaProxyShape(mayaProxyShape) {}
    };
    Amino::Array<UsdPortData> m_portData;

    void addStageForPort(Amino::String const& portName, int64_t id);
    bool removeStageForPort(Amino::String const& portName);
    typename Amino::Array<UsdTranslation::UsdPortData>::iterator getPortData(
        Amino::String const& portName);
    void removePortData(Amino::String const& portName);
};

extern "C" {
USD_MODULE_API BifrostGraph::Executor::TypeTranslation* createBifrostTypeTranslation(void);
}
