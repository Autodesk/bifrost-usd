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

/// \brief Factory for creating a GraphExecutor to execute a Bifrost graph.

#ifndef BIFROSTUSD_GRAPH_EXECUTOR_FACTORY_H
#define BIFROSTUSD_GRAPH_EXECUTOR_FACTORY_H

#include "GraphExecutorExport.h"

// Amino
#include <Amino/Core/Array.h>
#include <Amino/Core/String.h>
#include <Amino/Core/StringView.h>

// C++ Standard Library
#include <memory>

using StringArray = Amino::Array<Amino::String>;

namespace BifrostUsd::GraphExecutor {

class GraphExecutor;
using GraphExecutorPtr = std::unique_ptr<GraphExecutor>;

/// \brief Create a GraphExecutor for a given Bifrost compound.
///
/// The GraphExecutor wraps an Amino::Executable and provides the interface to
/// set inputs, execute the graph, and retrieve outputs. If an
/// Amino::Executable already exists for the given compound name, it is cloned
/// rather than recreated, avoiding redundant initialization overhead.
///
/// \param [in] compound_name The name of the Bifrost compound to instantiate.
/// \return If successful, a pointer to a GraphExecutor representing the
/// compound that can be executed; nullptr otherwise.
/// \note This function is thread-safe.
BIFROSTUSD_GRAPH_EXECUTOR_DECL
GraphExecutorPtr makeGraphExecutor(
    Amino::StringView compound_name);

/// \brief Create a GraphExecutor for a given Bifrost compound, with error
/// reporting.
///
/// The GraphExecutor wraps an Amino::Executable and provides the interface to
/// set inputs, execute the graph, and retrieve outputs. If an
/// Amino::Executable already exists for the given compound name, it is cloned
/// rather than recreated, avoiding redundant initialization overhead.
///
/// \param [in] compound_name The name of the Bifrost compound to instantiate.
/// \param [out] errors Output array to receive error messages encountered during
///               creation, if any. This array is cleared at the start of the
///               function call.
/// \return If successful, a pointer to a GraphExecutor representing the
/// compound that can be executed; nullptr otherwise.
/// \note This function is thread-safe.
BIFROSTUSD_GRAPH_EXECUTOR_DECL
GraphExecutorPtr makeGraphExecutor(
    Amino::StringView compound_name, StringArray& errors);

/// \brief Reset and reload the Amino::ConstLibrary singleton.
///
/// Clears all cached Amino::Executables and reinitializes the Amino library
/// from its config files. This is useful when an external application has
/// added or republished some compounds referenced by the Bifrost configs:
/// calling this function makes those changes immediately available for
/// subsequent calls to makeGraphExecutor().
///
/// \warning reloadLibrary() must not be called concurrently with
/// makeGraphExecutor() or while an Amino::Executable is being used. Reloading
/// is expected to happen only at startup or on explicit user request.
BIFROSTUSD_GRAPH_EXECUTOR_DECL
void reloadLibrary();

} // namespace BifrostUsd::GraphExecutor

#endif // BIFROSTUSD_GRAPH_EXECUTOR_FACTORY_H
