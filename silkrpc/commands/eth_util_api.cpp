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

#include <silkrpc/commands/eth_util_api.hpp>

#include <memory>
#include <vector>


#include <silkrpc/config.hpp> // NOLINT(build/include_order)

#include <asio/awaitable.hpp>
#include <asio/thread_pool.hpp>
#include <evmc/evmc.hpp>
#include <nlohmann/json.hpp>

#include <silkrpc/context_pool.hpp>

#include <silkworm/common/util.hpp>
#include <silkworm/common/base.hpp>
#include <silkworm/chain/config.hpp>
#include <silkworm/common/log.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/types/receipt.hpp>

#include <silkrpc/core/rawdb/accessors.hpp>
#include <silkrpc/core/rawdb/chain.hpp>
#include <silkrpc/types/block.hpp>
#include <silkrpc/types/chain_config.hpp>

#include <silkrpc/commands/block_cache.hpp>


namespace silkrpc::commands::api  {

asio::awaitable<silkworm::BlockWithHash> read_block_by_number(const silkrpc::Context &context, const silkrpc::core::rawdb::DatabaseReader& reader, uint64_t block_number) {
   const auto block_hash = co_await silkrpc::core::rawdb::read_canonical_block_hash(reader, block_number);
   auto option_block = context.block_cache->get(block_hash);
   if (option_block) {
      co_return *option_block;
   }
   auto block_with_hash = co_await silkrpc::core::rawdb::read_block(reader, block_hash, block_number);
   context.block_cache->insert(block_hash, block_with_hash);
   co_return block_with_hash;
}

asio::awaitable<silkworm::BlockWithHash> read_block_by_hash(const silkrpc::Context &context, const silkrpc::core::rawdb::DatabaseReader& reader, const evmc::bytes32& block_hash) {
   auto option_block = context.block_cache->get(block_hash);
   if (option_block) {
      co_return *option_block;
   }
   auto block_with_hash = co_await core::rawdb::read_block_by_hash(reader, block_hash);
   context.block_cache->insert(block_hash, block_with_hash);
   co_return block_with_hash;
}

} // namespace silkrpc::commands::api


