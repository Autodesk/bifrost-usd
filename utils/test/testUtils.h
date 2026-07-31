//-
// Copyright 2026 Autodesk, Inc.
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

#ifndef BIFROSTUSD_UTILS_TEST_TESTUTILS_H
#define BIFROSTUSD_UTILS_TEST_TESTUTILS_H

#include <Amino/Core/Array.h>
#include <Amino/Core/Ptr.h>
#include <Amino/Core/String.h>
#include <Amino/Core/StringView.h>

#include <Bifrost/FileUtils/FileUtils.h>
#include <BifrostGraph/Executor/Utility.h>
#include <BifrostUsd/Layer.h>

#include <pxr/usd/sdf/declareHandles.h>

#include <cstdlib>
#include <string>

#include "testUtils_export.h"

using StringArray = Amino::Array<Amino::String>;

namespace BifrostUsd {

namespace TestUtils {

inline Amino::String getTestOutputDir() {
    return BifrostGraph::Executor::Utility::getEnv("USD_TEST_OUTPUT_DIR");
}

inline Amino::String getTestOutputPath(const Amino::String& filename) {
    return Bifrost::FileUtils::filePath(getTestOutputDir(), filename);
}

/// \brief Join all messages from a StringArray into a single string for easier
/// printing.
USD_TESTUTILS_DECL
Amino::String printMessages(const StringArray& messages);

/// \brief Utility function to set an environment variable.
USD_TESTUTILS_DECL
void setEnv(const char* evName, const char* evValue);

/// \brief Creates a unique subdirectory within the specified base directory.
///
/// \param [in] baseDirectory The parent directory where the subdirectory will be created
/// \param [in] prefix The prefix for the subdirectory name (default: "tmp")
/// \param [in] maxAttempts Maximum number of attempts to find a unique name (default: 32)
/// \param [out] errorMessage On failure, will be set to a human-readable error message
/// \return The full path of the created subdirectory, or empty string on failure
USD_TESTUTILS_DECL
Amino::String createUniqueSubdir(Amino::StringView baseDirectory,
                                 Amino::StringView prefix       = "tmp",
                                 unsigned int      maxAttempts  = 32,
                                 Amino::String*    errorMessage = nullptr);

/// \class UniqueTestOutputSubdir
/// \brief Lazy RAII helper for a per-test unique output subdirectory.
///
/// Creates (on first use) a uniquely named child directory under the root
/// test output directory defined by getTestOutputDir(). The unique subdirectory
/// name is prefixed with the user-provided prefix.
///
/// Construction does not create the directory. The first call to getDir_abs(),
/// getDir_rel(), or getPath_abs() triggers creation (via createUniqueSubdir()).
/// If autoDelete is true, the destructor attempts best-effort recursive removal
/// of the created subdirectory.
///
/// Usage example:
///     UniqueTestOutputSubdir tmp{"myTestFile"};
///     auto filePath = tmp.getPath_abs("layer.usda"); // Create subdir and
///                                                    // return file path
///
class USD_TESTUTILS_DECL UniqueTestOutputSubdir {
public:
    explicit UniqueTestOutputSubdir(Amino::StringView prefix,
                                    bool              autoDelete = true)
        : m_prefix(prefix),
          m_subdir_abs(),
          m_created(false),
          m_autoDelete(autoDelete) {}
    ~UniqueTestOutputSubdir();

    /// @brief Gets the absolute path to the test subdirectory.
    /// This function returns the absolute path of the test output subdirectory.
    /// If the test subdirectory does not exist, it will be created.
    /// @return An Amino::String containing the absolute path to the directory.
    Amino::String getDir_abs();

    /// @brief Gets the relative path to the test subdirectory.
    /// This function returns the relative path of the test output subdirectory.
    /// If the test subdirectory does not exist, it will be created.
    /// For example, if the absolute path is "/tmp/baseDir/myTestFile_12345_1",
    /// the relative path would be "./myTestFile_12345_1".
    /// @return An Amino::String containing the relative path to the directory.
    Amino::String getDir_rel();

    /// @brief Converts a file name to an absolute file path.
    /// This function takes a filename and returns its absolute path within the
    /// test output subdirectory.
    /// If the test subdirectory does not exist, it will be created.
    /// @param filename The filename to convert to an absolute file path.
    /// @return An Amino::String containing the absolute path to the specified
    /// file.
    Amino::String getPath_abs(Amino::StringView filename);

private:
    bool ensureCreated();
    void reset();

    Amino::String m_prefix;
    Amino::String m_subdir_abs;
    bool          m_created    = false;
    bool          m_autoDelete = true;
};

inline Amino::String getResourcePath(const Amino::String& filename) {
    Amino::String dirPath =
        BifrostGraph::Executor::Utility::getEnv("USD_TEST_RESOURCES_DIR");
    return Bifrost::FileUtils::filePath(dirPath, filename);
}

inline Amino::String getResourcePath(const Amino::Array<Amino::String>& names) {
    Amino::String path =
        BifrostGraph::Executor::Utility::getEnv("USD_TEST_RESOURCES_DIR");
    for (const auto& name : names) {
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
bool addSubLayers(PXR_NS::SdfLayerRefPtr             sdfRootLayer,
                  const Amino::Array<Amino::String>& subNames,
                  Amino::String&                     errorMsg);

/// Helper to create an array of Bifrost USD layers.
///
/// \param [in] names The list of USD filenames for the USD layers to create.
/// \param [out] newLayers The array of newly created layers.
/// \param [out] errorMsg Describes the errors that occurred during execution,
///                       if any.
/// \return true if Bifrost layers were successfully created; false otherwise.
USD_TESTUTILS_DECL
bool createBifrostLayers(const Amino::Array<Amino::String>&           names,
                         Amino::Array<Amino::Ptr<BifrostUsd::Layer>>& newLayers,
                         Amino::String&                               errorMsg);

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
bool checkSdfSublayerPaths(const PXR_NS::SdfLayer&            sdfRootLayer,
                           const Amino::Array<Amino::String>& subNames,
                           Amino::String&                     errorMsg);

/// Helper to append elements of an Array to another.
///
/// \param [in,out] left The Array to which new elements will be appended.
/// \param [in] right    The Array with elements to append.
/// \return The modified array.
template <typename T>
Amino::Array<T>& operator+=(Amino::Array<T>&       left,
                            const Amino::Array<T>& right) {
    left.reserve(left.size() + right.size());
    for (auto it = right.begin(); it != right.end(); ++it) {
        left.push_back(*it);
    }
    return left;
}

} // namespace TestUtils

} // namespace BifrostUsd

#endif // BIFROSTUSD_UTILS_TEST_TESTUTILS_H
