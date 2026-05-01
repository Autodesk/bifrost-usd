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
/// \file  VariantContext.h
/// \brief Implement a WithVariantContext function that will call a lambda function in the current USD Variant Context.

#ifndef ADSK_USD_VARIANTCONTEXT_H
#define ADSK_USD_VARIANTCONTEXT_H

#include "Stage.h"

#include <Amino/Core/String.h>

// Note: To silence warnings coming from USD library
#include <bifusd/config/CfgWarningMacros.h>
#include <pxr/pxr.h>

BIFUSD_WARNING_PUSH

BIFUSD_WARNING_DISABLE_MSC(4003)
BIFUSD_WARNING_DISABLE_MSC(4244)
BIFUSD_WARNING_DISABLE_MSC(4305)
BIFUSD_WARNING_DISABLE_MSC(4800)

#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/variantSets.h>

BIFUSD_WARNING_POP

namespace BifrostUsd {

template <typename T>
class Span {
public:
    constexpr Span(T* begin, T* end) noexcept : m_begin(begin), m_end(end) {
        assert(m_begin <= m_end);
    }
    constexpr T* begin() const noexcept { return m_begin; }
    constexpr T* end() const noexcept { return m_end; }
    constexpr T& front() const noexcept {
        assert(!empty());
        return *m_begin;
    }
    constexpr size_t size() const noexcept {
        return std::distance(m_begin, m_end);
    }
    constexpr bool empty() const noexcept { return m_begin == m_end; }
    constexpr void advance_begin(size_t n) noexcept {
        m_begin += n;
        assert(m_begin <= m_end);
    }

private:
    T* m_begin;
    T* m_end;
};

template <typename T>
Span<T> make_span_n(T* begin, size_t n) {
    return Span<T>{begin, begin + n};
}

using Variant = std::pair<Amino::String, Amino::String>;

template <typename Func>
decltype(auto) SetVariantSelection(PXR_NS::UsdPrim const& prim,
                                   Span<Variant const>    variants,
                                   Func&&                 func) {
    if (!prim || variants.empty()) {
        return func();
    }

    auto const& sel        = variants.front();
    auto        variantSet = prim.GetVariantSet(sel.first.c_str());
    variantSet.SetVariantSelection(sel.second.c_str());
    PXR_NS::UsdEditContext ctx(variantSet.GetVariantEditContext());
    variants.advance_begin(1);

    return SetVariantSelection(prim, variants, func);
}

/// Set the current stage variants selection and call a lambda func
/// that can modify the BifrostUsd::Stage.
template <typename Func>
decltype(auto) WithVariantContext(BifrostUsd::Stage& stage, Func&& func) {
    const auto& variantSelection = stage.variantSelection();
    auto        prim             = PXR_NS::UsdPrim();
    if (!variantSelection.empty()) {
        auto variantSetPrimPath =
            PXR_NS::SdfPath(variantSelection.primPath().c_str());
        prim = stage->GetPrimAtPath(variantSetPrimPath);
    }

    const auto& stack = variantSelection.stack();
    return SetVariantSelection(prim, make_span_n(stack.data(), stack.size()),
                               func);
}

} // namespace BifrostUsd

#endif // ADSK_USD_VARIANTCONTEXT_H
