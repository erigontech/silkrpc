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

#ifndef SILKRPC_CORE_EXECUTOR_HPP_
#define SILKRPC_CORE_EXECUTOR_HPP_

#include <silkrpc/config.hpp> // NOLINT(build/include_order)

#include <asio/awaitable.hpp>
#include <asio/thread_pool.hpp>
#include <silkworm/chain/config.hpp>
#include <silkworm/common/util.hpp>
#include <silkworm/state/buffer.hpp>
#include <silkworm/types/block.hpp>
#include <silkworm/types/transaction.hpp>

#include <silkrpc/context_pool.hpp>
#include <silkrpc/core/remote_buffer.hpp>
#include <silkrpc/core/rawdb/accessors.hpp>

namespace silkrpc {

struct ExecutionResult {
    int64_t error_code;
    uint64_t gas_left;
    silkworm::Bytes data;
};

class Executor {
public:
    explicit Executor(const Context& context, const core::rawdb::DatabaseReader& db_reader, const silkworm::ChainConfig& config, uint64_t block_number)
    : context_(context), db_reader_(db_reader), config_(config), buffer_{*context.io_context, db_reader, block_number}, thread_pool_{10} {}
    virtual ~Executor() {}

    Executor(const Executor&) = delete;
    Executor& operator=(const Executor&) = delete;

    asio::awaitable<ExecutionResult> call(const silkworm::Block& block, const silkworm::Transaction& txn, uint64_t gas);

private:
    const Context& context_;
    const core::rawdb::DatabaseReader& db_reader_;
    const silkworm::ChainConfig& config_;
    RemoteBuffer buffer_;
    asio::thread_pool thread_pool_;
};

} // namespace silkrpc

#endif  // SILKRPC_CORE_EXECUTOR_HPP_
