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

#include <silkworm/execution/evm.hpp>
#include <silkworm/state/intra_block_state.hpp>

namespace silkrpc {

asio::awaitable<ExecutionResult> Executor::call(const silkworm::Block& block, const silkworm::Transaction& txn, uint64_t gas) {
    silkworm::IntraBlockState state{buffer_};
    silkworm::EVM evm{block, state, config_};

    const auto result{evm.execute(txn, gas)}; // must be run on thread pool

    co_return ExecutionResult{result.status, result.gas_left, result.data};
}

} // namespace silkrpc
