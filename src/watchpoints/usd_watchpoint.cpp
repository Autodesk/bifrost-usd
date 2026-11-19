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

#if defined(_WIN32)
#define USD_WATCHPOINT_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#define USD_WATCHPOINT_EXPORT __attribute__((visibility("default")))
#else
#define USD_WATCHPOINT_EXPORT
#endif

#include <Amino/Core/Array.h>
#include <Amino/Core/String.h>
#include <Amino/Core/StringView.h>
#include <Amino/RTTI/ValueView.h>

#include <BifrostGraph/Executor/Watchpoint.h>
#include <BifrostGraph/Executor/WatchpointLayout.h>
#include <BifrostGraph/Executor/WatchpointLayoutFactory.h>

#include <BifrostUsd/Attribute.h>
#include <BifrostUsd/Layer.h>
#include <BifrostUsd/Prim.h>
#include <BifrostUsd/Stage.h>

#include <any>
#include <cassert>
#include <cmath>
#include <functional>
#include <string>

// Note: To silence warnings coming from USD library
#include <bifusd/config/CfgWarningMacros.h>
BIFUSD_WARNING_PUSH
BIFUSD_WARNING_DISABLE_MSC(4003)
BIFUSD_WARNING_DISABLE_MSC(4267)
BIFUSD_WARNING_DISABLE_MSC(4244)
#include <pxr/base/vt/typeHeaders.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/primTypeInfo.h>

BIFUSD_WARNING_POP

#if defined(_WIN32)
#define UNREACHABLE _assume(false);
#else
#define UNREACHABLE __builtin_unreachable();
#endif

using Watcher               = BifrostGraph::Executor::Watchpoint::Watcher;
using WatchpointLayout      = BifrostGraph::Executor::WatchpointLayout;
using WatchpointLayoutArray = BifrostGraph::Executor::WatchpointLayoutArray;
using WatchpointLayoutArrayGeneric =
    BifrostGraph::Executor::WatchpointLayoutArrayGeneric;
using WatchpointLayoutComposite =
    BifrostGraph::Executor::WatchpointLayoutComposite;
using WatchpointLayoutFactory = BifrostGraph::Executor::WatchpointLayoutFactory;
using WatchpointLayoutPath    = BifrostGraph::Executor::WatchpointLayoutPath;
using WatchpointLayoutPtr     = BifrostGraph::Executor::WatchpointLayoutPtr;

using Field     = WatchpointLayoutArray::Sorter::Field;
using Sorter    = WatchpointLayoutArray::Sorter;
using Filters   = WatchpointLayoutArray::Filters;
using Filter    = WatchpointLayoutArray::Filter;
using CompareOp = WatchpointLayoutArray::Filter::Operation;
using Indices   = WatchpointLayoutArray::Indices;

using AttributePtr      = Amino::Ptr<BifrostUsd::Attribute>;
using ArrayOfAttributes = Amino::Ptr<Amino::Array<AttributePtr>>;
using LayerPtr          = Amino::Ptr<BifrostUsd::Layer>;
using ArrayOfLayers     = Amino::Ptr<Amino::Array<LayerPtr>>;
using StagePtr          = Amino::Ptr<BifrostUsd::Stage>;
using ArrayOfStages     = Amino::Ptr<Amino::Array<StagePtr>>;
using PrimPtr           = Amino::Ptr<BifrostUsd::Prim>;
using ArrayOfPrims      = Amino::Ptr<Amino::Array<PrimPtr>>;

using namespace PXR_NS;

namespace {
// VT_TYPE, VT_TYPE_NAME and VT_SCALAR_VALUE_TYPES are defined in
// <pxr/base/vt/types.h>
#if (PXR_MINOR_VERSION >= 24)
// TF_PP_SEQ_FOR_EACH is defined in <pxr/base/tf/preprocessorUtilsListe.h>
#define VT_HANDLE_VT_TYPE(unused, element) \
    HANDLE_VT_TYPE(element)
#define FOR_EACH_VT_VALUE_TYPES \
    TF_PP_SEQ_FOR_EACH(VT_HANDLE_VT_TYPE, ~, VT_SCALAR_VALUE_TYPES)
#else
#define VT_HANDLE_VT_TYPE(unused1, unused2, element) \
    HANDLE_VT_TYPE(element)
#define FOR_EACH_VT_VALUE_TYPES \
    BOOST_PP_SEQ_FOR_EACH(VT_HANDLE_VT_TYPE, ~, VT_SCALAR_VALUE_TYPES)
#endif

template <typename T>
constexpr bool is_pxr_scalar_type() {
    return std::is_same_v<T, bool> || std::is_same_v<T, char> ||
           std::is_same_v<T, unsigned char> || std::is_same_v<T, short> ||
           std::is_same_v<T, unsigned short> || std::is_same_v<T, int> ||
           std::is_same_v<T, unsigned int> || std::is_same_v<T, int64_t> ||
           std::is_same_v<T, uint64_t> || std::is_same_v<T, double> ||
           std::is_same_v<T, float> || std::is_same_v<T, GfHalf>;
}

template <typename T>
constexpr bool is_pxr_string_type() {
    return std::is_same_v<T, std::string> || std::is_same_v<T, TfToken> ||
           std::is_same_v<T, SdfAssetPath>;
}

template <typename T>
constexpr bool is_pxr_vector_type() {
    return std::is_same_v<T, GfVec2d> || std::is_same_v<T, GfVec2f> ||
           std::is_same_v<T, GfVec2h> || std::is_same_v<T, GfVec2i> ||
           std::is_same_v<T, GfVec3d> || std::is_same_v<T, GfVec3f> ||
           std::is_same_v<T, GfVec3h> || std::is_same_v<T, GfVec3i> ||
           std::is_same_v<T, GfVec4d> || std::is_same_v<T, GfVec4f> ||
           std::is_same_v<T, GfVec4h> || std::is_same_v<T, GfVec4i>;
}

template <typename T>
constexpr bool is_pxr_matrix_type() {
    return std::is_same_v<T, GfMatrix2d> || std::is_same_v<T, GfMatrix2f> ||
           std::is_same_v<T, GfMatrix3d> || std::is_same_v<T, GfMatrix3f> ||
           std::is_same_v<T, GfMatrix4d> || std::is_same_v<T, GfMatrix4f>;
}

template <typename T>
constexpr bool is_pxr_quaternion_type() {
    return std::is_same_v<T, GfQuatd> || std::is_same_v<T, GfQuatf> ||
           std::is_same_v<T, GfQuath> || std::is_same_v<T, GfQuaternion>;
}

template <typename T>
constexpr bool is_pxr_dual_quaternion_type() {
    return std::is_same_v<T, GfDualQuatd> || std::is_same_v<T, GfDualQuatf> ||
           std::is_same_v<T, GfDualQuath>;
}

template <typename T>
constexpr bool is_pxr_range_type() {
    return std::is_same_v<T, GfRange1d> || std::is_same_v<T, GfRange1f> ||
           std::is_same_v<T, GfRange2d> || std::is_same_v<T, GfRange2f> ||
           std::is_same_v<T, GfRange3d> || std::is_same_v<T, GfRange3f> ||
           std::is_same_v<T, GfInterval> || std::is_same_v<T, GfRect2i>;
}

template <typename T>
constexpr bool is_ptr_type() {
    return std::is_same_v<T, AttributePtr> || std::is_same_v<T, LayerPtr> ||
           std::is_same_v<T, StagePtr> || std::is_same_v<T, PrimPtr>;
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct QuaternionTraits {
    using RealType      = typename T::ScalarType;
    using ImaginaryType = typename T::ImaginaryType;
};

template <>
struct QuaternionTraits<GfQuaternion> {
    using RealType      = double;
    using ImaginaryType = GfVec3d;
};

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct DualQuaternionTraits;

template <>
struct DualQuaternionTraits<GfDualQuatd> {
    using QuaternionType = GfQuatd;
};

template <>
struct DualQuaternionTraits<GfDualQuatf> {
    using QuaternionType = GfQuatf;
};

template <>
struct DualQuaternionTraits<GfDualQuath> {
    using QuaternionType = GfQuath;
};

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct RangeTraits {
    using MinMaxType                   = typename T::MinMaxType;
    static const std::size_t dimension = T::dimension;
};

template <>
struct RangeTraits<GfInterval> {
    using MinMaxType                   = double;
    static const std::size_t dimension = 1;
};

template <>
struct RangeTraits<GfRect2i> {
    using MinMaxType                   = GfVec2i;
    static const std::size_t dimension = GfVec2i::dimension;
};

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
// Epsilon value used for floating point value comparison.
// This to match precision of value converted to string,
// and allow value copy/pasted from UI to be used as filter value.
template <typename TYPE>
constexpr TYPE epsilon_value() {
    if constexpr (std::is_same<TYPE, float>::value) {
        return 1.e-5f;
    } else if constexpr (std::is_same<TYPE, double>::value) {
        return 1.e-5;
    } else {
        static_assert(!std::is_same_v<TYPE, TYPE>,
                      "Epsilon value is only defined for floating point types");
    }
}

template <typename T>
bool is_equal(T const& v1, T const& v2) {
    if constexpr (is_pxr_vector_type<T>()) {
        bool res = is_equal(v1[0], v2[0]) && is_equal(v1[1], v2[1]);
        if constexpr (T::dimension > 2) {
            res = res && is_equal(v1[2], v2[2]);
            if constexpr (T::dimension > 3) {
                res = res && is_equal(v1[3], v2[3]);
            }
        }
        return res;
    } else if constexpr (is_pxr_matrix_type<T>()) {
        for (size_t row = 0; row < T::numRows; ++row) {
            for (size_t col = 0; col < T::numColumns; ++col) {
                if (!is_equal(
                        v1[static_cast<int>(row)][static_cast<int>(col)],
                        v2[static_cast<int>(row)][static_cast<int>(col)])) {
                    return false;
                }
            }
        }
        return true;
    } else if constexpr (is_pxr_quaternion_type<T>()) {
        return is_equal(v1.GetReal(), v2.GetReal()) &&
               is_equal(v1.GetImaginary(), v2.GetImaginary());
    } else if constexpr (is_pxr_dual_quaternion_type<T>()) {
        return is_equal(v1.GetReal(), v2.GetReal()) &&
               is_equal(v1.GetDual(), v2.GetDual());
    } else if constexpr (is_pxr_range_type<T>()) {
        return is_equal(v1.GetMin(), v2.GetMin()) &&
               is_equal(v1.GetMax(), v2.GetMax());
    } else if constexpr (std::is_same_v<T, GfHalf>) {
        return is_equal(static_cast<float>(v1), static_cast<float>(v2));
    } else if constexpr (std::is_same_v<T, double> ||
                         std::is_same_v<T, float>) {
        return std::isless(std::fabs(v1 - v2), epsilon_value<T>());
    } else {
        return v1 == v2;
    }
}

template <typename T>
bool is_not_equal(T const& v1, T const& v2) {
    return !is_equal(v1, v2);
}

template <typename T>
bool is_less(T const& v1, T const& v2) {
    if constexpr (is_pxr_vector_type<T>()) {
        bool res = is_less(v1[0], v2[0]) ||
                   (is_equal(v1[0], v2[0]) && is_less(v1[1], v2[1]));
        if constexpr (T::dimension > 2) {
            res = res || (is_equal(v1[1], v2[1]) && is_less(v1[2], v2[2]));
            if constexpr (T::dimension > 3) {
                res = res || (is_equal(v1[2], v2[2]) && is_less(v1[3], v2[3]));
            }
        }
        return res;
    } else if constexpr (is_pxr_matrix_type<T>()) {
        for (size_t row = 0; row < T::numRows; ++row) {
            for (size_t col = 0; col < T::numColumns; ++col) {
                if (is_less(v1[static_cast<int>(row)][static_cast<int>(col)],
                            v2[static_cast<int>(row)][static_cast<int>(col)])) {
                    return true;
                }
            }
        }
        return false;
    } else if constexpr (is_pxr_quaternion_type<T>()) {
        return is_less(v1.GetReal(), v2.GetReal()) ||
               (is_equal(v1.GetReal(), v2.GetReal()) &&
                is_less(v1.GetImaginary(), v2.GetImaginary()));
    } else if constexpr (is_pxr_dual_quaternion_type<T>()) {
        return is_less(v1.GetReal(), v2.GetReal()) ||
               (is_equal(v1.GetReal(), v2.GetReal()) &&
                is_less(v1.GetDual(), v2.GetDual()));
    } else if constexpr (is_pxr_range_type<T>()) {
        return is_less(v1.GetMin(), v2.GetMin()) ||
               (is_equal(v1.GetMin(), v2.GetMin()) &&
                is_less(v1.GetMax(), v2.GetMax()));
    } else if constexpr (std::is_same_v<T, std::string> ||
                         std::is_same_v<T, Amino::String> ||
                         std::is_same_v<T, TfToken> ||
                         std::is_same_v<T, SdfAssetPath>) {
        return v1 < v2;
    } else if constexpr (std::is_same_v<T, GfHalf>) {
        return std::isless(static_cast<float>(v1), static_cast<float>(v2));
    } else {
        return std::isless(v1, v2);
    }
}

template <typename T>
bool is_greater(T const& v1, T const& v2) {
    if constexpr (is_pxr_vector_type<T>()) {
        bool res = is_greater(v1[0], v2[0]) ||
                   (is_equal(v1[0], v2[0]) && is_greater(v1[1], v2[1]));
        if constexpr (T::dimension > 2) {
            res = res || (is_equal(v1[1], v2[1]) && is_greater(v1[2], v2[2]));
            if constexpr (T::dimension > 3) {
                res =
                    res || (is_equal(v1[2], v2[2]) && is_greater(v1[3], v2[3]));
            }
        }
        return res;
    } else if constexpr (is_pxr_matrix_type<T>()) {
        for (size_t row = 0; row < T::numRows; ++row) {
            for (size_t col = 0; col < T::numColumns; ++col) {
                if (is_greater(
                        v1[static_cast<int>(row)][static_cast<int>(col)],
                        v2[static_cast<int>(row)][static_cast<int>(col)])) {
                    return true;
                }
            }
        }
        return false;
    } else if constexpr (is_pxr_quaternion_type<T>()) {
        return is_greater(v1.GetReal(), v2.GetReal()) ||
               (is_equal(v1.GetReal(), v2.GetReal()) &&
                is_greater(v1.GetImaginary(), v2.GetImaginary()));
    } else if constexpr (is_pxr_dual_quaternion_type<T>()) {
        return is_greater(v1.GetReal(), v2.GetReal()) ||
               (is_equal(v1.GetReal(), v2.GetReal()) &&
                is_greater(v1.GetDual(), v2.GetDual()));
    } else if constexpr (is_pxr_range_type<T>()) {
        return is_greater(v1.GetMin(), v2.GetMin()) ||
               (is_equal(v1.GetMin(), v2.GetMin()) &&
                is_greater(v1.GetMax(), v2.GetMax()));
    } else if constexpr (std::is_same_v<T, std::string> ||
                         std::is_same_v<T, Amino::String> ||
                         std::is_same_v<T, TfToken> ||
                         std::is_same_v<T, SdfAssetPath>) {
        return v1 > v2;
    } else if constexpr (std::is_same_v<T, GfHalf>) {
        return std::isgreater(static_cast<float>(v1), static_cast<float>(v2));
    } else {
        return std::isgreater(v1, v2);
    }
}

template <typename T>
bool is_less_or_equal(T const& v1, T const& v2) {
    return !is_greater(v1, v2);
}

template <typename T>
bool is_greater_or_equal(T const& v1, T const& v2) {
    return !is_less(v1, v2);
}

template <typename T>
bool is_infinite(T const& v) {
    if constexpr (is_pxr_vector_type<T>()) {
        bool res = is_infinite(v[0]) || is_infinite(v[1]);
        if constexpr (T::dimension > 2) {
            res = res || is_infinite(v[2]);
            if constexpr (T::dimension > 3) {
                res = res || is_infinite(v[3]);
            }
        }
        return res;
    } else if constexpr (is_pxr_matrix_type<T>()) {
        for (size_t row = 0; row < T::numRows; ++row) {
            for (size_t col = 0; col < T::numColumns; ++col) {
                if (is_infinite(
                        v[static_cast<int>(row)][static_cast<int>(col)])) {
                    return true;
                }
            }
        }
        return false;
    } else if constexpr (is_pxr_quaternion_type<T>()) {
        return is_infinite(v.GetReal()) || is_infinite(v.GetImaginary());
    } else if constexpr (is_pxr_dual_quaternion_type<T>()) {
        return is_infinite(v.GetReal()) || is_infinite(v.GetDual());
    } else if constexpr (is_pxr_range_type<T>()) {
        return is_infinite(v.GetMin()) || is_infinite(v.GetMax());
    } else if constexpr (std::is_same_v<T, double> ||
                         std::is_same_v<T, float> ||
                         std::is_same_v<T, GfHalf>) {
        return std::isinf(v);
    } else if constexpr (std::is_same_v<T, std::string> ||
                         std::is_same_v<T, Amino::String> ||
                         std::is_same_v<T, TfToken> ||
                         std::is_same_v<T, SdfAssetPath>) {
        return false;
    } else {
        return std::isinf(static_cast<double>(v));
    }
}
template <typename T>
inline bool is_infinite_(T const& v, T const&) {
    return is_infinite<T>(v);
}

template <typename T>
bool is_not_a_number(T const& v) {
    if constexpr (is_pxr_vector_type<T>()) {
        bool res = is_not_a_number(v[0]) || is_not_a_number(v[1]);
        if constexpr (T::dimension > 2) {
            res = res || is_not_a_number(v[2]);
            if constexpr (T::dimension > 3) {
                res = res || is_not_a_number(v[3]);
            }
        }
        return res;
    } else if constexpr (is_pxr_matrix_type<T>()) {
        for (size_t row = 0; row < T::numRows; ++row) {
            for (size_t col = 0; col < T::numColumns; ++col) {
                if (is_not_a_number(
                        v[static_cast<int>(row)][static_cast<int>(col)])) {
                    return true;
                }
            }
        }
        return false;
    } else if constexpr (is_pxr_quaternion_type<T>()) {
        return is_not_a_number(v.GetReal()) ||
               is_not_a_number(v.GetImaginary());
    } else if constexpr (is_pxr_dual_quaternion_type<T>()) {
        return is_not_a_number(v.GetReal()) || is_not_a_number(v.GetDual());
    } else if constexpr (is_pxr_range_type<T>()) {
        return is_not_a_number(v.GetMin()) || is_not_a_number(v.GetMax());
    } else if constexpr (std::is_same_v<T, double> ||
                         std::is_same_v<T, float> ||
                         std::is_same_v<T, GfHalf>) {
        return std::isnan(v);
    } else if constexpr (std::is_same_v<T, std::string> ||
                         std::is_same_v<T, Amino::String> ||
                         std::is_same_v<T, TfToken> ||
                         std::is_same_v<T, SdfAssetPath>) {
        return false;
    } else {
        return std::isnan(static_cast<double>(v));
    }
}
template <typename T>
inline bool is_not_a_number_(T const& v, T const&) {
    return is_not_a_number<T>(v);
}

template <typename T>
std::function<bool(T const&, T const&)> getCompareFunc(CompareOp op) {
    switch (op) {
        case Filter::Operation::eLess: return &is_less<T>;
        case Filter::Operation::eLessOrEqual: return &is_less_or_equal<T>;
        case Filter::Operation::eEqual: return &is_equal<T>;
        case Filter::Operation::eNotEqual: return &is_not_equal<T>;
        case Filter::Operation::eGreaterOrEqual: return &is_greater_or_equal<T>;
        case Filter::Operation::eGreater: return &is_greater<T>;
        case Filter::Operation::eIsInfinite: return &is_infinite_<T>;
        case Filter::Operation::eIsNotANumber: return &is_not_a_number_<T>;
        case Filter::Operation::eSubFilters: assert(false); break;
    }
    UNREACHABLE
}

template <typename T>
bool compareValuesT(T const& a, T const& b, CompareOp op) {
    auto compareFn = getCompareFunc<T>(op);
    return compareFn(a, b);
}

template <>
bool compareValuesT<>(VtValue const& a, VtValue const& b, CompareOp op) {
    if (a.IsEmpty() || b.IsEmpty()) {
        return compareValuesT(a.IsEmpty(), b.IsEmpty(), op);
    }
#define HANDLE_TYPE(T)                                \
    if (a.IsHolding<T>() && b.IsHolding<T>()) {       \
        auto const& typedA = a.UncheckedGet<T>();     \
        auto const& typedB = b.UncheckedGet<T>();     \
        return compareValuesT<T>(typedA, typedB, op); \
    }

#define HANDLE_VT_TYPE(element) HANDLE_TYPE(VT_TYPE(element))
    FOR_EACH_VT_VALUE_TYPES
#undef HANDLE_VT_TYPE
#undef HANDLE_TYPE

    return compareValuesT(a.GetHash(), b.GetHash(), op);
}

///-------------------------------------------------------------------------
/// \brief Helper to get the name for the different supported types
template <typename T>
constexpr const char* vtGetTypeName() {
    if constexpr (std::is_same_v<T, Amino::String>) {
        return "String";
    } else if constexpr (std::is_same_v<T, std::size_t>) {
        return "size_t";
    } else if constexpr (std::is_same_v<T, SdfAssetPath>) {
        return "SdfAssetPath";
    } else if constexpr (std::is_same_v<T, VtArray<SdfAssetPath>>) {
        return "VtArray<SdfAssetPath>";
    }
    // TF_PP_CAT and TF_PP_STRINGIZE are defined in
    // <pxr/base/tf/preprocessorUtils.h>
#define HANDLE_VT_TYPE(element)                                        \
    else if constexpr (std::is_same_v<T, VT_TYPE(element)>) {          \
        return TF_PP_STRINGIZE(VT_TYPE_NAME(element));                 \
    }                                                                  \
    else if constexpr (std::is_same_v<T, VtArray<VT_TYPE(element)>>) { \
        return TF_PP_STRINGIZE(                                        \
            TF_PP_CAT(VtArray, VT_TYPE_NAME(element)));                \
    }
    FOR_EACH_VT_VALUE_TYPES
#undef HANDLE_VT_TYPE
    else {
        static_assert(!std::is_same_v<T, T>,
                      "vtGetTypeName is not implemented for this type");
    }
}

///-------------------------------------------------------------------------
/// \brief Helper to convert values to string
/// \{
template <typename T>
Amino::String to_string(T const& value) {
    if constexpr (std::is_same_v<T, const char*>) {
        return Amino::String(value);
    } else if constexpr (std::is_same_v<T, Amino::String>) {
        return value;
    } else if constexpr (std::is_same_v<T, std::string>) {
        return Amino::String(value.c_str());
    } else if constexpr (std::is_same_v<T, bool>) {
        return Amino::String(value ? "true" : "false");
    } else if constexpr (std::is_same_v<T, std::size_t>) {
        return to_string(
            std::to_string(static_cast<unsigned long long>(value)));
    } else if constexpr (std::is_same_v<T, GfHalf>) {
        return to_string(std::to_string(value));
    } else if constexpr (std::is_same_v<T, SdfAssetPath>) {
        return to_string(value.GetAssetPath());
    } else if constexpr (std::is_same_v<T, TfToken>) {
        return to_string(value.GetText());
    } else {
        return to_string(std::to_string(value).c_str());
    }
}
/// \}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
template <typename STR, typename T>
bool stringToValue(STR const& fromValue, T& toValue) {
    if constexpr (std::is_same_v<STR, T>) { // string to string conversion, just
                                            // return the string
        toValue = fromValue;
        return true;
    } else if constexpr (std::is_same_v<STR, Amino::String>) {
        return stringToValue(
            std::string(static_cast<std::string_view>(fromValue)), toValue);
    } else if constexpr (std::is_same_v<T, TfToken>) {
        toValue = TfToken::Find(fromValue);
        return true;
    } else if constexpr (std::is_same_v<T, SdfAssetPath>) {
        toValue = SdfAssetPath(fromValue);
        return true;
    } else if (!fromValue.empty()) {
        try {
            if constexpr (std::is_same_v<T, bool>) {
                toValue = static_cast<T>(std::stoi(fromValue));
            } else if constexpr (std::is_same_v<T, char>) {
                toValue = static_cast<T>(std::stoi(fromValue));
            } else if constexpr (std::is_same_v<T, unsigned char>) {
                toValue = static_cast<T>(std::stoi(fromValue));
            } else if constexpr (std::is_same_v<T, short>) {
                toValue = static_cast<T>(std::stoi(fromValue));
            } else if constexpr (std::is_same_v<T, unsigned short>) {
                toValue = static_cast<T>(std::stoi(fromValue));
            } else if constexpr (std::is_same_v<T, int>) {
                toValue = static_cast<T>(std::stoi(fromValue));
            } else if constexpr (std::is_same_v<T, unsigned int>) {
                toValue = static_cast<T>(std::stoull(fromValue));
            } else if constexpr (std::is_same_v<T, int64_t>) {
                toValue = static_cast<T>(std::stoll(fromValue));
            } else if constexpr (std::is_same_v<T, uint64_t>) {
                toValue = static_cast<T>(std::stoull(fromValue));
            } else if constexpr (std::is_same_v<T, std::size_t>) {
                toValue = static_cast<T>(std::stoull(fromValue));
            } else if constexpr (std::is_same_v<T, float>) {
                toValue = std::stof(fromValue);
            } else if constexpr (std::is_same_v<T, double>) {
                toValue = std::stod(fromValue);
            } else if constexpr (std::is_same_v<T, GfHalf>) {
                toValue = static_cast<GfHalf>(std::stof(fromValue));
            } else {
                static_assert(!std::is_same_v<T, T>,
                              "stringToValue is not implemented for this type");
            }
            return true;
        } catch (...) {
        }
    }
    return false;
}

using StringArray = std::vector<std::string>;

template <typename T>
bool stringArrayToValue(StringArray const& fromValues, T& toValue) {
    if constexpr (is_pxr_scalar_type<T>() || is_pxr_string_type<T>()) {
        if (fromValues.size() == 1) {
            return stringToValue(fromValues[0], toValue);
        }
    } else if constexpr (is_pxr_vector_type<T>()) {
        if (fromValues.size() == T::dimension) {
            for (size_t i = 0; i < static_cast<size_t>(T::dimension); ++i) {
                if (!stringToValue(fromValues[i], toValue[i])) {
                    return false;
                }
            }
            return true;
        }
    } else if constexpr (is_pxr_matrix_type<T>()) {
        if (fromValues.size() == T::numColumns * T::numRows) {
            for (size_t row = 0; row < T::numRows; ++row) {
                for (size_t col = 0; col < T::numColumns; ++col) {
                    if (!stringToValue(fromValues[row * T::numColumns + col],
                                       toValue[static_cast<int>(row)]
                                              [static_cast<int>(col)])) {
                        return false;
                    }
                }
            }
            return true;
        }
    } else if constexpr (is_pxr_quaternion_type<T>()) {
        if (fromValues.size() == 4) {
            typename QuaternionTraits<T>::RealType realValue;
            if (stringToValue(fromValues[0], realValue)) {
                typename QuaternionTraits<T>::ImaginaryType imaginaryValue;
                if (stringArrayToValue(
                        StringArray(fromValues.begin() + 1, fromValues.end()),
                        imaginaryValue)) {
                    toValue = T(realValue, imaginaryValue);
                    return true;
                }
            }
        }
    } else if constexpr (is_pxr_dual_quaternion_type<T>()) {
        if (fromValues.size() == 8) {
            typename DualQuaternionTraits<T>::QuaternionType realValue;
            if (stringArrayToValue(
                    StringArray(fromValues.begin(), fromValues.begin() + 4),
                    realValue)) {
                typename DualQuaternionTraits<T>::QuaternionType dualValue;
                if (stringArrayToValue(
                        StringArray(fromValues.begin() + 4, fromValues.end()),
                        dualValue)) {
                    toValue = T(realValue, dualValue);
                    return true;
                }
            }
        }
    } else if constexpr (is_pxr_range_type<T>()) {
        if (fromValues.size() == 2 * RangeTraits<T>::dimension) {
            typename RangeTraits<T>::MinMaxType minValue;
            if (stringArrayToValue(
                    StringArray(fromValues.begin(),
                                fromValues.begin() + RangeTraits<T>::dimension),
                    minValue)) {
                typename RangeTraits<T>::MinMaxType maxValue;
                if (stringArrayToValue(
                        StringArray(
                            fromValues.begin() + RangeTraits<T>::dimension,
                            fromValues.end()),
                        maxValue)) {
                    toValue = T(minValue, maxValue);
                    return true;
                }
            }
        }
    } else {
        static_assert(!std::is_same_v<T, T>,
                      "stringArrayToValue with StringArray is not implemented "
                      "for this type");
    }
    return false;
}

StringArray toStringArray(Amino::String const& fromValue) {
    StringArray result;
    if (!fromValue.empty()) {
        std::string value(fromValue);
        // Strip parentheses or braces if present
        if ((value.front() == '(' && value.back() == ')') ||
            (value.front() == '{' && value.back() == '}')) {
            value = value.substr(1, value.size() - 2);
        }
        std::string_view strView(value);
        std::size_t      start = 0;
        while (start < strView.size()) {
            auto commaPos = strView.find(',', start);
            if (commaPos == std::string_view::npos) {
                commaPos = strView.size();
            }
            auto token = strView.substr(start, commaPos - start);
            // Trim whitespace
            auto firstNonSpace = token.find_first_not_of(' ');
            auto lastNonSpace  = token.find_last_not_of(' ');
            if (firstNonSpace != std::string_view::npos &&
                lastNonSpace != std::string_view::npos) {
                result.emplace_back(token.substr(
                    firstNonSpace, lastNonSpace - firstNonSpace + 1));
            }
            start = commaPos + 1;
        }
    }
    return result;
}

template <typename T>
bool anyToValueAny(Amino::Any const& fromAny, std::any& toAny);

template <typename T>
bool anyToVtValueAny(Amino::Any const& fromAny, std::any& toAny);

bool anyToAny(Amino::Any const& fromAny, std::any& toAny) {
    auto const& toType = toAny.type();
    if (toType == typeid(VtValue)) {
        auto const& toVtValue = std::any_cast<VtValue>(toAny);
#define HANDLE_TYPE(T)                             \
    if (toVtValue.IsHolding<T>()) {                \
        return anyToVtValueAny<T>(fromAny, toAny); \
    }

#define HANDLE_VT_TYPE(element) HANDLE_TYPE(VT_TYPE(element))
        FOR_EACH_VT_VALUE_TYPES
#undef HANDLE_VT_TYPE
        HANDLE_TYPE(SdfAssetPath)
#undef HANDLE_TYPE

    } else {
#define HANDLE_TYPE(T)                           \
    if (toType == typeid(T)) {                   \
        return anyToValueAny<T>(fromAny, toAny); \
    }

#define HANDLE_VT_TYPE(element) HANDLE_TYPE(VT_TYPE(element))
        FOR_EACH_VT_VALUE_TYPES
#undef HANDLE_VT_TYPE
        HANDLE_TYPE(SdfAssetPath)
#undef HANDLE_TYPE
    }

    return false;
}

template <typename T>
bool anyToValue(Amino::Any const& fromAny, T& toValue) {
    if constexpr (std::is_same<T, std::any>::value) {
        return anyToAny(fromAny, toValue);
    } else if constexpr (std::is_same<T, std::string>::value ||
                         std::is_same_v<T, SdfAssetPath> ||
                         std::is_same_v<T, TfToken>) {
        if (fromAny.type() == Amino::getTypeId<Amino::String>()) {
            auto strValue = Amino::any_cast<Amino::String>(fromAny);
            return stringToValue(strValue, toValue);
        }
    } else {
        if (fromAny.type() == Amino::getTypeId<T>()) {
            toValue = Amino::any_cast<T>(fromAny);
            return true;
        }

        if constexpr (!std::is_same<T, Amino::String>::value) {
            if (fromAny.type() == Amino::getTypeId<Amino::String>()) {
                auto strValue = Amino::any_cast<Amino::String>(fromAny);
                if constexpr (is_pxr_vector_type<T>() ||
                              is_pxr_matrix_type<T>() ||
                              is_pxr_quaternion_type<T>() ||
                              is_pxr_dual_quaternion_type<T>() ||
                              is_pxr_range_type<T>()) {
                    return stringArrayToValue(toStringArray(strValue), toValue);
                } else {
                    if constexpr (std::is_same<T, bool>::value) {
                        std::transform(
                            strValue.begin(), strValue.end(), strValue.begin(),
                            [](unsigned char c) { return std::tolower(c); });
                        if (strValue == "true") {
                            toValue = true;
                            return true;
                        } else if (strValue == "false") {
                            toValue = false;
                            return true;
                        }
                    }
                    return stringToValue(strValue, toValue);
                }
            }
        }
    }

    return false;
}

template <typename T>
bool anyToValueAny(Amino::Any const& fromAny, std::any& toAny) {
    T outTypedValue;
    if (anyToValue(fromAny, outTypedValue)) {
        toAny = std::any(outTypedValue);
        return true;
    }
    return false;
}

template <typename T>
bool anyToVtValueAny(Amino::Any const& fromAny, std::any& toAny) {
    T outTypedValue;
    if (anyToValue(fromAny, outTypedValue)) {
        toAny = std::any(VtValue(outTypedValue));
        return true;
    }
    return false;
}


///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
class USDWatchpointLayoutArray {
public:
    USDWatchpointLayoutArray()          = default;
    virtual ~USDWatchpointLayoutArray() = default;

protected:
    bool getIndices_impl(std::size_t    size,
                         Filters const& filters,
                         Sorter const&  sorter,
                         Indices&       out_indices) const noexcept {
        try {
            if (size > 0) {
                Indices all_indices;
                all_indices.reserve(size);
                for (size_t idx = 0; idx < size; ++idx) {
                    all_indices.push_back(idx);
                }

                out_indices = all_indices;
                if (!applyFilters(filters, all_indices, out_indices)) {
                    return false;
                }

                if (out_indices.size() > 1) {
                    if (!sortIndices(sorter, out_indices)) {
                        return false;
                    }
                }
            }
            return true;
        } catch (...) {
            return false; // LCOV_EXCL_LINE
        }
    }

private:
    virtual bool filterByValue(CompareOp         compareOp,
                               Amino::Any const& filterValue,
                               Indices const&    indices,
                               Indices&          out_indices) const   = 0;
    virtual bool filterByElement(Amino::String const& elementName,
                                 CompareOp            compareOp,
                                 Amino::Any const&    filterValue,
                                 Indices const&       indices,
                                 Indices&             out_indices) const = 0;

    bool filterIndices(Filter const&  filter,
                       Indices const& indices,
                       Indices&       out_indices) const {
        if (indices.empty()) {
            return true;
        }
        if (filter.m_field == Field::eIndex) {
            std::size_t filterValue;
            if (anyToValue(filter.m_value, filterValue)) {
                auto compareFn =
                    getCompareFunc<std::size_t>(filter.m_operation);
                for (auto idx : indices) {
                    if (compareFn(idx, filterValue)) {
                        out_indices.push_back(idx);
                    }
                }
                return true;
            }
        } else if (filter.m_field == Field::eValue) {
            return filterByValue(filter.m_operation, filter.m_value, indices,
                                 out_indices);
        } else if (filter.m_field == Field::eElement) {
            return filterByElement(filter.m_elementName, filter.m_operation,
                                   filter.m_value, indices, out_indices);
        }
        return false; // LCOV_EXCL_LINE
    }

    bool applyFilter(Filter const&  filter,
                     Indices const& all_indices,
                     Indices&       out_indices) const {
        auto const& working_indices =
            filter.m_conjunction == Filter::Conjunction::eAnd ? out_indices
                                                              : all_indices;
        Indices new_indices;
        new_indices.reserve(working_indices.size());

        if (filter.m_operation == Filter::Operation::eSubFilters) {
            if (!applyFilters(filter.m_subFilters, all_indices, new_indices)) {
                return false;
            }
        } else if (!filterIndices(filter, working_indices, new_indices)) {
            return false;
        }

        out_indices.swap(new_indices);
        return true;
    }

    bool applyFilters(Filters const& filters,
                      Indices const& all_indices,
                      Indices&       out_indices) const {
        if (!filters.empty()) {
            if (filters.front().m_conjunction == Filter::Conjunction::eOr) {
                out_indices.clear();
            } else {
                out_indices = all_indices;
            }
            for (auto const& filter : filters) {
                Indices new_indices = out_indices;
                if (!applyFilter(filter, all_indices, new_indices)) {
                    return false;
                }

                if (new_indices.empty()) {
                    if (filter.m_conjunction == Filter::Conjunction::eAnd) {
                        out_indices.clear();
                    }
                } else {
                    if (out_indices.empty()) {
                        if (filter.m_conjunction == Filter::Conjunction::eOr) {
                            out_indices.swap(new_indices);
                        }
                    } else if (filter.m_conjunction ==
                               Filter::Conjunction::eAnd) {
                        Indices filtered_indices;
                        std::set_intersection(
                            out_indices.begin(), out_indices.end(),
                            new_indices.begin(), new_indices.end(),
                            std::back_inserter(filtered_indices));
                        out_indices.swap(filtered_indices);
                    } else {
                        Indices filtered_indices;
                        std::merge(out_indices.begin(), out_indices.end(),
                                   new_indices.begin(), new_indices.end(),
                                   std::back_inserter(filtered_indices));
                        // remove duplicates
                        auto last = std::unique(filtered_indices.begin(),
                                                filtered_indices.end());
                        filtered_indices.erase(last, filtered_indices.end());
                        out_indices.swap(filtered_indices);
                    }
                }
            }
        }
        return true;
    }

    virtual bool sortByValue(CompareOp compareOp,
                             Indices&  out_indices) const   = 0;
    virtual bool sortByElement(Amino::String const& elementName,
                               CompareOp            compareOp,
                               Indices&             out_indices) const = 0;

    bool sortIndices(Sorter const& sorter, Indices& out_indices) const {
        if (sorter.m_field == Field::eIndex) {
            std::sort(out_indices.begin(), out_indices.end());
            if (sorter.m_order == Sorter::Order::eDescending) {
                std::reverse(out_indices.begin(), out_indices.end());
            }
            return true;
        } else {
            auto compareOp = (sorter.m_order == Sorter::Order::eDescending
                                  ? CompareOp::eGreater
                                  : CompareOp::eLess);
            if (sorter.m_field == Field::eValue) {
                return sortByValue(compareOp, out_indices);
            } else if (sorter.m_field == Field::eElement) {
                return sortByElement(sorter.m_elementName, compareOp,
                                     out_indices);
            }
        }
        return false; // LCOV_EXCL_LINE
    }
};

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
template <typename T>
class USDWatchpointLayoutVtValue {
public:
    USDWatchpointLayoutVtValue(T const& value) : m_value(value) {}
    virtual ~USDWatchpointLayoutVtValue() = default;

    T const& value() const noexcept { return m_value; }

protected:
    T m_value;
};

template <typename T>
class USDWatchpointLayoutVtValueScalar : public USDWatchpointLayoutVtValue<T>,
                                         public WatchpointLayout {
public:
    using BaseClass = USDWatchpointLayoutVtValue<T>;

    USDWatchpointLayoutVtValueScalar(T const& value)
        : USDWatchpointLayoutVtValue<T>(value),
          WatchpointLayout(vtGetTypeName<T>()) {}
    ~USDWatchpointLayoutVtValueScalar() override = default;

    void deleteThis() noexcept final { delete this; }
    bool getValue(WatchpointLayoutPath& path,
                  Amino::String&        out_value) const noexcept final {
        if (path.empty()) {
            out_value = to_string(BaseClass::m_value);
            return true;
        }
        return false;
    }
};

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
constexpr std::array<const char*, 4> kVectorComponents = {
    {"s0", "s1", "s2", "s3"}};
constexpr std::array<std::array<const char*, 4>, 4> kMatrixComponents = {
    {{{"m00", "m01", "m02", "m03"}},
     {{"m10", "m11", "m12", "m13"}},
     {{"m20", "m21", "m22", "m23"}},
     {{"m30", "m31", "m32", "m33"}}}};
constexpr std::array<const char*, 4> kQuaternionComponents = {
    {"r", "i", "j", "k"}};
const std::array<const char*, 2> kDualQuaternionComponents = {{"real", "dual"}};
const std::array<const char*, 2> kRangeComponents          = {{"min", "max"}};

template <typename T>
class USDWatchpointLayoutVtValueComposite
    : public USDWatchpointLayoutVtValue<T>,
      public WatchpointLayoutComposite {
public:
    using BaseClass = USDWatchpointLayoutVtValue<T>;

    USDWatchpointLayoutVtValueComposite(T const& value)
        : USDWatchpointLayoutVtValue<T>(value),
          WatchpointLayoutComposite(vtGetTypeName<T>()) {
        try {
            buildLayouts();
        } catch (...) {
        }
    }
    ~USDWatchpointLayoutVtValueComposite() override = default;

    void deleteThis() noexcept final { delete this; }

    std::size_t size() const noexcept final { return m_layouts.size(); }

    WatchpointLayoutPtr get(Amino::String const& name) const noexcept final {
        for (size_t i = 0; i < m_layouts.size(); ++i) {
            if (name == m_layouts.at(i).name()) {
                return m_layouts.at(i).layout();
            }
        }
        return {};
    }

    SubLayout get(std::size_t index) const noexcept final {
        if (index < m_layouts.size()) {
            return m_layouts.at(index);
        }
        return {};
    }

private:
    void buildLayouts();

private:
    std::vector<SubLayout> m_layouts;
};

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
template <typename T>
class USDWatchpointLayoutVtValueArray : public USDWatchpointLayoutVtValue<T>,
                                        public WatchpointLayoutArray,
                                        public USDWatchpointLayoutArray {
public:
    using BaseClass = USDWatchpointLayoutVtValue<T>;

    USDWatchpointLayoutVtValueArray(T const& value)
        : USDWatchpointLayoutVtValue<T>(value),
          WatchpointLayoutArray(vtGetTypeName<T>()),
          USDWatchpointLayoutArray() {}
    ~USDWatchpointLayoutVtValueArray() override = default;

    void deleteThis() noexcept final { delete this; }

    std::size_t size() const noexcept final {
        return BaseClass::m_value.size();
    }

    WatchpointLayoutPtr get(std::size_t index) const noexcept final;

    bool getValue(WatchpointLayoutPath& path,
                  Amino::String&        out_value) const noexcept final {
        if (!path.empty() && path.frontIsIndex()) {
            auto index         = path.frontAsIndex();
            auto elementLayout = get(index);
            if (elementLayout) {
                return elementLayout->getValue(path.pop_front(), out_value);
            }
        }
        return false; // LCOV_EXCL_LINE
    }

    bool isSortable(Sorter::Field, Amino::String const&) const noexcept final {
        return !BaseClass::m_value.empty();
    }
    bool isFilterable(Filter::Field,
                      Amino::String const&) const noexcept final {
        return !BaseClass::m_value.empty();
    }

    bool getIndices(Filters const& filters,
                    Sorter const&  sorter,
                    Indices&       out_indices) const noexcept final {
        return getIndices_impl(BaseClass::m_value.size(), filters, sorter,
                               out_indices);
    }

private:
    bool filterByValue(CompareOp         compareOp,
                       Amino::Any const& filterValue_,
                       Indices const&    indices,
                       Indices&          out_indices) const final {
        typename T::value_type filterValue;
        if (anyToValue(filterValue_, filterValue)) {
            for (auto idx : indices) {
                auto const& value = BaseClass::m_value[idx];
                if (compareValuesT(value, filterValue, compareOp)) {
                    out_indices.push_back(idx);
                }
            }
            return true;
        }
        return false;
    }

    bool filterByElement(Amino::String const& elementName,
                         CompareOp            compareOp,
                         Amino::Any const&    filterValue_,
                         Indices const&       indices,
                         Indices&             out_indices) const final {
        using ValueType = typename T::value_type;
        if constexpr (is_pxr_vector_type<ValueType>()) {
            typename ValueType::ScalarType filterValue;
            if (anyToValue(filterValue_, filterValue)) {
                for (size_t eltIdx = 0; eltIdx < ValueType::dimension;
                     ++eltIdx) {
                    if (elementName == kVectorComponents[eltIdx]) {
                        for (auto idx : indices) {
                            auto const& value = BaseClass::m_value[idx][eltIdx];
                            if (compareValuesT(value, filterValue, compareOp)) {
                                out_indices.push_back(idx);
                            }
                        }
                        return true;
                    }
                }
            }
        } else if constexpr (is_pxr_matrix_type<ValueType>()) {
            typename ValueType::ScalarType filterValue;
            if (anyToValue(filterValue_, filterValue)) {
                for (size_t row = 0; row < ValueType::numRows; ++row) {
                    for (size_t col = 0; col < ValueType::numColumns; ++col) {
                        if (elementName == kMatrixComponents[row][col]) {
                            for (auto idx : indices) {
                                auto const& value =
                                    BaseClass::m_value[idx][static_cast<int>(
                                        row)][static_cast<int>(col)];
                                if (compareValuesT(value, filterValue,
                                                   compareOp)) {
                                    out_indices.push_back(idx);
                                }
                            }
                            return true;
                        }
                    }
                }
            }
        } else if constexpr (is_pxr_quaternion_type<ValueType>()) {
            for (size_t eltIdx = 0; eltIdx < kQuaternionComponents.size();
                 ++eltIdx) {
                if (elementName == kQuaternionComponents[eltIdx]) {
                    if (eltIdx == 0) {
                        typename QuaternionTraits<ValueType>::RealType
                            filterValue;
                        if (anyToValue(filterValue_, filterValue)) {
                            for (auto idx : indices) {
                                auto const& value =
                                    BaseClass::m_value[idx].GetReal();
                                if (compareValuesT(value, filterValue,
                                                   compareOp)) {
                                    out_indices.push_back(idx);
                                }
                            }
                            return true;
                        }
                    } else {
                        typename QuaternionTraits<
                            ValueType>::ImaginaryType::ScalarType filterValue;
                        if (anyToValue(filterValue_, filterValue)) {
                            for (auto idx : indices) {
                                auto const& value =
                                    BaseClass::m_value[idx]
                                        .GetImaginary()[eltIdx - 1];
                                if (compareValuesT(value, filterValue,
                                                   compareOp)) {
                                    out_indices.push_back(idx);
                                }
                            }
                            return true;
                        }
                    }
                }
            }
        } else if constexpr (is_pxr_dual_quaternion_type<ValueType>()) {
            for (size_t eltIdx = 0; eltIdx < kDualQuaternionComponents.size();
                 ++eltIdx) {
                if (elementName == kDualQuaternionComponents[eltIdx]) {
                    typename DualQuaternionTraits<ValueType>::QuaternionType
                        filterValue;
                    if (anyToValue(filterValue_, filterValue)) {
                        for (auto idx : indices) {
                            auto const& value =
                                eltIdx == 0 ? BaseClass::m_value[idx].GetReal()
                                            : BaseClass::m_value[idx].GetDual();
                            if (compareValuesT(value, filterValue, compareOp)) {
                                out_indices.push_back(idx);
                            }
                        }
                        return true;
                    }
                }
            }
        } else if constexpr (is_pxr_range_type<ValueType>()) {
            for (size_t eltIdx = 0; eltIdx < kRangeComponents.size();
                 ++eltIdx) {
                if (elementName == kRangeComponents[eltIdx]) {
                    typename RangeTraits<ValueType>::MinMaxType filterValue;
                    if (anyToValue(filterValue_, filterValue)) {
                        for (auto idx : indices) {
                            auto const& value =
                                eltIdx == 0 ? BaseClass::m_value[idx].GetMin()
                                            : BaseClass::m_value[idx].GetMax();
                            if (compareValuesT(value, filterValue, compareOp)) {
                                out_indices.push_back(idx);
                            }
                        }
                        return true;
                    }
                }
            }
        } else if constexpr (is_pxr_scalar_type<ValueType>() ||
                             is_pxr_string_type<ValueType>()) {
            return false; // LCOV_EXCL_LINE
        } else {
            static_assert(!std::is_same_v<ValueType, ValueType>,
                          "filterByElement is not implemented for this type");
        }
        return false;
    }

    bool sortByValue(CompareOp compareOp, Indices& out_indices) const final {
        std::sort(out_indices.begin(), out_indices.end(),
                  [&](std::size_t a, std::size_t b) {
                      auto const& valueA = BaseClass::m_value[a];
                      auto const& valueB = BaseClass::m_value[b];
                      return compareValuesT(valueA, valueB, compareOp);
                  });
        return true;
    }

    bool sortByElement(Amino::String const& elementName,
                       CompareOp            compareOp,
                       Indices&             out_indices) const final {
        using ValueType = typename T::value_type;
        if constexpr (is_pxr_vector_type<ValueType>()) {
            for (size_t eltIdx = 0; eltIdx < ValueType::dimension; ++eltIdx) {
                if (elementName == kVectorComponents[eltIdx]) {
                    std::sort(
                        out_indices.begin(), out_indices.end(),
                        [&](std::size_t a, std::size_t b) {
                            auto const& valueA = BaseClass::m_value[a][eltIdx];
                            auto const& valueB = BaseClass::m_value[b][eltIdx];
                            return compareValuesT(valueA, valueB, compareOp);
                        });
                    return true;
                }
            }
        } else if constexpr (is_pxr_matrix_type<ValueType>()) {
            for (size_t row = 0; row < ValueType::numRows; ++row) {
                for (size_t col = 0; col < ValueType::numColumns; ++col) {
                    if (elementName == kMatrixComponents[row][col]) {
                        std::sort(
                            out_indices.begin(), out_indices.end(),
                            [&](std::size_t a, std::size_t b) {
                                auto const& valueA =
                                    BaseClass::m_value[a][static_cast<int>(row)]
                                                      [static_cast<int>(col)];
                                auto const& valueB =
                                    BaseClass::m_value[b][static_cast<int>(row)]
                                                      [static_cast<int>(col)];
                                return compareValuesT(valueA, valueB,
                                                      compareOp);
                            });
                        return true;
                    }
                }
            }
        } else if constexpr (is_pxr_quaternion_type<ValueType>()) {
            for (size_t eltIdx = 0; eltIdx < kQuaternionComponents.size();
                 ++eltIdx) {
                if (elementName == kQuaternionComponents[eltIdx]) {
                    if (eltIdx == 0) {
                        std::sort(out_indices.begin(), out_indices.end(),
                                  [&](std::size_t a, std::size_t b) {
                                      auto const& valueA =
                                          BaseClass::m_value[a].GetReal();
                                      auto const& valueB =
                                          BaseClass::m_value[b].GetReal();
                                      return compareValuesT(valueA, valueB,
                                                            compareOp);
                                  });
                    } else {
                        std::sort(out_indices.begin(), out_indices.end(),
                                  [&](std::size_t a, std::size_t b) {
                                      auto const& valueA =
                                          BaseClass::m_value[a]
                                              .GetImaginary()[eltIdx - 1];
                                      auto const& valueB =
                                          BaseClass::m_value[b]
                                              .GetImaginary()[eltIdx - 1];
                                      return compareValuesT(valueA, valueB,
                                                            compareOp);
                                  });
                    }
                    return true;
                }
            }
        } else if constexpr (is_pxr_dual_quaternion_type<ValueType>()) {
            for (size_t eltIdx = 0; eltIdx < kDualQuaternionComponents.size();
                 ++eltIdx) {
                if (elementName == kDualQuaternionComponents[eltIdx]) {
                    std::sort(
                        out_indices.begin(), out_indices.end(),
                        [&](std::size_t a, std::size_t b) {
                            auto const& valueA =
                                eltIdx == 0 ? BaseClass::m_value[a].GetReal()
                                            : BaseClass::m_value[a].GetDual();
                            auto const& valueB =
                                eltIdx == 0 ? BaseClass::m_value[b].GetReal()
                                            : BaseClass::m_value[b].GetDual();
                            return compareValuesT(valueA, valueB, compareOp);
                        });
                    return true;
                }
            }
        } else if constexpr (is_pxr_range_type<ValueType>()) {
            for (size_t eltIdx = 0; eltIdx < kRangeComponents.size();
                 ++eltIdx) {
                if (elementName == kRangeComponents[eltIdx]) {
                    std::sort(
                        out_indices.begin(), out_indices.end(),
                        [&](std::size_t a, std::size_t b) {
                            auto const& valueA =
                                eltIdx == 0 ? BaseClass::m_value[a].GetMin()
                                            : BaseClass::m_value[a].GetMax();
                            auto const& valueB =
                                eltIdx == 0 ? BaseClass::m_value[b].GetMin()
                                            : BaseClass::m_value[b].GetMax();
                            return compareValuesT(valueA, valueB, compareOp);
                        });
                    return true;
                }
            }
        } else if constexpr (is_pxr_scalar_type<ValueType>() ||
                             is_pxr_string_type<ValueType>()) {
            return false; // LCOV_EXCL_LINE
        } else {
            static_assert(!std::is_same_v<ValueType, ValueType>,
                          "sortByElement is not implemented for this type");
        }
        return false; // LCOV_EXCL_LINE
    }
};

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
template <typename T>
WatchpointLayoutPtr getValueLayoutT(T const& value) {
    if constexpr (is_pxr_vector_type<T>() || is_pxr_matrix_type<T>() ||
                  is_pxr_quaternion_type<T>() ||
                  is_pxr_dual_quaternion_type<T>() || is_pxr_range_type<T>()) {
        return WatchpointLayout::createT<
            USDWatchpointLayoutVtValueComposite<T>>(value);
    } else {
        return WatchpointLayout::createT<USDWatchpointLayoutVtValueScalar<T>>(
            value);
    }
}

template <>
WatchpointLayoutPtr getValueLayoutT(VtValue const& value) {
    if (!value.IsEmpty()) {
#define HANDLE_TYPE(T)                                             \
    if (value.IsHolding<T>()) {                                    \
        auto const& vtValue = value.UncheckedGet<T>();             \
        return getValueLayoutT(vtValue);                           \
    }                                                              \
    if (value.IsHolding<VtArray<T>>()) {                           \
        auto const& vtArray = value.UncheckedGet<VtArray<T>>();    \
        return WatchpointLayout::createT<                          \
            USDWatchpointLayoutVtValueArray<VtArray<T>>>(vtArray); \
    }

#define HANDLE_VT_TYPE(element) HANDLE_TYPE(VT_TYPE(element))
        FOR_EACH_VT_VALUE_TYPES
#undef HANDLE_VT_TYPE
        HANDLE_TYPE(SdfAssetPath)
#undef HANDLE_TYPE

        assert(false &&
               "getValueLayoutT VtValue called for unsupported value type");
    }
    return {}; // LCOV_EXCL_LINE
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
WatchpointLayoutPtr getValueLayoutFromAny(
    WatchpointLayoutFactory const& factory,
    Amino::TypeRegistry const&     typeRegistry,
    std::any const&                value) {
    assert(value.has_value());

#define HANDLE_TYPE(T)                                    \
    if (value.type() == typeid(T)) {                      \
        auto const& typedValue = std::any_cast<T>(value); \
        return getValueLayoutT(typedValue);               \
    }

#define HANDLE_VT_TYPE(element) HANDLE_TYPE(VT_TYPE(element))
    FOR_EACH_VT_VALUE_TYPES
#undef HANDLE_VT_TYPE

    HANDLE_TYPE(VtValue)
    HANDLE_TYPE(Amino::String)
#undef HANDLE_TYPE

#define HANDLE_TYPE(T)                                       \
    if (value.type() == typeid(T)) {                         \
        auto const& typedValue = std::any_cast<T>(value);    \
        return factory.getLayoutT(typeRegistry, typedValue); \
    }
    HANDLE_TYPE(LayerPtr)
    HANDLE_TYPE(ArrayOfAttributes)
    HANDLE_TYPE(ArrayOfPrims)
#undef HANDLE_TYPE

    assert(false && "getValueLayoutFromAny called for unsupported value type");
    return {}; // LCOV_EXCL_LINE
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
bool compareValuesAny(std::any const& a, std::any const& b, CompareOp op) {
    if (!a.has_value() || !b.has_value()) {
        return compareValuesT(a.has_value(), b.has_value(), op);
    }
    if (a.type() == b.type()) {
#define HANDLE_TYPE(T)                                       \
    if (a.type() == typeid(T)) {                             \
        auto const& typedValueA = std::any_cast<T>(a);       \
        auto const& typedValueB = std::any_cast<T>(b);       \
        return compareValuesT(typedValueA, typedValueB, op); \
    }

#define HANDLE_VT_TYPE(element) HANDLE_TYPE(VT_TYPE(element))
        FOR_EACH_VT_VALUE_TYPES
#undef HANDLE_VT_TYPE

        HANDLE_TYPE(VtValue)
        HANDLE_TYPE(Amino::String)
#undef HANDLE_TYPE
    }

    assert(false && "compareValuesAny called for unsupported value type");
    return false; // LCOV_EXCL_LINE
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
template <typename T>
void USDWatchpointLayoutVtValueComposite<T>::buildLayouts() {
    if constexpr (is_pxr_quaternion_type<T>()) {
        m_layouts.reserve(4);
        m_layouts.emplace_back(kQuaternionComponents[0],
                               getValueLayoutT(BaseClass::m_value.GetReal()));
        m_layouts.emplace_back(
            kQuaternionComponents[1],
            getValueLayoutT(BaseClass::m_value.GetImaginary()[0]));
        m_layouts.emplace_back(
            kQuaternionComponents[2],
            getValueLayoutT(BaseClass::m_value.GetImaginary()[1]));
        m_layouts.emplace_back(
            kQuaternionComponents[3],
            getValueLayoutT(BaseClass::m_value.GetImaginary()[2]));
    } else if constexpr (is_pxr_matrix_type<T>()) {
        m_layouts.reserve(T::numColumns * T::numRows);
        for (size_t row = 0; row < T::numRows; ++row) {
            for (size_t col = 0; col < T::numColumns; ++col) {
                m_layouts.emplace_back(
                    kMatrixComponents[row][col],
                    getValueLayoutT(BaseClass::m_value[static_cast<int>(row)]
                                                      [static_cast<int>(col)]));
            }
        }
    } else if constexpr (is_pxr_vector_type<T>()) {
        m_layouts.reserve(T::dimension);
        m_layouts.emplace_back(kVectorComponents[0],
                               getValueLayoutT(BaseClass::m_value[0]));
        m_layouts.emplace_back(kVectorComponents[1],
                               getValueLayoutT(BaseClass::m_value[1]));
        if constexpr (T::dimension > 2) {
            m_layouts.emplace_back(kVectorComponents[2],
                                   getValueLayoutT(BaseClass::m_value[2]));
            if constexpr (T::dimension > 3) {
                m_layouts.emplace_back(kVectorComponents[3],
                                       getValueLayoutT(BaseClass::m_value[3]));
            }
        }
    } else if constexpr (is_pxr_dual_quaternion_type<T>()) {
        m_layouts.reserve(2);
        m_layouts.emplace_back(kDualQuaternionComponents[0],
                               getValueLayoutT(BaseClass::m_value.GetReal()));
        m_layouts.emplace_back(kDualQuaternionComponents[1],
                               getValueLayoutT(BaseClass::m_value.GetDual()));
    } else if constexpr (is_pxr_range_type<T>()) {
        m_layouts.reserve(2);
        m_layouts.emplace_back(kRangeComponents[0],
                               getValueLayoutT(BaseClass::m_value.GetMin()));
        m_layouts.emplace_back(kRangeComponents[1],
                               getValueLayoutT(BaseClass::m_value.GetMax()));
    } else {
        static_assert(!std::is_same_v<T, T>,
                      "buildLayouts is not implemented for this type");
    }
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
template <typename T>
WatchpointLayoutPtr USDWatchpointLayoutVtValueArray<T>::get(
    std::size_t index) const noexcept {
    if (index < BaseClass::m_value.size()) {
        return getValueLayoutT(BaseClass::m_value[index]);
    }
    return {};
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
auto getStagePrims(StagePtr const& stage) {
    auto usdPrimRange = const_cast<UsdStage&>(stage->get()).TraverseAll();
    auto arrayPrims = Amino::newMutablePtr<Amino::Array<PrimPtr>>();
    arrayPrims->reserve(
        std::distance(usdPrimRange.begin(), usdPrimRange.end()));
    for (const auto& usdPrim : usdPrimRange) {
        arrayPrims->push_back(
            Amino::newClassPtr<typename PrimPtr::element_type>(usdPrim, stage));
    }
    return arrayPrims.toImmutable();
}

auto getPrimAttributes(std::vector<UsdAttribute> const& usdAttributes,
                       PrimPtr const&                   prim) {
    auto arrayAttributes = Amino::newMutablePtr<Amino::Array<AttributePtr>>();
    arrayAttributes->reserve(usdAttributes.size());
    for (const auto& usdAttribute : usdAttributes) {
        arrayAttributes->push_back(
            Amino::newClassPtr<typename AttributePtr::element_type>(
                usdAttribute, prim));
    }
    return arrayAttributes.toImmutable();
};

auto getPrimKind(PrimPtr const& prim) {
    auto tfKind = TfToken();
    UsdModelAPI(prim->getPxrPrim()).GetKind(&tfKind);
    return tfKind.GetString();
}

auto getAttrValue(AttributePtr const& attr) {
    VtValue vtValue;
    (*attr)->Get(&vtValue);
    return vtValue;
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
#define HANDLE_T_ELEMENT(T, NAME, FUNC)                               \
    std::make_pair<Amino::String, std::function<std::any(T const&)>>( \
        NAME, [](T const& value) -> std::any { return FUNC; })

#define HANDLE_STAGE_ELEMENT(NAME, FUNC) HANDLE_T_ELEMENT(StagePtr, NAME, FUNC)
const std::
    array<std::pair<Amino::String, std::function<std::any(StagePtr const&)>>, 4>
        kStageElements = {
            HANDLE_STAGE_ELEMENT("last modified prim",
                                 value->last_modified_prim),
            HANDLE_STAGE_ELEMENT("root layer", value->getRootLayer()),
            HANDLE_STAGE_ELEMENT("variant selection",
                                 value->variantSelection().variantInfo()),
            HANDLE_STAGE_ELEMENT("prims", getStagePrims(value))};
#undef HANDLE_STAGE_ELEMENT

#define HANDLE_LAYER_ELEMENT(NAME, FUNC) HANDLE_T_ELEMENT(LayerPtr, NAME, FUNC)
const std::
    array<std::pair<Amino::String, std::function<std::any(LayerPtr const&)>>, 5>
        kLayerElements = {
            HANDLE_LAYER_ELEMENT("display_name", value->get().GetDisplayName()),
            HANDLE_LAYER_ELEMENT("file_path", value->getFilePath()),
            HANDLE_LAYER_ELEMENT("original_file_path",
                                 value->getOriginalFilePath()),
            HANDLE_LAYER_ELEMENT("start_time_code",
                                 value->get().GetStartTimeCode()),
            HANDLE_LAYER_ELEMENT("end_time_code",
                                 value->get().GetEndTimeCode())};
#undef HANDLE_LAYER_ELEMENT

#define HANDLE_PRIM_ELEMENT(NAME, FUNC) HANDLE_T_ELEMENT(PrimPtr, NAME, FUNC)
const std::array<
    std::pair<Amino::String, std::function<std::any(PrimPtr const&)>>,
    21>
    kPrimElements = {
        HANDLE_PRIM_ELEMENT("path", value->getPxrPrim().GetPath().GetString()),
        HANDLE_PRIM_ELEMENT("type",
                            value->getPxrPrim().GetTypeName().GetString()),
        HANDLE_PRIM_ELEMENT("kind", getPrimKind(value)),
        HANDLE_PRIM_ELEMENT("is_active", value->getPxrPrim().IsActive()),
        HANDLE_PRIM_ELEMENT("is_loaded", value->getPxrPrim().IsLoaded()),
        HANDLE_PRIM_ELEMENT("is_model", value->getPxrPrim().IsModel()),
        HANDLE_PRIM_ELEMENT("is_group", value->getPxrPrim().IsGroup()),
        HANDLE_PRIM_ELEMENT("is_abstract", value->getPxrPrim().IsAbstract()),
        HANDLE_PRIM_ELEMENT("is_defined", value->getPxrPrim().IsDefined()),
        HANDLE_PRIM_ELEMENT("has_variant_sets",
                            value->getPxrPrim().HasVariantSets()),
        HANDLE_PRIM_ELEMENT("has_authored_payloads",
                            value->getPxrPrim().HasAuthoredPayloads()),
        HANDLE_PRIM_ELEMENT("has_authored_inherits",
                            value->getPxrPrim().HasAuthoredInherits()),
        HANDLE_PRIM_ELEMENT("has_authored_references",
                            value->getPxrPrim().HasAuthoredReferences()),
        HANDLE_PRIM_ELEMENT("has_authored_specializes",
                            value->getPxrPrim().HasAuthoredSpecializes()),
        HANDLE_PRIM_ELEMENT("has_authored_instanceable",
                            value->getPxrPrim().HasAuthoredInstanceable()),
        HANDLE_PRIM_ELEMENT("is_instance", value->getPxrPrim().IsInstance()),
        HANDLE_PRIM_ELEMENT("is_instance_proxy",
                            value->getPxrPrim().IsInstanceProxy()),
        HANDLE_PRIM_ELEMENT("is_prototype", value->getPxrPrim().IsPrototype()),
        HANDLE_PRIM_ELEMENT("is_in_prototype",
                            value->getPxrPrim().IsInPrototype()),
        HANDLE_PRIM_ELEMENT(
            "attributes",
            getPrimAttributes(value->getPxrPrim().GetAttributes(), value)),
        HANDLE_PRIM_ELEMENT(
            "authored_attributes",
            getPrimAttributes(value->getPxrPrim().GetAuthoredAttributes(),
                              value))};
#undef HANDLE_PRIM_ELEMENT

#define HANDLE_ATTR_ELEMENT(NAME, FUNC) \
    HANDLE_T_ELEMENT(AttributePtr, NAME, FUNC)
const std::array<
    std::pair<Amino::String, std::function<std::any(AttributePtr const&)>>,
    7>
    kAttrElements = {
        HANDLE_ATTR_ELEMENT("name", (*value)->GetName().GetString()),
        HANDLE_ATTR_ELEMENT("type",
                            (*value)->GetTypeName().GetAsToken().GetString()),
        HANDLE_ATTR_ELEMENT("is_defined", (*value)->IsDefined()),
        HANDLE_ATTR_ELEMENT("is_custom", (*value)->IsCustom()),
        HANDLE_ATTR_ELEMENT("is_authored", (*value)->IsAuthored()),
        HANDLE_ATTR_ELEMENT("num_time_samples", (*value)->GetNumTimeSamples()),
        HANDLE_ATTR_ELEMENT("value", getAttrValue(value))};
#undef HANDLE_ATTR_ELEMENT

#undef HANDLE_T_ELEMENT

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
/// \brief Returns whether the USD object held by the given pointer can be
///        safely dereferenced by the element accessors.
///
/// A non-null Amino::Ptr does not imply a usable USD object. The default value
/// of BifrostUsd::Layer and BifrostUsd::Attribute is an invalid one, and nodes
/// such as USD::Layer::get_layer and USD::Prim::get_prim_at_path return invalid
/// objects to report a failure.
template <typename T>
bool isUsdValid(T const& value) {
    static_assert(is_ptr_type<T>(), "expected T to be a BifrostUsd class Ptr");
    if (!value || !*value) {
        return false;
    }
    if constexpr (std::is_same_v<T, PrimPtr>) {
        // BifrostUsd::Prim::operator bool only reports the validity of the
        // stage, not the one of the PXR_NS::UsdPrim, which is left invalid by
        // USD::Prim::get_prim_at_path when the path does not resolve.
        return value->getPxrPrim().IsValid();
    } else {
        return true;
    }
}

/// \brief Returns the value of the given element for the given USD object, or
///        an empty std::any if that object can't be safely dereferenced.
template <typename ELEMENT, typename T>
std::any getElementValue(ELEMENT const& element, T const& value) {
    if (!isUsdValid(value)) {
        return {};
    }
    return element.second(value);
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
template <typename T>
class USDWatchpointLayoutArrayGeneric : public WatchpointLayoutArrayGeneric,
                                        public USDWatchpointLayoutArray {
public:
    USDWatchpointLayoutArrayGeneric(WatchpointLayoutFactory const& factory,
                                    Amino::TypeRegistry const&     typeRegistry,
                                    Amino::ArrayValueView const&   value)
        : WatchpointLayoutArrayGeneric(factory, typeRegistry, value),
          USDWatchpointLayoutArray() {
        assert(value.getType().getTypeId() == Amino::getTypeId<T>());
        auto const* valuePtr = Amino::view_cast<T>(value);
        assert(valuePtr);
        m_value = *valuePtr;
    }
    ~USDWatchpointLayoutArrayGeneric() override = default;

    void deleteThis() noexcept final { delete this; }

    bool isSortable(Sorter::Field field,
                    Amino::String const&) const noexcept final {
        return m_value && !m_value->empty() && field != Field::eValue;
    }
    bool isFilterable(Filter::Field field,
                      Amino::String const&) const noexcept final {
        return m_value && !m_value->empty() && field != Field::eValue;
    }

    bool getIndices(Filters const& filters,
                    Sorter const&  sorter,
                    Indices&       out_indices) const noexcept final {
        return getIndices_impl(m_value ? m_value->size() : 0, filters, sorter,
                               out_indices);
    }

private:
    bool filterByValue(CompareOp,
                       Amino::Any const&,
                       Indices const&,
                       Indices&) const final {
        return false;
    }
    bool filterByElement(Amino::String const& elementName,
                         CompareOp            compareOp,
                         Amino::Any const&    filterValue_,
                         Indices const&       indices,
                         Indices&             out_indices) const final {
        if (!m_value || m_value->empty()) {
            return false;
        }

        auto const& elements = []() {
            if constexpr (std::is_same_v<T, ArrayOfStages>) {
                return kStageElements;
            } else if constexpr (std::is_same_v<T, ArrayOfLayers>) {
                return kLayerElements;
            } else if constexpr (std::is_same_v<T, ArrayOfPrims>) {
                return kPrimElements;
            } else {
                static_assert(std::is_same_v<T, ArrayOfAttributes>,
                              "expected T to be ArrayOfAttributes");
                return kAttrElements;
            }
        }();
        auto elementIt = std::find_if(
            elements.begin(), elements.end(),
            [&](auto const& element) { return element.first == elementName; });
        if (elementIt == elements.end()) {
            return false;
        }

        // Seed the filter value from the first element that can be safely
        // dereferenced (which is not necessarily the first element), so that
        // anyToValue knows which type to convert to.
        std::any filterValue;
        for (std::size_t idx = 0;
             idx < m_value->size() && !filterValue.has_value(); ++idx) {
            filterValue = getElementValue(*elementIt, (*m_value)[idx]);
        }
        if (!anyToValue(filterValue_, filterValue)) {
            return false;
        }

        for (auto idx : indices) {
            auto elementValue = getElementValue(*elementIt, (*m_value)[idx]);
            if (compareValuesAny(elementValue, filterValue, compareOp)) {
                out_indices.push_back(idx);
            }
        }
        return true;
    }

    bool sortByValue(CompareOp, Indices&) const final { return false; }
    bool sortByElement(Amino::String const& elementName,
                       CompareOp            compareOp,
                       Indices&             out_indices) const final {
        if (!m_value || m_value->empty()) {
            return false;
        }

        auto const& elements = []() {
            if constexpr (std::is_same_v<T, ArrayOfStages>) {
                return kStageElements;
            } else if constexpr (std::is_same_v<T, ArrayOfLayers>) {
                return kLayerElements;
            } else if constexpr (std::is_same_v<T, ArrayOfPrims>) {
                return kPrimElements;
            } else {
                static_assert(std::is_same_v<T, ArrayOfAttributes>,
                              "expected T to be ArrayOfAttributes");
                return kAttrElements;
            }
        }();
        auto elementIt = std::find_if(
            elements.begin(), elements.end(),
            [&](auto const& element) { return element.first == elementName; });
        if (elementIt == elements.end()) {
            return false;
        }

        std::sort(out_indices.begin(), out_indices.end(),
                  [&](std::size_t a, std::size_t b) {
                      auto valueA = getElementValue(*elementIt, (*m_value)[a]);
                      auto valueB = getElementValue(*elementIt, (*m_value)[b]);
                      return compareValuesAny(valueA, valueB, compareOp);
                  });
        return true;
    }

private:
    T m_value;
};

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
template <typename T>
class USDWatchpointLayoutComposite : public WatchpointLayoutComposite {
public:
    USDWatchpointLayoutComposite(WatchpointLayoutFactory const& factory,
                                 Amino::TypeRegistry const&     typeRegistry,
                                 Amino::ClassValueView const&   value)
        : WatchpointLayoutComposite(value) {
        try {
            assert(value.getType().getTypeId() == Amino::getTypeId<T>());
            auto const* valuePtr = Amino::view_cast<T>(value);
            assert(valuePtr);
            buildLayouts(factory, typeRegistry, *valuePtr);
        } catch (...) {
        }
    }

    ~USDWatchpointLayoutComposite() override = default;

    void deleteThis() noexcept final { delete this; }

    std::size_t size() const noexcept final { return m_layouts.size(); }

    WatchpointLayoutPtr get(Amino::String const& name) const noexcept final {
        auto it = std::find_if(m_layouts.begin(), m_layouts.end(),
                               [&](SubLayout const& subLayout) {
                                   return subLayout.name() == name;
                               });
        if (it != m_layouts.end()) {
            return it->layout();
        }
        return {};
    }

    SubLayout get(std::size_t index) const noexcept final {
        if (index < m_layouts.size()) {
            return m_layouts.at(index);
        }
        return {};
    }

private:
    void buildLayouts(WatchpointLayoutFactory const& factory,
                      Amino::TypeRegistry const&     typeRegistry,
                      T const&                       value) {
        if (isUsdValid(value)) {
            auto const& elements = []() {
                if constexpr (std::is_same_v<T, StagePtr>) {
                    return kStageElements;
                } else if constexpr (std::is_same_v<T, LayerPtr>) {
                    return kLayerElements;
                } else if constexpr (std::is_same_v<T, PrimPtr>) {
                    return kPrimElements;
                } else {
                    static_assert(std::is_same_v<T, AttributePtr>,
                                  "expected T to be AttributePtr");
                    return kAttrElements;
                }
            }();
            m_layouts.reserve(elements.size());
            for (auto const& element : elements) {
                auto layout = getValueLayoutFromAny(factory, typeRegistry,
                                                    element.second(value));
                if (layout) {
                    m_layouts.emplace_back(element.first, layout);
                }
            }
        }
    }

private:
    std::vector<SubLayout> m_layouts;
};

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
template <typename VIEW_TYPE, typename T>
class USDWatcher : public Watcher {
public:
    USDWatcher(WatchpointLayoutFactory const& factory,
               Amino::TypeRegistry const&     typeRegistry,
               VIEW_TYPE const&               value) noexcept
        : Watcher(value) {
        assert(value.getType().getTypeId() == Amino::getTypeId<T>());
        try {
            if constexpr (std::is_same_v<VIEW_TYPE, Amino::ClassValueView>) {
                m_layout =
                    WatchpointLayout::createT<USDWatchpointLayoutComposite<T>>(
                        factory, typeRegistry, value);
            } else if constexpr (std::is_same_v<VIEW_TYPE,
                                                Amino::ArrayValueView>) {
                m_layout = WatchpointLayout::createT<
                    USDWatchpointLayoutArrayGeneric<T>>(factory, typeRegistry,
                                                        value);
            }
        } catch (...) {
        }
    }
    ~USDWatcher() noexcept override = default;

    void deleteThis() noexcept final { delete this; }

    WatchpointLayoutPtr getLayout() const noexcept final { return m_layout; }

private:
    WatchpointLayoutPtr m_layout;
};
} // namespace

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
class USDWatchpoint : public BifrostGraph::Executor::Watchpoint {
public:
    explicit USDWatchpoint(WatchpointLayoutFactory const& factory) noexcept;
    ~USDWatchpoint() noexcept override;

    void deleteThis() noexcept override;

    void getSupportedTypeIds(TypeIdArray& out_typeIds) const noexcept final;

    Watcher* createWatcher(Amino::TypeRegistry const& typeRegistry,
                           Amino::ValueView const& value) const noexcept final;

    WatchpointLayoutPtr createLayout(
        Amino::TypeRegistry const& typeRegistry,
        Amino::ValueView const&    value) const noexcept final;

private:
    WatchpointLayoutFactory const& m_factory;
};

USDWatchpoint::USDWatchpoint(WatchpointLayoutFactory const& factory) noexcept
    : BifrostGraph::Executor::Watchpoint("USD Watchpoint"),
      m_factory(factory) {}
USDWatchpoint::~USDWatchpoint() noexcept = default;

void USDWatchpoint::deleteThis() noexcept { delete this; }

void USDWatchpoint::getSupportedTypeIds(
    TypeIdArray& out_typeIds) const noexcept {
    out_typeIds.push_back(Amino::getTypeId<AttributePtr>());
    out_typeIds.push_back(Amino::getTypeId<ArrayOfAttributes>());
    out_typeIds.push_back(Amino::getTypeId<LayerPtr>());
    out_typeIds.push_back(Amino::getTypeId<ArrayOfLayers>());
    out_typeIds.push_back(Amino::getTypeId<StagePtr>());
    out_typeIds.push_back(Amino::getTypeId<ArrayOfStages>());
    out_typeIds.push_back(Amino::getTypeId<PrimPtr>());
    out_typeIds.push_back(Amino::getTypeId<ArrayOfPrims>());
}

Watcher* USDWatchpoint::createWatcher(
    Amino::TypeRegistry const& typeRegistry,
    Amino::ValueView const&    value) const noexcept {
    try {
        auto typeId = value.getType().getTypeId();
        return Amino::visit(
            [&](auto const& v) -> Watcher* {
                using VIEW_TYPE = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<VIEW_TYPE,
                                             Amino::ClassValueView>) {
                    if (typeId == Amino::getTypeId<AttributePtr>()) {
                        return new USDWatcher<VIEW_TYPE, AttributePtr>(
                            m_factory, typeRegistry, v);
                    } else if (typeId == Amino::getTypeId<LayerPtr>()) {
                        return new USDWatcher<VIEW_TYPE, LayerPtr>(
                            m_factory, typeRegistry, v);
                    } else if (typeId == Amino::getTypeId<StagePtr>()) {
                        return new USDWatcher<VIEW_TYPE, StagePtr>(
                            m_factory, typeRegistry, v);
                    } else if (typeId == Amino::getTypeId<PrimPtr>()) {
                        return new USDWatcher<VIEW_TYPE, PrimPtr>(
                            m_factory, typeRegistry, v);
                    }
                } else if constexpr (std::is_same_v<VIEW_TYPE,
                                                    Amino::ArrayValueView>) {
                    if (typeId == Amino::getTypeId<ArrayOfAttributes>()) {
                        return new USDWatcher<VIEW_TYPE, ArrayOfAttributes>(
                            m_factory, typeRegistry, v);
                    } else if (typeId == Amino::getTypeId<ArrayOfLayers>()) {
                        return new USDWatcher<VIEW_TYPE, ArrayOfLayers>(
                            m_factory, typeRegistry, v);
                    } else if (typeId == Amino::getTypeId<ArrayOfStages>()) {
                        return new USDWatcher<VIEW_TYPE, ArrayOfStages>(
                            m_factory, typeRegistry, v);
                    } else if (typeId == Amino::getTypeId<ArrayOfPrims>()) {
                        return new USDWatcher<VIEW_TYPE, ArrayOfPrims>(
                            m_factory, typeRegistry, v);
                    }
                }
                return nullptr; // LCOV_EXCL_LINE
            },
            value);
    } catch (...) {
        return nullptr; // LCOV_EXCL_LINE
    }
}

WatchpointLayoutPtr USDWatchpoint::createLayout(
    Amino::TypeRegistry const& typeRegistry,
    Amino::ValueView const&    value) const noexcept {
    try {
        auto typeId = value.getType().getTypeId();
        return Amino::visit(
            [&](auto const& v) -> WatchpointLayoutPtr {
                using VIEW_TYPE = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<VIEW_TYPE,
                                             Amino::ClassValueView>) {
                    if (typeId == Amino::getTypeId<AttributePtr>()) {
                        return WatchpointLayout::createT<
                            USDWatchpointLayoutComposite<AttributePtr>>(
                            m_factory, typeRegistry, v);
                    } else if (typeId == Amino::getTypeId<LayerPtr>()) {
                        return WatchpointLayout::createT<
                            USDWatchpointLayoutComposite<LayerPtr>>(
                            m_factory, typeRegistry, v);
                    } else if (typeId == Amino::getTypeId<StagePtr>()) {
                        return WatchpointLayout::createT<
                            USDWatchpointLayoutComposite<StagePtr>>(
                            m_factory, typeRegistry, v);
                    } else if (typeId == Amino::getTypeId<PrimPtr>()) {
                        return WatchpointLayout::createT<
                            USDWatchpointLayoutComposite<PrimPtr>>(
                            m_factory, typeRegistry, v);
                    }
                } else if constexpr (std::is_same_v<VIEW_TYPE,
                                                    Amino::ArrayValueView>) {
                    if (typeId == Amino::getTypeId<ArrayOfAttributes>()) {
                        return WatchpointLayout::createT<
                            USDWatchpointLayoutArrayGeneric<ArrayOfAttributes>>(
                            m_factory, typeRegistry, v);
                    } else if (typeId == Amino::getTypeId<ArrayOfLayers>()) {
                        return WatchpointLayout::createT<
                            USDWatchpointLayoutArrayGeneric<ArrayOfLayers>>(
                            m_factory, typeRegistry, v);
                    } else if (typeId == Amino::getTypeId<ArrayOfStages>()) {
                        return WatchpointLayout::createT<
                            USDWatchpointLayoutArrayGeneric<ArrayOfStages>>(
                            m_factory, typeRegistry, v);
                    } else if (typeId == Amino::getTypeId<ArrayOfPrims>()) {
                        return WatchpointLayout::createT<
                            USDWatchpointLayoutArrayGeneric<ArrayOfPrims>>(
                            m_factory, typeRegistry, v);
                    }
                }
                return {}; // LCOV_EXCL_LINE
            },
            value);
    } catch (...) {
        return {}; // LCOV_EXCL_LINE
    }
}

extern "C" {
USD_WATCHPOINT_EXPORT BifrostGraph::Executor::Watchpoint*
                      createBifrostWatchpoint(WatchpointLayoutFactory const&);

USD_WATCHPOINT_EXPORT BifrostGraph::Executor::Watchpoint*
createBifrostWatchpoint(WatchpointLayoutFactory const& factory) {
    return new USDWatchpoint(factory);
}
}
