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

#include <silkrpc/config.hpp>

#include <asio/awaitable.hpp>
#include <evmc/evmc.hpp>

#include <silkworm/core/silkworm/types/block.hpp>
#include <silkrpc/core/rawdb/accessors.hpp>

namespace silkrpc::core::rawdb {

asio::awaitable<uint64_t> read_header_number(DatabaseReader& reader, evmc::bytes32 block_hash);

asio::awaitable<evmc::bytes32> read_canonical_block_hash(DatabaseReader& reader, uint64_t block_number);

asio::awaitable<silkworm::BlockWithHash> read_block_by_number(DatabaseReader& reader, uint64_t block_number);

asio::awaitable<silkworm::BlockWithHash> read_block(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number);

asio::awaitable<silkworm::BlockHeader> read_header(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number);

asio::awaitable<silkworm::BlockBody> read_body(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number);

asio::awaitable<silkworm::Bytes> read_header_rlp(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number);

asio::awaitable<silkworm::Bytes> read_body_rlp(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number);

} // namespace silkrpc::core::rawdb

#endif  // SILKRPC_CORE_RAWDB_CHAIN_H_
