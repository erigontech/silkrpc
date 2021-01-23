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

#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <evmc/evmc.hpp>
#include <nlohmann/json.hpp>

#include <silkworm/common/util.hpp>
#include <silkrpc/core/types/receipt.hpp>

namespace evmc {

void to_json(nlohmann::json& json, const address& addr);
void from_json(const nlohmann::json& json, address& addr);

void to_json(nlohmann::json& json, const bytes32& b32);
void from_json(const nlohmann::json& json, bytes32& b32);

} // namespace evmc

namespace silkrpc::core {

void to_json(nlohmann::json& json, const Log& log);
void from_json(const nlohmann::json& json, Log& log);

void to_json(nlohmann::json& json, const Receipt& receipt);
void from_json(const nlohmann::json& json, Receipt& receipt);

} // namespace silkrpc::core

namespace silkrpc::json {

nlohmann::json make_json_content(uint32_t id, const nlohmann::json& result);
nlohmann::json make_json_error(uint32_t id, const std::string& error);

// TODO: START move to silkrpc/types/filter.hpp
typedef std::vector<std::string> FilterAddresses;
typedef std::vector<std::string> FilterSubTopics;
typedef std::vector<FilterSubTopics> FilterTopics;

struct Filter {
    std::optional<uint64_t> from_block;
    std::optional<uint64_t> to_block;
    std::optional<FilterAddresses> addresses;
    std::optional<FilterTopics> topics;
    std::optional<std::string> block_hash;
};
// TODO: END move to silkrpc/types/filter.hpp

void to_json(nlohmann::json& json, const Filter& filter);
void from_json(const nlohmann::json& json, Filter& filter);

std::ostream& operator<<(std::ostream& out, const Filter& filter);

} // namespace silkrpc::json

#endif  // SILKRPC_JSON_ETH_H_
