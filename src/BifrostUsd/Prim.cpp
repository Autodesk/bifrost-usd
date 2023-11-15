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
#include <BifrostUsd/Prim.h>

#include <Amino/Cpp/ClassDefine.h>

// #include <core/logger.h>

/// \todo BIFROST-6874 remove PXR_NS::Work_EnsureDetachedTaskProgress();
#include <pxr/base/work/detachedTask.h>

namespace BifrostUsd {
Prim::Prim(PXR_NS::UsdPrim prim, Amino::Ptr<Stage> stage)
    : pxr_prim(std::move(prim)), stage_ptr(std::move(stage)) {
    assert((stage_ptr != nullptr) == (pxr_prim.IsValid()));
}
Prim::~Prim() = default;
} // namespace BifrostUsd

//------------------------------------------------------------------------------
//
template <>
Amino::Ptr<BifrostUsd::Prim> Amino::createDefaultClass() {
    // Destructor of USD instances are lauching threads. This result in
    // a deadlock on windows when unloading the library (which destroys the
    // default constructed object held in static variables).
    /// \todo BIFROST-6874 remove PXR_NS::Work_EnsureDetachedTaskProgress();
    PXR_NS::Work_EnsureDetachedTaskProgress();
    auto stage    = Amino::newClassPtr<BifrostUsd::Stage>();
    auto pxr_prim = stage->get().GetPseudoRoot();
    return Amino::newClassPtr<BifrostUsd::Prim>(pxr_prim, stage);
}
AMINO_DEFINE_DEFAULT_CLASS(BifrostUsd::Prim);
