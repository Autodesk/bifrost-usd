//-
// Copyright 2023 Autodesk, Inc.
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

#include <AminoValue.h>

namespace BifrostHd {

TypeTranslation::TypeTranslation()
    : BifrostGraph::Executor::TypeTranslation("BifrostHd Translation Table") {}

TypeTranslation::~TypeTranslation() {}

void TypeTranslation::deleteThis() { delete this; }

void TypeTranslation::getSupportedTypeNames(
    Amino::StringList& out_names) const {
    out_names.add("bool");
    out_names.add("float");
    out_names.add("double");
    out_names.add("long");
    out_names.add("ulong");
    out_names.add("int");
    out_names.add("uint");
    out_names.add("short");
    out_names.add("ushort");
    out_names.add("char");
    out_names.add("uchar");
    out_names.add("string");
    out_names.add("Math::float3");
    out_names.add("Object");
    // for graphs using time nodes
    out_names.add("Simulation::Time");
}

bool TypeTranslation::convertValueFromHost(
    Amino::Type const& type,
    Amino::Value&      value,
    BifrostGraph::Executor::TypeTranslation::ValueTranslationData const*
        valueTranslationData) const {
    assert(dynamic_cast<BifrostHd::ValueTranslationData const*>(
        valueTranslationData));

    auto bifrostHdValueTranslationData =
        static_cast<const BifrostHd::ValueTranslationData*>(
            valueTranslationData);
    const auto& typeName = type.getFullyQualifiedName();

    if (typeName == "Simulation::Time") {
        auto time =
            bifrostHdValueTranslationData->jobTranslationData().getTime();
        Amino::StructValue timeValue(type);
        timeValue["ticks"].setLong(static_cast<long>(1));
        timeValue["time"].setDouble(time.currentTime);
        timeValue["frame"].setDouble(time.currentFrame);
        timeValue["frameLength"].setDouble(time.frameLength);
        value = timeValue;
        return true;
    }

    value = bifrostHdValueTranslationData->getInput(type);
    return true;
}

bool TypeTranslation::convertValueToHost(
    Amino::Value const& value,
    BifrostGraph::Executor::TypeTranslation::ValueTranslationData*
        valueTranslationData) const {
    auto          elementType = value.getType();
    Amino::String typeName    = elementType.getFullyQualifiedName();

    assert(
        dynamic_cast<BifrostHd::ValueTranslationData*>(valueTranslationData));

    auto bifrostHdValueTranslationData =
        static_cast<BifrostHd::ValueTranslationData*>(valueTranslationData);

    return bifrostHdValueTranslationData->setOutput(value);
}

bool TypeTranslation::portAdded(
    Amino::String const& /*name*/,
    PortDirection /*direction*/,
    Amino::Type const& /*type*/,
    Amino::Metadata const& /*metadata*/,
    PortClass /*portClass*/,
    PortTranslationData* /*valueTranslationData*/) const {
    return true;
}

bool TypeTranslation::registerHostPlugins(
    const PluginHostData* /*hostData*/) const {
    return true;
}

bool TypeTranslation::unregisterHostPlugins(
    const PluginHostData* /*hostData*/) const {
    return false;
}

bool TypeTranslation::getDataTypeColorHint(Amino::Type const& /*dataType*/,
                                           Amino::String& /*colorHint*/) const {
    return false;
}

} // namespace BifrostHd

extern "C" {
HD_MODULE_API BifrostGraph::Executor::TypeTranslation*
              createBifrostTypeTranslation(void) {
                  return new BifrostHd::TypeTranslation();
}
}
