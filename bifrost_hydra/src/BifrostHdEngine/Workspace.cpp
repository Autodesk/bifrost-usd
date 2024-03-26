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

#include <BifrostHydra/Engine/Workspace.h>

#include <BifrostGraph/Executor/PreviewUtility.h>
//#include <BifrostGraph/Executor/Utility.h>

#include <AminoConfig.h>
#include <AminoStringList.h>

namespace {
std::unique_ptr<BifrostHd::Workspace> g_workspace;
} // namespace

namespace BifrostHd {

Workspace::Workspace(const Amino::String& name) : WorkspacePreview(name) {
    auto configEnv = BifrostGraph::Executor::makeOwner<
        BifrostGraph::Executor::Utility::ConfigEnv>();
    if (configEnv && configEnv->isValid()) {
        const auto& configFiles =
            configEnv->values("bifrost_pack_config_files");
        if (!configFiles.empty()) {
            Amino::Config config;
            readConfigFiles(configFiles, Amino::Array<Amino::String>{}, config);
            addConfigForUserDefaultResources(config);
            reportConfigData(config);
            loadAllResources(config);
        }
    }
}

Workspace::~Workspace() = default;

Workspace& Workspace::getInstance() {
    if (!g_workspace) {
        g_workspace = std::make_unique<Workspace>("BifrostHd");
        assert(g_workspace);
    }
    return *g_workspace;
}

void Workspace::onReportedMessage(BifrostGraph::Executor::MessageSource source,
                                  BifrostGraph::Executor::MessageCategory /*category*/,
                                  const Amino::String& message) const noexcept {
    reportMessage([source]() {
        switch(source) {
            case BifrostGraph::Executor::MessageSource::kWorkspace:
                return Amino::String("[Workspace] ");
            case BifrostGraph::Executor::MessageSource::kLibrary:
                return Amino::String("[Library] ");
            case BifrostGraph::Executor::MessageSource::kGraphContainer:
                return Amino::String("[GraphContainer] ");
            case BifrostGraph::Executor::MessageSource::kJob:
                return Amino::String("[Job] ");
            case BifrostGraph::Executor::MessageSource::kTranslation:
                return Amino::String("[Translation] ");
        }
        assert(false);
#if defined(_WIN32)
        __assume(false);
#else
        __builtin_unreachable();
#endif
        }()+ message);
}

} // namespace BifrostHd
