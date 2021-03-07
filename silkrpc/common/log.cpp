/*
   Copyright 2020 The Silkrpc Authors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "log.hpp"

#include <absl/strings/str_cat.h>

#include <string>

namespace silkworm {

bool AbslParseFlag(absl::string_view text, LogLevels* level, std::string* error) {
    if (text == "n") {
        *level = LogLevels::LogNone;
        return true;
    }
    if (text == "c") {
        *level = LogLevels::LogCritical;
        return true;
    }
    if (text == "e") {
        *level = LogLevels::LogError;
        return true;
    }
    if (text == "w") {
        *level = LogLevels::LogWarn;
        return true;
    }
    if (text == "i") {
        *level = LogLevels::LogInfo;
        return true;
    }
    if (text == "d") {
        *level = LogLevels::LogDebug;
        return true;
    }
    if (text == "t") {
        *level = LogLevels::LogTrace;
        return true;
    }
    *error = "unknown value for silkrpc::LogLevel";
    return false;
}

std::string AbslUnparseFlag(LogLevels level) {
    switch (level) {
        case LogLevels::LogNone: return "n";
        case LogLevels::LogCritical: return "c";
        case LogLevels::LogError: return "e";
        case LogLevels::LogWarn: return "w";
        case LogLevels::LogInfo: return "i";
        case LogLevels::LogDebug: return "d";
        case LogLevels::LogTrace: return "t";
        default: return absl::StrCat(level);
    }
}

} // namespace silkworm
