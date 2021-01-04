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
/// \file  VerInfo.h
/// \brief Bifusd::VersionInfo: Information about the version of the product
/// \see   Bifusd::VersionInfo

#ifndef BIFUSD_VER_VERSION_INFO_H
#define BIFUSD_VER_VERSION_INFO_H

//==============================================================================
// EXTERNAL DECLARATIONS
//==============================================================================

#include <string>

namespace Bifusd {

//==============================================================================
// CLASS DECLARATIONS
//==============================================================================

//==============================================================================
// CLASS VersionInfo
//==============================================================================

/// \brief Information about the version of the product
class VersionInfo {
public:
    /// \brief Enum that tracks our file format version numbers
    /// \details Add to the enum and update the kCurrent value
    /// as new formats are introduced
    enum class FileFormatVersionNumbers {
        // Jan 2015 : (major,minor) = (1,0)
        kInitial = 100,
        //
        kCurrent = kInitial
    };

    /*----- static member functions -----*/

    /// \brief Returns the components of the product version number.
    ///
    /// The version number is of the form "<major>.<minor>.<patch>"
    ///
    /// \{
    static int getMajorVersion();
    static int getMinorVersion();
    static int getPatchLevel();
    /// \}

    /// \brief Returns a short version identifier
    ///
    /// Returns a short version identifier such as for example "3.0.2".
    static std::string getVersionID();

    /// \brief Returns detailed information about the version of the product.
    ///
    /// Returns detailed information about the version of the product such
    /// as for example:
    ///
    ///  "MyProduct version 4.0.0 (Mon 09/28/2015, 201509282010, build 34567,
    ///  commit b5283ef)"
    ///
    /// Note that only the short git hash is included.
    static std::string getVersionInfo();

    /// \brief Returns the version suffix of the libraries
    ///
    /// Returns a short version identifier such as for example "4_0".
    static std::string getLibSuffix();

    /// \brief Returns the file version number
    ///
    /// File format version ID is specified by [major][minor]. The minor
    /// number is two digits and the major is 1 or 2 digits. For
    /// example 100 ( major = 1, minor = 0 )
    static int getFileFormatVersionID();

private:
    /*----- member functions -----*/

    /// \brief Prohibited and not implemented
    ///
    /// Don't allow construction since we only have static member functions.
    VersionInfo() = delete;
};

} // namespace Bifusd

#endif
