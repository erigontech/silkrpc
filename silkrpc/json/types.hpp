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

#ifndef SILKRPC_JSON_ETH_H_
#define SILKRPC_JSON_ETH_H_

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace silkrpc::json::eth {

struct Filter {
    std::optional<uint64_t> from_block;
    std::optional<uint64_t> to_block;
    std::optional<std::vector<std::string>> addresses;
    std::optional<std::vector<std::string>> topics;
    std::optional<std::string> block_hash;
};

void to_json(nlohmann::json& json, const Filter& filter) {
    json = nlohmann::json{};

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
        filter.from_block = json.at("fromBlock").get<uint64_t>();
    }
    if (json.count("toBlock") != 0) {
        filter.to_block = json.at("toBlock").get<uint64_t>();
    }
    if (json.count("address") != 0) {
        if (json.at("address").is_string()) {
            filter.addresses= {json.at("address").get<std::string>()};
        } else {
            filter.addresses = json.at("address").get<std::vector<std::string>>();
        }
    }
    if (json.count("topics") != 0) {
        filter.topics = json.at("topics").get<std::vector<std::string>>();
    }
    if (json.count("blockHash") != 0) {
        filter.block_hash = json.at("blockHash").get<std::string>();
    }
}

} // namespace silkrpc::json::eth

#endif  // SILKRPC_JSON_ETH_H_
