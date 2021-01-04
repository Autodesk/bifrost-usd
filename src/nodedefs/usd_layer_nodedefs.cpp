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

#include "usd_layer_nodedefs.h"

#include <Amino/Core/String.h>
#include <Bifrost/FileUtils/FileUtils.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <limits>

#include "return_guard.h"
#include "usd_utils.h"

using namespace USDUtils;

namespace {
Amino::MutablePtr<BifrostUsd::Layer> createInvalidLayer() {
    return Amino::newMutablePtr<BifrostUsd::Layer>(
        BifrostUsd::Layer::Invalid{});
}
Amino::Ptr<BifrostUsd::Layer> getInvalidLayer() {
    static Amino::Ptr<BifrostUsd::Layer> invalid = createInvalidLayer();
    return invalid;
}
} // namespace

void USD::Layer::get_root_layer(const BifrostUsd::Stage&        stage,
                                Amino::Ptr<BifrostUsd::Layer>&  layer) {
    layer = stage ? stage.getRootLayer() : getInvalidLayer();
    assert(layer);
}

void USD::Layer::get_layer(const BifrostUsd::Stage&         stage,
                           const int                        layer_index,
                           Amino::Ptr<BifrostUsd::Layer>&   layer) {
    // Use a lambda to ensure that layer is always assigned (from all branches).
    layer = [&stage, layer_index]() {
        if (!stage) {
            return getInvalidLayer();
        }
        // Special case to get root layer
        if (layer_index == -1) {
            return stage.getRootLayer();
        }
        // Check limits
        const size_t numLayers =
            stage.getRootLayer()->get().GetNumSubLayerPaths();
        if (layer_index < 0 ||
            numLayers > std::numeric_limits<int>::max() ||
            layer_index >= static_cast<int>(numLayers)) {
            return getInvalidLayer();
        }
        // Reverse the given index to match the order of sublayers
        // in the Pixar USD Layer:
        int reversedIndex = USDUtils::reversedSublayerIndex(
            layer_index, static_cast<int>(numLayers));
        // Get the sublayer
        const BifrostUsd::Layer& sublayer =
            stage.getRootLayer()->getSubLayer(reversedIndex);
        if (!sublayer) {
            return getInvalidLayer();
        }
        return Amino::Ptr<BifrostUsd::Layer>{
            Amino::newClassPtr<BifrostUsd::Layer>(sublayer)};
    }();
    assert(layer);
}

bool USD::Layer::replace_layer(BifrostUsd::Stage&       stage,
                               const int                sublayer_index,
                               const BifrostUsd::Layer& new_layer) {
    try {
        // First make sure replaceSubLayer() below would not fail for obvious
        // reasons:
        if (!stage || !stage.getRootLayer()->isValid() ||
            !stage.getRootLayer()->get().PermissionToEdit() ||
            !new_layer.isValid()) {
            return false;
        }
        // Should be the unique owner of the Root Layer.
        // Otherwise this could produce side effects.
        assert(stage.getRootLayer().use_count() == 1);
        // Check limits
        const size_t numLayers =
            stage.getRootLayer()->get().GetNumSubLayerPaths();
        if (sublayer_index < 0 || numLayers > std::numeric_limits<int>::max() ||
            sublayer_index >= static_cast<int>(numLayers)) {
            return false;
        }
        // Reverse the given index to match the order of sublayers
        // in the Pixar USD Layer:
        const int reversedIndex = USDUtils::reversedSublayerIndex(
            sublayer_index, static_cast<int>(numLayers));
        // Memorize current EditTarget.
        const int prevEditLayerIndex = stage.getEditLayerIndex();
        if (prevEditLayerIndex == reversedIndex) {
            // We can't let the Pixar USD stage continue to refer to the
            // current EditTarget while we replace it. Temporarily change
            // it to the root layer.
            stage.setEditLayerIndex(-1, false);
        }
        // Replace the sublayer
        auto root_layer = Amino::createPtrGuard(stage.getRootLayer(),
                                                Amino::PtrGuardUniqueFlag{});
        bool replaced   = root_layer->replaceSubLayer(new_layer, reversedIndex);
        if (prevEditLayerIndex == reversedIndex) {
            // Restore the EditTarget to what it was
            stage.setEditLayerIndex(prevEditLayerIndex, false);
        }
        return replaced;
    } catch (std::exception& e) {
        log_exception("replace_layer", e);
    }
    return false;
}

void USD::Layer::create_layer(const Amino::String&                  save_file,
                              Amino::MutablePtr<BifrostUsd::Layer>& layer) {
    // We use the name of the usd file for the tag
    auto tag = Bifrost::FileUtils::extractFilename(save_file);

    layer = Amino::newMutablePtr<BifrostUsd::Layer>(tag);
    try {
        if (!save_file.empty()) {
            layer->setFilePath(save_file);
        }
    } catch (std::exception& e) {
        log_exception("create_layer", e);
    }
    assert(layer);
}

void USD::Layer::open_layer(const Amino::String&                    file,
                            const Amino::String&                    save_file,
                            const bool                              read_only,
                            Amino::MutablePtr<BifrostUsd::Layer>&   layer) {
    // Use a lambda to ensure that layer is always assigned (from all branches).
    layer = [&file, &save_file, read_only]() {
        if (file.empty()) {
            return Amino::newMutablePtr<BifrostUsd::Layer>("empty");
        }
        if (read_only) {
            auto pxr_layer = pxr::SdfLayer::FindOrOpen(file.c_str());
            if (pxr_layer) {
                return Amino::newMutablePtr<BifrostUsd::Layer>(pxr_layer,
                                                                 false);
            }
            return createInvalidLayer();
        }
        auto savefilePath = save_file.empty() ? file : save_file;
        return Amino::newMutablePtr<BifrostUsd::Layer>(file, /*tag=*/"",
                                                         savefilePath);
    }();
    assert(layer);
}

void USD::Layer::duplicate_layer(
    const BifrostUsd::Layer&                layer,
    const Amino::String&                    save_file,
    Amino::MutablePtr<BifrostUsd::Layer>&   new_layer) {
    new_layer = layer ? Amino::newMutablePtr<BifrostUsd::Layer>(
                            layer, save_file.c_str())
                      : createInvalidLayer();
    assert(new_layer);
}

void USD::Layer::get_layer_identifier(const BifrostUsd::Layer&  layer,
                                      Amino::String&            identifier) {
    try {
        if (layer) {
            identifier = layer->GetIdentifier().c_str();
        }
    } catch (std::exception& e) {
        log_exception("get_layer_identifier", e);
    }
}

void USD::Layer::get_layer_file_path(const BifrostUsd::Layer&   layer,
                                     Amino::String&             file) {
    file = layer.getFilePath().c_str();
}

void USD::Layer::export_layer_to_string(const BifrostUsd::Layer&    layer,
                                        const bool                  export_sub_layers,
                                        Amino::String&              result) {
    try {
        // Export the layer
        if (layer) {
            result = layer.exportToString(export_sub_layers).c_str();
        }
    } catch (std::exception& e) {
        log_exception("export_layer_to_string", e);
    }
}

bool USD::Layer::export_layer_to_file(const BifrostUsd::Layer&  layer,
                                      const Amino::String&      file,
                                      const bool                relative_path) {
    try {
        // Export the layer
        return layer && layer.exportToFile(file, relative_path);
    } catch (std::exception& e) {
        log_exception("export_layer_to_file", e);
    }
    return false;
}

void USD::Layer::get_sublayer_paths(
    const BifrostUsd::Stage&                        stage,
    Amino::MutablePtr<Amino::Array<Amino::String>>& sub_layer_paths) {
    sub_layer_paths = Amino::newMutablePtr<Amino::Array<Amino::String>>();
    try {
        if (stage) {
            auto sdf_layer     = stage->GetRootLayer();
            auto subLayerPaths = sdf_layer->GetSubLayerPaths();

            const size_t numLayers = subLayerPaths.size();
            sub_layer_paths->resize(numLayers);
            for (size_t i = 0; i < numLayers; ++i) {
                // Sublayers at the nodedef level are exposed in the
                // reverse order of sublayers in a Pixar USD layer.
                // So we need to invert the order of Paths obtained by the
                // SdfLayer:
                const size_t arrayIndex = USDUtils::reversedSublayerIndex(
                    i, numLayers);
                std::string const& strpath      = subLayerPaths[i];
                (*sub_layer_paths)[arrayIndex]  = strpath.c_str();
            }
        }
    } catch (std::exception& e) {
        log_exception("get_sublayer_paths", e);
    }
    assert(sub_layer_paths);
}

void USD::Layer::add_sublayer(const BifrostUsd::Layer& sub_layer,
                              BifrostUsd::Layer&       layer) {
    try {
        if (layer && sub_layer) {
            if (!layer.insertSubLayer(sub_layer, 0)) {
                std::string msg = "Failed to add sublayer with Identifier=`";
                msg += sub_layer->GetIdentifier() + "`";
                throw std::runtime_error(msg);
            }
        }
    } catch (std::exception& e) {
        log_exception("add_sublayer", e);
    }
}

void USD::Layer::add_sublayer(
    const Amino::Array<Amino::Ptr<BifrostUsd::Layer>>& sub_layer,
    BifrostUsd::Layer&                                 layer) {
    try {
        if (layer) {
            // Sublayers at the nodedef level are exposed in the
            // reverse order of sublayers in a Pixar USD layer.
            // So we insert each new sublayer at the beginning of the
            // sublayers of the Pixar USD Layer, the last one we add
            // then becomes the first sublayer of the Pixar USD layer:
            for (size_t i = 0; i < sub_layer.size(); ++i) {
                if (sub_layer[i] && sub_layer[i]->isValid()) {
                    if (!layer.insertSubLayer(*sub_layer[i], 0)) {
                        std::string msg = "Failed to add sublayer with Identifier=`";
                        msg += sub_layer[i]->get().GetIdentifier() + "`";
                        throw std::runtime_error(msg);
                    }
                }
            }
        }
    } catch (std::exception& e) {
        log_exception("add_sublayer (array overload)", e);
    }
}
