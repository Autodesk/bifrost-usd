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

/// \brief A library to create C++ callable objects from Bifrost USD compounds.

#ifndef BIFROSTUSD_OBJECT_TO_STAGE_H
#define BIFROSTUSD_OBJECT_TO_STAGE_H

#include "UsdTranslatorExport.h"

// Amino
#include <Amino/Core/Ptr.h>
#include <Amino/Core/String.h>
#include <Amino/Core/ArrayFwd.h>
#include <Amino/Executor/ExecutionState.h>

// BifrostUSD
#include <BifrostUsd/Enum.h>
#include <BifrostUsd/Stage.h>

#define BIFROSTUSD_TRANSLATOR_API BIFROSTUSD_TRANSLATOR_LIBRARY_DECL

namespace Bifrost {
class Object;
}

namespace BifrostUsd::DynamicPayload {

BIFROSTUSD_TRANSLATOR_API
Amino::Ptr<BifrostUsd::Stage> object_to_stage(
    const Amino::Ptr<Bifrost::Object>& object,
    const Amino::String&               layer_name = Amino::String{"object.usd"},
    BifrostUsd::ImageablePurpose       purpose =
        BifrostUsd::ImageablePurpose::Default);

BIFROSTUSD_TRANSLATOR_API
Amino::Ptr<BifrostUsd::Stage> object_to_stage(
    const Amino::Ptr<Bifrost::Object>& object,
    Amino::ExecutionState&             translatorState,
    const Amino::String&               layer_name = Amino::String{"object.usd"},
    BifrostUsd::ImageablePurpose       purpose =
        BifrostUsd::ImageablePurpose::Default,
    float frame            = 0.0f,
    bool  varying_topology = false);

BIFROSTUSD_TRANSLATOR_API
Amino::Ptr<BifrostUsd::Stage> array_of_objects_to_stage(
    const Amino::Ptr<Amino::Array<Amino::Ptr<Bifrost::Object>>>& objects,
    const Amino::String&         layer_name = Amino::String{"objects.usd"},
    BifrostUsd::ImageablePurpose purpose =
        BifrostUsd::ImageablePurpose::Default);

} // namespace BifrostUsd::DynamicPayload

#endif // BIFROSTUSD_OBJECT_TO_STAGE_H
