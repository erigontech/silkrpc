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

#ifndef SILKRPC_TYPES_TRANSACTION_HPP_
#define SILKRPC_TYPES_TRANSACTION_HPP_

#include <iostream>
#include <map>
#include <optional>
#include <vector>
#include <string>

#include <evmc/evmc.hpp>
#include <intx/intx.hpp>

#include <silkworm/common/base.hpp>
#include <silkworm/types/block.hpp>
#include <silkworm/types/transaction.hpp>

namespace silkrpc {

struct Transaction : public silkworm::Transaction {
    evmc::bytes32 block_hash;
    uint64_t block_number{0};
    std::optional<intx::uint256> block_base_fee_per_gas{std::nullopt};
    uint64_t transaction_index{0};
    bool queued_in_pool{false};

    intx::uint256 effective_gas_price() const; // EIP-1559
};

struct Rlp  {
   silkworm::Bytes buffer;
};

struct TransactionWithBlock {
    silkworm::BlockWithHash block_with_hash;
    Transaction transaction;
};

using AccessList = std::vector<silkworm::AccessListEntry>;

struct AccessListResult {
    AccessList access_list;
    std::optional<std::string> error;
    uint64_t gas_used;
};


struct TxPoolStatusInfo {
   unsigned int base_fee;
   unsigned int pending;
   unsigned int queued;
};

struct CallBundleTxInfo {
   ethash_hash256 hash;
   uint64_t gas_used;
   evmc::bytes32 value;
   std::string error_message;
};

struct CallBundleInfo {
   ethash_hash256 bundle_hash;
   std::vector<CallBundleTxInfo> txs_info;
};

using TransactionContent = std::map <std::string, std::map<std::string, std::map<std::string, Transaction>>>;

std::ostream& operator<<(std::ostream& out, const Transaction& t);
std::ostream& operator<<(std::ostream& out, const silkworm::Transaction& t);

} // namespace silkrpc

#endif  // SILKRPC_TYPES_TRANSACTION_HPP_
