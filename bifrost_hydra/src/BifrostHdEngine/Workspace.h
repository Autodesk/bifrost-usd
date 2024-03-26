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

#ifndef BIFROST_HD_ENGINE_WORKSPACE_H_
#define BIFROST_HD_ENGINE_WORKSPACE_H_

#include <BifrostHydra/Engine/Export.h>

#include <BifrostGraph/Executor/WorkspacePreview.h>

#include <memory>

namespace BifrostHd {

class BIFROST_HD_ENGINE_SHARED_DECL Workspace final : public BifrostGraph::Executor::WorkspacePreview {
public:
    static Workspace& getInstance();

    explicit Workspace(const Amino::String& name);
    ~Workspace() override;

    void onReportedMessage(BifrostGraph::Executor::MessageSource   source,
                           BifrostGraph::Executor::MessageCategory category,
                           const Amino::String&                    message) const noexcept override;

    StringArray const& getMessages() const;

    bool hasMessages() const;

    void clearMessages();

    void reportMessage(Amino::String const& message) const;

private:
    mutable StringArray m_messages;
};

inline Workspace::StringArray const& Workspace::getMessages() const {
    return m_messages;
}

inline bool Workspace::hasMessages() const { return !m_messages.empty(); }

inline void Workspace::clearMessages() { m_messages.clear(); }

inline void Workspace::reportMessage(Amino::String const& message) const {
    m_messages.push_back(message);
}

} // namespace BifrostHd

#endif // BIFROST_HD_ENGINE_WORKSPACE_H_
