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

#include <Bifrost/Object/Object.h>
#include <BifrostGraph/Executor/PreviewUtility.h>
#include <BifrostGraph/Executor/Utility.h>

namespace {
std::unique_ptr<BifrostHd::Runtime> g_runtime;
Amino::StringList                   g_messages;
} // namespace

namespace BifrostHd {

Runtime::Runtime() : BifrostBoardRuntime("BifrostHd") {}

Runtime::~Runtime() = default;

/* static */
bool Runtime::IsValid() { return g_runtime.get(); }

/* static */
Runtime& Runtime::Get() {
    if (!g_runtime) {
        g_runtime = std::make_unique<BifrostHd::Runtime>();
        BifrostGraph::Executor::Utility::ConfigEnv config;
        if (BifrostGraph::Executor::Utility::getConfigFromEnvironment(config)) {
            const auto& configFiles = config.value("bifrost_pack_config_files");
            if (configFiles.size() > 0) {
                g_runtime->loadConfigFiles(configFiles);
            }
        }
    }

    return *g_runtime;
}

void Runtime::reportMessage(Amino::MessageCategory /*category*/,
                            Amino::String const& message) const {
    std::string runtime_msg{"[Runtime] "};
    runtime_msg += message.c_str();
    g_messages.add(runtime_msg.c_str());
}

bool Runtime::hasMessages() const { return g_messages.size() > 0u; }

Amino::StringList const& Runtime::getMessages() const { return g_messages; }

void Runtime::clearMessages() { g_messages = Amino::StringList(); }

} // namespace BifrostHd
