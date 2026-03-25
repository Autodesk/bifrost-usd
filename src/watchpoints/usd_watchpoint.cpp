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

#include <BifrostUsd/Attribute.h>
#include <BifrostUsd/Layer.h>
#include <BifrostUsd/Prim.h>
#include <BifrostUsd/Stage.h>

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
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/primTypeInfo.h>
// matrix float not included in <pxr/usd/sdf/types.h>
#include <pxr/base/gf/matrix2f.h>
#include <pxr/base/gf/matrix3f.h>
#include <pxr/base/gf/matrix4f.h>

BIFUSD_WARNING_POP

using Watcher               = BifrostGraph::Executor::Watchpoint::Watcher;
using WatchpointLayout      = BifrostGraph::Executor::WatchpointLayout;
using WatchpointLayoutArray = BifrostGraph::Executor::WatchpointLayoutArray;
using WatchpointLayoutComposite =
    BifrostGraph::Executor::WatchpointLayoutComposite;
using WatchpointLayoutFactory = BifrostGraph::Executor::WatchpointLayoutFactory;
using WatchpointLayoutPath    = BifrostGraph::Executor::WatchpointLayoutPath;
using WatchpointLayoutPtr     = BifrostGraph::Executor::WatchpointLayoutPtr;

using Field   = BifrostGraph::Executor::Watchpoint::Field;
using Sorter  = BifrostGraph::Executor::Watchpoint::Sorter;
using Filters = BifrostGraph::Executor::Watchpoint::Filters;
using Filter  = BifrostGraph::Executor::Watchpoint::Filter;
using Indices = BifrostGraph::Executor::Watchpoint::Indices;

using AttributePtr      = Amino::Ptr<BifrostUsd::Attribute>;
using ArrayOfAttributes = Amino::Ptr<Amino::Array<AttributePtr>>;
using LayerPtr          = Amino::Ptr<BifrostUsd::Layer>;
using ArrayOfLayers     = Amino::Ptr<Amino::Array<LayerPtr>>;
using StagePtr          = Amino::Ptr<BifrostUsd::Stage>;
using ArrayOfStages     = Amino::Ptr<Amino::Array<StagePtr>>;
using PrimPtr           = Amino::Ptr<BifrostUsd::Prim>;
using ArrayOfPrims      = Amino::Ptr<Amino::Array<PrimPtr>>;

namespace {
using namespace Amino::StringViewLiterals;

///-------------------------------------------------------------------------
/// \brief Constants for the different watchpoint layout fields
/// \{
// Stage watchpoint
Amino::String const kLastModifiedPrim = "last modified prim";
Amino::String const kRootLayer        = "root layer";
Amino::String const kVariantSelection = "variant selection";
Amino::String const kPrims            = "prims";

// Attribute watchpoint
Amino::String const kName               = "name";
Amino::String const kTypeName           = "type";
Amino::String const kIsDefined          = "is_defined";
Amino::String const kIsCustom           = "is_custom";
Amino::String const kIsAuthored         = "is_authored";
Amino::String const kValue              = "value";
Amino::String const kNumTimeSamples     = "num_time_samples";

// Prim watchpoint
Amino::String const kPrimPath                = "path";
Amino::String const kKind                    = "kind";
Amino::String const kPrimAttributes          = "attributes";
Amino::String const kPrimAuthoredAttributes  = "authored_attributes";
Amino::String const kIsActive                = "is_active";
Amino::String const kIsLoaded                = "is_loaded";
Amino::String const kIsModel                 = "is_model";
Amino::String const kIsGroup                 = "is_group";
Amino::String const kIsAbstract              = "is_abstract";
Amino::String const kHasVariantSets          = "has_variant_sets";
Amino::String const kHasAuthoredPayloads     = "has_authored_payloads";
Amino::String const kHasAuthoredInherits     = "has_authored_inherits";
Amino::String const kHasAuthoredReferences   = "has_authored_references";
Amino::String const kHasAuthoredSpecializes  = "has_authored_specializes";
Amino::String const kHasAuthoredInstanceable = "has_authored_instanceable";
Amino::String const kIsInstance              = "is_instance";
Amino::String const kIsInstanceProxy         = "is_instance_proxy";
Amino::String const kIsPrototype             = "is_prototype";
Amino::String const kIsInPrototype           = "is_in_prototype";

// Layer watchpoint
Amino::String const kLayerDisplayName         = "display_name";
Amino::String const kLayerFilePath            = "file_path";
Amino::String const kLayerOriginalFilePath    = "original_file_path";
Amino::String const kStartTimeCode            = "start_time_code";
Amino::String const kEndTimeCode              = "end_time_code";

// Usd built-in struct types fields
Amino::String const kVectorFields[4]     = {"s0", "s1", "s2", "s3"};
Amino::String const kMatrixFields[4][4]  = {{"m00", "m01", "m02", "m03"},
                                            {"m10", "m11", "m12", "m13"},
                                            {"m20", "m21", "m22", "m23"},
                                            {"m30", "m31", "m32", "m33"}};
Amino::String const kQuaternionFields[4] = {"r", "i", "j", "k"};
/// \}

template <typename T>
constexpr bool is_builtin_type() {
    return std::is_same_v<T, Amino::char_t> ||
           std::is_same_v<T, Amino::uchar_t> ||
           std::is_same_v<T, Amino::int_t> ||
           std::is_same_v<T, Amino::uint_t> ||
           std::is_same_v<T, Amino::short_t> ||
           std::is_same_v<T, Amino::ushort_t> ||
           std::is_same_v<T, Amino::long_t> ||
           std::is_same_v<T, Amino::ulong_t> ||
           std::is_same_v<T, Amino::float_t> ||
           std::is_same_v<T, Amino::double_t> ||
           std::is_same_v<T, Amino::bool_t> || std::is_same_v<T, std::string>;
}

template <typename T>
constexpr bool is_pxr_vector_type() {
    return std::is_same_v<T, PXR_NS::GfVec2d> ||
           std::is_same_v<T, PXR_NS::GfVec2f> ||
           std::is_same_v<T, PXR_NS::GfVec2h> ||
           std::is_same_v<T, PXR_NS::GfVec2i> ||
           std::is_same_v<T, PXR_NS::GfVec3d> ||
           std::is_same_v<T, PXR_NS::GfVec3f> ||
           std::is_same_v<T, PXR_NS::GfVec3h> ||
           std::is_same_v<T, PXR_NS::GfVec3i> ||
           std::is_same_v<T, PXR_NS::GfVec4d> ||
           std::is_same_v<T, PXR_NS::GfVec4f> ||
           std::is_same_v<T, PXR_NS::GfVec4h> ||
           std::is_same_v<T, PXR_NS::GfVec4i>;
}

template <typename T>
constexpr bool is_pxr_matrix_type() {
    return std::is_same_v<T, PXR_NS::GfMatrix2d> ||
           std::is_same_v<T, PXR_NS::GfMatrix2f> ||
           std::is_same_v<T, PXR_NS::GfMatrix3d> ||
           std::is_same_v<T, PXR_NS::GfMatrix3f> ||
           std::is_same_v<T, PXR_NS::GfMatrix4d> ||
           std::is_same_v<T, PXR_NS::GfMatrix4f>;
}

template <typename T>
constexpr bool is_pxr_quaternion_type() {
    return std::is_same_v<T, PXR_NS::GfQuatd> ||
           std::is_same_v<T, PXR_NS::GfQuatf> ||
           std::is_same_v<T, PXR_NS::GfQuath>;
}

template <typename T>
constexpr bool is_ptr_type() {
    return std::is_same_v<T, AttributePtr> || std::is_same_v<T, LayerPtr> ||
           std::is_same_v<T, StagePtr> || std::is_same_v<T, PrimPtr>;
}

#define FOR_EACH_SUPPORTED_TYPES(MACRO) \
    MACRO(Amino::char_t)                \
    MACRO(Amino::uchar_t)               \
    MACRO(Amino::int_t)                 \
    MACRO(Amino::uint_t)                \
    MACRO(Amino::short_t)               \
    MACRO(Amino::ushort_t)              \
    MACRO(Amino::long_t)                \
    MACRO(Amino::ulong_t)               \
    MACRO(Amino::float_t)               \
    MACRO(Amino::double_t)              \
    MACRO(Amino::bool_t)                \
    MACRO(std::string)                  \
    MACRO(PXR_NS::GfHalf)               \
    MACRO(PXR_NS::GfVec2d)              \
    MACRO(PXR_NS::GfVec2f)              \
    MACRO(PXR_NS::GfVec2h)              \
    MACRO(PXR_NS::GfVec2i)              \
    MACRO(PXR_NS::GfVec3d)              \
    MACRO(PXR_NS::GfVec3f)              \
    MACRO(PXR_NS::GfVec3h)              \
    MACRO(PXR_NS::GfVec3i)              \
    MACRO(PXR_NS::GfVec4d)              \
    MACRO(PXR_NS::GfVec4f)              \
    MACRO(PXR_NS::GfVec4h)              \
    MACRO(PXR_NS::GfVec4i)              \
    MACRO(PXR_NS::GfMatrix2d)           \
    MACRO(PXR_NS::GfMatrix2f)           \
    MACRO(PXR_NS::GfMatrix3d)           \
    MACRO(PXR_NS::GfMatrix3f)           \
    MACRO(PXR_NS::GfMatrix4d)           \
    MACRO(PXR_NS::GfMatrix4f)           \
    MACRO(PXR_NS::GfQuatd)              \
    MACRO(PXR_NS::GfQuatf)              \
    MACRO(PXR_NS::GfQuath)              \
    MACRO(PXR_NS::TfToken)              \
    MACRO(PXR_NS::SdfAssetPath)

inline PXR_NS::TfToken getPrimKind(PrimPtr const& primPtr) {
    auto modelAPI = PXR_NS::UsdModelAPI((*primPtr).getPxrPrim());
    auto tfKind   = PXR_NS::TfToken();
    modelAPI.GetKind(&tfKind);
    return tfKind;
}

///-------------------------------------------------------------------------
/// \brief Helpers get the type names for the different supported types
/// \{
Amino::String getShortTypeName(Amino::String typeName) {
    auto idx = typeName.find_last_of(':');
    if (idx != Amino::String::npos) {
        typeName.erase(0, idx + 1);
    }
    idx = typeName.find_first_of('_');
    if (idx != Amino::String::npos) {
        typeName.erase(idx);
    }
    return typeName;
}

template <typename T>
Amino::String getTypeName() {
#define HANDLE_TYPE(TYPE)                                             \
    if constexpr (std::is_same_v<T, TYPE>) {                          \
        return getShortTypeName(#TYPE);                               \
    }                                                                 \
    if constexpr (std::is_same_v<T, PXR_NS::VtArray<TYPE>>) {         \
        return Amino::String{"VtArray<"} + getTypeName<TYPE>() + ">"; \
    }

    FOR_EACH_SUPPORTED_TYPES(HANDLE_TYPE)

#undef HANDLE_TYPE
}
/// \}

///-------------------------------------------------------------------------
/// \brief Helpers to create the layout for the different supported types
/// \{
template <typename T>
WatchpointLayoutPtr getLayoutFromType(WatchpointLayoutFactory const& factory) {
    if constexpr (is_builtin_type<T>()) {
        if constexpr (std::is_same_v<T, std::string>) {
            return factory.get(Amino::getTypeId<Amino::String>());
        } else {
            return factory.get(Amino::getTypeId<T>());
        }
    } else {
        if (factory.exists(Amino::getTypeId<T>())) {
            return factory.get(Amino::getTypeId<T>());
        }

        WatchpointLayoutPtr layout;

        if constexpr (std::is_same_v<T, PXR_NS::GfHalf> ||
                      std::is_same_v<T, PXR_NS::TfToken> ||
                      std::is_same_v<T, PXR_NS::SdfAssetPath>) {
            layout = WatchpointLayout::create(factory, Amino::getTypeId<T>());
        }

        if constexpr (is_pxr_vector_type<T>()) {
            layout          = WatchpointLayoutComposite::create(factory,
                                                                Amino::getTypeId<T>());
            auto& composite = layout.getAs<WatchpointLayoutComposite>();
            auto  componentLayout =
                getLayoutFromType<typename T::ScalarType>(factory);
            assert(componentLayout);
            composite.add(kVectorFields[0], componentLayout);
            composite.add(kVectorFields[1], componentLayout);
            if constexpr (T::dimension > 2) {
                composite.add(kVectorFields[2], componentLayout);
                if constexpr (T::dimension > 3) {
                    composite.add(kVectorFields[3], componentLayout);
                }
            }
        }

        if constexpr (is_pxr_matrix_type<T>()) {
            layout = WatchpointLayoutComposite::create(factory,
                                                       Amino::getTypeId<T>());
            layout->setTypeKind("matrix");
            auto& composite = layout.getAs<WatchpointLayoutComposite>();
            auto  componentLayout =
                getLayoutFromType<typename T::ScalarType>(factory);
            assert(componentLayout);
            for (size_t row = 0; row < T::numRows; ++row) {
                for (size_t col = 0; col < T::numColumns; ++col) {
                    composite.add(kMatrixFields[row][col], componentLayout);
                }
            }
        }

        if constexpr (is_pxr_quaternion_type<T>()) {
            layout          = WatchpointLayoutComposite::create(factory,
                                                                Amino::getTypeId<T>());
            auto& composite = layout.getAs<WatchpointLayoutComposite>();
            auto  componentLayout =
                getLayoutFromType<typename T::ScalarType>(factory);
            assert(componentLayout);
            composite.add(kQuaternionFields[0], componentLayout);
            composite.add(kQuaternionFields[1], componentLayout);
            composite.add(kQuaternionFields[2], componentLayout);
            composite.add(kQuaternionFields[3], componentLayout);
        }

        assert(layout);
        layout->setTypeName(getTypeName<T>());
        const_cast<WatchpointLayoutFactory&>(factory).add(layout);
        return layout;
    }
}
/// \}

///-------------------------------------------------------------------------
/// \brief Helpers to create the layout for the different supported value types
/// \{
template <typename T>
WatchpointLayoutPtr createLayoutFromValue(
    WatchpointLayoutFactory const& factory, T const& value) {
    return factory.get(Amino::Any{value});
}

WatchpointLayoutPtr createLayoutFromVtValue(
    WatchpointLayoutFactory const& factory, PXR_NS::VtValue value) {
#define HANDLE_TYPE(TYPE)                                                      \
    if (value.IsHolding<TYPE>()) {                                             \
        return getLayoutFromType<TYPE>(factory);                               \
    }                                                                          \
    if (value.IsHolding<PXR_NS::VtArray<TYPE>>()) {                            \
        auto valuePtr = Amino::newClassPtr<PXR_NS::VtValue>(std::move(value)); \
        auto layout =                                                          \
            WatchpointLayoutArray::create(factory, Amino::Any{valuePtr});      \
        assert(layout);                                                        \
        layout->setTypeName(getTypeName<PXR_NS::VtArray<TYPE>>());             \
        layout.getAs<WatchpointLayoutArray>().setElementTypeName(              \
            getTypeName<TYPE>());                                              \
        if constexpr (is_pxr_matrix_type<TYPE>()) {                            \
            layout.getAs<WatchpointLayoutArray>().setElementTypeKind(          \
                "matrix");                                                     \
        }                                                                      \
        return layout;                                                         \
    }

    FOR_EACH_SUPPORTED_TYPES(HANDLE_TYPE)

#undef HANDLE_TYPE

    assert(false && "createLayoutFromVtValue called for unsupported type");
    return {};
}

template <typename PTR_TYPE, typename PARENT_TYPE, typename USD_ARRAY_TYPE>
WatchpointLayoutPtr createArrayLayout(WatchpointLayoutFactory const& factory,
                                      PARENT_TYPE const&             parent,
                                      USD_ARRAY_TYPE const&          values) {
    auto arrayPtr = Amino::newMutablePtr<Amino::Array<PTR_TYPE>>(
        std::distance(values.begin(), values.end()));
    unsigned index = 0;
    for (auto const& element : values) {
        arrayPtr->at(index++) =
            Amino::newClassPtr<typename PTR_TYPE::element_type>(element,
                                                                parent);
    }
    return createLayoutFromValue(factory, arrayPtr.toImmutable());
}

template <>
WatchpointLayoutPtr createLayoutFromValue<>(
    WatchpointLayoutFactory const& factory, AttributePtr const& attributePtr) {
    auto layout = WatchpointLayoutComposite::create(
        factory, Amino::getTypeId<AttributePtr>());
    if (attributePtr) {
        auto const& attribute = *attributePtr;
        auto&       composite = layout.getAs<WatchpointLayoutComposite>();

        composite.add(kName, factory.get(Amino::getTypeId<Amino::String>()));
        composite.add(kTypeName,
                      factory.get(Amino::getTypeId<Amino::String>()));
        composite.add(kIsDefined,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kIsCustom,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kIsAuthored,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kNumTimeSamples,
                      factory.get(Amino::getTypeId<Amino::ulong_t>()));

        PXR_NS::VtValue vtVal;
        if (attribute->Get(&vtVal)) {
            composite.add(kValue,
                          createLayoutFromVtValue(factory, std::move(vtVal)));
        }
    }
    return layout;
}

template <>
WatchpointLayoutPtr createLayoutFromValue<>(
    WatchpointLayoutFactory const& factory, PrimPtr const& primPtr) {
    auto layout =
        WatchpointLayoutComposite::create(factory, Amino::getTypeId<PrimPtr>());
    if (primPtr) {
        auto const& prim      = *primPtr;
        auto&       composite = layout.getAs<WatchpointLayoutComposite>();

        composite.add(kPrimPath,
                      factory.get(Amino::getTypeId<Amino::String>()));
        composite.add(kTypeName,
                      factory.get(Amino::getTypeId<Amino::String>()));
        composite.add(kKind, factory.get(Amino::getTypeId<Amino::String>()));
        composite.add(kIsActive,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kIsLoaded,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kIsModel, factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kIsGroup, factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kIsAbstract,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kIsDefined,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kHasVariantSets,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kHasAuthoredPayloads,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kHasAuthoredInherits,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kHasAuthoredReferences,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kHasAuthoredSpecializes,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kHasAuthoredInstanceable,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kIsInstance,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kIsInstanceProxy,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kIsPrototype,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kIsInPrototype,
                      factory.get(Amino::getTypeId<Amino::bool_t>()));
        composite.add(kPrimAttributes,
                      createArrayLayout<AttributePtr>(factory, primPtr,
                                                      prim->GetAttributes()));
        composite.add(kPrimAuthoredAttributes,
                      createArrayLayout<AttributePtr>(
                          factory, primPtr, prim->GetAuthoredAttributes()));
    }
    return layout;
}

template <>
WatchpointLayoutPtr createLayoutFromValue<>(
    WatchpointLayoutFactory const& factory, LayerPtr const& layerPtr) {
    auto layout = WatchpointLayoutComposite::create(
        factory, Amino::getTypeId<LayerPtr>());
    if (layerPtr) {
        auto& composite = layout.getAs<WatchpointLayoutComposite>();

        composite.add(kLayerDisplayName,
                      factory.get(Amino::getTypeId<Amino::String>()));
        composite.add(kLayerFilePath,
                      factory.get(Amino::getTypeId<Amino::String>()));
        composite.add(kLayerOriginalFilePath,
                      factory.get(Amino::getTypeId<Amino::String>()));
        composite.add(kStartTimeCode,
                      factory.get(Amino::getTypeId<Amino::double_t>()));
        composite.add(kEndTimeCode,
                      factory.get(Amino::getTypeId<Amino::double_t>()));
        // sublayers ?
    }
    return layout;
}

template <>
WatchpointLayoutPtr createLayoutFromValue<>(
    WatchpointLayoutFactory const& factory, StagePtr const& stagePtr) {
    auto layout = WatchpointLayoutComposite::create(
        factory, Amino::getTypeId<StagePtr>());
    if (stagePtr) {
        auto const& stage     = *stagePtr;
        auto&       composite = layout.getAs<WatchpointLayoutComposite>();

        composite.add(kLastModifiedPrim,
                      factory.get(Amino::getTypeId<Amino::String>()));
        composite.add(kRootLayer,
                      createLayoutFromValue(factory, stage.getRootLayer()));
        composite.add(kVariantSelection,
                      factory.get(Amino::getTypeId<Amino::String>()));
        composite.add(
            kPrims,
            createArrayLayout<PrimPtr>(
                factory, stagePtr,
                const_cast<PXR_NS::UsdStage&>(stage.get()).TraverseAll()));
    }
    return layout;
}
/// \}

///-------------------------------------------------------------------------
/// Helpers to compare values of different supported types
/// \{
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
    } else if constexpr (std::is_same_v<T, PXR_NS::GfHalf>) {
        return is_equal(static_cast<float>(v1), static_cast<float>(v2));
    } else if constexpr (std::is_same_v<T, double> ||
                         std::is_same_v<T, float>) {
        return std::isless(std::fabs(v1 - v2), std::numeric_limits<T>::min());
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
    } else if constexpr (std::is_same_v<T, std::string> ||
                         std::is_same_v<T, Amino::String> ||
                         std::is_same_v<T, PXR_NS::TfToken> ||
                         std::is_same_v<T, PXR_NS::SdfAssetPath>) {
        return v1 < v2;
    } else if constexpr (std::is_same_v<T, PXR_NS::GfHalf>) {
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
    } else if constexpr (std::is_same_v<T, std::string> ||
                         std::is_same_v<T, Amino::String> ||
                         std::is_same_v<T, PXR_NS::TfToken> ||
                         std::is_same_v<T, PXR_NS::SdfAssetPath>) {
        return v1 > v2;
    } else if constexpr (std::is_same_v<T, PXR_NS::GfHalf>) {
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
        return is_infinite(v.GetReal()) && is_infinite(v.GetImaginary());
    } else if constexpr (std::is_same_v<T, double> ||
                         std::is_same_v<T, float> ||
                         std::is_same_v<T, PXR_NS::GfHalf>) {
        return std::isinf(v);
    } else if constexpr (std::is_same_v<T, std::string> ||
                         std::is_same_v<T, Amino::String> ||
                         std::is_same_v<T, PXR_NS::TfToken> ||
                         std::is_same_v<T, PXR_NS::SdfAssetPath>) {
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
        return is_not_a_number(v.GetReal()) &&
               is_not_a_number(v.GetImaginary());
    } else if constexpr (std::is_same_v<T, double> ||
                         std::is_same_v<T, float> ||
                         std::is_same_v<T, PXR_NS::GfHalf>) {
        return std::isnan(v);
    } else if constexpr (std::is_same_v<T, std::string> ||
                         std::is_same_v<T, Amino::String> ||
                         std::is_same_v<T, PXR_NS::TfToken> ||
                         std::is_same_v<T, PXR_NS::SdfAssetPath>) {
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
std::function<bool(T const&, T const&)> getCompareFunc(Filter::Operation op) {
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

#if defined(_WIN32)
    _assume(false);
#else
    __builtin_unreachable();
#endif
}
/// \}

///-------------------------------------------------------------------------
/// \brief Helpers to sort and filter array values
/// \{
template <typename T>
T getValueFromAny(Amino::Any const& valueAny) {
    if constexpr (std::is_same_v<T, std::string>) {
        assert(valueAny.type() == Amino::getTypeId<Amino::String>());
        return std::string{Amino::any_cast<Amino::String>(valueAny).c_str()};
    } else if constexpr (std::is_same_v<T, PXR_NS::SdfAssetPath> ||
                         std::is_same_v<T, PXR_NS::SdfAssetPath>) {
        assert(valueAny.type() == Amino::getTypeId<Amino::String>());
        return T{std::string{Amino::any_cast<Amino::String>(valueAny).c_str()}};
    } else {
        assert(valueAny.type() == Amino::getTypeId<T>());
        return Amino::any_cast<T>(valueAny);
    }
}

/// \brief Dispatch to call doFn with the getter and compare functions of proper type.
template <typename T, typename GET_ELT_FN, typename DO_FN>
void doForElement(GET_ELT_FN&&      getEltFn,
                  Filter::Operation compareOp,
                  DO_FN&&           doFn) {
    using ELT_T =
        typename std::invoke_result<decltype(getEltFn), T const&>::type;
    auto compareFn = getCompareFunc<ELT_T>(compareOp);
    doFn(getEltFn, compareFn);
}

/// \brief Helper to map the element name with the proper getter for the given type T.
template <typename T, typename DO_FN>
bool doForElement(Amino::String const& elementName,
                  Filter::Operation    compareOp,
                  DO_FN&&              doFn) {
#define HANDLE_ELEMENT(NAME, GETTER)              \
    if (elementName == NAME) {                    \
        doForElement<T>(GETTER, compareOp, doFn); \
        return true;                              \
    }

    if constexpr (std::is_same_v<T, AttributePtr>) {
        HANDLE_ELEMENT(kName, [](T const& attrPtr) {
            return (*attrPtr)->GetName().GetString();
        })
        HANDLE_ELEMENT(kTypeName, [](T const& attrPtr) {
            return (*attrPtr)->GetTypeName().GetAsToken().GetString();
        })
        HANDLE_ELEMENT(kIsDefined,
                       [](T const& attrPtr) { return (*attrPtr)->IsDefined(); })
        HANDLE_ELEMENT(kIsCustom,
                       [](T const& attrPtr) { return (*attrPtr)->IsCustom(); })
        HANDLE_ELEMENT(kIsAuthored, [](T const& attrPtr) {
            return (*attrPtr)->IsAuthored();
        })
        HANDLE_ELEMENT(kNumTimeSamples, [](T const& attrPtr) {
            return (*attrPtr)->GetNumTimeSamples();
        })
    }

    if constexpr (std::is_same_v<T, LayerPtr>) {
        HANDLE_ELEMENT(kLayerDisplayName, [](T const& layerPtr) {
            return (*layerPtr)->GetDisplayName();
        })
        HANDLE_ELEMENT(kLayerFilePath, [](T const& layerPtr) {
            return (*layerPtr).getFilePath();
        })
        HANDLE_ELEMENT(kLayerOriginalFilePath, [](T const& layerPtr) {
            return (*layerPtr).getOriginalFilePath();
        })
        HANDLE_ELEMENT(kStartTimeCode, [](T const& layerPtr) {
            return (*layerPtr)->GetStartTimeCode();
        })
        HANDLE_ELEMENT(kEndTimeCode, [](T const& layerPtr) {
            return (*layerPtr)->GetEndTimeCode();
        })
    }

    if constexpr (std::is_same_v<T, StagePtr>) {
        HANDLE_ELEMENT(kLastModifiedPrim, [](T const& stagePtr) {
            return (*stagePtr).last_modified_prim;
        })
        HANDLE_ELEMENT(kVariantSelection, [](T const& stagePtr) {
            return (*stagePtr).variantSelection().variantInfo();
        })
    }

    if constexpr (std::is_same_v<T, PrimPtr>) {
        HANDLE_ELEMENT(kPrimPath, [](T const& primPtr) {
            return (*primPtr)->GetPath().GetString();
        })
        HANDLE_ELEMENT(kTypeName, [](T const& primPtr) {
            return (*primPtr)->GetTypeName().GetString();
        })
        HANDLE_ELEMENT(kKind, [](T const& primPtr) {
            return getPrimKind(primPtr).GetString();
        })
        HANDLE_ELEMENT(kIsActive,
                       [](T const& primPtr) { return (*primPtr)->IsActive(); })
        HANDLE_ELEMENT(kIsLoaded,
                       [](T const& primPtr) { return (*primPtr)->IsLoaded(); })
        HANDLE_ELEMENT(kIsModel,
                       [](T const& primPtr) { return (*primPtr)->IsModel(); })
        HANDLE_ELEMENT(kIsGroup,
                       [](T const& primPtr) { return (*primPtr)->IsGroup(); })
        HANDLE_ELEMENT(kIsAbstract, [](T const& primPtr) {
            return (*primPtr)->IsAbstract();
        })
        HANDLE_ELEMENT(kHasVariantSets, [](T const& primPtr) {
            return (*primPtr)->HasVariantSets();
        })
        HANDLE_ELEMENT(kHasAuthoredPayloads, [](T const& primPtr) {
            return (*primPtr)->HasAuthoredPayloads();
        })
        HANDLE_ELEMENT(kHasAuthoredInherits, [](T const& primPtr) {
            return (*primPtr)->HasAuthoredInherits();
        })
        HANDLE_ELEMENT(kHasAuthoredReferences, [](T const& primPtr) {
            return (*primPtr)->HasAuthoredReferences();
        })
        HANDLE_ELEMENT(kHasAuthoredSpecializes, [](T const& primPtr) {
            return (*primPtr)->HasAuthoredSpecializes();
        })
        HANDLE_ELEMENT(kHasAuthoredInstanceable, [](T const& primPtr) {
            return (*primPtr)->HasAuthoredInstanceable();
        })
        HANDLE_ELEMENT(kIsInstance, [](T const& primPtr) {
            return (*primPtr)->IsInstance();
        })
        HANDLE_ELEMENT(kIsInstanceProxy, [](T const& primPtr) {
            return (*primPtr)->IsInstanceProxy();
        })
        HANDLE_ELEMENT(kIsPrototype, [](T const& primPtr) {
            return (*primPtr)->IsPrototype();
        })
        HANDLE_ELEMENT(kIsInPrototype, [](T const& primPtr) {
            return (*primPtr)->IsInPrototype();
        })
    }

    if constexpr (is_pxr_vector_type<T>()) {
        HANDLE_ELEMENT(kVectorFields[0], [](T const& v) { return v[0]; })
        HANDLE_ELEMENT(kVectorFields[1], [](T const& v) { return v[1]; })
        if constexpr (T::dimension > 2) {
            HANDLE_ELEMENT(kVectorFields[2], [](T const& v) { return v[2]; })
            if constexpr (T::dimension > 3) {
                HANDLE_ELEMENT(kVectorFields[3],
                               [](T const& v) { return v[3]; })
            }
        }
    }

    if constexpr (is_pxr_matrix_type<T>()) {
        for (size_t row = 0; row < T::numRows; ++row) {
            for (size_t col = 0; col < T::numColumns; ++col) {
                HANDLE_ELEMENT(
                    (kMatrixFields[row][col]), ([row, col](T const& v) {
                        return v[static_cast<int>(row)][static_cast<int>(col)];
                    }))
            }
        }
    }

    if constexpr (is_pxr_quaternion_type<T>()) {
        HANDLE_ELEMENT(kQuaternionFields[0],
                       [](T const& v) { return v.GetReal(); })
        HANDLE_ELEMENT(kQuaternionFields[1],
                       [](T const& v) { return v.GetImaginary()[0]; })
        HANDLE_ELEMENT(kQuaternionFields[2],
                       [](T const& v) { return v.GetImaginary()[1]; })
        HANDLE_ELEMENT(kQuaternionFields[3],
                       [](T const& v) { return v.GetImaginary()[2]; })
    }

#undef HANDLE_ELEMENT

    return false;
}

template <typename ARRAY_TYPE,
          typename VALUE_TYPE = typename ARRAY_TYPE::value_type>
bool filterIndices(ARRAY_TYPE const& values,
                   Filter const&     filter,
                   Indices const&    indices,
                   Indices&          new_indices) {
    if (filter.m_field == Field::eIndex) {
        assert(filter.m_value.type() == Amino::getTypeId<std::size_t>());
        auto filterValue = Amino::any_cast<std::size_t>(filter.m_value);
        auto compareFn   = getCompareFunc<std::size_t>(filter.m_operation);
        for (auto idx : indices) {
            if (compareFn(idx, filterValue)) {
                new_indices.push_back(idx);
            }
        }
    } else if (filter.m_field == Field::eValue) {
        if constexpr (!is_ptr_type<VALUE_TYPE>()) {
            auto filterValue = getValueFromAny<VALUE_TYPE>(filter.m_value);
            auto compareFn   = getCompareFunc<VALUE_TYPE>(filter.m_operation);
            for (auto idx : indices) {
                assert(idx < values.size());
                if (compareFn(values[idx], filterValue)) {
                    new_indices.push_back(idx);
                }
            }
        }
    } else {
        assert(filter.m_field == Field::eElement);
        auto const& filterValueAny = filter.m_value;
        return doForElement<VALUE_TYPE>(
            filter.m_elementName, filter.m_operation,
            [&values, &indices, &new_indices, &filterValueAny](auto getEltFn,
                                                               auto compareFn) {
                using ELT_T =
                    typename std::invoke_result<decltype(getEltFn),
                                                VALUE_TYPE const&>::type;
                auto filterValue = getValueFromAny<ELT_T>(filterValueAny);
                for (auto idx : indices) {
                    assert(idx < values.size());
                    if (compareFn(getEltFn(values[idx]), filterValue)) {
                        new_indices.push_back(idx);
                    }
                }
            });
    }
    return true;
}

template <typename ARRAY_TYPE>
bool applyFilters(ARRAY_TYPE const& values,
                  Filters const&    filters,
                  Indices const&    all_indices,
                  Indices&          indices);

template <typename ARRAY_TYPE>
bool applyFilter(ARRAY_TYPE const& values,
                 Filter const&     filter,
                 Indices const&    all_indices,
                 Indices&          indices) {
    Indices const& working_indices =
        filter.m_conjunction == Filter::Conjunction::eAnd ? indices
                                                          : all_indices;
    Indices new_indices;
    new_indices.reserve(working_indices.size());

    if (filter.m_operation == Filter::Operation::eSubFilters) {
        if (!applyFilters(values, filter.m_subFilters, all_indices,
                          new_indices)) {
            return false;
        }
    } else if (!filterIndices(values, filter, working_indices, new_indices)) {
        return false;
    }

    indices.swap(new_indices);
    return true;
}

template <typename ARRAY_TYPE>
bool applyFilters(ARRAY_TYPE const& values,
                  Filters const&    filters,
                  Indices const&    all_indices,
                  Indices&          indices) {
    if (!filters.empty()) {
        if (filters.front().m_conjunction == Filter::Conjunction::eOr) {
            indices.clear();
        } else {
            indices = all_indices;
        }
        for (auto const& filter : filters) {
            Indices new_indices = indices;
            if (!applyFilter(values, filter, all_indices, new_indices)) {
                return false;
            }

            if (new_indices.empty()) {
                if (filter.m_conjunction == Filter::Conjunction::eAnd) {
                    indices.clear();
                }
            } else {
                if (indices.empty()) {
                    if (filter.m_conjunction == Filter::Conjunction::eOr) {
                        indices.swap(new_indices);
                    }
                } else if (filter.m_conjunction == Filter::Conjunction::eAnd) {
                    Indices filtered_indices;
                    std::set_intersection(indices.begin(), indices.end(),
                                          new_indices.begin(),
                                          new_indices.end(),
                                          std::back_inserter(filtered_indices));
                    indices.swap(filtered_indices);
                } else {
                    Indices filtered_indices;
                    std::merge(indices.begin(), indices.end(),
                               new_indices.begin(), new_indices.end(),
                               std::back_inserter(filtered_indices));
                    // remove duplicates
                    auto last = std::unique(filtered_indices.begin(),
                                            filtered_indices.end());
                    filtered_indices.erase(last, filtered_indices.end());
                    indices.swap(filtered_indices);
                }
            }
        }
    }
    return true;
}

template <typename ARRAY_TYPE>
bool sortIndices(ARRAY_TYPE const& values,
                 Sorter const&     sorter,
                 Indices&          indices) {
    auto compareOp = (sorter.m_order == Sorter::Order::eDescending
                          ? Filter::Operation::eGreater
                          : Filter::Operation::eLess);
    if (sorter.m_field == Field::eIndex) {
        std::sort(indices.begin(), indices.end());
        if (sorter.m_order == Sorter::Order::eDescending) {
            std::reverse(indices.begin(), indices.end());
        }
    } else if (sorter.m_field == Field::eValue) {
        using VALUE_TYPE = typename ARRAY_TYPE::value_type;
        if constexpr (!is_ptr_type<VALUE_TYPE>()) {
            auto compareFn = getCompareFunc<VALUE_TYPE>(compareOp);
            std::sort(indices.begin(), indices.end(),
                      [&values, &compareFn](std::size_t a, std::size_t b) {
                          return compareFn(values[a], values[b]);
                      });
        }
    } else {
        assert(sorter.m_field == Field::eElement);
        using VALUE_TYPE = typename ARRAY_TYPE::value_type;
        return doForElement<VALUE_TYPE>(
            sorter.m_elementName, compareOp,
            [&values, &indices](auto getEltFn, auto compareFn) {
                std::sort(indices.begin(), indices.end(),
                          [&values, &getEltFn, &compareFn](std::size_t a,
                                                           std::size_t b) {
                              return compareFn(getEltFn(values[a]),
                                               getEltFn(values[b]));
                          });
            });
    }
    return true;
}

template <typename ARRAY_TYPE>
bool sortAndFilterIndices(ARRAY_TYPE const& values,
                          Filters const&    filters,
                          Sorter const&     sorter,
                          Indices&          indices) {
    assert(!values.empty());
    Indices all_indices;
    all_indices.reserve(values.size());
    for (size_t idx = 0; idx < values.size(); ++idx) {
        all_indices.push_back(idx);
    }

    indices = all_indices;
    if (!applyFilters(values, filters, all_indices, indices)) {
        return false;
    }

    if (indices.size() > 1) {
        if (!sortIndices(values, sorter, indices)) {
            return false;
        }
    }

    return true;
}
/// \}

///-------------------------------------------------------------------------
/// \brief Helpers to convert values to string
/// \{
template <typename T>
Amino::String to_string(T const& value) {
    return Amino::to_string(value);
}

template <>
Amino::String to_string<>(std::string const& value) {
    return Amino::String(value.c_str());
}

template <>
Amino::String to_string<>(bool const& value) {
    return value ? "true" : "false";
}

template <>
Amino::String to_string<>(double const& value) {
    return Amino::String(std::to_string(value).c_str());
}

template <>
Amino::String to_string(float const& value) {
    return Amino::String(std::to_string(value).c_str());
}

template <>
Amino::String to_string<>(unsigned long const& value) {
    return to_string(static_cast<unsigned long long>(value));
}

template <>
Amino::String to_string(PXR_NS::GfHalf const& value) {
    return Amino::String(std::to_string(value).c_str());
}
/// \}

///-------------------------------------------------------------------------
/// \brief Helpers to get the value for the different supported types
/// \{
template <typename T>
bool getValue(T const& value,
              WatchpointLayoutFactory const& /*factory*/,
              WatchpointLayoutPath& path,
              Amino::String&        out_value) {
    if constexpr (is_builtin_type<T>()) {
        out_value = to_string(value);
        return path.empty();
    }

    if constexpr (is_pxr_vector_type<T>()) {
        if (!path.empty()) {
            auto fieldName = path.front();
            path.pop_front();
            if (fieldName == kVectorFields[0]) {
                out_value = to_string(value[0]);
                return path.empty();
            } else if (fieldName == kVectorFields[1]) {
                out_value = to_string(value[1]);
                return path.empty();
            }
            if constexpr (T::dimension > 2) {
                if (fieldName == kVectorFields[2]) {
                    out_value = to_string(value[2]);
                    return path.empty();
                }
                if constexpr (T::dimension > 3) {
                    if (fieldName == kVectorFields[3]) {
                        out_value = to_string(value[3]);
                        return path.empty();
                    }
                }
            }
        }
        return false;
    }

    if constexpr (is_pxr_matrix_type<T>()) {
        if (!path.empty()) {
            auto fieldName = path.front();
            path.pop_front();
            for (size_t row = 0; row < T::numRows; ++row) {
                for (size_t col = 0; col < T::numColumns; ++col) {
                    if (fieldName == kMatrixFields[row][col]) {
                        out_value = to_string(value[static_cast<int>(row)]
                                                   [static_cast<int>(col)]);
                        return path.empty();
                    }
                }
            }
        }
        return false;
    }

    if constexpr (is_pxr_quaternion_type<T>()) {
        if (!path.empty()) {
            auto fieldName = path.front();
            path.pop_front();
            if (fieldName == kQuaternionFields[0]) {
                out_value = to_string(value.GetReal());
                return path.empty();
            } else if (fieldName == kQuaternionFields[1]) {
                out_value = to_string(value.GetImaginary()[0]);
                return path.empty();
            } else if (fieldName == kQuaternionFields[2]) {
                out_value = to_string(value.GetImaginary()[1]);
                return path.empty();
            } else if (fieldName == kQuaternionFields[3]) {
                out_value = to_string(value.GetImaginary()[2]);
                return path.empty();
            }
        }
        return false;
    }

    if constexpr (std::is_same_v<T, PXR_NS::TfToken>) {
        out_value = value.GetText();
        return path.empty();
    }

    if constexpr (std::is_same_v<T, PXR_NS::SdfAssetPath>) {
        out_value = value.GetAssetPath().c_str();
        return path.empty();
    }

    assert(false && "getValue called for unsupported type");
    return false;
}

template <>
bool getValue<>(PXR_NS::VtValue const&         value,
                WatchpointLayoutFactory const& factory,
                WatchpointLayoutPath&          path,
                Amino::String&                 out_value) {
#define HANDLE_TYPE(TYPE)                                                      \
    if (value.IsHolding<TYPE>()) {                                             \
        return getValue(value.UncheckedGet<TYPE>(), factory, path, out_value); \
    }                                                                          \
    if (value.IsHolding<PXR_NS::VtArray<TYPE>>()) {                            \
        if (path.frontIsIndex()) {                                             \
            auto idx = path.frontAsIndex();                                    \
            path.pop_front();                                                  \
            auto const& vtArray = value.UncheckedGet<PXR_NS::VtArray<TYPE>>(); \
            if (vtArray.size() > idx) {                                        \
                return getValue(vtArray[idx], factory, path, out_value);       \
            }                                                                  \
        }                                                                      \
        return false;                                                          \
    }

    FOR_EACH_SUPPORTED_TYPES(HANDLE_TYPE)

#undef HANDLE_TYPE

    assert(false && "getValue called for unsupported type");
    return false;
}

template <typename PTR_TYPE, typename PARENT_TYPE, typename GET_ARRAY_FN>
bool getElementValue(PARENT_TYPE const&             parent,
                     WatchpointLayoutFactory const& factory,
                     WatchpointLayoutPath&          path,
                     Amino::String&                 out_value,
                     GET_ARRAY_FN&&                 getArrayFn) {
    if (!path.empty() && path.frontIsIndex()) {
        auto idx = path.frontAsIndex();
        path.pop_front();
        auto values = getArrayFn();
        if (std::distance(values.begin(), values.end()) >
            static_cast<std::ptrdiff_t>(idx)) {
            auto it = values.begin();
            std::advance(it, idx);
            auto elementPtr =
                Amino::newClassPtr<typename PTR_TYPE::element_type>(*it,
                                                                    parent);
            return getValue(elementPtr, factory, path, out_value);
        }
    }
    return false;
}

template <>
bool getValue<>(AttributePtr const&            attributePtr,
                WatchpointLayoutFactory const& factory,
                WatchpointLayoutPath&          path,
                Amino::String&                 out_value) {
    if (attributePtr && !path.empty()) {
        auto const& attribute = *attributePtr;
        if (path.front() == kName) {
            path.pop_front();
            out_value = attribute->GetName().GetText();
            return path.empty();
        } else if (path.front() == kTypeName) {
            path.pop_front();
            out_value = attribute->GetTypeName().GetAsToken().GetText();
            return path.empty();
        } else if (path.front() == kIsDefined) {
            path.pop_front();
            out_value = to_string(attribute->IsDefined());
            return path.empty();
        } else if (path.front() == kIsCustom) {
            path.pop_front();
            out_value = to_string(attribute->IsCustom());
            return path.empty();
        } else if (path.front() == kIsAuthored) {
            path.pop_front();
            out_value = to_string(attribute->IsAuthored());
            return path.empty();
        } else if (path.front() == kNumTimeSamples) {
            path.pop_front();
            out_value = to_string(attribute->GetNumTimeSamples());
            return path.empty();
        } else if (path.front() == kValue) {
            path.pop_front();
            PXR_NS::VtValue vtVal;
            if (attribute->Get(&vtVal)) {
                return getValue(vtVal, factory, path, out_value);
            }
        }
    }
    return false;
}

template <>
bool getValue<>(PrimPtr const&                 primPtr,
                WatchpointLayoutFactory const& factory,
                WatchpointLayoutPath&          path,
                Amino::String&                 out_value) {
    if (primPtr && !path.empty()) {
        auto const& prim = *primPtr;
        if (path.front() == kPrimPath) {
            path.pop_front();
            out_value = prim->GetPath().GetString().c_str();
            return path.empty();
        } else if (path.front() == kTypeName) {
            path.pop_front();
            out_value = prim->GetTypeName().GetText();
            return path.empty();
        } else if (path.front() == kKind) {
            path.pop_front();
            out_value = getPrimKind(primPtr).GetText();
            return path.empty();
        } else if (path.front() == kIsActive) {
            path.pop_front();
            out_value = to_string(prim->IsActive());
            return path.empty();
        } else if (path.front() == kIsLoaded) {
            path.pop_front();
            out_value = to_string(prim->IsLoaded());
            return path.empty();
        } else if (path.front() == kIsModel) {
            path.pop_front();
            out_value = to_string(prim->IsModel());
            return path.empty();
        } else if (path.front() == kIsGroup) {
            path.pop_front();
            out_value = to_string(prim->IsGroup());
            return path.empty();
        } else if (path.front() == kIsAbstract) {
            path.pop_front();
            out_value = to_string(prim->IsAbstract());
            return path.empty();
        } else if (path.front() == kHasVariantSets) {
            path.pop_front();
            out_value = to_string(prim->HasVariantSets());
            return path.empty();
        } else if (path.front() == kHasAuthoredPayloads) {
            path.pop_front();
            out_value = to_string(prim->HasAuthoredPayloads());
            return path.empty();
        } else if (path.front() == kHasAuthoredInherits) {
            path.pop_front();
            out_value = to_string(prim->HasAuthoredInherits());
            return path.empty();
        } else if (path.front() == kHasAuthoredReferences) {
            path.pop_front();
            out_value = to_string(prim->HasAuthoredReferences());
            return path.empty();
        } else if (path.front() == kHasAuthoredSpecializes) {
            path.pop_front();
            out_value = to_string(prim->HasAuthoredSpecializes());
            return path.empty();
        } else if (path.front() == kHasAuthoredInstanceable) {
            path.pop_front();
            out_value = to_string(prim->HasAuthoredInstanceable());
            return path.empty();
        } else if (path.front() == kIsInstance) {
            path.pop_front();
            out_value = to_string(prim->IsInstance());
            return path.empty();
        } else if (path.front() == kIsInstanceProxy) {
            path.pop_front();
            out_value = to_string(prim->IsInstanceProxy());
            return path.empty();
        } else if (path.front() == kIsPrototype) {
            path.pop_front();
            out_value = to_string(prim->IsPrototype());
            return path.empty();
        } else if (path.front() == kIsInPrototype) {
            path.pop_front();
            out_value = to_string(prim->IsInPrototype());
            return path.empty();
        } else if (path.front() == kPrimAttributes) {
            path.pop_front();
            return getElementValue<AttributePtr>(
                primPtr, factory, path, out_value,
                [&prim]() { return prim->GetAttributes(); });
        } else if (path.front() == kPrimAuthoredAttributes) {
            path.pop_front();
            return getElementValue<AttributePtr>(
                primPtr, factory, path, out_value,
                [&prim]() { return prim->GetAuthoredAttributes(); });
        }
    }
    return false;
}

template <>
bool getValue<>(LayerPtr const& layerPtr,
                WatchpointLayoutFactory const& /*factory*/,
                WatchpointLayoutPath& path,
                Amino::String&        out_value) {
    if (layerPtr && !path.empty()) {
        auto const& layer = *layerPtr;
        if (path.front() == kLayerDisplayName) {
            path.pop_front();
            out_value = layer->GetDisplayName().c_str();
            return path.empty();
        } else if (path.front() == kLayerFilePath) {
            path.pop_front();
            out_value = layer.getFilePath();
            return path.empty();
        } else if (path.front() == kLayerOriginalFilePath) {
            path.pop_front();
            out_value = layer.getOriginalFilePath();
            return path.empty();
        } else if (path.front() == kStartTimeCode) {
            path.pop_front();
            out_value = to_string(layer->GetStartTimeCode());
            return path.empty();
        } else if (path.front() == kEndTimeCode) {
            path.pop_front();
            out_value = to_string(layer->GetEndTimeCode());
            return path.empty();
        }
    }
    return false;
}

template <>
bool getValue<>(StagePtr const&                stagePtr,
                WatchpointLayoutFactory const& factory,
                WatchpointLayoutPath&          path,
                Amino::String&                 out_value) {
    if (stagePtr && !path.empty()) {
        auto const& stage = *stagePtr;
        if (path.front() == kLastModifiedPrim) {
            path.pop_front();
            out_value = stage.last_modified_prim;
            return path.empty();
        } else if (path.front() == kRootLayer) {
            path.pop_front();
            return getValue(stage.getRootLayer(), factory, path, out_value);
        } else if (path.front() == kVariantSelection) {
            path.pop_front();
            out_value = stage.variantSelection().variantInfo();
            return path.empty();
        } else if (path.front() == kPrims) {
            path.pop_front();
            return getElementValue<PrimPtr>(
                stagePtr, factory, path, out_value, [&stage]() {
                    return const_cast<PXR_NS::UsdStage&>(stage.get())
                        .TraverseAll();
                });
        }
    }
    return false;
}

/// \}

///-------------------------------------------------------------------------
/// \brief The usd watchpoint watcher to store the recorded values
/// and create the layout.
/// \{
template <typename T>
class USDWatcher : public Watcher {
public:
    USDWatcher() noexcept;
    ~USDWatcher() noexcept override;

    void deleteThis() noexcept override;

    void setValue(Amino::ValueView const& value) noexcept override;

    WatchpointLayoutPtr getLayout(
        WatchpointLayoutFactory& factory) const noexcept override;

    bool getValue(WatchpointLayoutFactory const&,
                  WatchpointLayoutPath& path,
                  Amino::String&        out_value) const noexcept override;

private:
    T                           m_value;
    mutable WatchpointLayoutPtr m_cachedLayout;
};

template <typename T>
USDWatcher<T>::USDWatcher() noexcept = default;

template <typename T>
USDWatcher<T>::~USDWatcher() noexcept = default;

template <typename T>
void USDWatcher<T>::deleteThis() noexcept {
    delete this;
}

template <typename T>
void USDWatcher<T>::setValue(Amino::ValueView const& value) noexcept {
    auto const* valuePtr = Amino::view_cast<T>(value);
    assert(valuePtr);
    m_value = *valuePtr;
    m_cachedLayout = {};
}

template <typename T>
WatchpointLayoutPtr USDWatcher<T>::getLayout(
    WatchpointLayoutFactory& factory) const noexcept {
    try {
        if (!m_cachedLayout) {
            m_cachedLayout = ::createLayoutFromValue<T>(factory, m_value);
        }
        return m_cachedLayout;
    } catch (...) {
        return {}; // LCOV_EXCL_LINE
    }
}

template <typename T>
bool USDWatcher<T>::getValue(WatchpointLayoutFactory const& factory,
                             WatchpointLayoutPath&          path,
                             Amino::String& out_value) const noexcept {
    try {
        return ::getValue(m_value, factory, path, out_value);
    } catch (...) {
        return false; // LCOV_EXCL_LINE
    }
}
/// \}

} // namespace

class USDWatchpoint : public BifrostGraph::Executor::Watchpoint {
public:
    USDWatchpoint() noexcept;
    ~USDWatchpoint() noexcept override;

    void deleteThis() noexcept override;

    void getSupportedTypeIds(TypeIdArray& out_typeIds) const noexcept override;

    Watcher* createWatcher(Amino::TypeId const& typeId,
                           Watcher::Flags       flags) const noexcept override;

    bool getValue(WatchpointLayoutFactory const& factory,
                  Amino::Any const&              any,
                  WatchpointLayoutPath&          path,
                  Amino::String& out_value) const noexcept override;

    WatchpointLayoutPtr createLayout(
        WatchpointLayoutFactory const& factory,
        Amino::Any const&              any) const noexcept override;

    std::size_t getArraySize(Amino::Any const& any) const noexcept override;

    WatchpointLayoutPtr getArrayElementLayout(
        WatchpointLayoutFactory const& factory,
        Amino::Any const&              any,
        std::size_t                    index) const noexcept override;

    bool getIndices(Amino::Any const& any,
                    Filters const&    filters,
                    Sorter const&     sorter,
                    Indices&          out_indices) const noexcept override;
};

USDWatchpoint::USDWatchpoint() noexcept
    : BifrostGraph::Executor::Watchpoint("USD Watchpoint") {}

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

    // Add Amino::Ptr<PXR_NS::VtValue> to support array element watching
    out_typeIds.push_back(Amino::getTypeId<Amino::Ptr<PXR_NS::VtValue>>());
}

Watcher* USDWatchpoint::createWatcher(Amino::TypeId const& typeId,
                                      Watcher::Flags /*flags*/) const noexcept {
    try {
#define HANDLE_TYPE(TYPE)                     \
    if (typeId == Amino::getTypeId<TYPE>()) { \
        return new USDWatcher<TYPE>{};        \
    }
        HANDLE_TYPE(AttributePtr)
        HANDLE_TYPE(ArrayOfAttributes)
        HANDLE_TYPE(LayerPtr)
        HANDLE_TYPE(ArrayOfLayers)
        HANDLE_TYPE(StagePtr)
        HANDLE_TYPE(ArrayOfStages)
        HANDLE_TYPE(PrimPtr)
        HANDLE_TYPE(ArrayOfPrims)
#undef HANDLE_TYPE
        assert(false && "createWatcher called for unsupported type");
    } catch (...) {
    }
    return nullptr; // LCOV_EXCL_LINE
}

bool USDWatchpoint::getValue(WatchpointLayoutFactory const& factory,
                             Amino::Any const&              any,
                             WatchpointLayoutPath&          path,
                             Amino::String& out_value) const noexcept {
    try {
        assert(any.has_value());
#define HANDLE_TYPE(TYPE)                                            \
    if (any.type() == Amino::getTypeId<TYPE>()) {                    \
        return ::getValue(Amino::any_cast<TYPE>(any), factory, path, \
                          out_value);                                \
    }
        HANDLE_TYPE(AttributePtr)
        HANDLE_TYPE(ArrayOfAttributes)
        HANDLE_TYPE(LayerPtr)
        HANDLE_TYPE(ArrayOfLayers)
        HANDLE_TYPE(StagePtr)
        HANDLE_TYPE(ArrayOfStages)
        HANDLE_TYPE(PrimPtr)
        HANDLE_TYPE(ArrayOfPrims)
#undef HANDLE_TYPE
        assert(false && "getValue called for unsupported type");
    } catch (...) {
    }
    return false; // LCOV_EXCL_LINE
}

WatchpointLayoutPtr USDWatchpoint::createLayout(
    WatchpointLayoutFactory const& factory,
    Amino::Any const&              any) const noexcept {
    try {
#define HANDLE_TYPE(TYPE)                                                 \
    if (any.type() == Amino::getTypeId<TYPE>()) {                         \
        return ::createLayoutFromValue<TYPE>(factory,                     \
                                             Amino::any_cast<TYPE>(any)); \
    }
        HANDLE_TYPE(AttributePtr)
        HANDLE_TYPE(ArrayOfAttributes)
        HANDLE_TYPE(LayerPtr)
        HANDLE_TYPE(ArrayOfLayers)
        HANDLE_TYPE(StagePtr)
        HANDLE_TYPE(ArrayOfStages)
        HANDLE_TYPE(PrimPtr)
        HANDLE_TYPE(ArrayOfPrims)
#undef HANDLE_TYPE
        assert(false && "createLayout called for unsupported type");
    } catch (...) {
    }
    return {}; // LCOV_EXCL_LINE
}

std::size_t USDWatchpoint::getArraySize(Amino::Any const& any) const noexcept {
    if (any.type() == Amino::getTypeId<Amino::Ptr<PXR_NS::VtValue>>()) {
        auto const& vtValuePtr =
            Amino::any_cast<Amino::Ptr<PXR_NS::VtValue>>(any);
        assert(vtValuePtr && vtValuePtr->IsArrayValued());
        return vtValuePtr->GetArraySize();
    }

    assert(false && "getArraySize called for unsupported value");
    return 0u; // LCOV_EXCL_LINE
}

WatchpointLayoutPtr USDWatchpoint::getArrayElementLayout(
    WatchpointLayoutFactory const& factory,
    Amino::Any const&              any,
    std::size_t /*index*/) const noexcept {
    if (any.type() == Amino::getTypeId<Amino::Ptr<PXR_NS::VtValue>>()) {
        auto const& vtValuePtr =
            Amino::any_cast<Amino::Ptr<PXR_NS::VtValue>>(any);
        assert(vtValuePtr && vtValuePtr->IsArrayValued());

#define HANDLE_TYPE(TYPE)                                 \
    if (vtValuePtr->IsHolding<PXR_NS::VtArray<TYPE>>()) { \
        return getLayoutFromType<TYPE>(factory);          \
    }
        FOR_EACH_SUPPORTED_TYPES(HANDLE_TYPE)
#undef HANDLE_TYPE
    }

    assert(false && "getArrayElementLayout called for unsupported value");
    return {};
}

bool USDWatchpoint::getIndices(Amino::Any const& any,
                               Filters const&    filters,
                               Sorter const&     sorter,
                               Indices&          out_indices) const noexcept {
#define HANDLE_TYPE(TYPE)                                            \
    if (any.type() == Amino::getTypeId<TYPE>()) {                    \
        auto const& valuesPtr = Amino::any_cast<TYPE>(any);          \
        if (valuesPtr && !valuesPtr->empty()) {                      \
            return sortAndFilterIndices(*valuesPtr, filters, sorter, \
                                        out_indices);                \
        }                                                            \
        return true;                                                 \
    }
    HANDLE_TYPE(ArrayOfAttributes)
    HANDLE_TYPE(ArrayOfLayers)
    HANDLE_TYPE(ArrayOfStages)
    HANDLE_TYPE(ArrayOfPrims)
#undef HANDLE_TYPE

    if (any.type() == Amino::getTypeId<Amino::Ptr<PXR_NS::VtValue>>()) {
        auto const& vtValuePtr =
            Amino::any_cast<Amino::Ptr<PXR_NS::VtValue>>(any);
        assert(vtValuePtr && vtValuePtr->IsArrayValued());
        if (vtValuePtr->GetArraySize() > 0) {
#define HANDLE_TYPE(TYPE)                                                   \
    if (vtValuePtr->IsHolding<PXR_NS::VtArray<TYPE>>()) {                   \
        auto const& vtArray =                                               \
            vtValuePtr->UncheckedGet<PXR_NS::VtArray<TYPE>>();              \
        return sortAndFilterIndices(vtArray, filters, sorter, out_indices); \
    }
            FOR_EACH_SUPPORTED_TYPES(HANDLE_TYPE)
#undef HANDLE_TYPE
        }
        return true;
    }

    assert(false && "getIndices called for unsupported value");
    return false;
}

#undef FOR_EACH_SUPPORTED_TYPES

extern "C" {
USD_WATCHPOINT_EXPORT BifrostGraph::Executor::Watchpoint* createBifrostWatchpoint(void);

USD_WATCHPOINT_EXPORT BifrostGraph::Executor::Watchpoint* createBifrostWatchpoint(void) {
    return new USDWatchpoint();
}
}
