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
#include <iostream>

#include <asio/co_spawn.hpp>
#include <asio/thread_pool.hpp>
#include <asio/use_future.hpp>
#include <boost/endian/conversion.hpp>
#include <catch2/catch.hpp>
#include <evmc/evmc.hpp>

#include <silkrpc/types/block.hpp>

namespace silkrpc::ego {

using Catch::Matchers::Message;

TEST_CASE("estimate gas") {
    asio::thread_pool pool{1};

    uint64_t count{0};
    std::vector<bool> steps;
    intx::uint256 kBalance{1'000'000'000};

    silkrpc::ExecutionResult kSuccessResult{evmc_status_code::EVMC_SUCCESS};
    silkrpc::ExecutionResult kFailureResult{evmc_status_code::EVMC_INSUFFICIENT_BALANCE};

    silkworm::BlockHeader kBlockHeader;
    kBlockHeader.gas_limit = kTxGas * 2;

    silkworm::Account kAccount{0, kBalance};

    Executor executor = [&steps, &count](const silkworm::Transaction &transaction) -> asio::awaitable<silkrpc::ExecutionResult> {
        bool success = steps[count++];
        silkrpc::ExecutionResult result{success ? evmc_status_code::EVMC_SUCCESS : evmc_status_code::EVMC_INSUFFICIENT_BALANCE};
        co_return result;
    };

    BlockHeaderProvider block_header_provider = [&kBlockHeader](uint64_t block_number) -> asio::awaitable<silkworm::BlockHeader> {
        co_return kBlockHeader;
    };

    AccountReader account_reader = [&kAccount](const evmc::address& address, uint64_t block_number) -> asio::awaitable<std::optional<silkworm::Account>> {
        co_return kAccount;
    };

    Call call;
    EstimateGasOracle estimate_gas_oracle{block_header_provider, account_reader, executor};

    SECTION("Call empty, always fails") {
        steps.resize(16);
        std::fill_n(steps.begin(), steps.size(), false);
        auto result = asio::co_spawn(pool, estimate_gas_oracle.estimate_gas(call, 0), asio::use_future);
        const intx::uint256 &estimate_gas = result.get();

        CHECK(estimate_gas == kTxGas * 2);
    }

    SECTION("Call empty, always succeed") {
        steps.resize(16);
        std::fill_n(steps.begin(), steps.size(), true);

        auto result = asio::co_spawn(pool, estimate_gas_oracle.estimate_gas(call, 0), asio::use_future);
        const intx::uint256 &estimate_gas = result.get();

        CHECK(estimate_gas == kTxGas);
    }

    SECTION("Call empty, alternatively fails and succeed") {
        int current = false;
        auto generate = [&current]() -> bool {
            return ++current % 2 == 0;;
        };
        steps.resize(16);
        std::generate_n(steps.begin(), steps.size(), generate);

        auto result = asio::co_spawn(pool, estimate_gas_oracle.estimate_gas(call, 0), asio::use_future);
        const intx::uint256 &estimate_gas = result.get();

        CHECK(estimate_gas == 0x88b6);
    }

    SECTION("Call empty, alternatively succeed and fails") {
        int current = false;
        auto generate = [&current]() -> bool {
            return current++ % 2 == 0;;
        };
        steps.resize(16);
        std::generate_n(steps.begin(), steps.size(), generate);

        auto result = asio::co_spawn(pool, estimate_gas_oracle.estimate_gas(call, 0), asio::use_future);
        const intx::uint256 &estimate_gas = result.get();

        CHECK(estimate_gas == 0x6d5e);
    }

    SECTION("Call with gas, always fails") {
        call.gas = kTxGas * 4;
        steps.resize(17);
        std::fill_n(steps.begin(), steps.size(), false);
        auto result = asio::co_spawn(pool, estimate_gas_oracle.estimate_gas(call, 0), asio::use_future);
        const intx::uint256 &estimate_gas = result.get();

        CHECK(estimate_gas == kTxGas * 4);
    }

    SECTION("Call with gas, always succeed") {
        call.gas = kTxGas * 4;
        steps.resize(17);
        std::fill_n(steps.begin(), steps.size(), true);
        auto result = asio::co_spawn(pool, estimate_gas_oracle.estimate_gas(call, 0), asio::use_future);
        const intx::uint256 &estimate_gas = result.get();

        CHECK(estimate_gas == kTxGas);
    }

    SECTION("Call with gas_price, gas not capped") {
        call.gas = kTxGas * 2;
        call.gas_price = intx::uint256{10'000};
        steps.resize(16);
        std::fill_n(steps.begin(), steps.size(), false);
        auto result = asio::co_spawn(pool, estimate_gas_oracle.estimate_gas(call, 0), asio::use_future);
        const intx::uint256 &estimate_gas = result.get();

        CHECK(estimate_gas == kTxGas * 2);
    }

    SECTION("Call with gas_price, gas capped") {
        call.gas = kTxGas * 2;
        call.gas_price = intx::uint256{40'000};
        steps.resize(16);
        std::fill_n(steps.begin(), steps.size(), false);
        auto result = asio::co_spawn(pool, estimate_gas_oracle.estimate_gas(call, 0), asio::use_future);
        const intx::uint256 &estimate_gas = result.get();

        CHECK(estimate_gas == 0x61a8);
    }

    SECTION("Call with gas_price and value, gas not capped") {
        call.gas = kTxGas * 2;
        call.gas_price = intx::uint256{10'000};
        call.value = intx::uint256{500'000'000};
        steps.resize(16);
        std::fill_n(steps.begin(), steps.size(), false);
        auto result = asio::co_spawn(pool, estimate_gas_oracle.estimate_gas(call, 0), asio::use_future);
        const intx::uint256 &estimate_gas = result.get();

        CHECK(estimate_gas == kTxGas * 2);
    }

    SECTION("Call with gas_price and value, gas capped") {
        call.gas = kTxGas * 2;
        call.gas_price = intx::uint256{20'000};
        call.value = intx::uint256{500'000'000};
        steps.resize(16);
        std::fill_n(steps.begin(), steps.size(), false);
        auto result = asio::co_spawn(pool, estimate_gas_oracle.estimate_gas(call, 0), asio::use_future);
        const intx::uint256 &estimate_gas = result.get();

        CHECK(estimate_gas == 0x61a8);
    }

    SECTION("Call gas above allowance, always succeed, gas capped") {
        call.gas = kGasCap * 2;
        steps.resize(26);
        std::fill_n(steps.begin(), steps.size(), false);
        auto result = asio::co_spawn(pool, estimate_gas_oracle.estimate_gas(call, 0), asio::use_future);
        const intx::uint256 &estimate_gas = result.get();

        CHECK(estimate_gas == kGasCap);
    }

    SECTION("Call gas below minimum, always succeed") {
        call.gas = kTxGas / 2;

        steps.resize(26);
        std::fill_n(steps.begin(), steps.size(), false);
        auto result = asio::co_spawn(pool, estimate_gas_oracle.estimate_gas(call, 0), asio::use_future);
        const intx::uint256 &estimate_gas = result.get();

        CHECK(estimate_gas == kTxGas * 2);
    }

    SECTION("Call with too high value, exception") {
        call.value = intx::uint256{2'000'000'000};
        steps.resize(16);
        std::fill_n(steps.begin(), steps.size(), false);

        // auto handler = [](std::exception const* pexception, intx::uint256 estimate_gas) -> void {
        //     std::cout << "exception pointer " << pexception << std::endl << std::flush;
            // REQUIRE(pexception != std::nullptr_t);            
        // };
        // asio::co_spawn(pool, estimate_gas_oracle.estimate_gas(call, 0), handler);
    }
}

} // namespace silkrpc::ego
