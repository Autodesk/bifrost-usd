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

/// \file Prim.h
/// \brief Bifrost USD Prim type

#ifndef USD_PRIM_H_
#define USD_PRIM_H_

#include <Amino/Cpp/Annotate.h>
#include <Amino/Cpp/ClassDeclare.h>

#include "Enum.h"
#include "BifrostUsdExport.h"

#ifndef DISABLE_PXR_HEADERS
#include "Stage.h"

// Note: To silence warnings coming from USD library
#include <bifusd/config/CfgWarningMacros.h>
BIFUSD_WARNING_PUSH

BIFUSD_WARNING_DISABLE_MSC(4003)
BIFUSD_WARNING_DISABLE_MSC(4244)
BIFUSD_WARNING_DISABLE_MSC(4305)
BIFUSD_WARNING_DISABLE_MSC(4800)

#include <pxr/usd/usd/prim.h>

BIFUSD_WARNING_POP

#endif // DISABLE_PXR_HEADERS

namespace BifrostUsd {
/// \class Prim Prim.h
/// \brief Bifrost USD Prim type that flows in the graph.
class AMINO_ANNOTATE("Amino::Class") USD_DECL Prim {
public:
#ifndef DISABLE_PXR_HEADERS
    Prim() = default;
    Prim(pxr::UsdPrim prim, Amino::Ptr<Stage> stage);

    Prim(const Prim&)      = default;
    Prim(Prim &&) noexcept = default;

    Prim& operator=(Prim const&) = default;
    Prim& operator=(Prim&&) noexcept = default;
    ~Prim();

    /// \brief Returns whether this Prim object is valid or not.
    explicit operator bool() const {
        assert((stage_ptr != nullptr) == (pxr_prim.IsValid()));
        return stage_ptr != nullptr;
    }

    pxr::UsdPrim const*       operator->() const { return &pxr_prim; }
    Amino::Ptr<Stage> const& getStage() const { return stage_ptr; }

    // This accessor is dangerous because pxr::UsdPrim has copy constructors
    // and copy assignement operators which can "cast aways" the constness.
    // This should be used with extreme caution...
    pxr::UsdPrim const& getPxrPrim() const { return pxr_prim; }

private:
    pxr::UsdPrim pxr_prim;

    // We need to keep a pointer to the stage since Prims can only exist on
    // stages and if we don't then Bifrost will cleanup the stages if they are
    // not used further in the graph
    Amino::Ptr<Stage> stage_ptr;
#endif // DISABLE_PXR_HEADERS
};

} // namespace BifrostUsd

AMINO_DECLARE_DEFAULT_CLASS(USD_DECL, BifrostUsd::Prim);

#endif // USD_PRIM_H_
