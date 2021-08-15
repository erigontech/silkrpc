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

using evmc::literals::operator""_address;

static const evmc::address kZeroAddress = 0x0000000000000000000000000000000000000000_address;
static const evmc::address kFrom1    = 0xe5ef458d37212a06e3f59d40c454e76150ae7c32_address;
static const evmc::address kFrom2 = 0xe5ef458d37212a06e3f59d40c454e76150ae7c33_address;

static silkworm::BlockHeader allocate_block_header(uint64_t block_number, uint64_t gas_limit) {
    silkworm::BlockHeader block_header;

    block.header.number = block_number;
    block.header.gas_limit = gas_limit;

    return block_header;
}

using Catch::Matchers::Message;

TEST_CASE("estimate gas") {
    asio::thread_pool pool{1};

    const std::map<uint64_t, silkworm::BlockHeader> kBlockNumber2Header; 
    const std::map<uint64_t, silkrpc::ExecutionResult> kBlockNumber2Result; 
    const std::map<evmc::address, silkworm::Account> kAddress2Account; 

    ego::Executor executor = [&kBlockNumber2Result](const silkworm::Transaction &transaction) -> asio::awaitable<silkrpc::ExecutionResult> {
        co_return kBlockNumber2Result[transaction.];
    };

    BlockHeaderProvider block_header_provider = [&kBlockNumber2Header](uint64_t block_number) -> asio::awaitable<silkworm::BlockHeader> {
        co_return kBlockNumber2Header[block_number];
    };

    ego::AccountReader account_reader = [&kAddress2Account](const evmc::address& address, uint64_t block_number) -> asio::awaitable<silkworm::Account> {
        co_return kAddress2Account[address];
    };

    EstimateGasOracle estimate_gas_oracle{block_header_provider, account_reader, executor};

    SECTION("TODO" ) {
    }
}

} // namespace silkrpc
