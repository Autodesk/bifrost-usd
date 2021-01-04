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
/// \file  VerFormat.h
/// \brief Bifusd::VersionFormat: Utility class for formatting version
///                                information.
/// \see   Bifusd::VersionFormat

#ifndef BIFUSD_VER_VERSION_FORMAT_H
#define BIFUSD_VER_VERSION_FORMAT_H

//==============================================================================
// EXTERNAL DECLARATIONS
//==============================================================================

#include <iomanip>
#include <sstream>

namespace Bifusd {

//==============================================================================
// CLASS DECLARATIONS
//==============================================================================

//==============================================================================
// CLASS VersionFormat
//==============================================================================

/// \brief Utility class for formatting version information.
///
/// The VersionTrait is typically generated using CMake configuration files. It
/// contains all of the version information of given component (library,
/// executable, etc.). This class contains functions to format this version
/// information to a human readable form.
///
/// The VersionTrait must provide the following static member functions:
///
///     static int         getMajorVersion();
///     static int         getMinorVersion();
///     static int         getPatchLevel();
///     static char const* getGitCommit();
///     static char const* getGitBranch();
///     static int         getBuildNumber();
///     static char const* getBuildDate();
///     static char const* getProductName();
///
/// \tparam VersionTrait CMake generated version trait.
template <class VersionTrait>
class VersionFormat {
public:
    /*----- static member functions -----*/

    /// \brief Returns a short version identifier for the given version trait.
    ///
    /// Returns a short version identifier such as for example "3.0.2".
    static std::string getVersionID() {
        std::ostringstream builder;
        builder << VersionTrait::getMajorVersion() << "."
                << VersionTrait::getMinorVersion() << "."
                << VersionTrait::getPatchLevel();
        return builder.str();
    }

    /// \brief Returns detailed information for the given version trait.
    ///
    /// Returns detailed information about the version of the binary such
    /// as for example:
    ///
    ///  "MyProduct version 4.0.0 (Mon 09/28/2015, 201509282010, build 34567,
    ///  commit b5283ef, branch origin/master)"
    ///
    /// Note that only the short git hash is included.
    static std::string getVersionInfo() {
        std::ostringstream builder;
        builder << VersionTrait::getProductName() << " version "
                << getVersionID() << " (" << VersionTrait::getBuildDate()
                << ", build " << VersionTrait::getBuildNumber() << ", commit "
                << std::setw(7)
                << std::string(VersionTrait::getGitCommit()).substr(0, 7)
                << ", branch " << VersionTrait::getGitBranch() << ")";
        return builder.str();
    }

private:
    /*----- member functions -----*/

    /// \brief Prohibited and not implemented
    ///
    /// Don't allow construction since we only have static member functions.
    VersionFormat() = delete;
};

} // namespace Bifusd

#endif
