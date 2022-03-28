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

#ifndef SILKRPC_JSON_TYPES_HPP_
#define SILKRPC_JSON_TYPES_HPP_

#include <optional>
#include <set>
#include <string>
#include <vector>

#include <intx/intx.hpp>
#include <evmc/evmc.hpp>
#include <nlohmann/json.hpp>

#include <silkrpc/types/block.hpp>
#include <silkrpc/types/call.hpp>
#include <silkrpc/types/chain_config.hpp>
#include <silkrpc/types/error.hpp>
#include <silkrpc/types/filter.hpp>
#include <silkrpc/types/issuance.hpp>
#include <silkrpc/types/log.hpp>
#include <silkrpc/types/transaction.hpp>
#include <silkrpc/types/execution_payload.hpp>
#include <silkrpc/types/receipt.hpp>
#include <silkworm/types/block.hpp>
#include <silkworm/types/transaction.hpp>

namespace evmc {

void to_json(nlohmann::json& json, const address& addr);
void from_json(const nlohmann::json& json, address& addr);

void to_json(nlohmann::json& json, const bytes32& b32);
void from_json(const nlohmann::json& json, bytes32& b32);

} // namespace evmc

namespace intx {

void from_json(const nlohmann::json& json, uint256& ui256);

} // namespace intx

namespace silkworm {

void to_json(nlohmann::json& json, const BlockHeader& ommer);

void to_json(nlohmann::json& json, const Transaction& transaction);


} // namespace silkworm

namespace silkrpc {

void to_json(nlohmann::json& json, const Rlp& rlp);

void to_json(nlohmann::json& json, const Block& b);

void to_json(nlohmann::json& json, const Transaction& transaction);

void from_json(const nlohmann::json& json, Call& call);

void to_json(nlohmann::json& json, const Log& log);
void from_json(const nlohmann::json& json, Log& log);

void to_json(nlohmann::json& json, const Receipt& receipt);
void from_json(const nlohmann::json& json, Receipt& receipt);

void to_json(nlohmann::json& json, const Filter& filter);
void from_json(const nlohmann::json& json, Filter& filter);

void to_json(nlohmann::json& json, const ExecutionPayload& execution_payload);
void from_json(const nlohmann::json& json, ExecutionPayload& execution_payload);

void to_json(nlohmann::json& json, const PayloadStatus& payload_status);

void to_json(nlohmann::json& json, const TransitionConfiguration& transition_configuration);
void from_json(const nlohmann::json& json, TransitionConfiguration& transition_configuration);

void to_json(nlohmann::json& json, const Forks& forks);

void to_json(nlohmann::json& json, const Issuance& issuance);

void to_json(nlohmann::json& json, const Error& error);
void to_json(nlohmann::json& json, const RevertError& error);

void to_json(nlohmann::json& json, const std::set<evmc::address>& addresses);

std::string to_hex_no_leading_zeros(uint64_t number);
std::string to_hex_no_leading_zeros(silkworm::ByteView bytes);

std::string to_quantity(uint64_t number);
std::string to_quantity(intx::uint256 number);
std::string to_quantity(silkworm::ByteView bytes);

nlohmann::json make_json_content(uint32_t id, const nlohmann::json& result);
nlohmann::json make_json_error(uint32_t id, int32_t code, const std::string& message);
nlohmann::json make_json_error(uint32_t id, const RevertError& error);

} // namespace silkrpc

namespace nlohmann {

template <>
struct adl_serializer<silkrpc::BlockNumberOrHash> {
    static silkrpc::BlockNumberOrHash from_json(const json& json) {
        if (json.is_string()) {
            return silkrpc::BlockNumberOrHash{json.get<std::string>()};
        } else if (json.is_number()) {
            return silkrpc::BlockNumberOrHash{json.get<std::uint64_t>()};
        }
        return silkrpc::BlockNumberOrHash{0};
    }
};

} // namespace nlohmann

#endif  // SILKRPC_JSON_TYPES_HPP_
