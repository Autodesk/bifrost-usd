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
#include <Amino/Core/String.h>
#include <gtest/gtest.h>
#include <nodedefs/usd_pack/usd_prim_nodedefs.h>
#include <nodedefs/usd_pack/usd_stage_nodedefs.h>
#include <nodedefs/usd_pack/usd_variantset_nodedefs.h>
#include <utils/test/testUtils.h>

#include <cstdlib>
#include <string>

using namespace BifrostUsd::TestUtils;

namespace {
auto addVariantSet(BifrostUsd::Stage& stage,
                   const Amino::String& prim_path,
                   const Amino::String& name) {
    auto primPath = pxr::SdfPath(prim_path.c_str());
    auto prim     = stage->DefinePrim(primPath);
    USD::VariantSet::add_variant_set(stage, primPath.GetText(), name);
    return prim;
}
} // namespace

TEST(VariantSetNodeDefs, add_variant_set) {
    BifrostUsd::Stage stage;
    auto                prim_path        = Amino::String("/a");
    auto                variant_set_name = Amino::String("look");

    auto prim = addVariantSet(stage, prim_path, variant_set_name);

    auto vset = prim.GetVariantSet("look");
    ASSERT_TRUE(vset);
}

TEST(VariantSetNodeDefs, add_variant) {
    BifrostUsd::Stage stage;
    auto                prim_path        = Amino::String("/a");
    auto                variant_set_name = Amino::String("look");
    auto prim = addVariantSet(stage, prim_path, variant_set_name);

    auto variant_name          = Amino::String("red");
    bool set_variant_selection = true;
    USD::VariantSet::add_variant(stage, prim_path, variant_set_name,
                                 variant_name, set_variant_selection);

    auto vset = prim.GetVariantSet("look");

    auto expectedVariants = std::vector<std::string>{"red"};
    ASSERT_EQ(vset.GetVariantNames(), expectedVariants);
    ASSERT_EQ(vset.GetVariantSelection(), std::string("red"));

    variant_name = Amino::String("green");
    USD::VariantSet::add_variant(stage, prim_path, variant_set_name,
                                 variant_name, set_variant_selection);

    expectedVariants = std::vector<std::string>{"green", "red"};
    ASSERT_EQ(vset.GetVariantNames(), expectedVariants);
    ASSERT_EQ(vset.GetVariantSelection(), std::string("green"));

    variant_name          = Amino::String("blue");
    set_variant_selection = false;
    USD::VariantSet::add_variant(stage, prim_path, variant_set_name,
                                 variant_name, set_variant_selection);

    expectedVariants = std::vector<std::string>{"blue", "green", "red"};
    ASSERT_EQ(vset.GetVariantNames(), expectedVariants);
    ASSERT_EQ(vset.GetVariantSelection(), std::string("green"));
}

TEST(VariantSetNodeDefs, set_variant_selection) {
    BifrostUsd::Stage stage;
    auto                prim_path        = Amino::String("/a");
    auto                variant_set_name = Amino::String("look");
    auto prim = addVariantSet(stage, prim_path, variant_set_name);

    auto variant_name          = Amino::String("red");
    bool set_variant_selection = false;
    USD::VariantSet::add_variant(stage, prim_path, variant_set_name,
                                 variant_name, set_variant_selection);

    auto vset = prim.GetVariantSet("look");
    ASSERT_EQ(vset.GetVariantSelection(), std::string());

    bool clear = false;
    USD::VariantSet::set_variant_selection(stage, prim_path, variant_set_name,
                                           variant_name, clear);
    ASSERT_EQ(vset.GetVariantSelection(), std::string("red"));

    clear = true;
    USD::VariantSet::set_variant_selection(stage, prim_path, variant_set_name,
                                           variant_name, clear);
    ASSERT_EQ(vset.GetVariantSelection(), std::string());
}

TEST(VariantSetNodeDefs, get_variant_sets) {
    BifrostUsd::Stage stage;
    auto                prim_path = Amino::String("/a");
    
    auto names = Amino::MutablePtr<Amino::Array<Amino::String>>();
    USD::VariantSet::get_variant_sets(stage, prim_path, names);
    ASSERT_EQ(names->size(), 0);

    auto variant_set_name = Amino::String("vset");
    addVariantSet(stage, prim_path, variant_set_name);
    
    names->clear();
    USD::VariantSet::get_variant_sets(stage, prim_path, names);
    ASSERT_EQ(names->size(), 1);
    ASSERT_EQ((*names)[0], variant_set_name);
}

TEST(VariantSetNodeDefs, get_variants) {
    BifrostUsd::Stage stage;
    auto                prim_path        = Amino::String("/a");
    auto                variant_set_name = Amino::String("vset");
    addVariantSet(stage, prim_path, variant_set_name);

    auto names = Amino::MutablePtr<Amino::Array<Amino::String>>();
    USD::VariantSet::get_variants(stage, prim_path, variant_set_name, names);
    ASSERT_EQ(names->size(), 0);

    auto variant_name          = Amino::String("a");
    bool set_variant_selection = false;
    USD::VariantSet::add_variant(stage, prim_path, variant_set_name,
                                 variant_name, set_variant_selection);

    names->clear();
    USD::VariantSet::get_variants(stage, prim_path, variant_set_name, names);
    ASSERT_EQ(names->size(), 1);
    ASSERT_EQ((*names)[0], variant_name);
}
