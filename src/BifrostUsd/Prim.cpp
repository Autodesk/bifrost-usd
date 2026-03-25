//-
// Copyright 2024 Autodesk, Inc.
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
    assert(stage_ptr != nullptr);
}
Prim::~Prim() = default;

//------------------------------------------------------------------------------
//
namespace {
Amino::Ptr<BifrostUsd::Prim> createDefaultPrim() {
    // Destructor of USD instances are lauching threads. This result in
    // a deadlock on windows when unloading the library (which destroys the
    // default constructed object held in static variables).
    /// \todo BIFROST-6874 remove PXR_NS::Work_EnsureDetachedTaskProgress();

#if ((PXR_MINOR_VERSION == 25) && (PXR_PATCH_VERSION >= 8)) || (PXR_MINOR_VERSION > 25)
    PXR_NS::WorkTBB_EnsureDetachedTaskProgress();
#else
    PXR_NS::Work_EnsureDetachedTaskProgress();
#endif
    auto stage    = Amino::newClassPtr<BifrostUsd::Stage>();
    auto pxr_prim = stage->get().GetPseudoRoot();
    return Amino::newClassPtr<BifrostUsd::Prim>(pxr_prim, stage);
}
} // namespace
} // namespace BifrostUsd

AMINO_DEFINE_DEFAULT_CLASS(BifrostUsd::Prim, BifrostUsd::createDefaultPrim());
