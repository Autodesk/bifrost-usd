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

/// \file  Attribute.h
/// \brief Bifrost USD Attribute
///

#ifndef USD_ATTRIBUTE_H_
#define USD_ATTRIBUTE_H_

#include <Amino/Cpp/Annotate.h>
#include <Amino/Cpp/ClassDeclare.h>

#include "BifrostUsdExport.h"

#ifndef DISABLE_PXR_HEADERS
#include "Prim.h"

// Note: To silence warnings coming from USD library
#include <bifusd/config/CfgWarningMacros.h>
BIFUSD_WARNING_PUSH

BIFUSD_WARNING_DISABLE_MSC(4003)
BIFUSD_WARNING_DISABLE_MSC(4244)
BIFUSD_WARNING_DISABLE_MSC(4305)
BIFUSD_WARNING_DISABLE_MSC(4800)

#include <pxr/usd/usd/attribute.h>

BIFUSD_WARNING_POP

#endif // DISABLE_PXR_HEADERS

namespace BifrostUsd {
/// \class Attribute Attribute.h
/// \brief Bifrost USD Attribute type that flows in the graph.
class AMINO_ANNOTATE("Amino::Class") USD_DECL Attribute {
public:
#ifndef DISABLE_PXR_HEADERS
    Attribute() = default;
    Attribute(PXR_NS::UsdAttribute attribute, Amino::Ptr<Prim> prim);

    Attribute(const Attribute&)      = default;
    Attribute(Attribute &&) noexcept = default;

    ~Attribute();

    Attribute& operator=(Attribute const&) = default;
    Attribute& operator=(Attribute&&) noexcept = default;

    /// \brief Returns whether this Attribute object is valid or not.
    explicit operator bool() const {
        assert((prim_ptr != nullptr) == (pxr_attribute.IsValid()));
        return prim_ptr != nullptr;
    }

    PXR_NS::UsdAttribute const* operator->() const { return &pxr_attribute; }
    Amino::Ptr<Prim> const& getPrim() const { return prim_ptr; }

private:
    PXR_NS::UsdAttribute pxr_attribute;

    // We need to keep a pointer to the prims since attributes can only exist on
    // prims and if we don't then Bifrost will cleanup the prims if they are
    // not used further in the Graph
    Amino::Ptr<Prim> prim_ptr;
#endif // DISABLE_PXR_HEADERS
};

} // namespace BifrostUsd

AMINO_DECLARE_DEFAULT_CLASS(USD_DECL, BifrostUsd::Attribute);

#endif // USD_ATTRIBUTE_H_
