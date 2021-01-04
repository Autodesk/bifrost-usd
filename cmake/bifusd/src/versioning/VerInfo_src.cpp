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
//==============================================================================
// EXTERNAL DECLARATIONS
//==============================================================================

#include <bifusd/versioning/VerFormat.h>
#include <bifusd/versioning/VerInfo.h>

//==============================================================================
// COMPONENT TRAIT
//==============================================================================

namespace {

//------------------------------------------------------------------------------
//
struct ComponentVersionTrait {
    // Using atoi here because these BIFUSD variables could have leading zeros.
    // clang-format off
    static int getMajorVersion() { return std::atoi("${BIFUSD_MAJOR_VERSION}");}
    static int getMinorVersion() { return std::atoi("${BIFUSD_MINOR_VERSION}");}
    static int getPatchLevel()   { return std::atoi("${BIFUSD_PATCH_LEVEL}");  }
    static int getBuildNumber()  { return std::atoi("${BIFUSD_JENKINS_BUILD}");}
    static char const* getGitCommit()    { return "${BIFUSD_GIT_COMMIT}";   }
    static char const* getGitBranch()    { return "${BIFUSD_GIT_BRANCH}";   }
    static char const* getBuildDate()    { return ${BIFUSD_PRODUCT_DATE};   }
    static char const* getProductName()  { return ${BIFUSD_PRODUCT_NAME};   }
    // clang-format on
};

} // namespace

namespace Bifusd {

//==============================================================================
// CLASS VersionInfo
//==============================================================================

//------------------------------------------------------------------------------
//
int VersionInfo::getMajorVersion() {
    return ComponentVersionTrait::getMajorVersion();
}

//------------------------------------------------------------------------------
//
int VersionInfo::getMinorVersion() {
    return ComponentVersionTrait::getMinorVersion();
}

//------------------------------------------------------------------------------
//
int VersionInfo::getPatchLevel() {
    return ComponentVersionTrait::getPatchLevel();
}

//------------------------------------------------------------------------------
//
std::string VersionInfo::getVersionID() {
    return VersionFormat<ComponentVersionTrait>::getVersionID();
}

//------------------------------------------------------------------------------
//
std::string VersionInfo::getVersionInfo() {
    return VersionFormat<ComponentVersionTrait>::getVersionInfo();
}

//------------------------------------------------------------------------------
//
std::string VersionInfo::getLibSuffix() {
    std::ostringstream builder;
    builder << "_" << getMajorVersion() << "_" << getMinorVersion();
    return builder.str();
}

//------------------------------------------------------------------------------
//
int VersionInfo::getFileFormatVersionID() {
    return int(FileFormatVersionNumbers::kCurrent);
}

} // namespace Bifusd
