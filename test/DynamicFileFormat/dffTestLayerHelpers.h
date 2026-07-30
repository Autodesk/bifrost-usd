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

#ifndef BIFROSTUSD_DFF_LAYER_HELPERS_H
#define BIFROSTUSD_DFF_LAYER_HELPERS_H

// Amino
#include <Amino/Core/String.h>

// Bifrost USD
#include <BifrostUsd/GraphExecutor/GraphExecutorTypes.h>
#include <utils/test/testUtils.h>

// Open USD
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/stage.h>

// C++ Standard Library
#include <string>
#include <utility>
#include <vector>

using BifrostUsd::GraphExecutor::VerbosityLevel;

namespace DffTestHelpers {

using ValueTypeNameAndValue =
    std::pair<PXR_NS::SdfValueTypeName, PXR_NS::VtValue>;
using AttributeName         = PXR_NS::TfToken;
using AttributeNameAndValue = std::pair<AttributeName, ValueTypeNameAndValue>;

/// Create an in-memory layer with a Root prim that:
///   - payloads the BifrostDynamicFile format
///   - references an inner DynamicFileFormatField prim
///
/// Returns the path of the saved layer.
Amino::String createRootLayerWithDff(
    const std::string&                             diagnosticsBucketKey,
    BifrostUsd::TestUtils::UniqueTestOutputSubdir& outputDir);

/// Create an in-memory layer with a Root prim that:
///   - payloads the BifrostDynamicFile format
///   - references an inner DynamicFileFormatField prim
///   - sets Bifrost compoundName = \p compoundName via the field dictionary
///   - sets Bifrost outputs = [\p outputName] (empty array when \p outputName
///     is empty)
///   - sets Bifrost settings reloadLibrary = false
///
/// Returns the path of the saved layer.
Amino::String createRootLayerWithDefaultDffFields(
    const std::string&                             diagnosticsBucketKey,
    BifrostUsd::TestUtils::UniqueTestOutputSubdir& outputDir,
    const std::string&                             compoundName,
    const std::string&                             outputName = {});

/// Author Bifrost "settings" verbosityLevel in the root layer of the
/// already-open \p stage. This helper performs a live stage edit and does not
/// save the layer.
void setLiveVerbosityLevelField(PXR_NS::UsdStageRefPtr& stage,
                                VerbosityLevel          level);

/// Author Bifrost "settings" reloadLibrary in the root layer of the
/// already-open \p stage. This helper performs a live stage edit and does not
/// save the layer.
void setLiveReloadLibraryField(PXR_NS::UsdStageRefPtr& stage, bool reload);

/// Author the Bifrost "globals" dictionary in the root layer of the
/// already-open \p stage. This helper performs a live stage edit and does not
/// save the layer.
void setLiveGlobalsField(PXR_NS::UsdStageRefPtr&     stage,
                         const PXR_NS::VtDictionary& paramsDict);

/// Author the Bifrost "inputs" dictionary in the root layer of the
/// already-open \p stage. This helper performs a live stage edit and does not
/// save the layer.
void setLiveInputsField(PXR_NS::UsdStageRefPtr&     stage,
                        const PXR_NS::VtDictionary& paramsDict);

/// Author Bifrost "outputs = [outputPortNames...]" in the root layer of the
/// of the already-open \p stage. This helper performs a live stage edit and
/// does not save the layer.
void setLiveOutputsField(PXR_NS::UsdStageRefPtr&         stage,
                         const std::vector<std::string>& outputPortNames);

/// Author default values for the attributes in \p attributeVector on `/Root`
/// of the already-open \p stage, creating missing attributes as needed.
/// This helper performs a live stage edit and does not save the layer.
void setLiveAttributeOverrides(
    PXR_NS::UsdStageRefPtr&                   stage,
    const std::vector<AttributeNameAndValue>& attributeVector);

/// Re-open \p layer as a stage, author Bifrost compoundName, then save the
/// modified layer to disk. This helper edits a persisted layer rather than a
/// caller-owned live stage.
void setOfflineCompoundNameAttr(const PXR_NS::SdfLayerRefPtr& layer,
                                const std::string&            overrideName);

} // namespace DffTestHelpers

#endif // BIFROSTUSD_DFF_LAYER_HELPERS_H
