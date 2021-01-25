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

#include "types.hpp"

#include <algorithm>

namespace evmc {

void to_json(nlohmann::json& json, const address& addr) {
    json = silkworm::to_hex(addr);
}

void from_json(const nlohmann::json& json, address& addr) {
    addr = silkworm::to_address(silkworm::from_hex(json.get<std::string>()));
}

void to_json(nlohmann::json& json, const bytes32& b32) {
    json = silkworm::to_hex(b32);
}

void from_json(const nlohmann::json& json, bytes32& b32) {
    b32 = silkworm::to_bytes32(silkworm::from_hex(json.get<std::string>()));
}

} // namespace evmc

namespace silkrpc {

void to_json(nlohmann::json& json, const Log& log) {
    json["address"] = log.address;
    json["topics"] = log.topics;
    json["data"] = log.data;
}

void from_json(const nlohmann::json& json, Log& log) {
    log.address = json.at("address").get<evmc::address>();
    log.topics = json.at("topics").get<std::vector<evmc::bytes32>>();
    log.data = json.at("data").get<silkworm::Bytes>();
}

void to_json(nlohmann::json& json, const Receipt& receipt) {
    json["success"] = receipt.success;
    json["cumulative_gas_used"] = receipt.cumulative_gas_used;
}

void from_json(const nlohmann::json& json, Receipt& receipt) {
    if (json.is_array()) {
        receipt.success = json[1] == 1u;
        receipt.cumulative_gas_used = json[2];
    } else {
        receipt.success = json.at("success").get<bool>();
        receipt.cumulative_gas_used = json.at("cumulative_gas_used").get<uint64_t>();
    }
}

void to_json(nlohmann::json& json, const Filter& filter) {
    if (filter.from_block != std::nullopt) {
        json["fromBlock"] = filter.from_block.value();
    }
    if (filter.to_block != std::nullopt) {
        json["toBlock"] = filter.to_block.value();
    }
    if (filter.addresses != std::nullopt) {
        if (filter.addresses.value().size() == 1) {
            json["address"] = filter.addresses.value()[0];
        } else {
            json["address"] = filter.addresses.value();
        }
    }
    if (filter.topics != std::nullopt) {
        json["topics"] = filter.topics.value();
    }
    if (filter.block_hash != std::nullopt) {
        json["blockHash"] = filter.block_hash.value();
    }
}

void from_json(const nlohmann::json& json, Filter& filter) {
    if (json.count("fromBlock") != 0) {
        auto json_from_block = json.at("fromBlock");
        if (json_from_block.is_string()) {
            filter.from_block = std::stol(json_from_block.get<std::string>(), 0, 16);
        } else {
            filter.from_block = json_from_block.get<uint64_t>();
        }
    }
    if (json.count("toBlock") != 0) {
        auto json_to_block = json.at("toBlock");
        if (json_to_block.is_string()) {
            filter.to_block = std::stol(json_to_block.get<std::string>(), 0, 16);
        } else {
            filter.to_block = json_to_block.get<uint64_t>();
        }
    }
    if (json.count("address") != 0) {
        if (json.at("address").is_string()) {
            filter.addresses= {json.at("address").get<std::string>()};
        } else {
            filter.addresses = json.at("address").get<FilterAddresses>();
        }
    }
    if (json.count("topics") != 0) {
        auto topics = json.at("topics");
        for (auto& topic_item : topics) {
            if (topic_item.is_null()) {
                topic_item = {""};
            }
            if (topic_item.is_string()) {
                topic_item = {topic_item};
            }
        }
        filter.topics = topics.get<FilterTopics>();
    }
    if (json.count("blockHash") != 0) {
        filter.block_hash = json.at("blockHash").get<std::string>();
    }
}

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

namespace silkrpc::json {

nlohmann::json make_json_content(uint32_t id, const nlohmann::json& result) {
    return {{"jsonrpc", "2.0"}, {"id", id}, {"result", result}};
}

nlohmann::json make_json_error(uint32_t id, const std::string& error) {
    return {{"jsonrpc", "2.0"}, {"id", id}, {"error", error}};
}

} // namespace silkrpc::json
