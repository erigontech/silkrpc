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

#ifndef SILKRPC_TYPES_ENGINE_HPP_
#define SILKRPC_TYPES_ENGINE_HPP_

#include <iostream>
#include <map>
#include <optional>
#include <stdint.h>
#include <string>

#include <evmc/evmc.hpp>
#include <intx/intx.hpp>
#include <nlohmann/json.hpp>
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
    evmc::address suggestedFeeRecipient;
    evmc::bytes32 state_root;
    evmc::bytes32 receipts_root;
    evmc::bytes32 parent_hash;
    evmc::bytes32 block_hash;
    evmc::bytes32 random;
    intx::uint256 base_fee;
    silkworm::Bloom logs_bloom;
    silkworm::Bytes extra_data;
    std::vector<silkworm::Bytes> transactions;
};

std::ostream& operator<<(std::ostream& out, const ExecutionPayload& p);
} // namespace silkrpc

#endif  // SILKRPC_TYPES_DUMP_ACCOUNT_HPP_
