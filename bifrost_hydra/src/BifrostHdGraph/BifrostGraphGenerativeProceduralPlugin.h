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

#ifndef BIFROST_HD_GRAPH_PROCEDURAL_PLUGIN_H
#define BIFROST_HD_GRAPH_PROCEDURAL_PLUGIN_H

#include <pxr/imaging/hdGp/generativeProceduralPlugin.h>

PXR_NAMESPACE_OPEN_SCOPE

class BifrostGraphGenerativeProceduralPlugin
    : public HdGpGenerativeProceduralPlugin {
public:
    BifrostGraphGenerativeProceduralPlugin();

    HdGpGenerativeProcedural* Construct(
        const SdfPath& proceduralPrimPath) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // BIFROST_HD_GRAPH_PROCEDURAL_PLUGIN_H
