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

#include "RecordingSceneIndexObserver.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace BifrostHdTest {

void RecordingSceneIndexObserver::PrimsAdded(const HdSceneIndexBase& /*sender*/,
                                             const AddedPrimEntries& entries) {
    for (const AddedPrimEntry& entry : entries) {
        _events.emplace_back(Event{EventType_PrimAdded, entry.primPath,
                                   entry.primType, HdDataSourceLocator()});
    }
}

void RecordingSceneIndexObserver::PrimsRemoved(
    const PXR_NS::HdSceneIndexBase& /*sender*/,
    const RemovedPrimEntries& entries) {
    for (const RemovedPrimEntry& entry : entries) {
        _events.emplace_back(Event{EventType_PrimRemoved, entry.primPath,
                                   TfToken{}, HdDataSourceLocator()});
    }
}

void RecordingSceneIndexObserver::PrimsDirtied(
    const PXR_NS::HdSceneIndexBase& /*sender*/,
    const DirtiedPrimEntries& entries) {
    for (const DirtiedPrimEntry& entry : entries) {
        for (const HdDataSourceLocator& locator : entry.dirtyLocators) {
            _events.emplace_back(Event{EventType_PrimDirtied, entry.primPath,
                                       TfToken(), locator});
        }
    }
}

#if PXR_VERSION >= 2308
void RecordingSceneIndexObserver::PrimsRenamed(
    const PXR_NS::HdSceneIndexBase& /*sender*/,
    const PXR_NS::HdSceneIndexObserver::RenamedPrimEntries& /*entries*/) {}
#endif

RecordingSceneIndexObserver::EventVector RecordingSceneIndexObserver::GetEvents() { return _events; }

void RecordingSceneIndexObserver::Clear() { _events.clear(); }

} // end namespace BifrostHdTest
