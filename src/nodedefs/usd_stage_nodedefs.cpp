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

#include "usd_stage_nodedefs.h"

#include <Amino/Core/String.h>
#include <Bifrost/FileUtils/FileUtils.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stageCacheContext.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdUtils/stageCache.h>

#include <string>

#include "return_guard.h"
#include "usd_type_converter.h"
#include "usd_utils.h"

using namespace USDUtils;
using namespace USDTypeConverters;

namespace {
template <typename T>
bool set_stage_metadata_impl(const Amino::String&   key,
                             T&&                    value,
                             BifrostUsd::Stage&     stage) {
    if (!stage) return false;

    try {
        return stage->SetMetadata(GetSdfFieldKey(key), toPxr(value));

    } catch (std::exception& e) {
        log_exception("set_stage_metadata", e);
    }
    return false;
}
template <typename T>
bool get_stage_metadata_impl(const BifrostUsd::Stage&   stage,
                             const Amino::String&       key,
                             const T&                   default_and_type,
                             T&                         value) {
    if (!stage) return false;

    try {
        if (stage->GetMetadata(GetSdfFieldKey(key), &value)) {
            return true;
        } else {
            value = default_and_type;
        }
    } catch (std::exception& e) {
        log_exception("get_prim_metadata", e);
        value = default_and_type;
    }
    return false;
}
template <typename... Args>
Amino::MutablePtr<BifrostUsd::Stage> createStage(Args&&... args) {
    return Amino::newMutablePtr<BifrostUsd::Stage>(
        std::forward<Args>(args)...);
}
Amino::MutablePtr<BifrostUsd::Stage> createInvalidStage() {
    return createStage(BifrostUsd::Stage::Invalid{});
}
int getRootOrReversedSublayerIndex(
    const BifrostUsd::Stage&    stage,
    const int                   sublayer_index) {

    if (!stage.isValid() || sublayer_index == -1) {
        return -1;  // use root layer as default EditTarget
    }
    const PXR_NS::SdfLayerHandle sdfLayer = stage.get().GetRootLayer();
    const int numLayers = static_cast<int>(sdfLayer->GetNumSubLayerPaths());
    if (sublayer_index < -1 || sublayer_index >= numLayers) {
        return -1;  // use root layer as default EditTarget
    }
    return USDUtils::reversedSublayerIndex(sublayer_index, numLayers);
}
} // namespace

void USD::Stage::open_stage_from_layer(
    const BifrostUsd::Layer&                root_layer,
    const Amino::Array<Amino::String>&      mask,
    const BifrostUsd::InitialLoadSet        load,
    const int                               layer_index,
    Amino::MutablePtr<BifrostUsd::Stage>&   stage) {
    try {
        stage = [&]() {
            if (!root_layer) {
                return createStage(BifrostUsd::Stage());
            }

            // Make the masked paths
            std::vector<PXR_NS::SdfPath> paths;
            for (size_t i = 0; i < mask.size(); ++i) {
                std::string path(mask[i].c_str());
                paths.push_back(PXR_NS::SdfPath(path));
            }
            return paths.empty()
                       ? createStage(root_layer, load)
                       : createStage(root_layer,
                                     PXR_NS::UsdStagePopulationMask(paths), load);
        }();

        // Reverse the given index to match the order of sublayers
        // in the Pixar USD Layer:
        int reversedIndex = getRootOrReversedSublayerIndex(
            *stage, layer_index);

        // Set the EditTarget layer in opened stage, forcing it to root
        // layer if given index is invalid:
        stage->setEditLayerIndex(reversedIndex, true);

    } catch (std::exception& e) {
        log_exception("open_stage_from_layer", e);
        stage = createInvalidStage();
    }

    assert(stage);
}

/// \todo BIFROST-6406 open_from_cache/send_to_cache don't seem safe in a
/// value semantics world. It should likely be reviewed.
void USD::Stage::open_stage_from_cache(const Amino::long_t              id,
                                       const int                        layer_index,
                                       Amino::Ptr<BifrostUsd::Stage>&   stage) {
    auto stage_returns = createReturnGuard(stage);
    try {
        // Requires cast to long int on MSVC to compile without errors.
        auto pxr_stage = PXR_NS::UsdUtilsStageCache::Get().Find(
            PXR_NS::UsdStageCache::Id::FromLongInt(static_cast<long int>(id)));

        if (pxr_stage) {
            auto stage_ =
                Amino::newMutablePtr<BifrostUsd::Stage>(BifrostUsd::Layer(
                    pxr_stage->GetRootLayer()->GetIdentifier().c_str(),
                    pxr_stage->GetRootLayer()->GetDisplayName().c_str()));

            // Reverse the given index to match the order of sublayers
            // in the Pixar USD Layer:
            int reversedIndex = getRootOrReversedSublayerIndex(
                *stage_, layer_index);

            // Set the EditTarget layer in opened stage, forcing it to root
            // layer if given index is invalid:
            stage_->setEditLayerIndex(reversedIndex, true);

            stage = std::move(stage_);
        }
    } catch (std::exception& e) {
        log_exception("open_stage_from_cache", e);
        stage = createInvalidStage();
    }

    assert(stage);
}

void USD::Stage::set_edit_layer(BifrostUsd::Stage&  stage,
                                const int           layer_index) {
    if (!stage) {
        return;
    }
    try {
        // First verify given index. We do nothing if it is invalid:
        const PXR_NS::SdfLayerHandle sdfLayer = stage.get().GetRootLayer();
        const int numLayers = static_cast<int>(sdfLayer->GetNumSubLayerPaths());
        if (layer_index >= -1 && layer_index < numLayers) {
            // Reverse the given index to match the order of sublayers
            // in the Pixar USD Layer:
            int reversedIndex = (layer_index == -1) ? -1 :
                USDUtils::reversedSublayerIndex(layer_index, numLayers);
            stage.setEditLayerIndex(reversedIndex, false);
        }
    } catch (std::exception& e) {
        log_exception("set_edit_layer", e);
    }
}

void USD::Stage::set_default_prim(BifrostUsd::Stage& stage,
                                  const Amino::String& prim_path) {
    if (!stage) return;
    try {
        auto prim = get_prim_at_path(prim_path, stage);
        if (prim) {
            stage->SetDefaultPrim(prim);
        }

    } catch (std::exception& e) {
        log_exception("set_default_prim", e);
    }
}

void USD::Stage::get_default_prim(const BifrostUsd::Stage& stage,
                                  Amino::String&             prim_path) {
    try {
        if (stage) {
            prim_path = stage->GetDefaultPrim().GetPrimPath().GetText();
        }
    } catch (std::exception& e) {
        log_exception("get_default_prim", e);
    }
}

bool USD::Stage::set_stage_up_axis(BifrostUsd::Stage&       stage,
                                   const BifrostUsd::UpAxis axis) {
    if (!stage) return false;

    try {
        // UsdStage upAxis can only be set to "Y" or "Z"
        PXR_NS::TfToken pxr_axis = [axis] {
            switch (axis) {
                case BifrostUsd::UpAxis::Y: return PXR_NS::UsdGeomTokens->y;
                case BifrostUsd::UpAxis::Z: return PXR_NS::UsdGeomTokens->z;
            }
            assert(false); // Should have returned in switch
            return PXR_NS::TfToken{};
        }();

        return PXR_NS::UsdGeomSetStageUpAxis(stage.getStagePtr(), pxr_axis);

    } catch (std::exception& e) {
        log_exception("set_stage_up_axis", e);
    }
    return false;
}

void USD::Stage::set_stage_time_code(BifrostUsd::Stage& stage,
                                     const float          start,
                                     const float          end) {
    if (!stage) return;

    try {
        stage->SetStartTimeCode(static_cast<double>(start));
        stage->SetEndTimeCode(static_cast<double>(end));

    } catch (std::exception& e) {
        log_exception("set_stage_time_code", e);
    }
}

bool USD::Stage::set_stage_metadata(BifrostUsd::Stage& stage,
                                    const Amino::String& key,
                                    const Amino::String& value) {
    return set_stage_metadata_impl(key, value, stage);
}

bool USD::Stage::set_stage_metadata(BifrostUsd::Stage&  stage,
                                    const Amino::String&  key,
                                    const Amino::float_t& value) {
    return set_stage_metadata_impl(key, value, stage);
}

bool USD::Stage::set_stage_metadata(BifrostUsd::Stage&   stage,
                                    const Amino::String&   key,
                                    const Amino::double_t& value) {
    return set_stage_metadata_impl(key, value, stage);
}

bool USD::Stage::set_stage_metadata(BifrostUsd::Stage& stage,
                                    const Amino::String& key,
                                    const Amino::int_t&  value) {
    return set_stage_metadata_impl(key, value, stage);
}

bool USD::Stage::set_stage_metadata(BifrostUsd::Stage& stage,
                                    const Amino::String& key,
                                    const Amino::long_t& value) {
    return set_stage_metadata_impl(key, value, stage);
}

bool USD::Stage::set_stage_metadata(BifrostUsd::Stage& stage,
                                    const Amino::String& key,
                                    const Amino::bool_t& value) {
    return set_stage_metadata_impl(key, value, stage);
}

bool USD::Stage::set_stage_metadata(BifrostUsd::Stage&   stage,
                                    const Amino::String&   key,
                                    const Bifrost::Object& value) {
    return set_stage_metadata_impl(key, value, stage);
}

bool USD::Stage::get_stage_metadata(const BifrostUsd::Stage& stage,
                                    const Amino::String&       key,
                                    const Amino::String&       default_and_type,
                                    Amino::String&             value) {
    try {
        // Use a temp that is compatible with a "Basic string"
        // The set_stage_metadata method stored a "Basic string"
        std::string temp;
        if (stage && stage->GetMetadata(GetSdfFieldKey(key), &temp)) {
            value = temp.c_str();
            return true;
        } else {
            value = default_and_type;
        }

    } catch (std::exception& e) {
        log_exception("get_stage_metadata", e);
        value = default_and_type;
    }
    return false;
}

bool USD::Stage::get_stage_metadata(const BifrostUsd::Stage& stage,
                                    const Amino::String&       key,
                                    const Amino::float_t&      default_and_type,
                                    Amino::float_t&            value) {
    return get_stage_metadata_impl(stage, key, default_and_type, value);
}

bool USD::Stage::get_stage_metadata(const BifrostUsd::Stage& stage,
                                    const Amino::String&       key,
                                    const Amino::double_t&     default_and_type,
                                    Amino::double_t&           value) {
    return get_stage_metadata_impl(stage, key, default_and_type, value);
}

bool USD::Stage::get_stage_metadata(const BifrostUsd::Stage& stage,
                                    const Amino::String&       key,
                                    const Amino::int_t&        default_and_type,
                                    Amino::int_t&              value) {
    return get_stage_metadata_impl(stage, key, default_and_type, value);
}

bool USD::Stage::get_stage_metadata(const BifrostUsd::Stage& stage,
                                    const Amino::String&       key,
                                    const Amino::long_t&       default_and_type,
                                    Amino::long_t&             value) {
    return get_stage_metadata_impl(stage, key, default_and_type, value);
}

bool USD::Stage::get_stage_metadata(const BifrostUsd::Stage& stage,
                                    const Amino::String&       key,
                                    const Amino::bool_t&       default_and_type,
                                    Amino::bool_t&             value) {
    return get_stage_metadata_impl(stage, key, default_and_type, value);
}

bool USD::Stage::get_stage_metadata(
    const BifrostUsd::Stage&         stage,
    const Amino::String&               key,
    const Amino::Ptr<Bifrost::Object>& default_and_type,
    Amino::Ptr<Bifrost::Object>&       value) {
    auto value_returns = createReturnGuard(
        value, [&default_and_type]() { return default_and_type; });

    try {
        PXR_NS::VtDictionary temp;
        if (stage && stage->GetMetadata(GetSdfFieldKey(key), &temp)) {
            value = fromPxr(temp);
            return true;
        } else {
            value = default_and_type;
        }

    } catch (std::exception& e) {
        log_exception("get_stage_metadata", e);
    }
    return false;
}

bool USD::Stage::save_stage(const BifrostUsd::Stage& stage,
                            const Amino::String&       file) {
    try {
        return stage && (file.empty() ? stage.save() : stage.save(file));

    } catch (std::exception& e) {
        log_exception("save_stage", e);
    }
    return false;
}

/// \todo BIFROST-6406 open_from_cache/send_to_cache don't seem safe in a
/// value semantics world. It should likely be reviewed.
Amino::long_t USD::Stage::send_stage_to_cache(
    const Amino::Ptr<BifrostUsd::Stage>& stage) {
    assert(stage);

    try {
        if (stage && *stage) {
            auto usdStage =
                const_cast<BifrostUsd::Stage&>(*stage).getStagePtr();
            return PXR_NS::UsdUtilsStageCache::Get().Insert(usdStage).ToLongInt();
        }

    } catch (std::exception& e) {
        log_exception("send_stage_to_cache", e);
    }
    return -1;
}

void USD::Stage::export_stage_to_string(const BifrostUsd::Stage& stage,
                                        Amino::String&             result) {
    if (!stage) return;

    try {
        // Export the stage

        std::string flattened;
        if (stage->ExportToString(&flattened)) {
            result = flattened.c_str();
        }

    } catch (std::exception& e) {
        log_exception("export_stage_to_string", e);
    }
}

bool USD::Stage::export_stage_to_file(const BifrostUsd::Stage& stage,
                                      const Amino::String&       file) {
    if (!stage) return false;

    try {
        // Export the stage
        return stage->Export(file.c_str());

    } catch (std::exception& e) {
        log_exception("export_stage_to_file", e);
    }
    return false;
}
