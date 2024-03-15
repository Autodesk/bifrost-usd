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

#include <BifrostHydra/Engine/Runtime.h>
#include <BifrostHydra/Engine/Workspace.h>

namespace {
std::unique_ptr<BifrostHd::Runtime> g_runtime;
} // namespace

namespace BifrostHd {

Runtime::Runtime(Workspace& workspace) : BifrostBoardRuntime(workspace) {}

Runtime::~Runtime() = default;

Runtime& Runtime::getInstance() {
    if (!g_runtime) {
        g_runtime = std::make_unique<Runtime>(Workspace::getInstance());
        assert(g_runtime);
    }
    return *g_runtime;
}

void Runtime::reportMessage(Amino::MessageCategory /*category*/,
                            Amino::String const& message) const {
    static_cast<Workspace const&>(getWorkspace()).reportMessage(Amino::String("[Runtime] ") + message);
}

} // namespace BifrostHd
