//-
// Copyright 2026 Autodesk, Inc.
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

#include <BifrostHydra/Engine/Workspace.h>

#include <BifrostGraph/Executor/Factory.h>
#include <BifrostGraph/Executor/Owner.h>

namespace {
Amino::String sourceToStr(BifrostGraph::Executor::MessageSource source) {
    switch (source) {
        case BifrostGraph::Executor::MessageSource::kWorkspace: return "Workspace";
        case BifrostGraph::Executor::MessageSource::kLibrary: return "Library";
        case BifrostGraph::Executor::MessageSource::kGraphContainer: return "GraphContainer";
        case BifrostGraph::Executor::MessageSource::kTranslation: return "Translation";
    }
    assert(false);
    return {};
}
} // namespace

namespace BifrostHd {

Workspace::Workspace(const Amino::String& name)
    : BifrostGraph::Executor::Workspace(name) {}

Workspace::~Workspace() = default;

void Workspace::onReportedMessage(BifrostGraph::Executor::MessageSource source,
                                  BifrostGraph::Executor::MessageCategory /*category*/,
                                  const Amino::String& message) const noexcept {
    const Amino::String fullMsg =
        Amino::String("[") + ::sourceToStr(source) + "] " + message;
    m_messages.push_back(fullMsg);
}

} // namespace BifrostHd
