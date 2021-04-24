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

bool AbslParseFlag(absl::string_view text, LogLevel* level, std::string* error) {
    if (text == "n") {
        *level = LogLevel::None;
        return true;
    }
    if (text == "c") {
        *level = LogLevel::Critical;
        return true;
    }
    if (text == "e") {
        *level = LogLevel::Error;
        return true;
    }
    if (text == "w") {
        *level = LogLevel::Warn;
        return true;
    }
    if (text == "i") {
        *level = LogLevel::Info;
        return true;
    }
    if (text == "d") {
        *level = LogLevel::Debug;
        return true;
    }
    if (text == "t") {
        *level = LogLevel::Trace;
        return true;
    }
    *error = "unknown value for LogLevel";
    return false;
}

std::string AbslUnparseFlag(LogLevel level) {
    switch (level) {
        case LogLevel::None: return "n";
        case LogLevel::Critical: return "c";
        case LogLevel::Error: return "e";
        case LogLevel::Warn: return "w";
        case LogLevel::Info: return "i";
        case LogLevel::Debug: return "d";
        case LogLevel::Trace: return "t";
        default: return absl::StrCat(level);
    }
}

} // namespace silkworm
