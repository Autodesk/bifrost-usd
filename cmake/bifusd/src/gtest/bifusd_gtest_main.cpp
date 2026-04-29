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

// Copyright 2006, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifdef BIFUSD_USING_TBB
#include <bifusd/tbb/TBBInitGuard.h>
#endif

#include "gtest/gtest.h"

#ifdef BIFUSD_USING_TBB
#include <optional>
#endif

#include <stdio.h>

#if GTEST_OS_WINDOWS
#include <crtdbg.h>
#include <iostream>
#include <stdlib.h>

namespace testing {

/// \brief Report hook function that sends the message to the output
///        and exits the application with a failure error code.
/// \param message The error message.
int GTest_ReportHookOverride(int, char* message, int*)
{
    std::cout << message << std::endl;
    exit(EXIT_FAILURE);
}

} // namespace testing
#endif

#ifdef BIFUSD_USING_TBB
// Ensure that TBB is initialized and terminated correctly.
std::optional<Bifusd::TBBInitGuard> s_tbbInitGuard;

void finalizeTbbGuard() {
    // Terminate TBB.
    s_tbbInitGuard.reset();
}
void initializeTbbGuard() {
    // Initialize TBB.
    Bifusd::TBBInitGuard::Policy policy =
#if GTEST_OS_WINDOWS
        // The 'relaxed' policy is required on Windows because std::atexit
        // doesn't work as expected on Windows when used in conjunction with
        // dynamic libraries (DLLs). We probably need to have a separate shared
        // lib that manages TBB initialization and finalization.
        // Maybe BifusdTBB could be a shared lib that allows for single
        // initialization/finalization point and link publicly against USD's TBB.
        Bifusd::TBBInitGuard::relaxed;
#else
        // The 'strict' policy was expected to work on Unix-like platforms,
        // but it is not. It fails to finalize the TBB scheduler when running
        // unit tests involving USD. So we use 'relaxed' policy here as well.
        Bifusd::TBBInitGuard::relaxed;
#endif
    s_tbbInitGuard.emplace(policy);

    // Using std::atexit rather than creating a guard at the beginning of
    // main(). This will allow other static objects that might use TBB to
    // also terminate TBB things before we do by allowing them to also register
    // their own finalize function with std::atexit(). This works since the
    // functions registered with std::atexit() are called in the reverse order
    // of their registration.
    std::atexit(finalizeTbbGuard);
}
#else
void initializeTbbGuard() {
    std::cout <<
        "Warning:\n" \
        "   bifusd_gtest_main.cpp uses empty initializeTbbGuard() implementation.\n" \
        "   This means that TBB may not be initialized and terminated correctly.\n" \
        "   If some unit tests fail at exit, that could be the reason." << std::endl;
}
#endif

GTEST_API_ int main(int argc, char** argv) {
    initializeTbbGuard();

    printf("Running main() from bifusd_gtest_main.cpp\n");
    testing::InitGoogleTest(&argc, argv);

#if GTEST_OS_WINDOWS
    // Prevent assertions from triggering the Windows report dialog.
    _CrtSetReportHook(testing::GTest_ReportHookOverride);
#endif

    int retVal = RUN_ALL_TESTS();

    return retVal;
}
