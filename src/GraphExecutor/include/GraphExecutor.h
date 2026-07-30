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

/// \brief A utility class that wraps an Amino::Executable to execute a graph.

#ifndef BIFROSTUSD_GRAPH_EXECUTOR_H
#define BIFROSTUSD_GRAPH_EXECUTOR_H

#include "GraphExecutorExport.h"

#include "GraphExecutorConstants.h"
#include "GraphExecutorTypes.h"

// Amino
#include <Amino/Core/Any.h>
#include <Amino/Core/Array.h>
#include <Amino/Core/String.h>
#include <Amino/Core/StringView.h>
#include <Amino/Executor/Closure.h>
#include <Amino/Executor/Executable.h>
#include <Amino/Executor/ExecutionInputs.h>
#include <Amino/Executor/ExecutionOutputs.h>
#include <Amino/Executor/ExecutionState.h>
#include <Amino/Executor/TerminalOutput.h>

// C++ Standard Library
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

/// \brief Alias for an array of Amino::String.
using StringArray = Amino::Array<Amino::String>;

namespace BifrostUsd::GraphExecutor {

/// \brief Convert a TerminalType enum value to its string representation.
///
/// \param [in] type The terminal type to convert.
/// \return The fully-qualified terminal port name string corresponding to
///     \p type.
BIFROSTUSD_GRAPH_EXECUTOR_DECL
Amino::StringView terminalTypeToString(TerminalType type);

//------------------------------------------------------------------------------
// Class GraphExecutor
//------------------------------------------------------------------------------

class GraphExecutor final {
public:
    /// \brief Destructor.
    BIFROSTUSD_GRAPH_EXECUTOR_DECL ~GraphExecutor() = default;

    /// \brief Retrieve the names of all input ports declared by the graph.
    ///
    /// Returns the names of all input ports of the underlying Bifrost
    /// compound, in graph declaration order. This method can be called at
    /// any time, independently of setInput() or execute().
    ///
    /// \return An array of input port name strings.
    BIFROSTUSD_GRAPH_EXECUTOR_DECL StringArray getInputPortNames() const;

    /// \brief Retrieve the names of all output ports declared by the graph.
    ///
    /// Returns the names of all output ports of the underlying Bifrost
    /// compound, in graph declaration order. This method can be called at
    /// any time, independently of execute().
    ///
    /// \return An array of output port name strings.
    BIFROSTUSD_GRAPH_EXECUTOR_DECL StringArray getOutputPortNames() const;

    /// \brief Set a single graph input port value for the next execution.
    ///
    /// Calls ensureInputsCreated() to lazily initialize the ExecutionInputs
    /// if needed, then sets the value for the named input port. An error is
    /// reported if the port name is not found in the graph or if the value
    /// type does not match the port's declared type.
    ///
    /// \param [in] name  The name of the input port to set.
    /// \param [in] value The value to assign. Must match the port's declared
    ///     type.
    /// \return true if the input was set successfully; false on any error.
    BIFROSTUSD_GRAPH_EXECUTOR_DECL bool setInput(Amino::StringView name,
                                                 const Amino::Any& value);

    /// \brief Set a single graph input port value for the next execution,
    /// collecting error messages.
    ///
    /// Equivalent to setInput(Amino::StringView, const Amino::Any&), but any
    /// error messages generated during the call are appended to \p messages.
    ///
    /// \param [in]  name     The name of the input port to set.
    /// \param [in]  value    The value to assign. Must match the port's
    ///     declared type.
    /// \param [out] messages Array that collects error messages. Cleared at
    ///     the start of the call.
    /// \return true if the input was set successfully; false on any error.
    BIFROSTUSD_GRAPH_EXECUTOR_DECL bool setInput(Amino::StringView name,
                                                 const Amino::Any& value,
                                                 StringArray&      messages);

    /// \brief Set the \c Simulation::timeline_info global variable.
    ///
    /// Updates the \c Simulation::timeline_info Amino's global variable on
    /// the \c Executable with the provided settings. If the global variable
    /// does not exist in the graph, this method has no effect.
    ///
    /// \note This method is optional and does not need to be called before
    ///     every call to execute().
    ///
    /// \param [in] settings The timeline settings to apply.
    BIFROSTUSD_GRAPH_EXECUTOR_DECL void setTimelineSettings(
        const TimelineSettings& settings);

    /// \brief Set the frame rate (frame-per-second). The fps is usually set
    /// once, independently of the timeline info settings, and must be set
    /// before calling setFrame().
    ///
    /// \note This method is optional and does not need to be called before
    ///     every call to execute().
    ///
    /// \param [in] fps The frame rate to set.
    BIFROSTUSD_GRAPH_EXECUTOR_DECL void setFps(double fps);

    /// \brief Set the \c Simulation::time global variable.
    ///
    /// Updates the \c Simulation::time Amino's global variable on the
    /// \c Executable. The time and framelength values are computed from
    /// \p frame and the current frame rate (see setFps()). If the global
    /// variable does not exist in the graph, this method has no effect.
    ///
    /// \note This method is optional and does not need to be called before
    ///     every call to execute().
    ///
    /// \param [in] frame The frame number to set.
    BIFROSTUSD_GRAPH_EXECUTOR_DECL void setFrame(double frame);

    /// \brief Execute the graph.
    ///
    /// Runs the graph using inputs previously set via setInput(), which
    /// must be called before every execution. Optionally,
    /// setTimelineSettings(), setFps(), and setFrame() may also be called
    /// beforehand to configure additional inputs. The ExecutionInputs are
    /// consumed by this call, so setInput() must be called again before
    /// the next execution.
    ///
    /// \param [in] verbosityLevel Controls the verbosity of diagnostic output
    ///     written to stdout/stderr during execution.
    /// \return true if execution completed and the output is valid;
    ///     false otherwise.
    BIFROSTUSD_GRAPH_EXECUTOR_DECL bool execute(
        VerbosityLevel verbosityLevel = VerbosityLevel::eSilent);

    /// \brief Execute the graph, collecting messages.
    ///
    /// Equivalent to execute(VerbosityLevel), but any error and/or diagnostic
    /// messages (depending on the given verbosity level) generated during
    /// execution are collected in \p messages.
    ///
    /// \param [out] messages Array that collects messages generated during
    ///     execution. This array is cleared at the start of the function call.
    /// \param [in] verbosityLevel Controls which messages are collected in
    ///     \p messages and written to stdout/stderr during execution.
    /// \return true if execution completed and the output is valid;
    ///     false otherwise.
    BIFROSTUSD_GRAPH_EXECUTOR_DECL bool execute(
        StringArray&   messages,
        VerbosityLevel verbosityLevel = VerbosityLevel::eSilent);

    /// \brief Extract the output closure for a named output port.
    ///
    /// Retrieves and removes the closure produced during the last execution
    /// for the specified regular output port. Subsequent calls for
    /// the same output port, and before another execution, will return an
    /// invalid Amino::Closure.
    ///
    /// \param [in] name The name of the output port to retrieve.
    /// \return The output closure for \p name, or a default-constructed
    ///     invalid Amino::Closure if the graph is invalid or if the
    ///     named output port does not exist.
    BIFROSTUSD_GRAPH_EXECUTOR_DECL Amino::Closure extractOutputClosure(
        Amino::StringView name);

    /// \brief Check whether the graph has any terminal ports.
    ///
    /// \return true if the graph declares at least one terminal port;
    ///     false otherwise.
    BIFROSTUSD_GRAPH_EXECUTOR_DECL bool hasTerminal() const;

    /// \brief Check whether the graph has a terminal port of the given type.
    ///
    /// \param [in] type The terminal type to look for.
    /// \return true if the graph has a terminal port matching \p type;
    ///     false otherwise.
    [[nodiscard]] BIFROSTUSD_GRAPH_EXECUTOR_DECL bool hasTerminalPort(
        TerminalType type) const;

    /// \brief Extract the terminal output for the given terminal type.
    ///
    /// Retrieves and removes the terminal output produced during the last
    /// execution for the specified terminal port type. Subsequent calls for
    /// the same type, and before another execution, will return an invalid
    /// TerminalOutput.
    ///
    /// \param [in] type The terminal type whose output should be extracted.
    /// \return The terminal output for \p type, or a default-constructed
    ///     invalid Amino::TerminalOutput if the graph is invalid or if
    ///     the terminal port does not exist.
    BIFROSTUSD_GRAPH_EXECUTOR_DECL Amino::TerminalOutput extractTerminalOutput(
        TerminalType type);

private:
    /// \brief Construction of a GraphExecutor from a valid Executable.
    /// \pre The provided Amino::Executable is valid.
    explicit GraphExecutor(Amino::Executable executable) noexcept;

    /// \brief GraphExecutor construction is only accessible via
    /// the GraphExecutorFactory functions.
    friend BIFROSTUSD_GRAPH_EXECUTOR_DECL std::unique_ptr<GraphExecutor>
                                          makeGraphExecutor(Amino::StringView);
    friend BIFROSTUSD_GRAPH_EXECUTOR_DECL std::unique_ptr<GraphExecutor>
    makeGraphExecutor(Amino::StringView, StringArray&);

    /// \brief GraphExecutor is not default-constructible, copyable, or movable.
    /// \{
    GraphExecutor()                                = delete;
    GraphExecutor(const GraphExecutor&)            = delete;
    GraphExecutor(GraphExecutor&&)                 = delete;
    GraphExecutor& operator=(const GraphExecutor&) = delete;
    GraphExecutor& operator=(GraphExecutor&&)      = delete;
    /// \}

    /// \brief Lazily initialize m_executionInputs if not already valid.
    ///
    /// Creates a fresh ExecutionInputs from the current Executable if
    /// m_executionInputs is not currently valid (e.g., after construction or
    /// after execute() has consumed the previous inputs).
    void ensureInputsCreated();

private:
    /// \brief The Amino Executable graph to run.
    Amino::Executable m_executable;
    /// \brief Lookup map from input port name to InputRef, built once in the
    /// constructor. Provides O(1) name lookup in setInput() instead of an O(m)
    /// linear scan through graph inputs on every call.
    /// The graph structure (input port names, indices, type IDs) is immutable
    /// for the lifetime of the Executable, so this is safe to build this map
    /// once and reuse it for the lifetime of the GraphExecutor.
    std::unordered_map<std::string, Amino::Graph::InputRef> m_inputRefByName;
    /// \brief The graph input arguments for the next execution.
    Amino::ExecutionInputs m_executionInputs;
    /// \brief The graph output results from the last execution.
    Amino::ExecutionOutputs m_executionOutputs;
    /// \brief The execution states to carry over from an execution to the next.
    Amino::ExecutionState m_executionState;
    /// \brief The frame rate (frames per second) for the graph execution.
    double m_fps{defaultFps};
};
} // namespace BifrostUsd::GraphExecutor

#endif // BIFROSTUSD_GRAPH_EXECUTOR_H
