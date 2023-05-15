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

#include <Amino/Core/Array.h>
#include <Amino/Core/Ptr.h>
#include <Amino/Core/String.h>

#include <Bifrost/FileUtils/FileUtils.h>
#include <BifrostGraph/Executor/Utility.h>

#include <pxr/usd/sdf/declareHandles.h>
#include <BifrostUsd/Layer.h>

#include <cstdlib>

#include "testUtils_export.h"

namespace BifrostUsd {

namespace TestUtils {

inline Amino::String getEnv(Amino::String const& envVarName) {
    Amino::String env;
    char*         buf = nullptr;
#if defined(_WIN32)
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, envVarName.c_str()) == 0 && buf != nullptr) {
        env = buf;
        free(buf);
    }
#else
    buf = std::getenv(envVarName.c_str());
    if (buf) {
        env = buf;
    }
#endif
    return env;
}

inline Amino::String getTestOutputDir() {
    return BifrostUsd::TestUtils::getEnv("USD_TEST_OUTPUT_DIR");
}

inline Amino::String getTestOutputPath(const Amino::String& filename) {
    return Bifrost::FileUtils::filePath(getTestOutputDir(), filename).c_str();
}

inline Amino::String getResourcePath(const Amino::String& filename) {
    Amino::String dirPath =
        BifrostUsd::TestUtils::getEnv("USD_TEST_RESOURCES_DIR");
    return Bifrost::FileUtils::filePath(dirPath, filename);
}

inline Amino::String getResourcePath(const Amino::Array<Amino::String>& names) {
    Amino::String path =
        BifrostUsd::TestUtils::getEnv("USD_TEST_RESOURCES_DIR");
    for (auto name : names) {
        path = Bifrost::FileUtils::filePath(path, name);
    }
    return path;
}

/// Helper to FindOrOpen some SdfLayers and add them as sublayers to the
/// given root SdfLayer.
///
/// \param [in,out] sdfRootLayer The root layer to which the sublayers will be
///                              added.
/// \param [in] subNames  The list of USD filenames for the sublayers to add to
///                       to the root layer. They must be listed from WEAKEST
///                       to STRONGEST.
/// \param [out] errorMsg Describes the errors that occurred during execution,
///                       if any.
/// \return true if the sublayers were successfully found and added to the
///              root layer; false otherwise.
USD_TESTUTILS_DECL
bool addSubLayers(
    pxr::SdfLayerRefPtr                 sdfRootLayer,
    const Amino::Array<Amino::String>&  subNames,
    Amino::String&                      errorMsg);

/// Helper to create an array of Bifrost USD layers.
///
/// \param [in] names The list of USD filenames for the USD layers to create.
/// \param [out] newLayers The array of newly created layers.
/// \param [out] errorMsg Describes the errors that occurred during execution,
///                       if any.
/// \return true if Bifrost layers were successfully created; false otherwise.
USD_TESTUTILS_DECL
bool createBifrostLayers(
    const Amino::Array<Amino::String>&              names,
    Amino::Array<Amino::Ptr<BifrostUsd::Layer>>&    newLayers,
    Amino::String&                                  errorMsg);

/// Helper to check the Paths of all sublayers of a root SdfLayer.
///
/// \param [in] sdfRootLayer The root layer.
/// \param [in] subNames  The list of USD names for the sublayers of the
///                       root layer. These names are expected to be
///                       contained in the Paths of the sub SdfLayers.
///                       These names must be listed from WEAKEST sublayer
///                       to the STRONGEST.
/// \param [out] errorMsg Describes the errors that occurred during execution,
///                       if any.
/// \return true if the root SdfLayer contains sublayers that match the
///         given USD names; false otherwise.
USD_TESTUTILS_DECL
bool checkSdfSublayerPaths(
    const pxr::SdfLayer&                sdfRootLayer,
    const Amino::Array<Amino::String>&  subNames,
    Amino::String&                      errorMsg);

/// Helper to append elements of an Array to another.
///
/// \param [in,out] left The Array to which new elements will be appended.
/// \param [in] right    The Array with elements to append.
/// \return The modified array.
template <typename T>
Amino::Array<T>& operator+=(Amino::Array<T>& left, const Amino::Array<T>& right) {
    left.reserve(left.size() + right.size());
    for(auto it = right.begin(); it != right.end(); ++it) {
        left.push_back(*it);
    }
    return left;
}

} // namespace TestUtils

} // namespace BifrostUsd
