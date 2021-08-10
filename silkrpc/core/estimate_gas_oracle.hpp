/*
   Copyright 2021 The Silkrpc Authors

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

#ifndef SILKRPC_CORE_ESTIMATE_GAS_ORACLE_HPP_
#define SILKRPC_CORE_ESTIMATE_GAS_ORACLE_HPP_

#include <functional>
#include <string>
#include <vector>

#include <silkrpc/config.hpp> // NOLINT(build/include_order)

#include <asio/awaitable.hpp>
#include <silkworm/chain/config.hpp>
#include <silkworm/common/util.hpp>
#include <silkworm/state/buffer.hpp>
#include <silkworm/types/block.hpp>

#include <silkrpc/core/blocks.hpp>
#include <silkrpc/core/rawdb/accessors.hpp>
#include <silkrpc/core/evm_executor.hpp>
#include <silkrpc/types/call.hpp>
#include <silkrpc/types/transaction.hpp>

namespace silkrpc {

const std::uint64_t kTxGas = 21'000;
const std::uint64_t kGasCap = 25'000'000;

typedef std::function<asio::awaitable<silkrpc::ExecutionResult>(const silkworm::Transaction &)> Executor;

class EstimateGasOracle {
public:
    explicit EstimateGasOracle(const core::rawdb::DatabaseReader& db_reader, const Executor& executor)
        : db_reader_(db_reader), executor_(executor) {}
    virtual ~EstimateGasOracle() {}

    EstimateGasOracle(const EstimateGasOracle&) = delete;
    EstimateGasOracle& operator=(const EstimateGasOracle&) = delete;

    asio::awaitable<intx::uint256> estimate_gas(const Call& call, uint64_t block_number);

private:
    asio::awaitable<bool> execution_test(const Call& call, uint64_t gas);

    const core::rawdb::DatabaseReader& db_reader_;
    const Executor& executor_;
};

} // namespace silkrpc

#endif  // SILKRPC_CORE_ESTIMATE_GAS_ORACLE_HPP_
