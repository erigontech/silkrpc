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

#include "estimate_gas_oracle.hpp"

#include <algorithm>
#include <utility>

#include <asio/compose.hpp>
#include <asio/post.hpp>
#include <asio/use_awaitable.hpp>
#include <silkrpc/core/blocks.hpp>
#include <silkrpc/core/rawdb/chain.hpp>

#include <silkrpc/common/log.hpp>

namespace silkrpc {

asio::awaitable<intx::uint256> EstimateGasOracle::estimate_gas(const Call& call, uint64_t block_number) {
    SILKRPC_LOG << "EstimateGasOracle::estimate_gas called\n";

    std::uint64_t hi;
    std::uint64_t lo = kTxGas - 1;

    SILKRPC_LOG << "Ready to check call.gas\n";
    if (call.gas.value_or(0) >= kTxGas) {
        SILKRPC_LOG << "Set HI\n";
        hi = call.gas.value(); 
    } else {
        SILKRPC_LOG << "Evaluate HI\n";
        // TODO(sixtysixter) serve mettere test sul valore ritornato?
        const auto header = co_await core::rawdb::read_header_by_number(db_reader_, block_number);
        hi = header.gas_limit;
    }

    SILKRPC_LOG << "Ready to check call.gas_price\n";
    intx::uint256 gas_price = call.gas_price.value_or(0);
    if (gas_price != 0) {
        SILKRPC_LOG << "call.gas_price != 0\n";

        evmc::address from = call.from.value_or(evmc::address{0});
    
        StateReader state_reader{db_reader_};
        std::optional<silkworm::Account> account{co_await state_reader.read_account(from, block_number + 1)};
        
        intx::uint256 balance = account->balance;
        SILKRPC_LOG << "balance for address " << from << ": 0x" << intx::hex(balance) << "\n";
        if (call.value.value_or(0) > balance) {
            throw std::runtime_error{"insufficient funds for transfer"};
        }
        auto available = balance - call.value.value_or(0);
        int64_t allowance = int64_t(available / gas_price);
        if (hi > allowance) {
            SILKRPC_WARN << "gas estimation capped by limited funds: original " << hi 
                << ", balance 0x" << intx::hex(balance)
				<< ", sent" << intx::hex(call.value.value_or(0))
                << ", gasprice" << intx::hex(gas_price)
                << ", fundable" << allowance
                << "\n";
            hi = allowance;
        }
    } else {
        SILKRPC_LOG << "call.gas_price null or == 0\n";
    }

    if (hi > kGasCap) {
        SILKRPC_WARN << "caller gas above allowance, capping: requested " << hi << ", cap " << kGasCap << "\n";
        hi = kGasCap;
    }
    auto cap = hi;
    // auto executable = [this, &call](uint64_t gas) {
    //     SILKRPC_LOG << "test execution with gas " << gas << "\n";
    //     silkworm::Transaction transaction{call.to_transaction()};
    //     const auto execution_result = co_await executor_(transaction);

    //     return false;
    // };

    while (lo + 1 < hi) {
        auto mid = (hi + lo) / 2;
        auto failed = co_await execution_test(call, mid);

        if (failed) {
            lo = mid;
        } else {
            hi = mid;
        }
    }

    if (hi == cap) {
        SILKRPC_LOG << "HI is equal to cap test again...\n";
        auto failed = co_await execution_test(call, hi);
    }
        // EVMExecutor executor{context_, tx_database, *chain_config_ptr, workers_, block_number};
        // const auto block_with_hash = co_await core::rawdb::read_block_by_number(tx_database, block_number);
        // silkworm::Transaction txn{call.to_transaction()};
        // const auto execution_result = co_await executor.call(block_with_hash.block, txn);

        // if (execution_result.pre_check_error) {
        //     reply = make_json_error(request["id"], -32000, execution_result.pre_check_error.value());
        // } else if (execution_result.error_code == evmc_status_code::EVMC_SUCCESS) {
        //     reply = make_json_content(request["id"], "0x" + silkworm::to_hex(execution_result.data));
        // } else {
        //     const auto error_message = EVMExecutor::get_error_message(execution_result.error_code, execution_result.data);
        //     if (execution_result.data.empty()) {
        //         reply = make_json_error(request["id"], -32000, error_message);
        //     } else {
        //         reply = make_json_error(request["id"], {3, error_message, execution_result.data});
        //     }
        // }

    SILKRPC_LOG << "EstimateGasOracle::estimate_gas returning hi " << hi << "\n";
    co_return hi;
}

asio::awaitable<bool> EstimateGasOracle::execution_test(const Call& call, uint64_t gas) {
    SILKRPC_LOG << "test execution with gas " << gas << "\n";
    silkworm::Transaction transaction{call.to_transaction()};

    SILKRPC_LOG << "calling executor \n";
    const auto result = co_await executor_(transaction);
    // SILKRPC_LOG << "executor result " << result << " \n";

    co_return false;
}

} // namespace silkrpc
