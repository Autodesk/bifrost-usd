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

/// \file Layer.h
///
/// \brief Bifrost USD Layer type
///

#ifndef VALUE_SEMANTIC_USD_LAYER_H
#define VALUE_SEMANTIC_USD_LAYER_H

#include <Amino/Core/Array.h>
#include <Amino/Core/String.h>
#include <Amino/Cpp/Annotate.h>
#include <Amino/Cpp/ClassDeclare.h>

#include "BifrostUsdExport.h"

#ifndef DISABLE_PXR_HEADERS

// Note: To silence warnings coming from USD library
#include <bifusd/config/CfgWarningMacros.h>
BIFUSD_WARNING_PUSH

BIFUSD_WARNING_DISABLE_MSC(4003)
BIFUSD_WARNING_DISABLE_MSC(4244)
BIFUSD_WARNING_DISABLE_MSC(4305)
BIFUSD_WARNING_DISABLE_MSC(4800)

#include <pxr/usd/sdf/layer.h>

BIFUSD_WARNING_POP

#endif // DISABLE_PXR_HEADERS

namespace BifrostUsd {

class Stage;

/// \class Layer Layer.h
/// \brief Bifrost USD Layer type that flows in the graph.
class AMINO_ANNOTATE("Amino::Class") USD_DECL Layer {
#ifndef DISABLE_PXR_HEADERS
public:
    struct Invalid {};
    explicit Layer(Invalid);

    explicit Layer(const Amino::String& tag = "");

    explicit Layer(const Amino::String& filePath, const Amino::String& tag,
                   const Amino::String& savefilePath = "",
                   const bool           isEditable   = true);

    explicit Layer(const PXR_NS::SdfLayerRefPtr& layer,
                   const bool                 isEditable       = true,
                   const Amino::String&       originalFilePath = "");

    friend void swap(Layer & first, Layer & second) noexcept {
        first.m_layer.swap(second.m_layer);
        first.m_filePath.swap(second.m_filePath);
        first.m_originalFilePath.swap(second.m_originalFilePath);
        first.m_tag.swap(second.m_tag);
        first.m_subLayers.swap(second.m_subLayers);
    }

    Layer(const Layer& other, const Amino::String& originalFilePath);

    Layer(const Layer& other);
    Layer& operator=(const Layer& other);

    Layer(Layer && other) noexcept;
    Layer& operator=(Layer&& other) noexcept;

    bool operator==(const Layer& rhs) const;
    bool operator!=(const Layer& rhs) const;

    bool     isValid() const { return m_layer != nullptr; }
    explicit operator bool() const { return isValid(); }

    /// \brief Accessors to the underlying PXR_NS::SdfLayer.
    ///
    /// These accessor should be used instead of getting the underlying
    /// PXR_NS::SdfLayerRefPtr directly, because they propagate constness to the
    /// PXR_NS::SdfLayer pointer.
    ///
    /// This helps avoiding unintentionally creating side effects in other
    /// pointers to the same \ref BifrostUsd::Layer.
    /// \{
    PXR_NS::SdfLayer&       get() { return *m_layer; }
    PXR_NS::SdfLayer&       operator*() { return *m_layer; }
    PXR_NS::SdfLayer*       operator->() { return m_layer.operator->(); }
    PXR_NS::SdfLayer const& get() const { return *m_layer; }
    PXR_NS::SdfLayer const& operator*() const { return *m_layer; }
    PXR_NS::SdfLayer const* operator->() const { return m_layer.operator->(); }
    /// \}

    // This function is purposefully non-const. Be careful with it.
    PXR_NS::SdfLayerRefPtr getLayerPtr() { return m_layer; }

    void                       setFilePath(const Amino::String& filePath);
    void                       setFileFormat(const Amino::String& fileFormat);
    const Amino::String&       getFilePath() const;
    const Amino::String&       getOriginalFilePath() const;
    const Amino::Array<Layer>& getSubLayers() const;

    /// This function inserts the given sublayer in the list of sublayers
    /// at the given index.
    ///
    /// \param [in] layer The sublayer to insert.
    /// \param [in] index The index where to insert the sublayer, where 0
    ///     corresponds to the strongest sublayer in the Pixar SdfLayer.
    ///     If -1, the sublayer will be appended to the list and will become
    ///     the weakest of the sublayers.
    /// \returns true if sublayer was inserted successfully; false if
    ///     the index is invalid, or if this layer or the given sublayer is
    ///     invalid, or if modifications to this layer are not allowed.
    bool insertSubLayer(const Layer& layer, int index = -1);

    /// This function replaces a sublayer at the specified index by the given
    /// new sublayer.
    ///
    /// \param [in] layer The new sublayer to replace an existing one.
    /// \param [in] index The index of the sublayer to replace, where 0
    ///     corresponds to the strongest sublayer in the Pixar SdfLayer.
    ///     If -1 and at least one sublayer is present, the last and weakest
    ///     of the sublayers is replaced.
    /// \returns true if sublayer was replaced successfully; false if
    ///     the index is invalid, or if this layer or the given sublayer is
    ///     invalid, or if modifications to this layer are not allowed.
    bool replaceSubLayer(const Layer& layer, int index);

    /// This function returns the sublayer at the given index.
    ///
    /// \param [in] index The index of the sublayer to retrieve, where 0
    ///     corresponds to the strongest sublayer in the Pixar SdfLayer.
    ///     If -1 and at least one sublayer is present, the last and weakest
    ///     of the sublayers is retrieved.
    /// \returns if successful, returns the sublayer at the given index;
    ///     returns an invalid sublayer if the index is invalid, or if this
    //      layer is invalid.
    Layer const& getSubLayer(int index) const;

    /// This function returns the index of the weakest sublayer of this layer.
    /// The weakest sublayer is the last layer of the list of sublayers
    /// since we use the same order than the stack of sublayers of a Pixar
    /// SdfLayer.
    ///
    /// \returns The index of the weakest sublayer within this layer.
    ///     If this layer is invalid or has no sublayer, -1 is returned.
    int getWeakestSubLayerIndex() const {
        return (!isValid() || m_subLayers.empty()) ? -1 :
            static_cast<int>(m_subLayers.size()) - 1; }

    bool exportToFile(const Amino::String& filePath     = "",
                      bool                 relativePath = false) const;
    Amino::String exportToString(bool exportSubLayers = true) const;

    /// This helper method converts its input string into a valid tag.
    /// A valid tag is a filename with a non-empty stem and a valid USD file
    /// extension (.usd, .usda or .usdc).
    /// If the input string does not contain a stem, the default "bifrost"
    /// stem is used. If the input string does not contain a valid USD file
    /// extension, the default ".usd" extension is used.
    /// If the input string contains a directory, it is discarded.
    ///
    /// Examples on Unix platforms:
    ///     Input               Returns             Remark
    ///     -----               -------             ------
    ///     ""                  "bifrost.usd"       Default tag is returned
    ///     "."                 "bifrost.usd"       Default tag is returned
    ///     ".."                "bifrost.usd"       Default tag is returned
    ///     "../dir/"           "bifrost.usd"       Default tag is returned and directory is discarded
    ///     ".usd"              "bifrost.usd"       Default stem is prepended
    ///     ".usda"             "bifrost.usda"      Default stem is prepended
    ///     ".usdc"             "bifrost.usdc"      Default stem is prepended
    ///     "abc"               "abc.usd"           Default extension is appended
    ///     "abc.usd"           "abc.usd"           No change
    ///     "dir/abc.usda"      "abc.usda"          Directory is discarded
    ///     "../dir/abc.usdc"   "abc.usdc"          Directory is discarded
    ///
    /// \param [in] tag The input tag string
    /// \returns A valid tag, with a non-empty stem and a valid extension.
    static Amino::String getTagWithValidUsdFileFormat(const Amino::String& tag = "");

    /// This helper method converts its input string into a valid USD path.
    /// A valid path has an optional directory path, followed by a filename with
    /// a non-empty stem and a valid USD file extension (.usd, .usda or .usdc).
    /// This functions behaves exactly as \ref getTagWithValidUsdFileFormat
    /// except that it keeps the directory if it is present in input string.
    ///
    /// Examples on Unix platforms:
    ///     Input               Returns                 Remark
    ///     -----               -------                 ------
    ///     ""                  "bifrost.usd"           Default tag is returned
    ///     "."                 "./bifrost.usd"         Default tag is appended
    ///     ".."                "../bifrost.usd"        Default tag is appended
    ///     "../dir/"           "../dir/bifrost.usd"    Default tag is appended
    ///     "dir/.usd"          "dir/bifrost.usd"       Default stem is prepended
    ///     "../abc"            "../abc.usd"            Default extension is appended
    ///     "abc.usd"           "abc.usd"               No change
    ///     "dir/abc.usda"      "dir/abc.usda"          No change
    ///     "../dir/abc.usdc"   "../dir/abc.usdc"       No change
    ///
    /// \param [in] path The input path string
    /// \returns A valid path, with a non-empty stem and a valid extension.
    static Amino::String getPathWithValidUsdFileFormat(const Amino::String& path);

private:
    friend Stage;

    /// The underlying anonymous sdf layer.
    PXR_NS::SdfLayerRefPtr m_layer;

    /// The file path where to save this Layer.
    /// Can be an empty string, which will disable file export if no other
    /// valid path is provided as argument.
    /// If non-empty, its filename part must have a valid USD file extension
    /// (.usd, .usda or .usdc).
    Amino::String       m_filePath;

    /// The file format used when saving this Layer.
    /// If it is an empty string, the file path extension will be used to deduce
    /// the file format.
    /// If set to "usda", it will allow to save the file in ASCII format.
    /// If set to "usdc", it will allow to save the file in binary format (only
    /// if the file path is not ending with
    /// ".usda").
    Amino::String m_fileFormat = "";

    /// The original file path from where this Layer was read.
    /// Can be an empty string.
    /// If non-empty, its filename part must have a valid USD file extension
    /// (.usd, .usda or .usdc).
    Amino::String       m_originalFilePath;

    /// The tag is used as the display name of the underlying anonymous sdf layer.
    Amino::String       m_tag;

    /// The list of sublayers of this Layer.
    /// Note that the sublayers in this Layer are in same order than the
    /// stack of sublayers of a Pixar SdfLayer, with the first element of the
    /// list being the strongest of the sublayers in the Pixar SdfLayer.
    Amino::Array<Layer> m_subLayers;
#endif // DISABLE_PXR_HEADERS
};

} // namespace BifrostUsd

AMINO_DECLARE_DEFAULT_CLASS(USD_DECL, BifrostUsd::Layer);

#endif /* VALUE_SEMANTIC_USD_LAYER_H */
