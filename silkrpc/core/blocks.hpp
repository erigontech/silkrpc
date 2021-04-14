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

#ifndef SILKRPC_CORE_BLOCKS_HPP_
#define SILKRPC_CORE_BLOCKS_HPP_

#include <silkrpc/config.hpp>

#include <asio/awaitable.hpp>

#include <silkrpc/core/rawdb/accessors.hpp>

namespace silkrpc::core {

constexpr uint64_t kEarliestBlockNumber{0ul};
constexpr uint64_t kLatestBlockNumber{-1ul};
constexpr uint64_t kPendingBlockNumber{-2ul};

asio::awaitable<uint64_t> get_block_number(uint64_t number, const core::rawdb::DatabaseReader& reader);

asio::awaitable<uint64_t> get_current_block_number(const core::rawdb::DatabaseReader& reader);

asio::awaitable<uint64_t> get_highest_block_number(const core::rawdb::DatabaseReader& reader);

asio::awaitable<uint64_t> get_latest_block_number(const core::rawdb::DatabaseReader& reader);

} // namespace silkrpc::core

#endif  // SILKRPC_CORE_BLOCKS_HPP_
