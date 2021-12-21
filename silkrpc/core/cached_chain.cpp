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

#include <silkrpc/context_pool.hpp>
#include <silkrpc/core/cached_chain.hpp>
#include <silkrpc/core/rawdb/chain.hpp>

namespace silkrpc::core  {

asio::awaitable<silkworm::BlockWithHash> read_block_by_number(const std::shared_ptr<BlockCache> cache, const rawdb::DatabaseReader& reader, uint64_t block_number) {
   const auto block_hash = co_await rawdb::read_canonical_block_hash(reader, block_number);
   auto option_block = cache->get(block_hash);
   if (option_block) {
      co_return *option_block;
   }
   auto block_with_hash = co_await rawdb::read_block(reader, block_hash, block_number);
   cache->insert(block_hash, block_with_hash);
   co_return block_with_hash;
}

asio::awaitable<silkworm::BlockWithHash> read_block_by_hash(const std::shared_ptr<BlockCache> cache, const rawdb::DatabaseReader& reader, const evmc::bytes32& block_hash) {
   auto option_block = cache->get(block_hash);
   if (option_block) {
      co_return *option_block;
   }
   auto block_with_hash = co_await rawdb::read_block_by_hash(reader, block_hash);
   cache->insert(block_hash, block_with_hash);
   co_return block_with_hash;
}

} // namespace silkrpc::core


