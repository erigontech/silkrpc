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

#include "executor.hpp"

#include <asio/compose.hpp>
#include <asio/post.hpp>
#include <asio/use_awaitable.hpp>
#include <silkworm/execution/evm.hpp>
#include <silkworm/state/intra_block_state.hpp>

#include <silkrpc/common/log.hpp>

namespace silkrpc {

asio::awaitable<ExecutionResult> Executor::call(const silkworm::Block& block, const silkworm::Transaction& txn, uint64_t gas) {
    SILKRPC_DEBUG << "Executor::call block: " << block.header.number << " txn: " << &txn << " gas: " << gas << " start\n";

    const auto exec_result = co_await asio::async_compose<decltype(asio::use_awaitable), void(ExecutionResult)>(
        [this, block, txn, gas](auto&& self) {
            SILKRPC_TRACE << "Executor::call post block: " << block.header.number << " txn: " << &txn << " gas: " << gas << "\n";
            asio::post(thread_pool_, [this, block, txn, gas, self = std::move(self)]() mutable {
                silkworm::IntraBlockState state{buffer_};
                silkworm::EVM evm{block, state, config_};

                SILKRPC_TRACE << "Executor::call execute on EVM block: " << block.header.number << " txn: " << &txn << " start\n";
                const auto result{evm.execute(txn, gas)};
                SILKRPC_TRACE << "Executor::call execute on EVM block: " << block.header.number << " txn: " << &txn << " end\n";

                ExecutionResult exec_result{result.status, result.gas_left, result.data};
                asio::post(*context_.io_context, [exec_result, self = std::move(self)]() mutable {
                    self.complete(exec_result);
                });
            });
        },
        asio::use_awaitable
    );

    SILKRPC_DEBUG << "Executor::call exec_result: " << exec_result.error_code << " end\n";

    co_return exec_result;
}

} // namespace silkrpc
