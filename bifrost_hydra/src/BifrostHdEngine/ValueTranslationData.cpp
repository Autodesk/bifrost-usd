//-
// Copyright 2025 Autodesk, Inc.
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

#include <BifrostHydra/Engine/ValueTranslationData.h>

#include <BifrostHydra/Engine/JobTranslationData.h>
#include <BifrostHydra/Engine/Parameters.h>
#include <BifrostHydra/Translators/GeometryFn.h>

#include <Bifrost/Math/Types.h>
#include <Bifrost/Object/Object.h>

#include <Amino/Core/Any.h>
#include <Amino/Core/Array.h>
#include <Amino/Core/Ptr.h>
#include <Amino/Core/String.h>

#include <pxr/usd/sdf/types.h>
#include "pxr/imaging/hd/tokens.h"

namespace BifrostHd {

InputValueData::InputValueData(JobTranslationData& jobTranslationData,
                               std::string         name,
                               Amino::Any          defaultVal)
    : BifrostGraph::Executor::TypeTranslation::ValueData(),
      m_jobTranslationData(jobTranslationData),
      m_name(std::move(name)),
      m_defaultVal(std::move(defaultVal)) {}

InputValueData::~InputValueData() = default;

Amino::Any InputValueData::getInput() const {
    const auto& inputs     = m_jobTranslationData.getParameters().inputs();
    const auto& inputScene = m_jobTranslationData.getParameters().inputScene();

    auto search = inputs.find(m_name);
    if (search != inputs.end()) {
        const auto& vtValue = search->second;

        if (vtValue.IsHolding<bool>()) {
            return Amino::Any{vtValue.UncheckedGet<bool>()};
        } else if (vtValue.IsHolding<unsigned int>()) {
            return Amino::Any{vtValue.UncheckedGet<unsigned int>()};
        } else if (vtValue.IsHolding<int>()) {
            return Amino::Any{vtValue.UncheckedGet<int>()};
        } else if (vtValue.IsHolding<float>()) {
            return Amino::Any{vtValue.UncheckedGet<float>()};
        } else if (vtValue.IsHolding<PXR_NS::GfVec3f>()) {
            auto                  gf3 = vtValue.UncheckedGet<PXR_NS::GfVec3f>();
            Bifrost::Math::float3 flt3{gf3[0], gf3[1], gf3[2]};
            return Amino::Any{flt3};
        } else if (vtValue.IsHolding<std::int64_t>()) {
            return Amino::Any{static_cast<Amino::long_t>(
                vtValue.UncheckedGet<std::int64_t>())};
        } else if (vtValue.IsHolding<std::string>()) {
            return Amino::Any{Amino::String{vtValue.UncheckedGet<std::string>().c_str()}};
        } else if (vtValue.IsHolding<PXR_NS::VtArray<PXR_NS::SdfPath>>()) {
            auto paths = vtValue.UncheckedGet<PXR_NS::VtArray<PXR_NS::SdfPath>>();
            if (paths.size() == 1) {
                auto sourceMeshPrim = inputScene->GetPrim(paths[0]);
                if (sourceMeshPrim.primType == PXR_NS::HdPrimTypeTokens->mesh) {
                    auto obj = BifrostHd::CreateBifrostMesh(sourceMeshPrim);
                    if (obj) {
                        return Amino::Any{obj};
                    }
                }
            }
        }
    }
    return m_defaultVal;
}

const JobTranslationData& InputValueData::jobTranslationData() const {
    return m_jobTranslationData;
}

OutputValueData::OutputValueData(JobTranslationData& jobTranslationData,
                                 std::string         name)
    : BifrostGraph::Executor::TypeTranslation::ValueData(),
      m_jobTranslationData(jobTranslationData),
      m_name(std::move(name)) {}

OutputValueData::~OutputValueData() = default;

bool OutputValueData::setOutput(const Amino::Any& value) {
    auto& output = m_jobTranslationData.getParameters().output();
    if (output.first == m_name) {
        // Covers Terminals which are array<array<objects>>
        bool is0DArray = value.type() == Amino::getTypeId<Amino::Ptr<Bifrost::Object>>();
        bool is1DArray = value.type() == Amino::getTypeId<Amino::Ptr<Amino::ArrayD_t<1, Amino::Ptr<Bifrost::Object>>>>();
        bool is2DArray = value.type() == Amino::getTypeId<Amino::Ptr<Amino::ArrayD_t<2, Amino::Ptr<Bifrost::Object>>>>();
        if( is2DArray )
        {
            auto obj2DArray = Amino::any_cast<Amino::Ptr<Amino::ArrayD_t<2, Amino::Ptr<Bifrost::Object>>>>(value);
            if (obj2DArray != nullptr && !obj2DArray->empty()) {
                for (auto& obj1DArray : *obj2DArray) {
                    for (auto& object : *obj1DArray) {
                        output.second.push_back(object);
                    }
                }
                return true;
            }
        }
        else if( is1DArray )
        {
            auto obj1DArray = Amino::any_cast<Amino::Ptr<Amino::ArrayD_t<1, Amino::Ptr<Bifrost::Object>>>>(value);
            if (obj1DArray != nullptr && !obj1DArray->empty()) {
                for (auto& object : *obj1DArray) {
                    output.second.push_back(object);
                }
                return true;
            }
        }
        else if( is0DArray )
        {   auto object = Amino::any_cast<Amino::Ptr<Bifrost::Object>>(value);
            if (object) {
                output.second.push_back(object);
                return true;
            }
        }
    }
    return false;
}

const JobTranslationData& OutputValueData::jobTranslationData() const {
    return m_jobTranslationData;
}

} // namespace BifrostHd
