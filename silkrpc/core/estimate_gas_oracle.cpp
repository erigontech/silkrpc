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

namespace silkrpc::ego {

asio::awaitable<intx::uint256> EstimateGasOracle::estimate_gas(const Call& call, uint64_t block_number) {
    SILKRPC_LOG << "EstimateGasOracle::estimate_gas called\n";

    std::uint64_t hi;
    std::uint64_t lo = kTxGas - 1;

    SILKRPC_LOG << "Ready to check call.gas\n";
    if (call.gas.value_or(0) >= kTxGas) {
        SILKRPC_LOG << "Set HI with gas in args: " << call.gas.value_or(0) << "\n";
        hi = call.gas.value(); 
    } else {
        // TODO(sixtysixter) worths it to test if returned header is not null?
        const auto header = co_await block_header_provider_(block_number);
        hi = header.gas_limit;
        SILKRPC_LOG << "Evaluate HI with gas in block " << header.gas_limit << "\n";
    }

    SILKRPC_LOG << "Ready to check call.gas_price\n";
    intx::uint256 gas_price = call.gas_price.value_or(0);
    if (gas_price != 0) {
        SILKRPC_LOG << "call.gas_price != 0\n";

        evmc::address from = call.from.value_or(evmc::address{0});
    
        std::optional<silkworm::Account> account{co_await account_reader_(from, block_number + 1)};
        
        intx::uint256 balance = account->balance;
        SILKRPC_LOG << "balance for address " << from << ": 0x" << intx::hex(balance) << "\n";
        if (call.value.value_or(0) > balance) {
            throw std::runtime_error{"insufficient funds for transfer"};
        }
        auto available = balance - call.value.value_or(0);
        int64_t allowance = int64_t(available / gas_price);
        SILKRPC_LOG << "allowance: " << allowance << ", available: 0x" << intx::hex(available) << ", balance: 0x" << intx::hex(balance)  << "\n";
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

    SILKRPC_LOG << "hi: " << hi << ", lo: " << lo << ", cap: " << cap << "\n";

    silkworm::Transaction transaction{call.to_transaction()};
    while (lo + 1 < hi) {
        auto mid = (hi + lo) / 2;
        transaction.gas_limit = mid;
    
        auto failed = co_await execution_test(transaction);
        SILKRPC_LOG << "execution test, gas: " << mid << " failed: " << (failed ? "true" : "false") << "\n";

        if (failed) {
            lo = mid;
        } else {
            hi = mid;
        }
    }

    if (hi == cap) {
        SILKRPC_LOG << "HI is equal to cap test again...\n";
        transaction.gas_limit = hi;
        auto failed = co_await execution_test(transaction);
    }

    SILKRPC_LOG << "EstimateGasOracle::estimate_gas returning hi " << hi << "\n";
    co_return hi;
}

asio::awaitable<bool> EstimateGasOracle::execution_test(const silkworm::Transaction& transaction) {
    silkrpc::Transaction tnx{transaction};
    SILKRPC_LOG << "test execution transaction with gas " << transaction.gas_limit << " transaction" << tnx << "\n";
    // silkworm::Transaction transaction{call.to_transaction()};
    // transaction.gas_limit = gas;

    // SILKRPC_LOG << "calling executor \n";
    const auto result = co_await executor_(transaction);
    // SILKRPC_LOG << "executor returned \n";

    // bool failed = false;
    // if (result.pre_check_error || result.error_code != evmc_status_code::EVMC_SUCCESS) {
    //     failed = true;
    // }
    bool failed = true;
    if (result.pre_check_error) {
        SILKRPC_LOG << "result error " << result.pre_check_error.value() << "\n";
    } else if (result.error_code == evmc_status_code::EVMC_SUCCESS) {
        SILKRPC_LOG << "result SUCCESS\n";
        failed = false;
    } else if (result.error_code == evmc_status_code::EVMC_INSUFFICIENT_BALANCE) {
        SILKRPC_LOG << "result INSUFFICIENTE BALANCE\n";
    } else {
        const auto error_message = EVMExecutor::get_error_message(result.error_code, result.data);
        SILKRPC_LOG << "result message " << error_message << ", code " << result.error_code << "\n";
    }

    co_return failed;
}

} // namespace silkrpc
