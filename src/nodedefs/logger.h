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

/// \file  logger.h
/// \brief Logger helper

#ifndef BIFROST_USD_LOGGER_H
#define BIFROST_USD_LOGGER_H

#include <Amino/Core/String.h>
#include <iostream>

namespace Amino {
class RuntimeServices;
}

namespace USDUtils {

/// \class Logger logger.h
/// \brief Helper class used to log information via the Amino RuntimeServices.
/// \note Work in progress
class Logger {
public:
    /// \brief Constructor 
    /// \param in_runtimeServices Amino runtime services
    Logger(const Amino::RuntimeServices& in_runtimeServices);

    /// \brief Return the error logging level
    static int errorVerboseLevel();

    /// \brief Set the error logging level
    /// \param in_errorVerboseLevel Logging level
    static void setErrorVerboseLevel(int in_errorVerboseLevel);

    /// \brief Log Amino info
    /// \param in_msg Message to log.
    void info(const Amino::String& in_msg) const;
    /// \brief Log Amino warning
    /// \param in_msg Message to log.
    void warn(const Amino::String& in_msg) const;
    /// \brief Log Amino error
    /// \param in_msg Message to log.
    void error(const Amino::String& in_msg) const;

private:
    const Amino::RuntimeServices& m_runtimeServices;
};

/// \brief Helper to log exception messages
/// \param func_name Function name.
/// \param e Exception
void log_exception(const char* func_name, std::exception const& e);

} // namespace USDUtils

#endif // BIFROST_USD_LOGGER_H
