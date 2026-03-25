//-
// Copyright 2025 Autodesk, Inc.
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

#include "TypeTranslation.h"

#include <BifrostHydra/Engine/JobTranslationData.h>
#include <BifrostHydra/Engine/ValueTranslationData.h>

#include <Bifrost/Geometry/GeometryTypes.h>
#include <BifrostGraph/Executor/Utility.h>

namespace BifrostHd {

TypeTranslation::TypeTranslation() noexcept
    : BifrostGraph::Executor::TypeTranslation("BifrostHd Translation Table") {}

TypeTranslation::~TypeTranslation() noexcept {}

void TypeTranslation::deleteThis() noexcept { delete this; }

void TypeTranslation::getSupportedTypeNames(
    StringArray& out_names) const noexcept {
    out_names.push_back("bool");
    out_names.push_back("float");
    out_names.push_back("double");
    out_names.push_back("long");
    out_names.push_back("ulong");
    out_names.push_back("int");
    out_names.push_back("uint");
    out_names.push_back("short");
    out_names.push_back("ushort");
    out_names.push_back("char");
    out_names.push_back("uchar");
    out_names.push_back("string");
    out_names.push_back("Math::float3");
    out_names.push_back("Object");
    // for graphs using time nodes
    out_names.push_back("Simulation::Time");
}

bool TypeTranslation::convertValueFromHost(
    const Amino::TypeId& typeId,
    Amino::Any&          value,
    const BifrostGraph::Executor::TypeTranslation::ValueData*
        valueTranslationData) const noexcept {
    assert(
        dynamic_cast<BifrostHd::InputValueData const*>(valueTranslationData));

    auto inputValueData =
        static_cast<const BifrostHd::InputValueData*>(valueTranslationData);

    if (typeId == Amino::getTypeId<Bifrost::Simulation::Time>()) {
        auto time = inputValueData->jobTranslationData().getTime();

        value = Bifrost::Simulation::Time{
            static_cast<long>(1),
            time.currentTime,
            time.currentFrame,
            time.frameLength};
        return true;
    }

    value = inputValueData->getInput();
    return true;
}

bool TypeTranslation::convertValueToHost(
    const Amino::Any&                                   value,
    BifrostGraph::Executor::TypeTranslation::ValueData* valueTranslationData)
    const noexcept {
    assert(dynamic_cast<BifrostHd::OutputValueData*>(valueTranslationData));

    auto outputValueData =
        static_cast<BifrostHd::OutputValueData*>(valueTranslationData);

    return outputValueData->setOutput(value);
}

} // namespace BifrostHd

extern "C" {
BIFROST_HD_TRANSLATION_SHARED_DECL BifrostGraph::Executor::TypeTranslation*
              createBifrostTypeTranslation(void) {
                  return new BifrostHd::TypeTranslation();
}
}
