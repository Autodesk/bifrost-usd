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

#ifndef BIFROST_DYNAMIC_FILE_FORMAT_H
#define BIFROST_DYNAMIC_FILE_FORMAT_H

#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/pcp/dynamicFileFormatInterface.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/spec.h>

#include <iosfwd>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/* clang-format off */
#define BIFROST_DYNAMIC_FILE_FORMAT_TOKENS           \
    ((Id, "bifrostDynamicFile"))                     \
    ((Version, "1.0"))                               \
    ((Target, "usd"))                                \
    ((Extension, "bifrostDynamicFile"))              \
    ((CompoundName, "BifrostGraph_CompoundName"))    \
    ((OutputName, "BifrostGraph_OutputName"))        \
    ((Options, "BifrostGraph_Options"))              \
    ((Globals, "BifrostGraph_Globals"))              \
    ((ReloadLibrary, "BifrostGraph_ReloadLibrary"))  \
    ((Params, "BifrostGraph_Params"))
/* clang-format on */

TF_DECLARE_PUBLIC_TOKENS(BifrostDynamicFileFormatTokens,
                         BIFROST_DYNAMIC_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(BifrostDynamicFileFormat);

class BifrostDynamicFileFormat : public SdfFileFormat,
                                 public PcpDynamicFileFormatInterface {
public:
    /// Determines whether the file format can read the specified file.
    ///
    /// Since the graph is loaded from the BifrostUsd::DynamicPayload Library,
    /// there is no file to read from and this member function will always
    /// return false.
    bool CanRead(const std::string& file) const override;

    /// When USD open a stage and compose its layers, if a prim is payloading an
    /// asset with our extension, it will call \ref Read to create data in the
    /// pointed layer object. Note that \ref ComposeFieldsForFileFormatArguments
    /// is called before \ref Read.
    bool Read(SdfLayer*          layer,
              const std::string& resolvedPath,
              bool               metadataOnly) const override;

    /// We override WriteToString and WriteToStream methods so
    /// SdfLayer::ExportToString() etc, work. Writing this layer will write out
    /// the generated layer contents.
    /// We do NOT implement WriteToFile as it doesn't make sense to write to
    /// files of this format when the contents are completely generated from the
    /// file format arguments.
    bool WriteToString(
        const SdfLayer&    layer,
        std::string*       str,
        const std::string& comment = std::string()) const override;
    bool WriteToStream(const SdfSpecHandle& spec,
                       std::ostream&        out,
                       size_t               indent) const override;

public:
    /// A required PcpDynamicFileFormatInterface override for generating
    /// the file format arguments in context.
    ///
    /// From the `context` parameter, we can create the arguments that will be
    /// used to setup and execute a Bifrost graph. This is the first access
    /// point to the plugin when opening a stage. It outputs an args object of
    /// type \ref FileFormatArguments. Such type is just a map of string to string
    /// used to store the arguments names and "stringified" values that will be
    /// passed to the \ref Read function member.
    void ComposeFieldsForFileFormatArguments(
        const std::string&                 assetPath,
        const PcpDynamicFileFormatContext& context,
        FileFormatArguments*               args,
        VtValue* dependencyContextData) const override;

    // Optional overrides
    /// A PcpDynamicFileFormatInterface override for more finely processing
    /// whether a field change may affect the file format arguments within a
    /// given context.
    bool CanFieldChangeAffectFileFormatArguments(
        const TfToken& field,
        const VtValue& oldValue,
        const VtValue& newValue,
        const VtValue& dependencyContextData) const override;

    // bool CanAttributeDefaultValueChangeAffectFileFormatArguments(
    //     const TfToken& attributeName,
    //     const VtValue& oldValue,
    //     const VtValue& newValue,
    //     const VtValue& dependencyContextData) const override;

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    bool _ShouldSkipAnonymousReload() const override;
    bool _ShouldReadAnonymousLayers() const override;

    ~BifrostDynamicFileFormat() override;
    BifrostDynamicFileFormat();

private:
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // BIFROST_DYNAMIC_FILE_FORMAT_H
