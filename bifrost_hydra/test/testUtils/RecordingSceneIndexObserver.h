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

#ifndef BIFROST_HD_RECORDING_SCENE_INDEX_OBSERVER_H
#define BIFROST_HD_RECORDING_SCENE_INDEX_OBSERVER_H

#include <pxr/imaging/hd/sceneIndexObserver.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>

namespace BifrostHdTest {

using UsdImagingGLEngineSharedPtr = std::shared_ptr<class PXR_NS::UsdImagingGLEngine>;

// used for capturing scene change notices
class RecordingSceneIndexObserver : public PXR_NS::HdSceneIndexObserver {
public:
    enum EventType {
        EventType_PrimAdded = 0,
        EventType_PrimRemoved,
        EventType_PrimDirtied,
    };

    struct Event {
        EventType           eventType;
        PXR_NS::SdfPath             primPath;
        PXR_NS::TfToken             primType;
        PXR_NS::HdDataSourceLocator locator;
    };

    using EventVector = std::vector<Event>;

    void PrimsAdded(const PXR_NS::HdSceneIndexBase& sender,
                    const AddedPrimEntries& entries) override;

    void PrimsRemoved(const PXR_NS::HdSceneIndexBase&   sender,
                      const RemovedPrimEntries& entries) override;

    void PrimsDirtied(const PXR_NS::HdSceneIndexBase&   sender,
                      const DirtiedPrimEntries& entries) override;

#if PXR_VERSION >= 2308
    void PrimsRenamed(const PXR_NS::HdSceneIndexBase& sender,
                      const RenamedPrimEntries&    entries) override;
#endif

    EventVector GetEvents();

    void Clear();

private:
    EventVector _events;
};

} // end namespace BifrostHdTest
#endif // BIFROST_HD_RECORDING_SCENE_INDEX_OBSERVER_H
