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

#include <BifrostUsd/Attribute.h>
#include <BifrostUsd/Prim.h>

#include <Amino/Cpp/ClassDefine.h>

/// \todo BIFROST-6874 remove PXR_NS::Work_EnsureDetachedTaskProgress();
#include <pxr/base/work/detachedTask.h>

namespace BifrostUsd {
Attribute::Attribute(PXR_NS::UsdAttribute attribute, Amino::Ptr<Prim> prim)
    : pxr_attribute(std::move(attribute)), prim_ptr(std::move(prim)) {
    assert((prim_ptr != nullptr) == (pxr_attribute.IsValid()));
}
Attribute::~Attribute() = default;
} // namespace BifrostUsd

//------------------------------------------------------------------------------
//
template <>
Amino::Ptr<BifrostUsd::Attribute> Amino::createDefaultClass() {
    // Destructor of USD instances are lauching threads. This result in
    // a deadlock on windows when unloading the library (which destroys the
    // default constructed object held in static variables).
    /// \todo BIFROST-6874 remove PXR_NS::Work_EnsureDetachedTaskProgress();
    PXR_NS::Work_EnsureDetachedTaskProgress();
    auto stage    = Amino::newClassPtr<BifrostUsd::Stage>();
    auto pxr_prim = stage->get().GetPseudoRoot();
    auto prim     = Amino::newClassPtr<BifrostUsd::Prim>(pxr_prim, stage);
    auto pxr_attr =
        pxr_prim.CreateAttribute(PXR_NS::TfToken(""), PXR_NS::SdfValueTypeName());
    return Amino::newClassPtr<BifrostUsd::Attribute>(pxr_attr, prim);
}
AMINO_DEFINE_DEFAULT_CLASS(BifrostUsd::Attribute);
