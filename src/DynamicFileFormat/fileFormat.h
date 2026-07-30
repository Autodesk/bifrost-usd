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

#ifndef BIFROSTUSD_DYNAMIC_FILE_FORMAT_H
#define BIFROSTUSD_DYNAMIC_FILE_FORMAT_H

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

//==============================================================================
// Plugin Tokens
//==============================================================================
#define BIFROST_DFF_PLUGIN_TOKENS                    \
    ((Id, "bifrostDynamicFile"))                     \
    ((Version, "1.0"))                               \
    ((Target, "usd"))                                \
    ((Extension, "bifrostDynamicFile"))
TF_DECLARE_PUBLIC_TOKENS(BifrostDffPluginTokens, BIFROST_DFF_PLUGIN_TOKENS);

//==============================================================================
// Grouping Tokens
//==============================================================================
// These tokens are used to define the grouping of fields and attributes in the
// File Format Arguments and in the prim's attributes.
// The Grouping Tokens define the top-level structure of each group: a Field
// name (e.g., "bifrostGlobals") identifying the VtDictionary-valued USD field,
// and an attribute prefix (e.g., "bifrost:global:") for the corresponding prim
// attributes. Within a group, Field Key Tokens name the individual dictionary
// entries (e.g., "time_fps"), and Attribute Tokens are the full attribute names
// formed by prepending the group prefix (e.g., "bifrost:global:time_fps").
// Both paths carry the same value to the File Format Arguments - the attribute
// overriding the field when present.
#define BIFROST_DFF_GROUPING_TOKENS                 \
    ((CompoundField,        "bifrostCompound"))     \
    ((CompoundAttrPrefix,   "bifrost:compound:"))   \
    ((GlobalsField,         "bifrostGlobals"))      \
    ((GlobalsAttrPrefix,    "bifrost:global:"))     \
    ((InputsField,          "bifrostInputs"))       \
    ((InputsAttrPrefix,     "bifrost:in:"))         \
    ((OutputsField,         "bifrostOutputs"))      \
    ((OutputsAttrPrefix,    "bifrost:out:"))        \
    ((SettingsField,        "bifrostSettings"))     \
    ((SettingsAttrPrefix,   "bifrost:setting:"))
TF_DECLARE_PUBLIC_TOKENS(BifrostDffGroupingTokens, BIFROST_DFF_GROUPING_TOKENS);

//==============================================================================
// Compound Tokens
//==============================================================================
//  Field Key Tokens                | Attribute Tokens
//  ------------------------------- | -----------------------------------------
//  name                            | bifrost:compound:name
#define BIFROST_DFF_COMPOUND_FIELD_TOKENS           \
    ((CompoundNameField,  "name"))
TF_DECLARE_PUBLIC_TOKENS(BifrostDffCompoundFieldTokens, BIFROST_DFF_COMPOUND_FIELD_TOKENS);

#define BIFROST_DFF_COMPOUND_ATTR_TOKENS            \
    ((CompoundNameAttr,   "bifrost:compound:name"))
TF_DECLARE_PUBLIC_TOKENS(BifrostDffCompoundAttrTokens, BIFROST_DFF_COMPOUND_ATTR_TOKENS);

//==============================================================================
// Globals Tokens
//==============================================================================
//  Field Key Tokens                | Attribute Tokens
//  ------------------------------- | -----------------------------------------
//  timeline_info_start_frame       | bifrost:global:timeline_info_start_frame
//  timeline_info_end_frame         | bifrost:global:timeline_info_end_frame
//  time_fps                        | bifrost:global:time_fps
#define BIFROST_DFF_START_FRAME "timeline_info_start_frame"
#define BIFROST_DFF_END_FRAME   "timeline_info_end_frame"
#define BIFROST_DFF_FPS         "time_fps"

#define BIFROST_DFF_GLOBALS_FIELD_TOKENS                                \
    ((StartFrameField,      BIFROST_DFF_START_FRAME))                   \
    ((EndFrameField,        BIFROST_DFF_END_FRAME))                     \
    ((FpsField,             BIFROST_DFF_FPS))
TF_DECLARE_PUBLIC_TOKENS(BifrostDffGlobalsFieldTokens, BIFROST_DFF_GLOBALS_FIELD_TOKENS);

#define BIFROST_DFF_GLOBALS_ATTR_TOKENS                                 \
    ((StartFrameAttr,       "bifrost:global:" BIFROST_DFF_START_FRAME)) \
    ((EndFrameAttr,         "bifrost:global:" BIFROST_DFF_END_FRAME))   \
    ((FpsAttr,              "bifrost:global:" BIFROST_DFF_FPS))
TF_DECLARE_PUBLIC_TOKENS(BifrostDffGlobalsAttrTokens, BIFROST_DFF_GLOBALS_ATTR_TOKENS);

//==============================================================================
// Settings Tokens
//==============================================================================
//  Field Key Tokens                | Attribute Tokens
//  ------------------------------- | -----------------------------------------
//  reloadLibrary                   | bifrost:setting:reloadLibrary
//  verbosityLevel                  | bifrost:setting:verbosityLevel
#define BIFROST_DFF_SETTINGS_FIELD_TOKENS                       \
    ((ReloadLibraryField,   "reloadLibrary"))                   \
    ((VerbosityLevelField,  "verbosityLevel"))
TF_DECLARE_PUBLIC_TOKENS(BifrostDffSettingsFieldTokens, BIFROST_DFF_SETTINGS_FIELD_TOKENS);

#define BIFROST_DFF_SETTINGS_ATTR_TOKENS                        \
    ((ReloadLibraryAttr,    "bifrost:setting:reloadLibrary"))   \
    ((VerbosityLevelAttr,   "bifrost:setting:verbosityLevel"))
TF_DECLARE_PUBLIC_TOKENS(BifrostDffSettingsAttrTokens, BIFROST_DFF_SETTINGS_ATTR_TOKENS);

/* clang-format on */

TF_DECLARE_WEAK_AND_REF_PTRS(BifrostDynamicFileFormat);

class BifrostDynamicFileFormat : public SdfFileFormat,
                                 public PcpDynamicFileFormatInterface {
public:
    /// Determines whether the file format can read the specified file.
    ///
    /// Since the graph is loaded from the BifrostUsdDynamicFileFormat Library,
    /// there is no file to read from and this member function will always
    /// return false.
    bool CanRead(const std::string& file) const override;

    /// When USD opens a stage and composes its layers, if a prim is payloading
    /// an asset with our extension it will call \ref Read to create data in the
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
    /// used to set up and execute a Bifrost graph. This is the first entry
    /// point to the plugin when opening a stage. It is responsible for
    /// retrieving the strongest opinion for each known field and checking for
    /// the presence of a corresponding attribute override on the prim.
    ///
    /// It outputs an args object of type \ref FileFormatArguments. Such type is
    /// just a map ("<arg name>" -> "<type and value>"). Bifrost DFF uses an
    /// internal convention to store the argument names and type/value pairs
    /// that will then be passed to the \ref Read function member.
    ///
    /// The \ref Read function member will then parse the type/value pairs and
    /// use them to configure the Bifrost graph execution.
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

#endif // BIFROSTUSD_DYNAMIC_FILE_FORMAT_H
