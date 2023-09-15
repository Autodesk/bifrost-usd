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

#include <gtest/gtest.h>
#include <maya_plugin/usd_pack/usd_translator.h>
#include <pxr/usd/usdUtils/stageCache.h>
#include <utils/test/testUtils.h>
#include <BifrostUsd/Stage.h>

using namespace BifrostUsd::TestUtils;

#include <cstdlib>
#include <string>

TEST(UsdTranslationTests, addStageToCache) {
    auto translator =
        dynamic_cast<UsdTranslation*>(createBifrostTypeTranslation());

    ASSERT_TRUE(translator != nullptr);

    auto stage = Amino::newClassPtr<BifrostUsd::Stage>();

    auto cacheId = translator->AddStageToCache(stage, "blah");

    // On windows we need to cast to long int, otheriwse its a warning as error
    ASSERT_TRUE(
        PXR_NS::UsdUtilsStageCache::Get().Find(PXR_NS::UsdStageCache::Id::FromLongInt(
            static_cast<long int>(cacheId))) ==
        const_cast<BifrostUsd::Stage&>(*stage).getStagePtr());

    delete translator;
}

TEST(UsdTranslationTests, portRemoved) {
    auto translator =
        dynamic_cast<UsdTranslation*>(createBifrostTypeTranslation());

    ASSERT_TRUE(translator != nullptr);

    auto stage = Amino::newClassPtr<BifrostUsd::Stage>();

    auto cacheId = translator->AddStageToCache(stage, "someproxy.somestage");
    translator->portRemoved("somestage", "someproxy");

    // On windows we need to cast to long int, otheriwse its a warning as error
    ASSERT_FALSE(PXR_NS::UsdUtilsStageCache::Get().Find(
        PXR_NS::UsdStageCache::Id::FromLongInt(static_cast<long int>(cacheId))));

    delete translator;
}
