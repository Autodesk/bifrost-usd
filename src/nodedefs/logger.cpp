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
#include "logger.h"

#include <Amino/Core/RuntimeMessage.h>
#include <Amino/Core/RuntimeServices.h>

namespace {
static int errorVerboseLevel = 1;

}

namespace USDUtils {

Logger::Logger(const Amino::RuntimeServices& in_runtimeServices)
    : m_runtimeServices(in_runtimeServices) {}

int Logger::errorVerboseLevel() { return ::errorVerboseLevel; }

void Logger::setErrorVerboseLevel(int in_errorVerboseLevel) {
    ::errorVerboseLevel = in_errorVerboseLevel;
}

void Logger::info(const Amino::String& in_message) const {
    auto msg =
        Amino::RuntimeMessage(Amino::RuntimeMessageCategory::kInfo, in_message);
    m_runtimeServices.logMessage(msg);
}

void Logger::warn(const Amino::String& in_message) const {
    auto msg = Amino::RuntimeMessage(Amino::RuntimeMessageCategory::kWarning,
                                     in_message);
    m_runtimeServices.logMessage(msg);
}

void Logger::error(const Amino::String& in_message) const {
    auto msg = Amino::RuntimeMessage(Amino::RuntimeMessageCategory::kError,
                                     in_message);
    m_runtimeServices.logMessage(msg);
}

void log_exception(const char* func_name, std::exception const& e) {
    if (Logger::errorVerboseLevel() > 0) {
        std::cerr << func_name << " failed: " << e.what() << std::endl;
    }
}

} // namespace USDUtils
