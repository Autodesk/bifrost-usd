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

#ifndef BIFROST_HD_PARAMETERS_H
#define BIFROST_HD_PARAMETERS_H

#include <Bifrost/Object/Object.h>

#include <pxr/imaging/hd/sceneIndex.h>

#include <memory>
#include <unordered_map>

namespace BifrostHd {

using Inputs = std::unordered_map<std::string, PXR_NS::VtValue>;
using Output = std::pair<std::string, Amino::Array<Amino::Ptr<Bifrost::Object>>>;

/// \class Parameters
///
/// Interface to access the Bifrost Graph and parameters stored
/// in a HdSceneIndexPrim of type hydraGenerativeProcedural
///

class Parameters {
public:
    Parameters();

    ~Parameters();

    void setInputScene(PXR_NS::HdSceneIndexBaseRefPtr inputScene);

    PXR_NS::HdSceneIndexBaseRefPtr inputScene() const;

    void setInputs(PXR_NS::HdSceneIndexPrim const& prim);

    const std::string& compoundName() const;

    const Inputs& inputs() const;

    Output& output();

public:
    /// Disabled
    /// \{
    Parameters(const Parameters&)             = delete;
    Parameters(Parameters&&)            = delete;
    Parameters& operator=(const Parameters&)  = delete;
    Parameters& operator=(Parameters&&) = delete;
    /// \}

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace BifrostHd

#endif // BIFROST_HD_PARAMETERS_H
