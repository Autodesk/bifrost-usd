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

#include <BifrostUsd/StageCache.h>
#include <BifrostUsd/Stage.h>

#include <pxr/usd/usdUtils/stageCache.h>

#include <limits>

namespace BifrostUsd {

bool StageCache::addEmptyStageToCache( int64_t& outId ){

    auto stage = Amino::newClassPtr<BifrostUsd::Stage>();

    return StageCache::addStageToCache(stage, outId);
}

bool StageCache::addStageToCache(const Amino::Ptr<BifrostUsd::Stage>& stage,
                                 int64_t& outId) {
    outId = -1;
    if (stage) {
        auto usdStagePtr =
            const_cast<Stage&>(*stage).getStagePtr();
        auto id = pxr::UsdUtilsStageCache::Get().Insert(usdStagePtr);

        if (id && id.IsValid()) {
            // Note: on the USD side, this is implemented as a long int
            // Which is platform dependent.  We interface ith a 64 bit int.
            outId = id.ToLongInt();
        }
    }

    return outId != -1;
}

bool StageCache::removeStageFromCache(int64_t id) {
#if defined(_WIN32)
        // Note: on the USD side, this is implemented as a long int
        // Which is platform dependent.  We interface ith a 64 bit int.
        // So we cast.
        assert(id <= std::numeric_limits<int>::max());
        return pxr::UsdUtilsStageCache::Get().Erase(
            pxr::UsdStageCache::Id::FromLongInt(static_cast<int>(id)));
#else
        return pxr::UsdUtilsStageCache::Get().Erase(
            pxr::UsdStageCache::Id::FromLongInt(id));
#endif
}

} // namespace BifrostUsd
