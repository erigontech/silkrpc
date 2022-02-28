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

#ifndef SILKRPC_TYPES_EXECUTION_PAYLOAD_HPP_
#define SILKRPC_TYPES_EXECUTION_PAYLOAD_HPP_

#include <vector>
#include <optional>
#include <string>

#include <evmc/evmc.hpp>
#include <intx/intx.hpp>
#include <silkworm/common/base.hpp>
#include <silkworm/types/bloom.hpp>

namespace silkrpc {

/*
*   Execution Payload as specified by https://github.com/ethereum/execution-apis/blob/main/src/engine/specification.md
*/
struct ExecutionPayload {
    uint64_t number;
    uint64_t timestamp;
    uint64_t gas_limit;
    uint64_t gas_used;
    evmc::address suggested_fee_recipient;
    evmc::bytes32 state_root;
    evmc::bytes32 receipts_root;
    evmc::bytes32 parent_hash;
    evmc::bytes32 block_hash;
    evmc::bytes32 prev_randao;
    intx::uint256 base_fee;
    silkworm::Bloom logs_bloom;
    silkworm::Bytes extra_data;
    std::vector<silkworm::Bytes> transactions;
};

/*
*   Payload Status as specified by https://github.com/ethereum/execution-apis/blob/main/src/engine/specification.md
*/
struct PayloadStatus {
    std::string status;
    std::optional<evmc::bytes32> latest_valid_hash;
    std::optional<std::string> validation_error;
};

struct TransitionConfiguration{
    uint256_t total_terminal_difficulty;
    uint64_t terminal_block_number;
    evmc::bytes32 terminal_block_hash;
};

std::ostream& operator<<(std::ostream& out, const ExecutionPayload& payload);
std::ostream& operator<<(std::ostream& out, const PayloadStatus& payload_status);

} // namespace silkrpc

#endif  // SILKRPC_TYPES_EXECUTION_PAYLOAD_HPP_
