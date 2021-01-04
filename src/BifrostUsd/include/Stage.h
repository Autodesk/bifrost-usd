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

/// \file Stage.h
///
/// \brief Bifrost USD Stage type
///

#ifndef VALUE_SEMANTIC_USD_STAGE_H
#define VALUE_SEMANTIC_USD_STAGE_H

#include <Amino/Core/Ptr.h>
#include <Amino/Core/String.h>
#include <Amino/Cpp/Annotate.h>
#include <Amino/Cpp/ClassDeclare.h>

#include "BifrostUsdExport.h"

#ifndef DISABLE_PXR_HEADERS

// Note: To silence warnings coming from USD library
#include <bifusd/config/CfgWarningMacros.h>
BIFUSD_WARNING_PUSH

BIFUSD_WARNING_DISABLE_MSC(4003)
BIFUSD_WARNING_DISABLE_MSC(4244)
BIFUSD_WARNING_DISABLE_MSC(4305)
BIFUSD_WARNING_DISABLE_MSC(4800)

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/variantSets.h>

BIFUSD_WARNING_POP

#include "Layer.h"

#endif // DISABLE_PXR_HEADERS

namespace BifrostUsd {

/// \page Enum

/// \section InitialLoadSet
///
/// Specifies the initial set of prims to load when opening a UsdStage.
/// <ul>
/// <li>LoadAll: Load all loadable prims.</li>
/// <li>LoadNone: Load no loadable prims.</li>
/// </ul>
enum /*@cond true*/ class AMINO_ANNOTATE(
    "Amino::Enum") /*@endcond*/ InitialLoadSet : int {
    LoadAll,
    LoadNone
};

/// \section UpAxis
///
/// Enum version of pxr::UpAxis tokens.
//  UpAxis can only be set to "Y" or "Z"
/// <ul>
/// <li>Y: Up Axis is the Y-Axis.</li>
/// <li>Z: Up Axis is the Z-Axis.</li>
/// </ul>
enum /*@cond true*/ class AMINO_ANNOTATE("Amino::Enum") /*@endcond*/ UpAxis : int {
    Y,
    Z
};

/// \section UsdListPosition
///
/// Specifies a position to add items to lists.
/// <ul>
/// <li>UsdListPositionFrontOfPrependList: The position at the front of the
/// prepend list. An item added at this position will, after composition is
/// applied, be stronger than other items prepended in this layer, and stronger
/// than items added by weaker layers.</li>
/// <li>UsdListPositionBackOfPrependList: The position at the back of the
/// prepend list. An item added at this position will, after composition is
/// applied, be weaker than other items prepended in this layer, but stronger
/// than items added by weaker layers.</li>
/// <li>UsdListPositionFrontOfAppendList: The position at the front of the
/// append list.An item added at this position will, after composition is
/// applied, be stronger than other items appended in this layer, and stronger
/// than items added by weaker layers.</li> <li>UsdListPositionBackOfAppendList:
/// The position at the back of the append list. An item added at this position
/// will, after composition is applied, be weaker than other items appended in
/// this layer, but stronger than items added by weaker layers.</li>
/// </ul>
enum /*@cond true*/ AMINO_ANNOTATE(
    "Amino::Enum") /*@endcond*/ UsdListPosition : int {
    UsdListPositionFrontOfPrependList,
    UsdListPositionBackOfPrependList,
    UsdListPositionFrontOfAppendList,
    UsdListPositionBackOfAppendList
};

/// \section PrimDescendantMode
///
/// Specifies the way prim descendants should be filtered.
/// <ul>
/// <li>UsdPrimChildren: Return this prim's active, loaded, defined,
///                      non-abstract children.</li>
/// <li>UsdPrimAllChildren: Return all this prim's
///                         children.</li>
/// <li>UsdPrimDescendants: Return this prim's active, loaded,
///                         defined, non-abstract descendants.</li>
/// <li>UsdPrimAllDescendants: Return all this prim's descendants.</li>
/// </ul>
enum /*@cond true*/ AMINO_ANNOTATE(
    "Amino::Enum") /*@endcond*/ PrimDescendantMode : int {
    UsdPrimChildren,
    UsdPrimAllChildren,
    UsdPrimDescendants,
    UsdPrimAllDescendants
};

/// \class Stage Stage.h
/// \brief Bifrost USD Stage type that flows in the graph.
class AMINO_ANNOTATE("Amino::Class") USD_DECL Stage {
public:
    Stage();
#ifndef DISABLE_PXR_HEADERS
    struct Invalid {};

    Stage(Invalid);
    explicit Stage(const Layer&         rootLayer,
                   const InitialLoadSet load = InitialLoadSet::LoadAll);
    explicit Stage(const Layer&                       rootLayer,
                   const pxr::UsdStagePopulationMask& mask,
                   const InitialLoadSet load = InitialLoadSet::LoadAll);
    explicit Stage(const Amino::String& filePath,
                   const InitialLoadSet load = InitialLoadSet::LoadAll);
    explicit Stage(const Amino::String&               filePath,
                   const pxr::UsdStagePopulationMask& mask,
                   const InitialLoadSet load = InitialLoadSet::LoadAll);

    Stage(const Stage& other);
    Stage& operator=(const Stage& other);

    Stage(Stage&& other) noexcept;
    Stage& operator=(Stage&& other) noexcept;

    bool     isValid() const { return m_stage != nullptr; }
    explicit operator bool() const { return isValid(); }

    /// \brief Accessors to the underlying pxr::UsdStage.
    ///
    /// These accessor should be used instead of getting the underlying
    /// pxr::UsdStageRefPtr directly, because they propagate constness to the
    /// pxr::UsdStage pointer.
    ///
    /// This helps avoiding unintentionally creating side effects in other
    /// pointers to the same \ref BifrostUsd::Stage.
    /// \{
    /// \return UsdStage
    pxr::UsdStage&       get() { return *m_stage; }
    pxr::UsdStage const& get() const { return *m_stage; }
    pxr::UsdStage&       operator*() { return *m_stage; }
    pxr::UsdStage const& operator*() const { return *m_stage; }
    pxr::UsdStage*       operator->() { return m_stage.operator->(); }
    pxr::UsdStage const* operator->() const { return m_stage.operator->(); }
    /// \}

    // This function is purposefully non-const. Be careful with it.
    pxr::UsdStageRefPtr getStagePtr() { return m_stage; }

    Amino::Ptr<Layer>&       getRootLayer() { return m_rootLayer; }
    const Amino::Ptr<Layer>& getRootLayer() const { return m_rootLayer; }

    /// Get the index of the current stage's EditTarget layer.
    ///
    /// \return The index of the layer that is set as the current EditTarget.
    ///     -1 is returned to indicate that the stage's root layer is the
    ///     EditTarget; otherwise this method returns the index of the
    ///     sublayer EditTarget from the list of sublayers.
    int getEditLayerIndex() const { return m_editLayerIndex; }

    /// Set the stage's EditTarget layer.
    ///
    /// \param [in] layerIndex The sublayer index to set as the stage's
    ///     EditTarget. The index -1 identifies the root layer.
    ///     If given index is invalid and defaultToRoot is false, this method
    ///     does nothing and the current stage's EditTarget is preserved.
    /// \param [in] defaultToRoot If given layerIndex is invalid, this
    ///     parameter indicates if the stage's root layer should be set as the
    ///     default EditTarget or not.
    /// \return true if the stage's EditTarget has been set; false if the stage
    ///     is invalid, or if layerIndex is invalid and defaultToRoot was
    ///     false.
    bool setEditLayerIndex(const int layerIndex, bool defaultToRoot);

    bool hasLastModifiedVariantSetPrim() const {
        return !last_modified_variant_set_prim.empty();
    }

    pxr::UsdVariantSet getLastModifedVariantSet() const;

    bool save(const Amino::String& filePath = "") const;

    Amino::String filePath() const {
        return static_cast<Amino::String>(
            getRootLayer()->getFilePath().c_str());
    }

    Amino::String last_modified_prim;
    Amino::String last_modified_variant_set_prim;
    Amino::String last_modified_variant_set_name;
    Amino::String last_modified_variant_name;

private:
    Amino::Ptr<Layer>   m_rootLayer;
    pxr::UsdStageRefPtr m_stage;
    int                 m_editLayerIndex{-1};
#endif // DISABLE_PXR_HEADERS
};
} // namespace BifrostUsd

AMINO_DECLARE_DEFAULT_CLASS(USD_DECL, BifrostUsd::Stage);

#endif /* VALUE_SEMANTIC_USD_STAGE_H */
