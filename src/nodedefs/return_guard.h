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

/// \file  return_guard.h
/// \brief Helper class to never return Amino nullptr.

#include <Amino/Core/Ptr.h>

namespace USD {

/// \class ReturnGuard return_guard.h
/// \brief Helper class used to ensure that Amino::Ptr outputs of nodedefs are
/// never returned as nullptr. Amino should never see nullptr flowing in the
/// graph.
///
/// \note BIFROST-6356 This helper class will help ensuring a smooth and
/// incremental transition, because implementing BIFROST-6356 will require all
/// Amino::Ptr to be dereferenceable. That being said, it's unclear if it will
/// still be necessary once the transition is completed, or we also might want
/// to change the implementation to avoid creating new Amino::Ptr, but rather
/// copy an Amino::Ptr pointing to an invalid (but non null) pointee.
///
template <typename T, typename Func>
class ReturnGuard {
public:
    ReturnGuard(T& retVal, Func func) : m_retVal(retVal), m_func(func) {}
    ReturnGuard(ReturnGuard&&) noexcept = default;
    ~ReturnGuard() {
        if (!m_retVal) m_retVal = m_func();
        assert(m_retVal); // must not be null!
    }

private:
    T&   m_retVal;
    Func m_func;
};

template <typename T, typename Func>
auto createReturnGuard(Amino::Ptr<T>& out, Func func) {
    return ReturnGuard<Amino::Ptr<T>, decltype(func)>(out, func);
}
template <typename T>
auto createReturnGuard(Amino::Ptr<T>& out) {
    auto func = []() { return Amino::newClassPtr<T>(); };
    return ReturnGuard<Amino::Ptr<T>, decltype(func)>(out, func);
}
} // namespace USD
