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

#include "filter.hpp"

#include <silkworm/common/util.hpp>

namespace silkrpc {

std::ostream& operator<<(std::ostream& out, const std::optional<std::vector<std::string>>& v) {
    if (v.has_value()) {
        out << "[";
        for (auto i = v.value().begin(); i != v.value().end(); ++i) {
            out << *i << " ";
        }
        out << "]";
    } else {
        out << "null";
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const std::optional<FilterAddresses>& addresses) {
    if (addresses.has_value()) {
        auto address_vector = addresses.value();
        out << "[";
        for (auto i = address_vector.begin(); i != address_vector.end(); ++i) {
            out << "0x" << silkworm::to_hex((*i).bytes) << " ";
        }
        out << "]";
    } else {
        out << "null";
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const std::optional<FilterTopics>& topics) {
    if (topics.has_value()) {
        auto subtopics = topics.value();
        out << "[";
        for (auto i = subtopics.begin(); i != subtopics.end(); ++i) {
            out << *i << " ";
        }
        out << "]";
    } else {
        out << "null";
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const Filter& filter) {
    out << "from_block: " << filter.from_block.value_or(0) << " ";
    out << "to_block: " << filter.to_block.value_or(0) << " ";
    out << "address: " << filter.addresses << " ";
    out << "topics: " << filter.topics << " ";
    out << "block_hash: " << filter.block_hash.value_or("null");
    return out;
}

} // namespace silkrpc
