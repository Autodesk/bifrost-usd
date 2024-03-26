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

#if defined(_WIN32)
#define USD_WATCHPOINT_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#define USD_WATCHPOINT_EXPORT __attribute__((visibility("default")))
#else
#define USD_WATCHPOINT_EXPORT
#endif

namespace Amino {
class Type;
class StringList;
} // namespace Amino

#include <Amino/Core/Array.h>
#include <Amino/Core/String.h>
#include <BifrostGraph/Executor/Callbacks.h>
#include <BifrostGraph/Executor/Utility.h>
#include <BifrostGraph/Executor/Watchpoint.h>
#include <BifrostUsd/Attribute.h>
#include <BifrostUsd/Layer.h>
#include <BifrostUsd/Prim.h>
#include <BifrostUsd/Stage.h>

#include <map>
#include <mutex>
#include <string>

// Note: To silence warnings coming from USD library
#include <bifusd/config/CfgWarningMacros.h>
BIFUSD_WARNING_PUSH
BIFUSD_WARNING_DISABLE_MSC(4003)
BIFUSD_WARNING_DISABLE_MSC(4267)
BIFUSD_WARNING_DISABLE_MSC(4244)
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/primTypeInfo.h>

BIFUSD_WARNING_POP

using CallBackFunc = BifrostGraph::Executor::Watchpoint::CallBack;
using Records      = BifrostGraph::Executor::Watchpoint::Records;

namespace {

// Type names
Amino::String const kUsdAttributeName = "BifrostUsd::Attribute";
Amino::String const kUsdLayerName     = "BifrostUsd::Layer";
Amino::String const kUsdStageName     = "BifrostUsd::Stage";
Amino::String const kUsdPrimName      = "BifrostUsd::Prim";

// Stage watchpoint
Amino::String const kLastModifiedPrim = "last modified prim";
Amino::String const kLastModifiedPrimDesc =
    "The prim that was modified last on the stage";
Amino::String const kLastModifiedVariant = "last_modified_variant";
Amino::String const kLastModifiedVariantDesc =
    "The variant that was modifed last on the stage";
Amino::String const kStageFilePaths     = "file_paths";
Amino::String const kStageFilePathsDesc = "The stage file paths";
Amino::String const kLayerStack         = "layer_stack";
Amino::String const kLayerStackDesc =
    "The layer stack, organized from strongest to weakest";
Amino::String const kStats = "stats";
Amino::String const kStatsDesc =
    "Some stats about te file, like num of prims, meshes etc";

// Attribute watchpoint
Amino::String const kIsDefined      = "is_defined";
Amino::String const kIsDefinedDesc  = "Is the attribute defined?";
Amino::String const kIsCustom       = "is_custom";
Amino::String const kIsCustomDesc   = "Is the attribute custom?";
Amino::String const kIsAuthored     = "is_authored";
Amino::String const kIsAuthoredDesc = "Is the attribute authored?";
Amino::String const kTypeName       = "type";
Amino::String const kTypeNameDesc   = "The type name";
Amino::String const kValue          = "value";
Amino::String const kValueDesc      = "The value";

Amino::String const kNumTimeSamples     = "num_time_samples";
Amino::String const kNumTimeSamplesDesc = "The number of time samples";

// Prim watchpoint
Amino::String const kPrimPath           = "path";
Amino::String const kPrimPathDesc       = "The prim's path";
Amino::String const kPrimInfo           = "info";
Amino::String const kPrimInfoDesc       = "Some info about the prim";
Amino::String const kPrimAttributes     = "attributes";
Amino::String const kPrimAttributesDesc = "The prim's attributes";
Amino::String const kArraySize          = "size";
Amino::String const kArraySizeDesc =
    "Record the number of elements in the array";

// Layer watchpoint
Amino::String const kLayerDisplayName         = "display_name";
Amino::String const kLayerDisplayNameNameDesc = "The layer's display name";

Amino::String const kLayerFilePath         = "file_path";
Amino::String const kLayerFilePathNameDesc = "The layer's file path";

Amino::String const kLayerOriginalFilePath = "original_file_path";
Amino::String const kLayerOriginalFilePathDesc =
    "The layer's original file path";

Amino::String const kStartTimeCode     = "start_time_code";
Amino::String const kStartTimeCodeDesc = "The layer start time code";
Amino::String const kEndTimeCode       = "end_time_code";
Amino::String const kEndTimeCodeDesc   = "The layer end time code";

// XML formating
std::string const kElement  = "element";
std::string const kElements = "elements";
std::string const kSpace    = " ";

std::string const kBeginXMLElement = "<";
std::string const kEndXMLElement   = ">";

std::string const kEscapedBeginXMLElement = "&lt;";
std::string const kEscapedEndXMLElement   = "&gt;";

std::string const kBeginXMLOpenElement  = kBeginXMLElement;
std::string const kBeginXMLCloseElement = kBeginXMLElement + "/";

using AttributePtr      = Amino::Ptr<BifrostUsd::Attribute>;
using ArrayOfAttributes = Amino::Ptr<Amino::Array<AttributePtr>>;

using LayerPtr      = Amino::Ptr<BifrostUsd::Layer>;
using ArrayOfLayers = Amino::Ptr<Amino::Array<LayerPtr>>;

using StagePtr      = Amino::Ptr<BifrostUsd::Stage>;
using ArrayOfStages = Amino::Ptr<Amino::Array<StagePtr>>;

using PrimPtr      = Amino::Ptr<BifrostUsd::Prim>;
using ArrayOfPrims = Amino::Ptr<Amino::Array<PrimPtr>>;

#ifndef NDEBUG
bool isUsdAttributeType(Amino::Type const& type) {
    return kUsdAttributeName ==
           BifrostGraph::Executor::Utility::getInnermostElementTypeName(type);
}
#endif

bool isUsdLayerType(Amino::Type const& type) {
    return kUsdLayerName ==
           BifrostGraph::Executor::Utility::getInnermostElementTypeName(type);
}

bool isUsdStageType(Amino::Type const& type) {
    return kUsdStageName ==
           BifrostGraph::Executor::Utility::getInnermostElementTypeName(type);
}

bool isUsdPrimType(Amino::Type const& type) {
    return kUsdPrimName ==
           BifrostGraph::Executor::Utility::getInnermostElementTypeName(type);
}

bool isArrayType(Amino::Type const& type) {
    return BifrostGraph::Executor::Utility::isArrayType(type);
}

std::string convertToString(bool boolean) { return boolean ? "true" : "false"; }

std::string convertToString(const PXR_NS::VtValue& value) {
    std::string result;
    if (value.IsHolding<int>()) {
        result = std::to_string(value.UncheckedGet<int>());
    } else if (value.IsHolding<float>()) {
        result = std::to_string(value.UncheckedGet<float>());
    } else if (value.IsHolding<double>()) {
        result = std::to_string(value.UncheckedGet<double>());
    } else if (value.IsHolding<bool>()) {
        result = convertToString(value.UncheckedGet<bool>());
    } else if (value.IsArrayValued()) {
        result = "(array, size: " + std::to_string(value.GetArraySize()) + ")";
    }

    return result;
}

///-------------------------------------------------------------------------
/// \brief The usd watchpoint client data
/// \{
class USDWPClientData {
public:
    explicit USDWPClientData(Records& recordedValues)
        : m_recordedValues(recordedValues)

    {}

    USDWPClientData()                       = delete;
    USDWPClientData(USDWPClientData const&) = delete;
    USDWPClientData(USDWPClientData&&)      = delete;
    virtual ~USDWPClientData()              = default;

    void record(AttributePtr const& attribute);
    void record(ArrayOfAttributes const& attributes);
    void record(LayerPtr const& layer);
    void record(ArrayOfLayers const& layers);
    void record(StagePtr const& stage);
    void record(ArrayOfStages const& stages);
    void record(PrimPtr const& prim);
    void record(ArrayOfPrims const& prims);

private:
    Records& m_recordedValues;
};

void addXmlElement(std::ostringstream& oss,
                   const Amino::String& name,
                   const std::string&   value) {
    auto openXMLElement = [&oss](const char* tagName) {
        oss << kBeginXMLOpenElement << tagName << kEndXMLElement;
    };
    auto closeXMLElement = [&oss](const char* tagName) {
        oss << kBeginXMLCloseElement << tagName << kEndXMLElement;
    };

    openXMLElement(name.c_str());
    oss << value.c_str();
    closeXMLElement(name.c_str());
}

void USDWPClientData::record(AttributePtr const& attributePtr) {
    m_recordedValues.clear();
    if (attributePtr && *attributePtr) {
        auto const&        attribute = *attributePtr;
        std::ostringstream oss;

        addXmlElement(oss, kTypeName,
                      attribute->GetTypeName().GetAsToken().GetText());

        PXR_NS::VtValue vtVal;
        if (attribute->Get(&vtVal)) {
            addXmlElement(oss, kValue, convertToString(vtVal));
        }

        addXmlElement(oss, kIsDefined, convertToString(attribute->IsDefined()));
        addXmlElement(oss, kIsCustom, convertToString(attribute->IsCustom()));
        addXmlElement(oss, kIsAuthored,
                      convertToString(attribute->IsAuthored()));
        addXmlElement(oss, kNumTimeSamples,
                      std::to_string(attribute->GetNumTimeSamples()));

        m_recordedValues.set(kTypeName.c_str(), oss.str().c_str());
    }
}

void USDWPClientData::record(ArrayOfAttributes const& attributes) {
    m_recordedValues.clear();
    m_recordedValues.set(kArraySize,
                         std::to_string(attributes->size()).c_str());
}

void USDWPClientData::record(LayerPtr const& layer) {
    m_recordedValues.clear();
    if (layer && *layer) {
        std::ostringstream oss;
        auto&              pxr_layer = layer->get();
        addXmlElement(oss, kLayerDisplayName, pxr_layer.GetDisplayName());
        addXmlElement(oss, kLayerFilePath, layer->getFilePath().c_str());
        addXmlElement(oss, kLayerOriginalFilePath,
                      layer->getOriginalFilePath().c_str());
        addXmlElement(oss, kStartTimeCode,
                      std::to_string(pxr_layer.GetStartTimeCode()));
        addXmlElement(oss, kEndTimeCode,
                      std::to_string(pxr_layer.GetEndTimeCode()));
        m_recordedValues.set(pxr_layer.GetDisplayName().c_str(),
                             oss.str().c_str());

        // Add info from sublayers too
        auto sublayers = layer->getSubLayers();
        // We need a reverse iterator in order to be displayed sublayers in same
        // order than the USD Layer Editor
        for (int i = static_cast<int>(sublayers.size()) - 1; i >= 0; i--) {
            std::ostringstream subOss;
            auto               subLayer     = sublayers[i];
            auto&              pxr_subLayer = subLayer.get();

            std::string subLayerDisplayName =
                "    " + pxr_subLayer.GetDisplayName();

            addXmlElement(subOss, kLayerDisplayName, subLayerDisplayName);
            addXmlElement(subOss, kLayerFilePath,
                          subLayer.getFilePath().c_str());
            addXmlElement(subOss, kLayerOriginalFilePath,
                          subLayer.getOriginalFilePath().c_str());
            addXmlElement(subOss, kStartTimeCode,
                          std::to_string(pxr_subLayer.GetStartTimeCode()));
            addXmlElement(subOss, kEndTimeCode,
                          std::to_string(pxr_subLayer.GetEndTimeCode()));

            m_recordedValues.set(subLayerDisplayName.c_str(),
                                 subOss.str().c_str());
        }
    }
}

void USDWPClientData::record(ArrayOfLayers const& layers) {
    m_recordedValues.clear();
    m_recordedValues.set(kArraySize, std::to_string(layers->size()).c_str());
}

void USDWPClientData::record(StagePtr const& stage) {
    m_recordedValues.clear();

    if (stage && *stage) {
        m_recordedValues.set(kLastModifiedPrim, stage->last_modified_prim);

        auto rootLayer = stage->getRootLayer();
        if (rootLayer) {
            Amino::String filePaths;
            auto          originalFilePath = rootLayer->getOriginalFilePath();
            if (!originalFilePath.empty()) {
                filePaths.append("input: ");
                filePaths.append(originalFilePath.c_str());
                filePaths.append("\n");
            }

            filePaths.append("ouput: ");
            filePaths.append(rootLayer->getFilePath().c_str());
            m_recordedValues.set(kStageFilePaths, filePaths);
        }

        if (!stage->last_modified_variant_name.empty()) {
            m_recordedValues.set(kLastModifiedVariant,
                                 stage->last_modified_variant_set_prim + ": " +
                                     stage->last_modified_variant_set_name +
                                     "::" + stage->last_modified_variant_name);
        }

        Amino::String layerStackString = "";
        for (const auto& layer : stage->get().GetLayerStack()) {
            if (layer == stage->get().GetSessionLayer()) {
                continue;
            }
            layerStackString.append(layer->GetDisplayName().c_str());
            layerStackString.append("\n");
        }
        m_recordedValues.set(kLayerStack, layerStackString);
        unsigned int                        prim_count = 0;
        std::map<std::string, unsigned int> prim_types;

        /// Is TraverseAll() really non const?
        for (const auto& prim :
             const_cast<PXR_NS::UsdStage&>(stage->get()).TraverseAll()) {
            prim_count++;
            const PXR_NS::UsdPrimTypeInfo& prim_typeinfo = prim.GetPrimTypeInfo();
            std::string                 type_name = prim_typeinfo.GetTypeName();
            if (!type_name.empty()) {
                if (prim_types.find(type_name) == prim_types.end()) {
                    prim_types[type_name] = 1;
                } else {
                    prim_types[type_name] = prim_types[type_name] + 1;
                }
            }

            const PXR_NS::TfTokenVector& applied_schemas =
                prim_typeinfo.GetAppliedAPISchemas();
            for (std::string const& schema : applied_schemas) {
                if (!schema.empty() && schema != type_name) {
                    if (prim_types.find(schema) == prim_types.end()) {
                        prim_types[schema] = 1;
                    } else {
                        prim_types[schema] = prim_types[type_name] + 1;
                    }
                }
            }
        }
        std::ostringstream ss;
        ss << "Total Prims: " << std::to_string(prim_count) << std::endl;
        for (auto const& prim_type : prim_types) {
            ss << prim_type.first
               << " Count: " << std::to_string(prim_type.second) << std::endl;
        }
        m_recordedValues.set(kStats, ss.str().c_str());
    }
}

void USDWPClientData::record(ArrayOfStages const& stages) {
    m_recordedValues.clear();
    m_recordedValues.set(kArraySize, std::to_string(stages->size()).c_str());
}

void USDWPClientData::record(PrimPtr const& primPtr) {
    m_recordedValues.clear();
    if (primPtr && *primPtr) {
        auto const& prim = *primPtr;
        m_recordedValues.set(kPrimPath, prim->GetPrimPath().GetText());

        // Info on Prim
        std::ostringstream info_ss;
        info_ss << "Type Name: " << prim->GetTypeName().GetText() << std::endl;
        info_ss << "IsActive: " << convertToString(prim->IsActive())
                << std::endl;
        info_ss << "IsLoaded: " << convertToString(prim->IsLoaded())
                << std::endl;
        info_ss << "IsModel: " << convertToString(prim->IsModel()) << std::endl;
        info_ss << "IsGroup: " << convertToString(prim->IsGroup()) << std::endl;
        info_ss << "IsAbstract: " << convertToString(prim->IsAbstract())
                << std::endl;
        info_ss << "IsDefined: " << convertToString(prim->IsDefined())
                << std::endl;
        info_ss << "HasVariantSets: " << convertToString(prim->HasVariantSets())
                << std::endl;
        info_ss << "HasAuthoredPayloads: "
                << convertToString(prim->HasAuthoredPayloads()) << std::endl;
        info_ss << "HasAuthoredInherits: "
                << convertToString(prim->HasAuthoredInherits()) << std::endl;
        info_ss << "HasAuthoredSpecializes: "
                << convertToString(prim->HasAuthoredSpecializes()) << std::endl;
        info_ss << "HasAuthoredInstanceable: "
                << convertToString(prim->HasAuthoredInstanceable())
                << std::endl;
        info_ss << "IsInstance: " << convertToString(prim->IsInstance())
                << std::endl;
        info_ss << "IsInstanceProxy: "
                << convertToString(prim->IsInstanceProxy()) << std::endl;
        info_ss << "IsPrototype: " << convertToString(prim->IsPrototype())
                << std::endl;
        info_ss << "IsInPrototype: " << convertToString(prim->IsInPrototype())
                << std::endl;
        m_recordedValues.set(kPrimInfo, info_ss.str().c_str());

        // Attributes of prim
        std::ostringstream             attr_ss;
        std::vector<PXR_NS::UsdAttribute> attributes = prim->GetAttributes();
        for (auto const& attribute : attributes) {
            attr_ss << attribute.GetName().GetText() << ": "
                    << attribute.GetTypeName().GetAsToken().GetText()
                    << std::endl;
        }
        m_recordedValues.set(kPrimAttributes, attr_ss.str().c_str());
    }
}

void USDWPClientData::record(ArrayOfPrims const& prims) {
    m_recordedValues.clear();
    m_recordedValues.set(kArraySize, std::to_string(prims->size()).c_str());
}
/// \}

///-------------------------------------------------------------------------
/// \brief The callback function
/// \{
template <typename T>
void wpCallBack(void const* data, Amino::ulong_t, void const* value) {
    if (data == nullptr || value == nullptr) return;

    static std::mutex           s_mutex;
    std::lock_guard<std::mutex> lock(s_mutex);

    auto* wpClientData =
        reinterpret_cast<USDWPClientData*>(const_cast<void*>(data));
    auto const* typedValue = reinterpret_cast<T const*>(value);

    wpClientData->record(*typedValue);
}
/// \}

} // namespace

class USDWatchpoint : public BifrostGraph::Executor::Watchpoint {
public:
    USDWatchpoint() noexcept;
    ~USDWatchpoint() noexcept override;

    void deleteThis() noexcept override;

    void getSupportedTypeNames(StringArray& out_names) const noexcept override;

    CallBackFunc getCallBackFunction(
        Amino::Type const& type) const noexcept override;

    bool getAvailableParameters(
        Amino::Type const& type,
        StringArray&       out_parameters) const noexcept override;

    bool getParameterDetails(
        Amino::Type const&   type,
        Amino::String const& parameter,
        Amino::String&       out_description,
        StringArray&         out_values,
        StringArray&         out_descriptions) const noexcept override;

    void const* createClientData(Amino::Type const& type,
                                 Records& records) const noexcept override;

    bool releaseClientData(Amino::Type const& type,
                           void const* clientData) const noexcept override;
};

USDWatchpoint::USDWatchpoint() noexcept
    : BifrostGraph::Executor::Watchpoint("USD Watchpoint") {}

USDWatchpoint::~USDWatchpoint() noexcept = default;

void USDWatchpoint::deleteThis() noexcept { delete this; }

void USDWatchpoint::getSupportedTypeNames(
    StringArray& out_names) const noexcept {
    out_names.push_back(kUsdLayerName);
    out_names.push_back(kUsdStageName);
    out_names.push_back(kUsdPrimName);
    out_names.push_back(kUsdAttributeName);
}

CallBackFunc USDWatchpoint::getCallBackFunction(
    Amino::Type const& dataType) const noexcept {
    if (isUsdStageType(dataType)) {
        return isArrayType(dataType) ? &wpCallBack<ArrayOfStages>
                                     : &wpCallBack<StagePtr>;
    } else if (isUsdPrimType(dataType)) {
        return isArrayType(dataType) ? &wpCallBack<ArrayOfPrims>
                                     : &wpCallBack<PrimPtr>;
    } else if (isUsdLayerType(dataType)) {
        return isArrayType(dataType) ? &wpCallBack<ArrayOfLayers>
                                     : &wpCallBack<LayerPtr>;
    } else {
        return isArrayType(dataType) ? &wpCallBack<ArrayOfAttributes>
                                     : &wpCallBack<AttributePtr>;
    }
}

bool USDWatchpoint::getAvailableParameters(
    Amino::Type const& dataType, StringArray& out_parameters) const noexcept {
    if (isArrayType(dataType)) {
        out_parameters.push_back(kArraySize);
        return true;
    } else if (isUsdStageType(dataType)) {
        out_parameters.push_back(kLastModifiedPrim);
        out_parameters.push_back(kLastModifiedVariant);
        out_parameters.push_back(kStageFilePaths);
        out_parameters.push_back(kLayerStack);
        out_parameters.push_back(kStats);
        return true;
    } else if (isUsdPrimType(dataType)) {
        out_parameters.push_back(kPrimPath);
        out_parameters.push_back(kPrimInfo);
        out_parameters.push_back(kPrimAttributes);
        return true;
    }

    return false;
}

bool USDWatchpoint::getParameterDetails(
    Amino::Type const&   dataType,
    Amino::String const& parameter,
    Amino::String&       out_description,
    StringArray&         out_values,
    StringArray&         out_descriptions) const noexcept {
    if (isArrayType(dataType)) {
        if (parameter == kArraySize) {
            out_description = kArraySizeDesc;
            return true;
        }
    } else if (isUsdStageType(dataType)) {
        if (parameter == kLastModifiedPrim) {
            out_description = kLastModifiedPrimDesc;
        } else if (parameter == kLastModifiedVariant) {
            out_description = kLastModifiedVariantDesc;
        } else if (parameter == kLayerStack) {
            out_description = kLayerStackDesc;
        } else if (parameter == kStats) {
            out_description = kStatsDesc;
        }

        return true;
    }

    else if (isUsdPrimType(dataType)) {
        if (parameter == kPrimPath) {
            out_description = kPrimPathDesc;
        } else if (parameter == kPrimInfo) {
            out_description = kPrimInfoDesc;
        } else if (parameter == kPrimAttributes) {
            out_description = kPrimAttributesDesc;
        }

        return true;
    }

    out_values.push_back("enable");
    out_values.push_back("disable");
    out_descriptions.push_back("enable");
    out_descriptions.push_back("disable");

    return true;
}

void const* USDWatchpoint::createClientData(Amino::Type const& dataType,
                                            Records& records) const noexcept {
    (void)dataType;
    assert(isUsdStageType(dataType) || isUsdPrimType(dataType) ||
           isUsdLayerType(dataType) || isUsdAttributeType(dataType));
    return new USDWPClientData(records);
}

bool USDWatchpoint::releaseClientData(Amino::Type const& dataType,
                                      void const* clientData) const noexcept {
    (void)dataType;
    assert(isUsdStageType(dataType) || isUsdPrimType(dataType) ||
           isUsdLayerType(dataType) || isUsdAttributeType(dataType));
    assert(clientData != nullptr);
    delete reinterpret_cast<USDWPClientData const*>(clientData);
    return true;
}

extern "C" {
USD_WATCHPOINT_EXPORT BifrostGraph::Executor::Watchpoint* createBifrostWatchpoint(void);

USD_WATCHPOINT_EXPORT BifrostGraph::Executor::Watchpoint* createBifrostWatchpoint(void) {
    return new USDWatchpoint();
}
}
