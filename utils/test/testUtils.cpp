//-
// Copyright 2022 Autodesk, Inc.
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

#include "testUtils.h"
#include <pxr/usd/sdf/layer.h>
#include <sstream>
#include <string>

namespace BifrostUsd {

namespace TestUtils {

bool addSubLayers(
    PXR_NS::SdfLayerRefPtr                 sdfRootLayer,
    const Amino::Array<Amino::String>&  subNames,
    Amino::String&                      errorMsg) {

    errorMsg.clear();
    if(sdfRootLayer == nullptr) {
        errorMsg = "sdfRootLayer == nullptr";
        return false;
    }

    // FindOrOpen and add some Pixar sublayers:
    bool success = true;
    int i = 0;
    for(auto it = subNames.begin(); it != subNames.end(); ++i, ++it) {
        const std::string name(it->c_str());
        PXR_NS::SdfLayerRefPtr sdfSublayer{PXR_NS::SdfLayer::FindOrOpen(name)};
        if(sdfSublayer == nullptr) {
            std::ostringstream stream;
            stream << "SdfLayer::FindOrOpen() failed [i="
                << i << "; subName=`" << name << "`]   ";
            errorMsg += stream.str().c_str();
            success = false;
        } else {
            // Make sure the sublayer's Id contains the sublayer's filename,
            // as this is our way to check for their correspondance:
            const std::string sublayerId = sdfSublayer->GetIdentifier();
            auto pos = sublayerId.find(it->c_str());
            if(pos == std::string::npos) {
                std::ostringstream stream;
                stream << "Unexpected SdfLayer identifier [i=" << i
                    << "; subName=`" << name
                    << "`; id=`" << sublayerId
                    << "`]   ";
                errorMsg += stream.str().c_str();
                success = false;
            } else {
                // push each new layer on top of the stack, becoming the strongest:
                sdfRootLayer->InsertSubLayerPath(sdfSublayer->GetIdentifier(), 0);
            }
        }
    }
    return success;
}

bool createBifrostLayers(
    const Amino::Array<Amino::String>&              names,
    Amino::Array<Amino::Ptr<BifrostUsd::Layer>>&    newLayers,
    Amino::String&                                  errorMsg) {

    bool success = true;
    errorMsg.clear();
    newLayers.clear();
    int i = 0;
    for(auto it = names.begin(); it != names.end(); ++i, ++it) {
        const std::string name(it->c_str());
        Amino::Ptr<BifrostUsd::Layer> ptrLayer =
            Amino::newClassPtr<BifrostUsd::Layer>(
                getResourcePath(name.c_str()).c_str(), "");
        if(!ptrLayer || !ptrLayer->isValid()) {
            std::ostringstream stream;
            stream << "BifrostUsd::Layer() failed [i="
                << i << "; name=`" << name << "`]   ";
            errorMsg += stream.str().c_str();
            success = false;
        } else {
            newLayers.push_back(ptrLayer);

            // Make sure the layer's Id contains the layer's filename,
            // as this is our way to check for their correspondance:
            const std::string layerId = ptrLayer->get().GetIdentifier();
            auto pos = layerId.find(name);
            if(pos == std::string::npos) {
                std::ostringstream stream;
                stream << "Unexpected SdfLayer identifier [i=" << i
                    << "; name=`" << name
                    << "`; id=`" << layerId
                    << "`]   ";
                errorMsg += stream.str().c_str();
                success = false;
            }
        }
    }
    return success;
}

bool checkSdfSublayerPaths(
    const PXR_NS::SdfLayer&                sdfRootLayer,
    const Amino::Array<Amino::String>&  subNames,
    Amino::String&                      errorMsg) {
    errorMsg.clear();
    // Retrieve the asset paths of all sublayers.
    // These Pixar asset paths are in order from STRONGEST to WEAKEST.
    const PXR_NS::SdfSubLayerProxy sdfSublayerPaths =
        sdfRootLayer.GetSubLayerPaths();
    const size_t numSubLayers = sdfSublayerPaths.size();
    if(numSubLayers != subNames.size()) {
        std::ostringstream stream;
        stream << "GetSubLayerPaths() error [numPaths=" << numSubLayers
            << "; numSubNames=" << subNames.size() << "]   ";
        errorMsg += stream.str().c_str();
        return false;
    }

    // Check the Paths of all sublayers
    bool success = true;
    for (size_t i = 0; i < numSubLayers; ++i) {
        // Reverse the Pixar USD Layer stack index to match the order of
        // names in the Amino::Array (WEAKEST to STRONGEST):
        size_t arrayIndex = (numSubLayers - 1) - i;
        const std::string name(subNames[arrayIndex].c_str());

        // Check that the Path contains the expected name
        auto pos = sdfSublayerPaths[i].find(name);
        if(pos == std::string::npos) {
            std::ostringstream stream;
            stream << "Unexpected SdfLayer path [i=" << i
                << "; subName=`" << name
                << "`; path=`" << sdfSublayerPaths[i]
                << "`]   ";
            errorMsg += stream.str().c_str();
            success = false;
        }
    }
    return success;
}

} // namespace TestUtils

} // namespace BifrostUsd
