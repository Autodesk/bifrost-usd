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

/// \file dffExecuteGraphFrameToLayer.h
///
/// \brief Single-frame Bifrost graph evaluation for the BifrostUsd Dynamic
/// File Format.

#ifndef BIFROSTUSD_DFF_EXECUTE_GRAPH_FRAME_TO_LAYER_H
#define BIFROSTUSD_DFF_EXECUTE_GRAPH_FRAME_TO_LAYER_H

#include "dffData.h"

#include <BifrostUsd/GraphExecutor/GraphExecutorTypes.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \brief Execute one frame on the given executor and return the flattened
/// layer assembled from its outputs.
///
/// The \p executor is owned and configured by the caller (it carries the
/// timeline/fps settings and, crucially, the execution state that accumulates
/// across frames for stateful simulations). The graph is executed at \p frame
/// (the graph's \c Simulation::time is set accordingly when the payload is
/// animated) but the resulting USD attribute values are authored as \e defaults
/// so callers can read a single frame's scene uniformly.
///
/// \param [in] executor The configured, persistent executor to run.
/// \param [in] params   The decoded payload parameters.
/// \param [in] frame    The frame to evaluate the graph at.
/// \return The flattened in-memory layer for \p frame, or a null ref pointer
///     if the graph produced no usable output.
SdfLayerRefPtr executeGraphFrameToLayer(
    BifrostUsd::GraphExecutor::GraphExecutorPtr& executor,
    const BifrostDffDataParams&                  params,
    double                                       frame);

/// \brief Execute one frame on the given executor without extracting or
/// converting its outputs to a USD layer.
///
/// Used to replay a frame whose output is already cached (e.g. the start
/// frame, when a backward scrub forces the persistent executor to be
/// recreated and replayed from the beginning): the executor's internal
/// simulation state still needs to be advanced through this frame for later
/// frames' stateful accumulation to be correct, but the resulting output can
/// be discarded, so the (comparatively expensive) output extraction and USD
/// stage construction that \ref executeGraphFrameToLayer performs is skipped.
///
/// \param [in] executor The configured, persistent executor to run.
/// \param [in] params   The decoded payload parameters.
/// \param [in] frame    The frame to evaluate the graph at.
/// \return true if the graph executed successfully; false otherwise.
bool advanceGraphFrame(BifrostUsd::GraphExecutor::GraphExecutorPtr& executor,
                       const BifrostDffDataParams&                  params,
                       double                                       frame);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // BIFROSTUSD_DFF_EXECUTE_GRAPH_FRAME_TO_LAYER_H
