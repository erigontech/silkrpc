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

#include "blocks.hpp"

#include <silkrpc/common/log.hpp>
#include <silkrpc/core/rawdb/chain.hpp>
#include <silkrpc/stagedsync/stages.hpp>
#include <silkworm/common/assert.hpp>

namespace silkrpc::core {

boost::asio::awaitable<uint64_t> get_block_number(const std::string& block_id, const core::rawdb::DatabaseReader& reader) {
    uint64_t block_number;
    if (block_id == kEarliestBlockId) {
        block_number = kEarliestBlockNumber;
    } else if (block_id == kLatestBlockId || block_id == kPendingBlockId) {
        block_number = co_await get_latest_block_number(reader);
    } else {
        block_number = std::stol(block_id, 0, 0);
    }
    SILKRPC_DEBUG << "get_block_number block_number: " << block_number << "\n";
    co_return block_number;
}

boost::asio::awaitable<uint64_t> get_current_block_number(const core::rawdb::DatabaseReader& reader) {
    const auto current_block_number = co_await stages::get_sync_stage_progress(reader, stages::kFinish);
    co_return current_block_number;
}

boost::asio::awaitable<uint64_t> get_highest_block_number(const core::rawdb::DatabaseReader& reader) {
    const auto highest_block_number = co_await stages::get_sync_stage_progress(reader, stages::kHeaders);
    co_return highest_block_number;
}

boost::asio::awaitable<uint64_t> get_latest_block_number(const core::rawdb::DatabaseReader& reader) {
    const auto latest_block_number = co_await stages::get_sync_stage_progress(reader, stages::kExecution);
    co_return latest_block_number;
}

boost::asio::awaitable<bool> is_latest_block_number(const BlockNumberOrHash& bnoh, const core::rawdb::DatabaseReader& reader) {
    if (bnoh.is_tag()) {
        co_return bnoh.tag() == core::kLatestBlockId || bnoh.tag() == core::kPendingBlockId;
    } else {
        const auto latest_block_number = co_await get_latest_block_number(reader);
        if (bnoh.is_number()) {
            co_return bnoh.number() == latest_block_number;
        } else {
            SILKWORM_ASSERT(bnoh.is_hash());
            const auto block_number = co_await rawdb::read_header_number(reader, bnoh.hash());
            co_return block_number == latest_block_number;
        }
    }
}

}  // namespace silkrpc::core
