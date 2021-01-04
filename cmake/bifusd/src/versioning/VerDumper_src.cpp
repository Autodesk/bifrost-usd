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
/// \file  VerDumper.src.cpp
/// \brief Version information dumper for each component
///
/// This file contains the "file template" for generating a version information
/// dumper for each shared libraries and executable. It is referenced by
/// the CMake functions bifusd_create_shared_lib() bifusd_create_executable().
///
/// The version information dumper is activated by the
/// BIFUSD_DUMP_VERSION_INFO environment variable.

//==============================================================================
// EXTERNAL DECLARATIONS
//==============================================================================

#include <bifusd/config/CfgWarningMacros.h>
#include <bifusd/versioning/VerFormat.h>

#include <cstdlib>
#include <iostream>

//==============================================================================
// PRIVATE FUNCTIONS
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

//------------------------------------------------------------------------------
//
bool getEnvAsBool(char const* evName) {
    BIFUSD_WARNING_PUSH
    // warning C4996: 'getenv': This function or variable may be
    //                unsafe. Consider using _dupenv_s instead. To disable
    //                deprecation, use _CRT_SECURE_NO_WARNINGS. See online
    //                help for details.
    BIFUSD_WARNING_DISABLE_MSC(4996)
    char const* evStrVal = std::getenv(evName);
    BIFUSD_WARNING_POP

    return evStrVal ? std::atoi(evStrVal) != 0 : false;
}

//------------------------------------------------------------------------------
//
bool dumpVersionInfo() {
    bool doDump = getEnvAsBool("BIFUSD_DUMP_VERSION_INFO");

#ifdef BIFUSD_WINDOWS
    // It's possible that _fileno fails if the host application does not yet
    // have a proper pipe setup for stderr. In this case, avoid outputting to
    // std:cerr as this would make the stream in an unrecoverable fail state.

    // From Microsoft:
    // If stdout or stderr is not associated with an output stream(for example,
    // in a Windows application without a console window), the file descriptor
    // returned is - 2. In previous versions, the file descriptor returned was
    // - 1. This change allows applications to distinguish this condition from
    // an error.
    if (_fileno(stderr) < 0) {
        doDump = false;
    }
#endif

    if (doDump) {
        std::cerr
            << std::setw(16) << "${BIFUSD_COMPONENT_NAME}"
            << ": "
            << Bifusd::VersionFormat<ComponentVersionTrait>::getVersionInfo()
            << std::endl;
    }

    return doDump;
}

//------------------------------------------------------------------------------
//
bool initializer = dumpVersionInfo();

} // namespace
