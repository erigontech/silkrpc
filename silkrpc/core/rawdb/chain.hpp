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

#ifndef SILKRPC_CORE_RAWDB_CHAIN_H_
#define SILKRPC_CORE_RAWDB_CHAIN_H_

#include <vector>

#include <silkrpc/config.hpp>

#include <asio/awaitable.hpp>
#include <evmc/evmc.hpp>
#include <intx/intx.hpp>
#include <nlohmann/json.hpp>

#include <silkworm/types/block.hpp>
#include <silkworm/types/transaction.hpp>
#include <silkrpc/core/rawdb/accessors.hpp>
#include <silkrpc/types/receipt.hpp>

namespace silkrpc::core::rawdb {

typedef std::vector<evmc::address> Addresses;
typedef std::vector<silkworm::Transaction> Transactions;

asio::awaitable<uint64_t> read_header_number(const DatabaseReader& reader, const evmc::bytes32& block_hash);

asio::awaitable<nlohmann::json> read_chain_config(const DatabaseReader& reader);

asio::awaitable<uint64_t> read_chain_id(const DatabaseReader& reader);

asio::awaitable<evmc::bytes32> read_canonical_block_hash(const DatabaseReader& reader, uint64_t block_number);

asio::awaitable<intx::uint256> read_total_difficulty(const DatabaseReader& reader, const evmc::bytes32& block_hash, uint64_t block_number);

asio::awaitable<silkworm::BlockWithHash> read_block_by_hash(const DatabaseReader& reader, const evmc::bytes32& block_hash);

asio::awaitable<silkworm::BlockWithHash> read_block_by_number(const DatabaseReader& reader, uint64_t block_number);

asio::awaitable<silkworm::BlockWithHash> read_block(const DatabaseReader& reader, const evmc::bytes32& block_hash, uint64_t block_number);

asio::awaitable<silkworm::BlockHeader> read_header(const DatabaseReader& reader, const evmc::bytes32& block_hash, uint64_t block_number);

asio::awaitable<silkworm::BlockBody> read_body(const DatabaseReader& reader, const evmc::bytes32& block_hash, uint64_t block_number);

asio::awaitable<silkworm::Bytes> read_header_rlp(const DatabaseReader& reader, const evmc::bytes32& block_hash, uint64_t block_number);

asio::awaitable<silkworm::Bytes> read_body_rlp(const DatabaseReader& reader, const evmc::bytes32& block_hash, uint64_t block_number);

asio::awaitable<Addresses> read_senders(const DatabaseReader& reader, const evmc::bytes32& block_hash, uint64_t block_number);

asio::awaitable<Receipts> read_raw_receipts(const DatabaseReader& reader, const evmc::bytes32& block_hash, uint64_t block_number);

asio::awaitable<Receipts> read_receipts(const DatabaseReader& reader, const evmc::bytes32& block_hash, uint64_t block_number);

asio::awaitable<Transactions> read_transactions(const DatabaseReader& reader, uint64_t base_txn_id, uint64_t txn_count);

} // namespace silkrpc::core::rawdb

#endif  // SILKRPC_CORE_RAWDB_CHAIN_H_
