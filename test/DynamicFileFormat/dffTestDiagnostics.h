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

#ifndef BIFROSTUSD_DFF_TEST_DIAGNOSTICS_H
#define BIFROSTUSD_DFF_TEST_DIAGNOSTICS_H

#include "dffDiagnosticsRuntime.h"

#include <gtest/gtest.h>

#include <regex>
#include <string>
#include <string_view>
#include <vector>

using DffTestDiagnosticCollector = PXR_NS::DffDiagnosticCollector;

/// Return all recorded error messages for \p bucketKey.
std::vector<std::string> dffGetErrorMessages(
    const DffTestDiagnosticCollector& collector, std::string_view bucketKey);

/// Return all recorded warning messages for \p bucketKey.
std::vector<std::string> dffGetWarningMessages(
    const DffTestDiagnosticCollector& collector, std::string_view bucketKey);

/// Return all recorded status messages for \p bucketKey.
std::vector<std::string> dffGetStatusMessages(
    const DffTestDiagnosticCollector& collector, std::string_view bucketKey);

/// Returns AssertionSuccess if no error message was collected; returns
/// AssertionFailure with a diagnostic listing all collected errors otherwise.
::testing::AssertionResult dffHasNoError(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey);

/// Returns AssertionSuccess if any collected error message contains \p substr,
/// or AssertionFailure with a diagnostic listing all collected errors.
::testing::AssertionResult dffHasError(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey,
    std::string_view                  substr);

/// Returns AssertionSuccess if any collected error message matches \p pattern,
/// or AssertionFailure with a diagnostic listing all collected errors.
::testing::AssertionResult dffHasErrorRegex(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey,
    const std::regex&                 pattern);

/// Returns AssertionSuccess if no warning message was collected; returns
/// AssertionFailure with a diagnostic listing all collected warnings otherwise.
::testing::AssertionResult dffHasNoWarning(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey);

/// Returns AssertionSuccess if any collected warning message contains
/// \p substr, or AssertionFailure with a diagnostic listing all collected
/// warnings.
::testing::AssertionResult dffHasWarning(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey,
    std::string_view                  substr);

/// Returns AssertionSuccess if any collected warning message matches \p
/// pattern, or AssertionFailure with a diagnostic listing all collected
/// warnings.
::testing::AssertionResult dffHasWarningRegex(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey,
    const std::regex&                 pattern);

/// Returns AssertionSuccess if no status message was collected; returns
/// AssertionFailure with a diagnostic listing all collected statuses otherwise.
::testing::AssertionResult dffHasNoStatus(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey);

/// Returns AssertionSuccess if any collected status message contains \p substr,
/// or AssertionFailure with a diagnostic listing all collected statuses.
::testing::AssertionResult dffHasStatus(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey,
    std::string_view                  substr);

/// Returns AssertionSuccess if any collected status message matches \p pattern,
/// or AssertionFailure with a diagnostic listing all collected statuses.
::testing::AssertionResult dffHasStatusRegex(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey,
    const std::regex&                 pattern);

#endif // BIFROSTUSD_DFF_TEST_DIAGNOSTICS_H
