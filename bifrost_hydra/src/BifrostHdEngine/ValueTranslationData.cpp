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

#include <BifrostHydra/Engine/ValueTranslationData.h>

#include <BifrostHydra/Engine/JobTranslationData.h>
#include <BifrostHydra/Engine/Parameters.h>
#include <BifrostHydra/Translators/GeometryFn.h>

#include <Bifrost/Math/Types.h>
#include <Bifrost/Object/Object.h>

#include <pxr/usd/sdf/types.h>
#include "pxr/imaging/hd/tokens.h"

namespace BifrostHd {

ValueTranslationData::ValueTranslationData(
    JobTranslationData& jobTranslationData,
    Amino::Any          defaultVal,
    std::string         name)
    : BifrostGraph::Executor::JobPreview::ValueData(),
      m_defaultVal(std::move(defaultVal)),
      m_jobTranslationData(jobTranslationData),
      m_name(std::move(name)) {}

ValueTranslationData::~ValueTranslationData() = default;

Amino::Any ValueTranslationData::getInput(Amino::Type const& /*type*/) const {
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

bool ValueTranslationData::setOutput(const Amino::Any& value) {
    auto& output = m_jobTranslationData.getParameters().output();
    if (output.first == m_name) {
        if (value.type() == Amino::getTypeId<Amino::Ptr<Amino::Array<Amino::Ptr<Bifrost::Object>>>>()) {
            auto objectArray = Amino::any_cast<Amino::Ptr<Amino::Array<Amino::Ptr<Bifrost::Object>>>>(value);
            assert(objectArray != nullptr);
            if (objectArray != nullptr && !objectArray->empty()) {
                for (auto& object : *objectArray) {
                    output.second.push_back(object);
                }
                return true;
            }
         } else if (value.type() == Amino::getTypeId<Amino::Ptr<Bifrost::Object>>()) {
            auto object = Amino::any_cast<Amino::Ptr<Bifrost::Object>>(value);
            if (object) {
                output.second.push_back(object);
                return true;
            }
        }
    }
    return false;
}

const JobTranslationData& ValueTranslationData::jobTranslationData() const {
    return m_jobTranslationData;
}
} // namespace BifrostHd
