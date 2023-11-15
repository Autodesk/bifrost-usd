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

#include "BifrostGraphGenerativeProceduralPlugin.h"

#include "BifrostGraphGenerativeProcedural.h"

#include <pxr/imaging/hdGp/generativeProceduralPluginRegistry.h>

PXR_NAMESPACE_USING_DIRECTIVE

BifrostGraphGenerativeProceduralPlugin::
    BifrostGraphGenerativeProceduralPlugin() = default;

HdGpGenerativeProcedural* BifrostGraphGenerativeProceduralPlugin::Construct(
    const SdfPath& proceduralPrimPath) {
    return new BifrostGraphGenerativeProcedural(proceduralPrimPath);
}

///////////////////////////////////////////////////////////////////////////////

TF_REGISTRY_FUNCTION(TfType) {
    HdGpGenerativeProceduralPluginRegistry::Define<
        BifrostGraphGenerativeProceduralPlugin,
        HdGpGenerativeProceduralPlugin>();
}
