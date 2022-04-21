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

#ifndef SILKRPC_CORE_EVM_EXECUTOR_HPP_
#define SILKRPC_CORE_EVM_EXECUTOR_HPP_

#include <memory>
#include <string>

#include <silkrpc/config.hpp> // NOLINT(build/include_order)

#include <asio/awaitable.hpp>
#include <asio/thread_pool.hpp>
#include <silkworm/execution/evm.hpp>
#include <silkworm/chain/config.hpp>
#include <silkworm/common/util.hpp>
#include <silkworm/core/silkworm/execution/evm.hpp>
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
    std::optional<std::string> pre_check_error{std::nullopt};
};

template<typename WorldState = silkworm::IntraBlockState, typename VM = silkworm::EVM>
class EVMExecutor {
public:
    static std::string get_error_message(int64_t error_code, const silkworm::Bytes& error_data, const bool full_error = true);

    explicit EVMExecutor(const Context& context, const core::rawdb::DatabaseReader& db_reader, const silkworm::ChainConfig& config, asio::thread_pool& workers, uint64_t block_number)
    : context_(context), db_reader_(db_reader), config_(config), workers_{workers}, remote_state_{*context.io_context, db_reader, block_number}, state_{remote_state_} {}
    virtual ~EVMExecutor() {}

    EVMExecutor(const EVMExecutor&) = delete;
    EVMExecutor& operator=(const EVMExecutor&) = delete;

    asio::awaitable<ExecutionResult> call(const silkworm::Block& block, const silkworm::Transaction& txn, bool refund = true,
                                          bool gas_bailout = false, std::shared_ptr<silkworm::EvmTracer> tracer = {});

private:
    std::optional<std::string> pre_check(const VM& evm, const silkworm::Transaction& txn, const intx::uint256 base_fee_per_gas, const intx::uint128 g0);
    uint64_t refund_gas(const VM& evm, const silkworm::Transaction& txn, uint64_t gas_left);


    const Context& context_;
    const core::rawdb::DatabaseReader& db_reader_;
    const silkworm::ChainConfig& config_;
    asio::thread_pool& workers_;
    state::RemoteState remote_state_;
    WorldState state_;
};

} // namespace silkrpc

#endif  // SILKRPC_CORE_EVM_EXECUTOR_HPP_
