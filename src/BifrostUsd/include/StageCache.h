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

/// \file StageCache.h
///
/// \brief Bifrost USD Stage Cache wrapper
///

#ifndef VALUE_SEMANTIC_USD_STAGE_CACHE_H
#define VALUE_SEMANTIC_USD_STAGE_CACHE_H

#include "BifrostUsdExport.h"

#include <Amino/Core/String.h>
#include <Amino/Core/Ptr.h>

// Forward
namespace BifrostUsd{
class Stage;
}

namespace BifrostUsd {

/// \brief Wrap the USD stage cache.
struct USD_DECL StageCache{
    /// \brief Add an empty stage in the cache.
    /// \param [out] outId id
    ///
    /// \return True if sucessfull.
    static bool  addEmptyStageToCache(int64_t& outId);

    /// \brief Add a stage to the cache.
    /// \param [in]  stage Input stage.
    /// \param [out] outId Id of the inserted stage, if success.
    ///
    /// \return True if insertion succeeded, false otherwise.
    static bool addStageToCache(const Amino::Ptr<BifrostUsd::Stage>& stage,
                            int64_t& outId);

    /// \brief Remove a stage from the cache.
    /// \param [in] id Stage id to remove.
    ///
    /// \return True if removed (erased) from the cache.
    static bool removeStageFromCache(int64_t id);
};
} // namespace BifrostUsd
#endif /* VALUE_SEMANTIC_USD_STAGE_CACHE_H */
