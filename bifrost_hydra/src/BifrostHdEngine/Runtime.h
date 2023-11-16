//-
// Copyright 2023 Autodesk, Inc.
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

#ifndef BIFROST_HD_ENGINE_RUNTIME_H_
#define BIFROST_HD_ENGINE_RUNTIME_H_

#include <BifrostGraph/Executor/BifrostBoardRuntime.h>

namespace BifrostHd {

class Runtime final : public BifrostBoardRuntime {
public:
    Runtime();
    ~Runtime() override;

    static bool IsValid();

    explicit operator bool() const { return IsValid(); }

    static Runtime& Get();

    void reportMessage(Amino::MessageCategory category,
                       Amino::String const&   message) const override;

    bool hasMessages() const;

    Amino::StringList const& getMessages() const;

    void clearMessages();

public:
    /// Disabled
    /// \{
    Runtime(Runtime const&)            = delete;
    Runtime(Runtime&&)                 = delete;
    Runtime& operator=(const Runtime&) = delete;
    Runtime& operator=(Runtime&&)      = delete;
    /// \}
};

} // namespace BifrostHd
#endif // BIFROST_HD_ENGINE_RUNTIME_H_
