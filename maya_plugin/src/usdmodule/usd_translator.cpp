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

#include "usd_translator.h"

#include <Amino/Core/Any.h>

#include <BifrostGraph/Executor/Utility.h>
#include <BifrostUsd/StageCache.h>

#include <maya/MDagModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnPlugin.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MPxNode.h>
#include <maya/MSelectionList.h>
#include <maya/MUuid.h>

#include <BifrostGraph/Maya/HostData.h>
#include <pxr/usd/usdUtils/stageCache.h>

UsdTranslation::UsdTranslation() noexcept
    : BifrostGraph::Executor::TypeTranslation("USD Translation Table"),
      m_portData() {}

UsdTranslation::~UsdTranslation() noexcept {}

void UsdTranslation::deleteThis() noexcept { delete this; }

int64_t UsdTranslation::AddStageToCache(
    const Amino::Ptr<BifrostUsd::Stage>& stage, const Amino::String& name) {
    int64_t cacheId = -1;
    if (BifrostUsd::StageCache::addStageToCache(stage, cacheId)) {
        (void)addStageForPort(name, cacheId);
    }

    return cacheId;
}

void UsdTranslation::getSupportedTypeNames(
    StringArray& out_names) const noexcept {
    out_names.push_back("BifrostUsd::Stage");
}

bool UsdTranslation::convertValueFromHost(
    Amino::Type const& type,
    Amino::Any&        value,
    ValueData const*   translationData) const noexcept {
    (void)type;
    (void)value;
    (void)translationData;
    return true;
}

bool UsdTranslation::convertValueToHost(
    Amino::Any const& amAny, ValueData* translationData) const noexcept {
    assert(dynamic_cast<BifrostGraph::MayaTranslation::ValueData const*>(
        translationData));
    auto const* mayaHostdata =
        static_cast<BifrostGraph::MayaTranslation::ValueData const*>(
            translationData);
    auto&       block = mayaHostdata->getDataBlock();
    auto const& plug  = mayaHostdata->getPlug();

    auto dataHandle = block.outputValue(plug);

    auto stage = Amino::any_cast<Amino::Ptr<BifrostUsd::Stage>>(amAny);

    int64_t cacheId = -1;
    bool    success;
    if (!stage) {
        // In this case, the Amino::Value does not contain any data,
        // (probably because the graph had a compilation error)
        // Add an empty object to keep the behaviour of this function
        // consistent with the case when the output port is disconnected.
        success = BifrostUsd::StageCache::addEmptyStageToCache(cacheId);
    } else {
        success = BifrostUsd::StageCache::addStageToCache(stage, cacheId);
    }

    if (success) {
        (const_cast<UsdTranslation*>(this))
            ->addStageForPort(
                (MFnDependencyNode(plug.node()).uuid().asString() + "." +
                 plug.partialName())
                    .asChar(),
                cacheId);
    }

    dataHandle.set(cacheId);
    block.setClean(plug);
    return success;
}

bool UsdTranslation::portAdded(Amino::String const&                  name,
                               BifrostGraph::Executor::PortDirection direction,
                               Amino::Type const&                    type,
                               Amino::Metadata const&                metadata,
                               BifrostGraph::Executor::PortClass     portClass,
                               PortData* translationData) const noexcept {
    (void)type;
    (void)metadata;

    assert(dynamic_cast<BifrostGraph::MayaTranslation::PortCreationData*>(
        translationData));
    auto* mayaHostdata =
        static_cast<BifrostGraph::MayaTranslation::PortCreationData*>(
            translationData);

    MObject const& object   = mayaHostdata->m_object;
    auto&          modifier = mayaHostdata->m_modifier;

    MString const attrName = name.c_str();
    bool const    isInput =
        portClass != BifrostGraph::Executor::PortClass::eTerminal &&
        direction == BifrostGraph::Executor::PortDirection::kInput;

    MStatus             status;
    MFnNumericAttribute attr;
    MObject             obj =
        attr.create(attrName, attrName, MFnNumericData::kInt64, -1, &status);
    CHECK_MSTATUS_AND_RETURN(status, false)
    if (obj.isNull()) return false;

    attr.setReadable(!isInput);
    attr.setWritable(isInput);
    attr.setKeyable(isInput);

    status = modifier.addAttribute(object, obj);
    CHECK_MSTATUS_AND_RETURN(status, false)
    status = modifier.doIt();
    CHECK_MSTATUS_AND_RETURN(status, false)

    MFnDependencyNode fnDep(object);

    // If it is an output automatically attach a Maya USD Proxy Node to it
    // so it immediately shows up in the viewport.
    if (!isInput) {
        // Check if the Maya Usd Plugin is loaded
        static MString const usdPluginName = "mayaUsdPlugin";
        MObject              usdPlugin = MFnPlugin::findPlugin(usdPluginName);

        if (usdPlugin != MObject::kNullObj) {
            MObject proxyObj;
            // Check if we are converting
            if (mayaHostdata->m_conversionData.m_conversionMode !=
                BifrostGraph::MayaTranslation::PortCreationData::
                    ConversionMode::kNone) {
                MSelectionList selList;
                status = selList.add(mayaHostdata->m_conversionData
                                         .m_convertFromNodeName.c_str());
                CHECK_MSTATUS_AND_RETURN(status, false)
                MObject oldObj;
                status = selList.getDependNode(0, oldObj);
                CHECK_MSTATUS_AND_RETURN(status, false)

                // get the port data
                auto it = (const_cast<UsdTranslation*>(this))
                              ->getPortData(
                                  (MString(mayaHostdata->m_conversionData
                                               .m_convertFromNodeUUID.c_str()) +
                                   "." + name.c_str())
                                      .asChar());
                if (it == m_portData.end()) {
                    // Should never get here
                    return false;
                }
                // update the name to the new node
                it->m_portName =
                    (fnDep.uuid().asString() + "." + name.c_str()).asChar();

                // Find connected proxyShape
                auto oldAttr =
                    MFnDependencyNode(oldObj).attribute(name.c_str());
                assert(!oldAttr.isNull());
                MPlug oldPlug(oldObj, oldAttr);
                if (oldPlug.isConnected()) {
                    MPlugArray destinations;
                    oldPlug.destinations(destinations);
                    for (unsigned d = 0; d < destinations.length(); ++d) {
                        auto const& dest = destinations[d];
                        if (MFnDependencyNode(dest.node()).typeName() ==
                            "mayaUsdProxyShape") {
                            assert(dest.partialName(false, false, false, false,
                                                    false,
                                                    true) == "stageCacheId");
                            proxyObj = dest.node();
                            // Disconnect the old node
                            status = modifier.disconnect(
                                oldObj, oldAttr, proxyObj, dest.attribute());
                            CHECK_MSTATUS_AND_RETURN(status, false)
                            status = modifier.doIt();
                            CHECK_MSTATUS_AND_RETURN(status, false)
                            break;
                        }
                    }
                }
            } else {
                // mayaUsdProxyShape is a dagObject, so will need to cast to
                // MDagModifier as MDGModifier::createNode is not virtual !
                // and createNode will NOT return the shape but the parent
                // transform
                assert(dynamic_cast<MDagModifier*>(&modifier));
                auto proxyTransformObj =
                    static_cast<MDagModifier&>(modifier).createNode(
                        "mayaUsdProxyShape");
                assert(!proxyTransformObj.isNull());
                status = modifier.doIt();
                CHECK_MSTATUS_AND_RETURN(status, false)

                MFnTransform proxyTransform(proxyTransformObj);
                assert(proxyTransform.childCount() == 1u);
                proxyObj = proxyTransform.child(0u);
                MFnDependencyNode proxyNode(proxyObj);
                // Contrarily to the mel "createNode mayaUsdProxyShape" command
                // which renames the transform based on the shape name, the
                // modifier operation keeps the default name ("transform").
                // To keep previous behaviour rename the transform.
                auto transformName = proxyNode.name();
                transformName.substitute("Shape", "");
                modifier.renameNode(proxyTransformObj, transformName);
                status = modifier.doIt();
                CHECK_MSTATUS_AND_RETURN(status, false)

                (const_cast<UsdTranslation*>(this))
                    ->m_portData.push_back(UsdPortData(
                        (fnDep.uuid().asString() + "." + name.c_str()).asChar(),
                        -1, proxyNode.name().asChar()));
            }

            if (!proxyObj.isNull()) {
                MFnDependencyNode proxyNode(proxyObj);
                auto shareStageAttr = proxyNode.attribute("shareStage");
                if (!shareStageAttr.isNull()) {
                    status = modifier.newPlugValueBool(
                        MPlug(proxyObj, shareStageAttr), false);
                    CHECK_MSTATUS_AND_RETURN(status, false)
                }

                status = modifier.connect(object, obj, proxyObj,
                                          proxyNode.attribute("stageCacheId"));
                CHECK_MSTATUS_AND_RETURN(status, false)

                // Find the time node if its connected to the proxy already
                MObject    timeObj;
                MPlugArray connections;
                auto       timeAttr = proxyNode.attribute("time");
                MPlug      timeToPlug(proxyObj, timeAttr);
                auto       timeFromPlug = timeToPlug.source();
                if (!timeFromPlug.isNull()) {
                    timeObj = timeFromPlug.node();
                }

                // Find the time node in the scene
                if (timeObj.isNull()) {
                    MSelectionList selList;
                    selList.add("time1");
                    if (!selList.isEmpty()) {
                        selList.getDependNode(0, timeObj);
                    }
                }

                // Connect the time to the mayaproxy
                if (!timeObj.isNull()) {
                    status = modifier.connect(
                        timeObj,
                        MFnDependencyNode(timeObj).attribute("outTime"),
                        proxyObj, timeAttr);
                    CHECK_MSTATUS_AND_RETURN(status, false)
                    status = modifier.doIt();
                    CHECK_MSTATUS_AND_RETURN(status, false)
                }
            }
        }
    }

    return true;
}

bool UsdTranslation::portRemoved(
    Amino::String const& name, Amino::String const& graphName) const noexcept {
    auto const fullName = graphName + "." + name;
    (const_cast<UsdTranslation*>(this))->removeStageForPort(fullName);
    (const_cast<UsdTranslation*>(this))->removePortData(fullName);
    return true;
}

bool UsdTranslation::portRenamed(
    Amino::String const& prevName,
    Amino::String const& name,
    Amino::String const& graphName) const noexcept {
    auto const prevFullName = graphName + "." + prevName;
    auto const fullName     = graphName + "." + name;

    auto item = (const_cast<UsdTranslation*>(this))->getPortData(name);
    if (item != m_portData.end()) {
        item->m_portName = fullName;
        return true;
    }

    return false;
}

void UsdTranslation::addStageForPort(Amino::String const& portName,
                                     int64_t              id) {
    auto it = getPortData(portName);
    if (it != m_portData.end()) {
        // Remove the old stage from the cache
        if (it->m_cacheId >= 0 && it->m_cacheId != id) {
            (void)removeStageForPort(portName);
        }
        it->m_cacheId = id;
    } else {
        m_portData.push_back(UsdPortData(portName, id, ""));
    }
}

bool UsdTranslation::removeStageForPort(Amino::String const& portName) {
    auto it = getPortData(portName);
    if (it != m_portData.end()) {
        // We need to check that there's no other port is pointing to this stage
        bool idStillInUse = false;
        for (auto const& item : m_portData) {
            if (it->m_portName == item.m_portName) continue; // skip yourself
            if (it->m_cacheId == item.m_cacheId) {
                idStillInUse = true;
                break;
            }
        }
        if (!idStillInUse) {
            return BifrostUsd::StageCache::removeStageFromCache(
                it->m_cacheId);
        }
    }
    return false;
}

typename Amino::Array<UsdTranslation::UsdPortData>::iterator
UsdTranslation::getPortData(Amino::String const& portName) {
    for (auto it = m_portData.begin(); it != m_portData.end(); ++it) {
        if (it->m_portName == portName) {
            return it;
        }
    }

    return m_portData.end();
}

void UsdTranslation::removePortData(Amino::String const& portName) {
    auto it = getPortData(portName);
    if (it != m_portData.end()) {
        m_portData.erase(it);
    }
}

extern "C" {
USD_MODULE_API BifrostGraph::Executor::TypeTranslation* createBifrostTypeTranslation(void) {
    return new UsdTranslation();
}
}
