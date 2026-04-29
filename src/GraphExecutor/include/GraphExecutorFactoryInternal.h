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

#ifndef BIFROSTUSD_GRAPH_EXECUTOR_FACTORY_INTERNAL_H
#define BIFROSTUSD_GRAPH_EXECUTOR_FACTORY_INTERNAL_H

#include "GraphExecutorExport.h"

// Amino
#include <Amino/Core/Array.h>
#include <Amino/Core/String.h>
#include <Amino/Core/StringView.h>
#include <Amino/Executor/Executable.h>

// C++ Standard Library
#include <memory>

using StringArray = Amino::Array<Amino::String>;

namespace BifrostUsd::GraphExecutor {

/// \brief Internal function to create an Amino::Executable for a compound.
///
/// \param compound_name The name of the Bifrost compound to instantiate.
/// \return If successful, a valid Amino::Executable representing the
/// compound that can be executed; an invalid Amino::Executable otherwise.
/// \note This function is thread-safe.
BIFROSTUSD_GRAPH_EXECUTOR_DECL
Amino::Executable makeExecutable(Amino::StringView compound_name);

/// \brief Internal function to create an Amino::Executable for a compound,
/// with error reporting.
///
/// \param compound_name The name of the Bifrost compound to instantiate.
/// \param errors Output array to receive error messages encountered during
///               creation, if any. This array is cleared at the start of the
///               function call.
/// \return If successful, a valid Amino::Executable representing the
/// compound that can be executed; an invalid Amino::Executable otherwise.
/// \note This function is thread-safe.
BIFROSTUSD_GRAPH_EXECUTOR_DECL
Amino::Executable makeExecutable(Amino::StringView compound_name,
                                 StringArray&      errors);

} // namespace BifrostUsd::GraphExecutor

#endif // BIFROSTUSD_GRAPH_EXECUTOR_FACTORY_INTERNAL_H
