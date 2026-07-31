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


/// \brief A simple task observer that prints task events to output streams.

#ifndef BIFROSTUSD_GRAPH_EXECUTOR_STREAM_OBSERVER_H
#define BIFROSTUSD_GRAPH_EXECUTOR_STREAM_OBSERVER_H

#include <BifrostUsd/GraphExecutor/GraphExecutorTypes.h>

// Amino
#include <Amino/Core/Array.h>
#include <Amino/Core/TaskObserver.h>
#include <Amino/Core/String.h>
#include <Amino/Core/StringView.h>

using StringArray = Amino::Array<Amino::String>;

namespace BifrostUsd::GraphExecutor {

//------------------------------------------------------------------------------
//
/// \brief A task observer that prints task events to output streams.
template <typename Os>
class StreamObserver : public Amino::ITaskObserver {
public:
    /// \brief Construct a StreamObserver that prints all informational and
    /// error messages to the given output stream.
    explicit StreamObserver(Os& all) : StreamObserver{all, all} {}

    /// \brief Construct a StreamObserver that prints informational messages to
    /// the given output stream and error messages to the given error stream.
    StreamObserver(Os& out, Os& err)
        : m_outStream{out}, m_errStream{err}, m_messages{nullptr} {}

    /// \brief Construct a StreamObserver that prints the informational messages
    /// to the given output stream, the error messages to the given error stream
    /// and collects all messages in the given output array.
    StreamObserver(Os& out, Os& err, StringArray& messages)
        : m_outStream{out}, m_errStream{err}, m_messages{&messages} {
        m_messages->clear();
    }
    ~StreamObserver() override = default;

    /// \brief Enable or disable which messages are printed to the output
    /// and error streams, and collected to the optional output message array.
    void setVerbosityLevel(VerbosityLevel level) { m_verbosityLevel = level; }

    /// \brief Returns true if no messages should be printed to streams or
    /// collected.
    inline bool isSilent() const {
        return m_verbosityLevel == VerbosityLevel::eSilent;
    }

    /// \brief Returns true if at least warning messages should be printed to
    /// streams or collected.
    inline bool isReportingWarnings() const {
        return m_verbosityLevel >= VerbosityLevel::eErrorsAndWarnings;
    }

    /// \brief Returns true if at least error messages should be printed to
    /// streams or collected.
    inline bool isReportingErrors() const {
        return m_verbosityLevel >= VerbosityLevel::eErrorsOnly;
    }

    /// \brief Returns true if all messages (informational and others) should be
    /// printed to streams or collected.
    inline bool isReportingAll() const {
        return m_verbosityLevel >= VerbosityLevel::eAllMessages;
    }

    /// \brief Set string prefix for all messages printed by this observer to
    /// the output and error streams. The prefix is not applied to messages
    /// collected in the optional output message array.
    void setPrintPrefix(Amino::StringView const& prefix) { m_prefix = prefix; }

    /// \brief Called when the observed task starts.
    void onStart() noexcept override {
        if (isReportingAll()) reportStart();
    }

    /// \brief Called when the observed task makes progress.
    void onProgress(Amino::StringView const& title,
                    unsigned                 num,
                    unsigned                 denom) noexcept override {
        if (isReportingAll()) reportProgress(title, num, denom);
    }

    /// \brief Called when the observed task sends a message.
    void onMessage(Amino::Message const& message) noexcept override {
        reportMessage(message);
    }

    /// \brief Called when the observed task is done.
    void onDone(Amino::TaskStatus status) noexcept override {
        reportDone(status);
    }

protected:
    /// \brief Returns the output stream used for informational messages.
    Os& getOutputStream() const { return m_outStream; }

private:
    /// \brief Reports that the task has started.
    void reportStart() const {
        static constexpr const char* kStartMsg = "Bifrost task started";
        m_outStream << m_prefix << kStartMsg << '\n';
        if(m_messages) {
            m_messages->push_back(kStartMsg);
        }
    }

    /// \brief Reports that the task has made progress.
    void reportProgress(Amino::StringView const& title,
                        unsigned                 num,
                        unsigned                 denom) const {
        std::string progressMsg = std::string("  - ") + title.data() + " " +
                                  std::to_string(num) + "/" +
                                  std::to_string(denom);
        m_outStream << m_prefix << progressMsg << '\n';
        if (m_messages) {
            m_messages->push_back(progressMsg.c_str());
        }
    }

    /// \brief Reports that the task has sent a message.
    void reportMessage(Amino::Message const& message) const {
        const auto kind = message.getKind();
        if(kind == Amino::MessageKind::eError) {
            if(isReportingErrors()) {
                m_errStream << m_prefix;
                message.toStream(m_errStream, true /*newline*/);
                if(m_messages) {
                    m_messages->push_back(message.getText());
                }
            }
        } else if (kind == Amino::MessageKind::eWarning) {
            if(isReportingWarnings()) {
                m_outStream << m_prefix;
                message.toStream(m_outStream, true /*newline*/);
                if(m_messages) {
                    m_messages->push_back(message.getText());
                }
            }
        } else if(isReportingAll()) {
            m_outStream << m_prefix;
            message.toStream(m_outStream, true /*newline*/);
            if(m_messages) {
                m_messages->push_back(message.getText());
            }
        }
    }

    /// \brief Reports that the task is done.
    void reportDone(Amino::TaskStatus status) const {
        static constexpr const char* kErrorMsg =
            "Bifrost task completed with errors";
        static constexpr const char* kCancelMsg =
            "Bifrost task completed after explicit cancellation";
        static constexpr const char* kDropMsg =
            "Bifrost task completed after implicit cancellation";
        static constexpr const char* kSuccessMsg =
            "Bifrost task completed successfully";

        switch (status) {
            case Amino::TaskStatus::eError:
                if (isReportingErrors()) {
                    m_errStream << m_prefix << kErrorMsg << '\n';
                    if (m_messages) {
                        m_messages->push_back(kErrorMsg);
                    }
                }
                break;
            case Amino::TaskStatus::eCancelled:
                if (isReportingAll()) {
                    m_outStream << m_prefix << kCancelMsg << '\n';
                    if (m_messages) {
                        m_messages->push_back(kCancelMsg);
                    }
                }
                break;
            case Amino::TaskStatus::eDropped:
                if (isReportingAll()) {
                    m_outStream << m_prefix << kDropMsg << '\n';
                    if (m_messages) {
                        m_messages->push_back(kDropMsg);
                    }
                }
                break;
            case Amino::TaskStatus::eSuccess:
                if (isReportingAll()) {
                    m_outStream << m_prefix << kSuccessMsg << '\n';
                    if (m_messages) {
                        m_messages->push_back(kSuccessMsg);
                    }
                }
                break;
        }
    }

    /// \brief Output stream for normal/informational messages.
    Os& m_outStream;

    /// \brief Output stream for error messages.
    Os& m_errStream;

    /// \brief Optional storage for collecting messages.
    StringArray* m_messages{nullptr};

    /// \brief Controls which messages are printed or collected.
    VerbosityLevel m_verbosityLevel = VerbosityLevel::eSilent;

    /// \brief Prefix to prepend to all messages printed by this observer.
    std::string m_prefix = "  ";
};

} // namespace BifrostUsd::GraphExecutor

#endif // BIFROSTUSD_GRAPH_EXECUTOR_STREAM_OBSERVER_H
