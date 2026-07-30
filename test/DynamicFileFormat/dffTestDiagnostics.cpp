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

#include "dffTestDiagnostics.h"

#include <regex>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

std::vector<std::string> getMessages(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey,
    DffDiagnosticSeverity             severity) {
    std::vector<std::string> messages;
    for (const auto& record : collector.getSnapshotForBucket(bucketKey)) {
        if (record.severity == severity) {
            messages.push_back(record.formattedMessage);
        }
    }
    return messages;
}

::testing::AssertionResult hasNoMessage(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey,
    DffDiagnosticSeverity             severity,
    std::string_view                  kind) {
    auto messages = getMessages(collector, bucketKey, severity);
    if (messages.empty()) {
        return ::testing::AssertionSuccess();
    }
    ::testing::AssertionResult result = ::testing::AssertionFailure();
    result << "Expected no " << kind << " messages for bucket \"" << bucketKey
           << "\", but found " << messages.size() << ":\n";
    for (const auto& msg : messages) {
        result << "  - " << msg << "\n";
    }
    return result;
}

::testing::AssertionResult hasMessageContaining(
    const std::vector<std::string>& messages,
    std::string_view                substr,
    std::string_view                kind,
    std::string_view                bucketKey) {
    for (const auto& msg : messages) {
        if (msg.find(substr) != std::string::npos) {
            return ::testing::AssertionSuccess();
        }
    }
    ::testing::AssertionResult result = ::testing::AssertionFailure();
    result << "No " << kind << " message containing \"" << substr
           << "\" was found for bucket \"" << bucketKey << "\".\n";
    result << "The collected " << kind << " messages (" << messages.size()
           << ") are:\n";
    for (const auto& msg : messages) {
        result << "  - " << msg << "\n";
    }
    return result;
}

::testing::AssertionResult hasMessageMatchingRegex(
    const std::vector<std::string>& messages,
    const std::regex&               pattern,
    std::string_view                kind,
    std::string_view                bucketKey) {
    for (const auto& msg : messages) {
        if (std::regex_search(msg, pattern)) {
            return ::testing::AssertionSuccess();
        }
    }
    ::testing::AssertionResult result = ::testing::AssertionFailure();
    result << "No " << kind << " message matched the given regex for bucket \""
           << bucketKey << "\".\n";
    result << "The collected " << kind << " messages (" << messages.size()
           << ") are:\n";
    for (const auto& msg : messages) {
        result << "  - " << msg << "\n";
    }
    return result;
}

} // namespace

std::vector<std::string> dffGetErrorMessages(
    const DffTestDiagnosticCollector& collector, std::string_view bucketKey) {
    return getMessages(collector, bucketKey, DffDiagnosticSeverity::Error);
}

std::vector<std::string> dffGetWarningMessages(
    const DffTestDiagnosticCollector& collector, std::string_view bucketKey) {
    return getMessages(collector, bucketKey, DffDiagnosticSeverity::Warning);
}

std::vector<std::string> dffGetStatusMessages(
    const DffTestDiagnosticCollector& collector, std::string_view bucketKey) {
    return getMessages(collector, bucketKey, DffDiagnosticSeverity::Status);
}

::testing::AssertionResult dffHasNoError(
    const DffTestDiagnosticCollector& collector, std::string_view bucketKey) {
    return hasNoMessage(collector, bucketKey, DffDiagnosticSeverity::Error,
                        "error");
}

::testing::AssertionResult dffHasError(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey,
    std::string_view                  substr) {
    return hasMessageContaining(dffGetErrorMessages(collector, bucketKey),
                                substr, "error", bucketKey);
}

::testing::AssertionResult dffHasErrorRegex(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey,
    const std::regex&                 pattern) {
    return hasMessageMatchingRegex(dffGetErrorMessages(collector, bucketKey),
                                   pattern, "error", bucketKey);
}

::testing::AssertionResult dffHasNoWarning(
    const DffTestDiagnosticCollector& collector, std::string_view bucketKey) {
    return hasNoMessage(collector, bucketKey, DffDiagnosticSeverity::Warning,
                        "warning");
}

::testing::AssertionResult dffHasWarning(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey,
    std::string_view                  substr) {
    return hasMessageContaining(dffGetWarningMessages(collector, bucketKey),
                                substr,
                                "warning",
                                bucketKey);
}

::testing::AssertionResult dffHasWarningRegex(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey,
    const std::regex&                 pattern) {
    return hasMessageMatchingRegex(
        dffGetWarningMessages(collector, bucketKey),
        pattern,
        "warning",
        bucketKey);
}

::testing::AssertionResult dffHasNoStatus(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey) {
    return hasNoMessage(collector, bucketKey, DffDiagnosticSeverity::Status,
                        "status");
}

::testing::AssertionResult dffHasStatus(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey,
    std::string_view                  substr) {
    return hasMessageContaining(
        dffGetStatusMessages(collector, bucketKey), substr, "status", bucketKey);
}

::testing::AssertionResult dffHasStatusRegex(
    const DffTestDiagnosticCollector& collector,
    std::string_view                  bucketKey,
    const std::regex&                 pattern) {
    return hasMessageMatchingRegex(
        dffGetStatusMessages(collector, bucketKey), pattern, "status", bucketKey);
}
