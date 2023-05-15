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

#include <BifrostUsd/Layer.h>

#include <Bifrost/FileUtils/FileUtils.h>

#include <Amino/Cpp/ClassDefine.h>

/// \todo BIFROST-6874 remove pxr::Work_EnsureDetachedTaskProgress();
#include <pxr/base/work/detachedTask.h>

// Note: To silence warnings coming from USD library
#include <bifusd/config/CfgWarningMacros.h>
BIFUSD_WARNING_PUSH

BIFUSD_WARNING_DISABLE_MSC(4003)
BIFUSD_WARNING_DISABLE_MSC(4244)
BIFUSD_WARNING_DISABLE_MSC(4305)
BIFUSD_WARNING_DISABLE_MSC(4800)

#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

BIFUSD_WARNING_POP

#include <Amino/Cpp/ClassDefine.h>

#include <string>

namespace {

const std::string defaultStem("bifrost");
const std::string dotUsdExtension(".usd");
const std::string dotUsdaExtension(".usda");
const std::string dotUsdcExtension(".usdc");

Amino::String getFilenameWithValidUsdFileFormat(const Amino::String& filename = "") {
    size_t nameSize = filename.size();
    size_t extSize = 0;

    auto matchExtension = [&filename, nameSize, &extSize]() {
        extSize = dotUsdExtension.size();
        if (nameSize >= extSize &&
            filename.compare(nameSize - extSize, extSize, dotUsdExtension.c_str()) == 0) {
            return true;
        }
        extSize = dotUsdaExtension.size();
        if (nameSize >= extSize &&
            filename.compare(nameSize - extSize, extSize, dotUsdaExtension.c_str()) == 0) {
            return true;
        }
        extSize = dotUsdcExtension.size();
        if (nameSize >= extSize &&
            filename.compare(nameSize - extSize, extSize, dotUsdcExtension.c_str()) == 0) {
            return true;
        }
        extSize = 0;
        return false;
    };

    Amino::String name;
    if (nameSize == 0) {
        // No filename is provided. Use default stem + default extension
        name = Amino::String(defaultStem.data(), defaultStem.size());
        name.append(dotUsdExtension.data(), dotUsdExtension.size());
    } else if (matchExtension()) {
        // A valid extension is already present
        if (nameSize == extSize) {
            // Only an extension is provided. Prepend a default stem
            name = Amino::String(defaultStem.data(), defaultStem.size());
            name.append(filename);
        } else {
            name = filename;
        }
    } else {
        // No valid extension. Append default extension
        name = filename;
        name.append(dotUsdExtension.data(), dotUsdExtension.size());
    }
    return name;
}

BifrostUsd::Layer const s_invalidLayer{BifrostUsd::Layer::Invalid{}};

struct Sublayer {
    pxr::SdfLayerRefPtr sublayer;
    Amino::String       sublayerPath;
};

Amino::Array<Sublayer> getSublayers(const pxr::SdfLayerRefPtr layer) {
    Amino::Array<Sublayer> layers;
    if (layer) {
        auto subLayerPaths = layer->GetSubLayerPaths();
        for (size_t i = 0; i < subLayerPaths.size(); ++i) {
            auto sublayer = pxr::SdfLayer::FindOrOpenRelativeToLayer(
                layer, subLayerPaths[i]);
            std::string sublayerPath = subLayerPaths[i];
            if (sublayer) {
                layers.push_back(Sublayer{sublayer, sublayerPath.c_str()});
            }
        }
    }
    return layers;
}

} // namespace

namespace BifrostUsd {

Layer::Layer(Layer::Invalid) { assert(!isValid()); }

Layer::Layer(const Amino::String& tag) {
    m_tag = getTagWithValidUsdFileFormat(tag);
    m_layer = pxr::SdfLayer::CreateAnonymous(m_tag.c_str());
}

Layer::Layer(const Amino::String& filePath,
             const Amino::String& tag,
             const Amino::String& savefilePath,
             const bool           isEditable) {
    m_filePath = savefilePath.empty() ? "" :
        getPathWithValidUsdFileFormat(savefilePath);
    m_originalFilePath = filePath.empty() ? "" :
        getPathWithValidUsdFileFormat(filePath);

    if (tag.empty()) {
        // If saveFilePath is not empty, we use its name else with we use the
        // name of filePath for the tag.
        auto path = savefilePath.empty() ? filePath : savefilePath;
        m_tag     = getTagWithValidUsdFileFormat(path);
    } else {
        m_tag = getTagWithValidUsdFileFormat(tag);
    }

    // Temporary Stage that will help in loading relative files and give us
    // access to the sublayers
    auto stage = pxr::UsdStage::Open(m_originalFilePath.c_str());
    if (stage) {
        if (isEditable) {
            m_layer = pxr::SdfLayer::CreateAnonymous(m_tag.c_str());
            m_layer->TransferContent(stage->GetRootLayer());
            auto sublayers = getSublayers(stage->GetRootLayer());
            // Clear the sublayerPaths
            m_layer->SetSubLayerPaths(std::vector<std::string>());
            // Re-create and add the subLayers and subLayerPaths:
            for (size_t i = 0; i < sublayers.size(); ++i) {
                m_subLayers.push_back(Layer(sublayers[i].sublayer, true,
                                            sublayers[i].sublayerPath));
                m_layer->InsertSubLayerPath(m_subLayers[i]->GetIdentifier(),
                                            static_cast<int>(i));
            }
        } else {
            m_layer = stage->GetRootLayer();
            m_layer->SetPermissionToEdit(false);
            auto sublayers = getSublayers(stage->GetRootLayer());
            // block edits from sublayers
            for (size_t i = 0; i < sublayers.size(); ++i) {
                sublayers[i].sublayer->SetPermissionToEdit(false);
                m_subLayers.push_back(Layer(sublayers[i].sublayer, false,
                                            sublayers[i].sublayerPath));
            }
        }
    }
}

Layer::Layer(const pxr::SdfLayerRefPtr layer,
             const bool                isEditable,
             const Amino::String&      originalFilePath) {
    auto validOriginalPath = originalFilePath.empty() ? "" :
        getPathWithValidUsdFileFormat(originalFilePath);

    m_filePath         = isEditable ? validOriginalPath : "";
    m_originalFilePath = (originalFilePath.empty() && layer != nullptr)
                            ? layer->GetIdentifier().c_str()
                            : validOriginalPath;

    // If the given SdfLayer is anonymous, it may have no tag, and hence no
    // display name, or it may have a display name with no valid USD file
    // extension. Make sure the tag we store is valid (non-empty with a valid
    // extension):
    m_tag = getTagWithValidUsdFileFormat(layer != nullptr
                                            ? layer->GetDisplayName().c_str()
                                            : "");
    if (layer == nullptr) {
        return;
    }
    auto sublayers = getSublayers(layer);
    if (isEditable) {
        m_layer = pxr::SdfLayer::CreateAnonymous(m_tag.c_str());
        m_layer->TransferContent(layer);
        // Clear the sublayerPaths
        m_layer->SetSubLayerPaths(std::vector<std::string>());
        // Re-create and add the subLayers and subLayerPaths:
        for (size_t i = 0; i < sublayers.size(); ++i) {
            sublayers[i].sublayer->SetPermissionToEdit(true);
            m_subLayers.push_back(
                Layer(sublayers[i].sublayer, true, sublayers[i].sublayerPath));
            m_layer->InsertSubLayerPath(m_subLayers[i]->GetIdentifier(),
                                        static_cast<int>(i));
        }
    } else {
        m_layer = layer;
        m_layer->SetPermissionToEdit(false);
        // block edits from sublayers
        for (size_t i = 0; i < sublayers.size(); ++i) {
            sublayers[i].sublayer->SetPermissionToEdit(false);
            m_subLayers.push_back(
                Layer(sublayers[i].sublayer, false, sublayers[i].sublayerPath));
        }
    }
}

Layer::Layer(const Layer& other, const Amino::String& savefilePath)
    : m_filePath(savefilePath.empty() ? other.m_filePath :
        getPathWithValidUsdFileFormat(savefilePath)),
      m_originalFilePath(other.m_originalFilePath),
      m_tag(other.m_tag),
      m_subLayers(other.m_subLayers) {

    if (!other.isValid()) {
        return;
    }

    if (other.m_layer->PermissionToEdit()) {
        m_layer = pxr::SdfLayer::CreateAnonymous(m_tag.c_str());
        m_layer->TransferContent(other.m_layer);

        // Replace sublayers coming from the other layer by the one created by
        // m_subLayers assignement.
        for (int i = 0; i < static_cast<int>(m_subLayers.size()); ++i) {
            m_layer->RemoveSubLayerPath(i);
            m_layer->InsertSubLayerPath(m_subLayers[i]->GetIdentifier(), i);
        }
    } else {
        m_layer = other.m_layer;

#ifndef NDEBUG
        auto subLayerPaths = m_layer->GetSubLayerPaths();
        for (int i = 0; i < static_cast<int>(m_subLayers.size()); ++i) {
            assert(subLayerPaths[i] ==
                   m_subLayers[i].m_originalFilePath.c_str());
        }
#endif
    }
}

Layer::Layer(const Layer& other) : Layer(other, "") {}

Layer& Layer::operator=(const Layer& other) {
    return *this = Layer(other);
}

Layer::Layer(Layer&& other) noexcept {
    swap(*this, other);
}

Layer& Layer::operator=(Layer&& other) noexcept {
    swap(*this, other);
    return *this;
}

bool Layer::operator==(const Layer& rhs) const {
    if (m_layer != rhs.m_layer || m_filePath != rhs.m_filePath ||
        m_originalFilePath != rhs.m_originalFilePath || m_tag != rhs.m_tag ||
        m_subLayers.size() != rhs.m_subLayers.size()) {
        return false;
    }

    for (std::size_t i = 0; i < m_subLayers.size(); ++i) {
        if (m_subLayers[i] != rhs.m_subLayers[i]) {
            return false;
        }
    }

    return true;
}

bool Layer::operator!=(const Layer& rhs) const { return !operator==(rhs); }

void Layer::setFilePath(const Amino::String& filePath) {
    m_filePath = filePath.empty() ? "" :
        getPathWithValidUsdFileFormat(filePath);
}

const Amino::String& Layer::getFilePath() const { return m_filePath; }

const Amino::String& Layer::getOriginalFilePath() const {
    return m_originalFilePath;
}

const Amino::Array<Layer>& Layer::getSubLayers() const { return m_subLayers; }

bool Layer::insertSubLayer(const Layer& layer, int index) {
    if (!isValid() || !layer.isValid() || !m_layer->PermissionToEdit() ||
        index < -1 ||
        index > static_cast<int>(m_layer->GetNumSubLayerPaths())) {
        return false;
    }
    if (index == -1) {
        // special case: the new sublayer is added at the bottom of the
        // stack and will become the weakest
        index = static_cast<int>(m_layer->GetNumSubLayerPaths());
    }

    m_subLayers.insert(m_subLayers.begin() + index, layer);
    m_layer->InsertSubLayerPath(m_subLayers[index]->GetIdentifier(), index);

    return true;
}

bool Layer::replaceSubLayer(const Layer& layer, int index) {
    if (!isValid() || !layer.isValid() || !m_layer->PermissionToEdit()) {
        return false;
    }
    const size_t numLayers = m_layer->GetNumSubLayerPaths();
    if (index < -1 || (index == -1 && numLayers == 0) ||
        index >= static_cast<int>(numLayers)) {
        return false;
    }
    if (index == -1) {
        // special case: we replace the current weakest sublayer
        index = getWeakestSubLayerIndex();
        assert(index >= 0 && index < static_cast<int>(numLayers));
    }

    // replace the sublayer
    m_subLayers[index] = layer;

    m_layer->RemoveSubLayerPath(index);
    m_layer->InsertSubLayerPath(m_subLayers[index]->GetIdentifier(), index);

    return true;
}

Layer const& Layer::getSubLayer(int index) const {
    if (!isValid()) {
        return s_invalidLayer;
    }
    const size_t numLayers = m_layer->GetNumSubLayerPaths();
    if (index < -1 || (index == -1 && numLayers == 0) ||
        index >= static_cast<int>(numLayers)) {
        return s_invalidLayer;
    }
    if (index == -1) {
        // special case: we return the current weakest sublayer
        index = getWeakestSubLayerIndex();
        assert(index >= 0 && index < static_cast<int>(numLayers));
    }
    return m_subLayers[index];
}

bool Layer::exportToFile(const Amino::String& filePath,
                         bool                 relativePath) const {
    std::string outFilePath =
        filePath.empty() ? m_filePath.c_str() :
            getPathWithValidUsdFileFormat(filePath).c_str();
    if (outFilePath.empty() || !m_layer->PermissionToEdit()) {
        return false;
    }

    auto outLayer = pxr::SdfLayer::CreateNew(outFilePath);
    if (!outLayer) {
        return false;
    }
    outLayer->TransferContent(m_layer);

    // Replace anonymous sublayer identifier by real layer file path
    for (int i = 0; i < static_cast<int>(m_subLayers.size()); ++i) {
        const auto& layer = m_subLayers[i];
        Amino::String sdfLayerIdentifier = layer.m_filePath.empty() ?
            layer.m_originalFilePath : layer.m_filePath;
        if (relativePath) {
            Amino::String relPath = "";
            if (Bifrost::FileUtils::getRelativePath(
                    sdfLayerIdentifier,
                    Bifrost::FileUtils::extractParentPath(outFilePath.c_str())
                        .c_str(),
                    relPath)) {
                sdfLayerIdentifier = relPath;
                std::replace(sdfLayerIdentifier.begin(), sdfLayerIdentifier.end(), '\\', '/');
            }
        }
        if (sdfLayerIdentifier.empty()) {
            return false;
        }

        layer.exportToFile(layer.m_filePath, relativePath);

        outLayer->RemoveSubLayerPath(i);
        outLayer->InsertSubLayerPath(sdfLayerIdentifier.c_str(), i);
    }

    return outLayer->Save();
}

Amino::String Layer::exportToString(bool exportSubLayers) const {
    auto outLayer = pxr::SdfLayer::CreateAnonymous(
        getTagWithValidUsdFileFormat().c_str());
    outLayer->TransferContent(m_layer);

    if (exportSubLayers) {
        for (int i = 0; i < static_cast<int>(m_subLayers.size()); ++i) {
            const auto& layer = m_subLayers[i];
            auto filepath = layer.m_filePath.empty() ? layer.m_originalFilePath
                                                     : layer.m_filePath;
            outLayer->RemoveSubLayerPath(i);
            outLayer->InsertSubLayerPath(filepath.c_str(), i);
        }
    }

    std::string result;
    outLayer->ExportToString(&result);
    return result.c_str();
}

Amino::String Layer::getTagWithValidUsdFileFormat(const Amino::String& tag) {
    // Extract the filename (if any) from given tag.
    // If tag has no filename, or filename is the current or parent directory
    // name, then consider the filename to be empty.
    Amino::String filename = Bifrost::FileUtils::extractFilename(tag);
    if (filename.empty() || filename == "." || filename == "..") {
        filename.clear();
    }
    return getFilenameWithValidUsdFileFormat(filename);
}

Amino::String Layer::getPathWithValidUsdFileFormat(const Amino::String& path) {
    // Extract the filename (if any) from given path.
    // If path has no filename, or filename is the current or parent directory
    // name, then consider the filename to be empty.
    // Preserve the directory part (if any).
    Amino::String dir;
    Amino::String filename = Bifrost::FileUtils::extractFilename(path);
    if (filename.empty() || filename == "." || filename == "..") {
        dir = path;
        filename.clear();
    } else {
        dir = path.substr(0, path.size() - filename.size());
    }
    filename = getFilenameWithValidUsdFileFormat(filename);

    // Combine directory (if present) and filename
    if (dir.empty()) {
        return filename;
    } else {
        return Bifrost::FileUtils::filePath(dir, filename);
    }
}

} // namespace BifrostUsd

//------------------------------------------------------------------------------
//
template <>
Amino::Ptr<BifrostUsd::Layer> Amino::createDefaultClass() {
    // Destructor of USD instances are lauching threads. This result in
    // a deadlock on windows when unloading the library (which destroys the
    // default constructed object held in static variables).
    /// \todo BIFROST-6874 remove pxr::Work_EnsureDetachedTaskProgress();
    pxr::Work_EnsureDetachedTaskProgress();
    return Amino::newClassPtr<BifrostUsd::Layer>(
        BifrostUsd::Layer::Invalid{});
}
AMINO_DEFINE_DEFAULT_CLASS(BifrostUsd::Layer);
