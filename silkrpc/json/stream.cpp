/*
    Copyright 2022 The Silkrpc Authors

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

#include "stream.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include <boost/asio/write.hpp>

namespace json {

static std::uint8_t OBJECT_OPEN = 1;
static std::uint8_t ARRAY_OPEN = 2;
static std::uint8_t FIELD_WRITTEN = 3;
static std::uint8_t ENTRY_WRITTEN = 4;

static std::string OPEN_BRACE{"{"}; // NOLINT(runtime/string)
static std::string CLOSE_BRACE{"}"}; // NOLINT(runtime/string)
static std::string OPEN_BRACKET{"["}; // NOLINT(runtime/string)
static std::string CLOSE_BRACKET{"]"}; // NOLINT(runtime/string)
static std::string FIELD_SEPARATOR{","}; // NOLINT(runtime/string)

void Stream::open_object() {
    writer_.write(OPEN_BRACE);
    stack_.push(OBJECT_OPEN);
}

void Stream::close_object() {
    if (stack_.top() == FIELD_WRITTEN) {
        stack_.pop();
    }
    stack_.pop();
    writer_.write(CLOSE_BRACE);
}

void Stream::open_array() {
    writer_.write(OPEN_BRACKET);
    stack_.push(ARRAY_OPEN);
}

void Stream::close_array() {
    if (stack_.top() == ENTRY_WRITTEN) {
        stack_.pop();
    }
    stack_.pop();
    writer_.write(CLOSE_BRACKET);
}

void Stream::write_json(const nlohmann::json& json) {
    bool isEntry = stack_.size() > 0 && (stack_.top() == ARRAY_OPEN || stack_.top() == ENTRY_WRITTEN);
    if (isEntry) {
        if (stack_.top() != ENTRY_WRITTEN) {
            stack_.push(ENTRY_WRITTEN);
        } else {
            writer_.write(FIELD_SEPARATOR);
        }
    }

    const auto content = json.dump(/*indent=*/-1, /*indent_char=*/' ', /*ensure_ascii=*/false, nlohmann::json::error_handler_t::replace);
    writer_.write(content);
}

void Stream::write_field(const std::string& name) {
    ensure_separator();

    write_string(name);
    writer_.write(":");
}

void Stream::write_field(const std::string& name, const nlohmann::json& value) {
    ensure_separator();

    const auto content = value.dump(/*indent=*/-1, /*indent_char=*/' ', /*ensure_ascii=*/false, nlohmann::json::error_handler_t::replace);

    write_string(name);
    writer_.write(":");
    writer_.write(content);
}

void Stream::write_string(const std::string& str) {
   writer_.write("\"" + str + "\"");
}

void Stream::ensure_separator() {
    if (stack_.size() > 0) {
        if (stack_.top() != FIELD_WRITTEN) {
            stack_.push(FIELD_WRITTEN);
        } else {
            writer_.write(FIELD_SEPARATOR);
        }
    }
}

} // namespace json
