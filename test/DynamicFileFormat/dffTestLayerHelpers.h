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
#include <optional>

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
/// already-open \p stage. This helper performs a stage edit and does not
/// save the layer.
void setVerbosityLevelField(PXR_NS::UsdStageRefPtr& stage,
                            VerbosityLevel          level);

/// Author Bifrost "settings" reloadLibrary in the root layer of the
/// already-open \p stage. This helper performs a stage edit and does not
/// save the layer.
void setReloadLibraryField(PXR_NS::UsdStageRefPtr& stage, bool reload);

/// Author the Bifrost "globals" dictionary in the root layer of the
/// already-open \p stage. This helper performs a stage edit and does not
/// save the layer.
void setGlobalsField(PXR_NS::UsdStageRefPtr&     stage,
                     const PXR_NS::VtDictionary& paramsDict);

/// Author the Bifrost "inputs" dictionary in the root layer of the
/// already-open \p stage. This helper performs a stage edit and does not
/// save the layer.
void setInputsField(PXR_NS::UsdStageRefPtr&     stage,
                    const PXR_NS::VtDictionary& paramsDict);

/// Author Bifrost "outputs = [outputPortNames...]" in the root layer of the
/// of the already-open \p stage. This helper performs a stage edit and
/// does not save the layer.
void setOutputsField(PXR_NS::UsdStageRefPtr&         stage,
                     const std::vector<std::string>& outputPortNames);

/// Author default values for the attributes in \p attributeVector on `/Root`
/// of the already-open \p stage, creating missing attributes as needed.
/// This helper performs a stage edit and does not save the layer.
void setAttributeOverrides(
    PXR_NS::UsdStageRefPtr&                   stage,
    const std::vector<AttributeNameAndValue>& attributeVector);

/// Re-open \p layer as a stage, author Bifrost compoundName, then save the
/// modified layer to disk. This helper edits a persisted layer rather than a
/// caller-owned stage.
void setCompoundNameAttrAndSave(const PXR_NS::SdfLayerRefPtr& layer,
                                const std::string&            overrideName);

// Create a stage with a dynamic payload using a Bifrost compound.
PXR_NS::UsdStageRefPtr createStageWithCompoundPayload(
    const std::string& fullyQualifiedCompoundName,
    const std::string& outputName,
    const std::string&                             bucketKey,
    BifrostUsd::TestUtils::UniqueTestOutputSubdir& outputDir);

/// Return the generated Bifrost Dynamic Payload layer composed into \p stage
/// (the read-only layer produced by the DFF plugin), or a null handle if none
/// is found. Identified by its file format id "bifrostDynamicFile". If several
/// payload prims exist, the first match in composition order is returned.
PXR_NS::SdfLayerHandle getGeneratedPayloadLayer(
    const PXR_NS::UsdStageRefPtr& stage);

// Create a stage with dynamic payload using the "move_up_point_simulation" compound.
// This compound uses a feedback loop that moves a single point up on the Y-axis
// by vertical_step units per frame.
PXR_NS::UsdStageRefPtr createMoveUpPointSimulationStage(
    const std::string&                             bucketKey,
    BifrostUsd::TestUtils::UniqueTestOutputSubdir& outputDir);

// Setup the "move_up_point_simulation" dynamic payload.
//
// The timeline values are authored atomically in a single "set globals field"
// call so the payload never recomposes through a transient invalid state (e.g.
// start_frame set before end_frame), which would emit a diagnostic.
//
// For example:
//   - At startFrame the point is at:
//      (0, 0, 0)
//   - At endFrame   the point is at:
//      (0, (endFrame - startFrame) * (vertical_step / fps), 0)
PXR_NS::UsdAttribute setUpMoveUpPointSimulationFields(PXR_NS::UsdStageRefPtr& stage,
                                                float  verticalStep = 1.f,
                                                double startFrame   = 1.0,
                                                double endFrame     = 3.0,
                                                double fps          = 1.0);

// Resolves the given points attribute at the requested time and returns its
// first point, driving the query-time graph evaluation for that frame. The
// EXPECT_TRUE asserts the value resolves, so a failed graph evaluation surfaces
// as a test failure here. Returns std::nullopt when the resolved array is
// empty, letting the caller distinguish "no point" from a point at the origin.
std::optional<PXR_NS::GfVec3f> firstPointAt(
    const PXR_NS::UsdAttribute& pointsAttr, PXR_NS::UsdTimeCode time);

/// Query each frame in the range in ascending order to ensure that
/// the simulation is evaluated in order.
void checkMoveUpPointSimulation(const PXR_NS::UsdStageRefPtr& stage,
                                double                        startFrame,
                                double                        endFrame,
                                double                        fps = 1.0,
                                float verticalStep                = 1.0f);

} // namespace DffTestHelpers

#endif // BIFROSTUSD_DFF_LAYER_HELPERS_H
