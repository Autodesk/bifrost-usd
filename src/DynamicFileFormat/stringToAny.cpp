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

#include "stringToAny.h"

#include "dffTypeNames.h"
#include "parsingTools.h"

#include <Amino/Core/String.h>

namespace BifrostUsd {
namespace DynamicPayload {

StringToAnyResult stringToAny(const std::string& typeName,
                              const std::string& str) {
    auto success = [](Amino::Any&& value) {
        return StringToAnyResult{std::move(value), ConversionStatus::kSuccess};
    };
    auto invalidInput = []() {
        return StringToAnyResult{Amino::Any{},
                                 ConversionStatus::kFailure_InvalidInput};
    };

    if (typeName == DffTypeNames::kBool) {
        const auto valueOpt = maybeStob(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kChar) {
        const auto valueOpt = maybeStoIntegral<Amino::char_t>(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kUchar) {
        const auto valueOpt = maybeStoIntegral<Amino::uchar_t>(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kShort) {
        const auto valueOpt = maybeStoIntegral<Amino::short_t>(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kUshort) {
        const auto valueOpt = maybeStoIntegral<Amino::ushort_t>(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kInt) {
        const auto valueOpt = maybeStoIntegral<Amino::int_t>(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kUint) {
        const auto valueOpt = maybeStoIntegral<Amino::uint_t>(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kLong) {
        const auto valueOpt = maybeStoIntegral<Amino::long_t>(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kUlong) {
        const auto valueOpt = maybeStoIntegral<Amino::ulong_t>(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kFloat) {
        const auto valueOpt = maybeStof(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kDouble) {
        const auto valueOpt = maybeStod(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kFloat2) {
        const auto valueOpt = maybeGetFloat2FromString(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kFloat3) {
        const auto valueOpt = maybeGetFloat3FromString(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kFloat4) {
        const auto valueOpt = maybeGetFloat4FromString(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kDouble2) {
        const auto valueOpt = maybeGetDouble2FromString(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kDouble3) {
        const auto valueOpt = maybeGetDouble3FromString(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kDouble4) {
        const auto valueOpt = maybeGetDouble4FromString(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kInt2) {
        const auto valueOpt = maybeGetInt2FromString(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kInt3) {
        const auto valueOpt = maybeGetInt3FromString(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kInt4) {
        const auto valueOpt = maybeGetInt4FromString(str);
        if (!valueOpt) {
            return invalidInput();
        }
        return success(Amino::Any{*valueOpt});
    } else if (typeName == DffTypeNames::kString) {
        return success(Amino::Any{Amino::String{str.c_str()}});
    } else {
        return StringToAnyResult{Amino::Any{},
                                 ConversionStatus::kFailure_UnsupportedType};
    }
}

} // namespace DynamicPayload
} // namespace BifrostUsd
