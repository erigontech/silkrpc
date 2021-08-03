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

#include "gas_price_oracle.hpp"
#include <boost/endian/conversion.hpp>

#include <algorithm>
#include <iostream>
#include <catch2/catch.hpp>
#include <asio/co_spawn.hpp>
#include <asio/thread_pool.hpp>
#include <asio/use_future.hpp>

#include <evmc/evmc.hpp>

#include <silkrpc/types/block.hpp>

namespace silkrpc {

using evmc::literals::operator""_address;

static std::vector<silkworm::BlockWithHash> kBlocks;

static const evmc::address kBeneficiary = 0xe5ef458d37212a06e3f59d40c454e76150ae7c31_address;
static const evmc::address kFromTnx1 = 0xe5ef458d37212a06e3f59d40c454e76150ae7c32_address;
static const evmc::address kFromTnx2 = 0xe5ef458d37212a06e3f59d40c454e76150ae7c33_address;

static silkworm::BlockWithHash allocate_block(uint64_t block_number,
    const evmc::address &beneficiary,
    const intx::uint256 &base_fee,
    const intx::uint256 &max_priority_fee_per_gas_tx1,
    const intx::uint256 &max_fee_per_gas_tx1,
    const intx::uint256 &max_priority_fee_per_gas_tx2,
    const intx::uint256 &max_fee_per_gas_tx2) {

    silkworm::BlockWithHash block_with_hash;

    block_with_hash.block.header.number = block_number;
    block_with_hash.block.header.beneficiary = beneficiary;
    block_with_hash.block.header.base_fee_per_gas = base_fee;

    block_with_hash.block.transactions.resize(2);
    block_with_hash.block.transactions[0].max_priority_fee_per_gas = max_priority_fee_per_gas_tx1;
    block_with_hash.block.transactions[0].max_fee_per_gas = max_fee_per_gas_tx1;
    block_with_hash.block.transactions[0].from = kFromTnx1;

    block_with_hash.block.transactions[1].max_priority_fee_per_gas = max_priority_fee_per_gas_tx2;
    block_with_hash.block.transactions[1].max_fee_per_gas = max_fee_per_gas_tx2;
    block_with_hash.block.transactions[1].from = kFromTnx2;

    return block_with_hash;
}

static void clear_blocks_vector() {
    kBlocks.clear();
}

static void fill_blocks_vector(uint16_t size,
    const evmc::address &beneficiary,
    const intx::uint256 &base_fee,
    const intx::uint256 &max_priority_fee_per_gas_tx1,
    const intx::uint256 &max_fee_per_gas_tx1,
    const intx::uint256 &max_priority_fee_per_gas_tx2,
    const intx::uint256 &max_fee_per_gas_tx2) {

    kBlocks.reserve(size);
    for (auto idx = 0; idx < size; idx++) {
        silkworm::BlockWithHash block_with_hash = allocate_block(idx, beneficiary, base_fee,
            max_priority_fee_per_gas_tx1, max_fee_per_gas_tx1,
            max_priority_fee_per_gas_tx2, max_fee_per_gas_tx2);
        kBlocks.push_back(block_with_hash);
    }
}

static void fill_blocks_vector(uint16_t size,
    const evmc::address &beneficiary,
    const intx::uint256 &base_fee,
    const intx::uint256 &max_priority_fee_per_gas,
    const int delta_max_priority_fee_per_gas,
    const intx::uint256 &max_fee_per_gas,
    const int delta_max_fee_per_gas) {

    kBlocks.reserve(size);
    for (auto idx = 0; idx < size; idx++) {
        int64_t max_priority = int64_t(max_priority_fee_per_gas) + delta_max_priority_fee_per_gas * idx;
        if (max_priority < 0) {
            max_priority = 0;
        }
        int64_t  max_fee = int64_t(max_fee_per_gas) + delta_max_fee_per_gas * idx;
        if (max_fee < 0) {
            max_fee = 0;
        }

        silkworm::BlockWithHash block_with_hash = allocate_block(idx, beneficiary, base_fee,
            intx::uint256{max_priority},
            intx::uint256{max_fee},
            intx::uint256{max_priority},
            intx::uint256{max_fee});
        kBlocks.push_back(block_with_hash);
    }
}

asio::awaitable<silkworm::BlockWithHash> get_block(uint64_t block_number) {
    REQUIRE(kBlocks.size() > block_number);

    const silkworm::BlockWithHash block = kBlocks[block_number];

    co_return block;
}

BlockProvider block_provider = [](uint64_t block_number) {
    return get_block(block_number);
};

asio::thread_pool pool{1};

using Catch::Matchers::Message;
TEST_CASE("0 blocks") {
    const intx::uint256 base_fee = 0;
    const intx::uint256 max_priority_fee_per_gas = 0x32;
    const intx::uint256 max_fee_per_gas = 0x32;
    const intx::uint256 expected_price = kDefaultPrice;

    clear_blocks_vector();
    fill_blocks_vector(1, kBeneficiary, base_fee, max_priority_fee_per_gas, max_fee_per_gas, max_priority_fee_per_gas, max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(0), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("1 block with 0x0 base fee") {
    const intx::uint256 base_fee = 0;
    const intx::uint256 max_priority_fee_per_gas = 0x32;
    const intx::uint256 max_fee_per_gas = 0x32;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas + base_fee, max_fee_per_gas);

    clear_blocks_vector();
    fill_blocks_vector(2, kBeneficiary, base_fee, max_priority_fee_per_gas, max_fee_per_gas, max_priority_fee_per_gas, max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("1 block with 0x7 base_fee and same max_priority and max_fee in tnxs") {
    const intx::uint256 base_fee = 0x7;
    const intx::uint256 max_priority_fee_per_gas = 0x32;
    const intx::uint256 max_fee_per_gas = 0x32;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas + base_fee, max_fee_per_gas);

    clear_blocks_vector();
    fill_blocks_vector(2, kBeneficiary, base_fee, max_priority_fee_per_gas, max_fee_per_gas, max_priority_fee_per_gas, max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("1 block with 0x7 base_fee and different max_priority and max_fee in tnxs") {
    const intx::uint256 base_fee = 0x7;
    const intx::uint256 max_priority_fee_per_gas_tx1 = 0x0;
    const intx::uint256 max_fee_per_gas_tx1 = 0x32;
    const intx::uint256 max_priority_fee_per_gas_tx2 = 0x32;
    const intx::uint256 max_fee_per_gas_tx2 = 0x32;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas_tx1 + base_fee, max_fee_per_gas_tx1);

    clear_blocks_vector();
    fill_blocks_vector(2, kBeneficiary, base_fee, max_priority_fee_per_gas_tx1, max_fee_per_gas_tx1, max_priority_fee_per_gas_tx2, max_fee_per_gas_tx2);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("20 block with 0x0 base_fee and same max_priority and max_fee") {
    const intx::uint256 base_fee = 0x0;
    const intx::uint256 max_priority_fee_per_gas = 0x32;
    const intx::uint256 max_fee_per_gas = 0x32;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas + base_fee, max_fee_per_gas);

    clear_blocks_vector();
    fill_blocks_vector(20, kBeneficiary, base_fee, max_priority_fee_per_gas, max_fee_per_gas, max_priority_fee_per_gas, max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("20 block with 0x7 base_fee and different max_priority and max_fee in tnxs") {
    const intx::uint256 base_fee = 0x7;
    const intx::uint256 max_priority_fee_per_gas_tx1 = 0x0;
    const intx::uint256 max_fee_per_gas_tx1 = 0x32;
    const intx::uint256 max_priority_fee_per_gas_tx2 = 0x32;
    const intx::uint256 max_fee_per_gas_tx2 = 0x32;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas_tx1 + base_fee, max_fee_per_gas_tx1);

    clear_blocks_vector();
    fill_blocks_vector(20, kBeneficiary, base_fee, max_priority_fee_per_gas_tx1, max_fee_per_gas_tx1, max_priority_fee_per_gas_tx2, max_fee_per_gas_tx2);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("30 block with 0x0 base_fee and same max_priority and max_fee") {
    const intx::uint256 base_fee = 0x0;
    const intx::uint256 max_priority_fee_per_gas = 0x32;
    const intx::uint256 max_fee_per_gas = 0x32;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas + base_fee, max_fee_per_gas);

    clear_blocks_vector();
    fill_blocks_vector(30, kBeneficiary, base_fee, max_priority_fee_per_gas, max_fee_per_gas, max_priority_fee_per_gas, max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("30 block with 0x7 base_fee and different max_priority and max_fee in tnxs") {
    const intx::uint256 base_fee = 0x7;
    const intx::uint256 max_priority_fee_per_gas_tx1 = 0x0;
    const intx::uint256 max_fee_per_gas_tx1 = 0x32;
    const intx::uint256 max_priority_fee_per_gas_tx2 = 0x32;
    const intx::uint256 max_fee_per_gas_tx2 = 0x32;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas_tx1 + base_fee, max_fee_per_gas_tx1);

    clear_blocks_vector();
    fill_blocks_vector(30, kBeneficiary, base_fee, max_priority_fee_per_gas_tx1, max_fee_per_gas_tx1, max_priority_fee_per_gas_tx2, max_fee_per_gas_tx2);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x0 base_fee and same max_priority and max_fee") {
    const intx::uint256 base_fee = 0x0;
    const intx::uint256 max_priority_fee_per_gas = 0x32;
    const intx::uint256 max_fee_per_gas = 0x32;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas + base_fee, max_fee_per_gas);

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas, max_fee_per_gas, max_priority_fee_per_gas, max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x7 base_fee and different max_priority and max_fee in tnxs") {
    const intx::uint256 base_fee = 0x7;
    const intx::uint256 max_priority_fee_per_gas_tx1 = 0x0;
    const intx::uint256 max_fee_per_gas_tx1 = 0x32;
    const intx::uint256 max_priority_fee_per_gas_tx2 = 0x32;
    const intx::uint256 max_fee_per_gas_tx2 = 0x32;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas_tx1 + base_fee, max_fee_per_gas_tx1);

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas_tx1, max_fee_per_gas_tx1, max_priority_fee_per_gas_tx2, max_fee_per_gas_tx2);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x7 base_fee and max_priority > max_fee") {
    const intx::uint256 base_fee = 0x7;
    const intx::uint256 max_priority_fee_per_gas = 0x40;
    const intx::uint256 max_fee_per_gas = 0x32;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas + base_fee, max_fee_per_gas);

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas, max_fee_per_gas, max_priority_fee_per_gas, max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x7 base_fee and max_priority < max_fee") {
    const intx::uint256 base_fee = 0x7;
    const intx::uint256 max_priority_fee_per_gas = 0x32;
    const intx::uint256 max_fee_per_gas = 0x40;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas + base_fee, max_fee_per_gas);

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas, max_fee_per_gas, max_priority_fee_per_gas, max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x7 base_fee and different max_priority and max_fee in tnxs, beneficiary == tx1 from") {
    const intx::uint256 base_fee = 0x7;
    const intx::uint256 max_priority_fee_per_gas_tx1 = 0x0;
    const intx::uint256 max_fee_per_gas_tx1 = 0x32;
    const intx::uint256 max_priority_fee_per_gas_tx2 = 0x32;
    const intx::uint256 max_fee_per_gas_tx2 = 0x32;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas_tx2 + base_fee, max_fee_per_gas_tx2);

    clear_blocks_vector();
    fill_blocks_vector(60, kFromTnx1, base_fee, max_priority_fee_per_gas_tx1, max_fee_per_gas_tx1, max_priority_fee_per_gas_tx2, max_fee_per_gas_tx2);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x7 base_fee and different max_priority and max_fee in tnxs, beneficiary == tx2 from") {
    const intx::uint256 base_fee = 0x7;
    const intx::uint256 max_priority_fee_per_gas_tx1 = 0x0;
    const intx::uint256 max_fee_per_gas_tx1 = 0x32;
    const intx::uint256 max_priority_fee_per_gas_tx2 = 0x32;
    const intx::uint256 max_fee_per_gas_tx2 = 0x32;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas_tx1 + base_fee, max_fee_per_gas_tx1);

    clear_blocks_vector();
    fill_blocks_vector(60, kFromTnx2, base_fee, max_priority_fee_per_gas_tx1, max_fee_per_gas_tx1, max_priority_fee_per_gas_tx2, max_fee_per_gas_tx2);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x0 base fee and 1 tnx with fee == kDefaultMinPrice") {
    const intx::uint256 base_fee = 0;
    const intx::uint256 max_priority_fee_per_gas_tx1 = 0x32;
    const intx::uint256 max_fee_per_gas_tx1 = 0x32;
    const intx::uint256 max_priority_fee_per_gas_tx2 = kDefaultMinPrice;
    const intx::uint256 max_fee_per_gas_tx2 = kDefaultMinPrice;
    const intx::uint256 expected_price = kDefaultMinPrice;

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas_tx1, max_fee_per_gas_tx1, max_priority_fee_per_gas_tx2, max_fee_per_gas_tx2);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x0 base fee and 1 tnx with fee < kDefaultMinPrice") {
    const intx::uint256 base_fee = 0;
    const intx::uint256 max_priority_fee_per_gas_tx1 = 0x32;
    const intx::uint256 max_fee_per_gas_tx1 = 0x32;
    const intx::uint256 max_priority_fee_per_gas_tx2 = kDefaultMinPrice - 1;
    const intx::uint256 max_fee_per_gas_tx2 = kDefaultMinPrice - 1;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas_tx1 + base_fee, max_fee_per_gas_tx1);

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas_tx1, max_fee_per_gas_tx1, max_priority_fee_per_gas_tx2, max_fee_per_gas_tx2);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x0 base fee with fee == kDefaultMaxPrice") {
    const intx::uint256 base_fee = 0;
    const intx::uint256 max_priority_fee_per_gas = kDefaultMaxPrice;
    const intx::uint256 max_fee_per_gas = kDefaultMaxPrice;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas + base_fee, max_fee_per_gas);

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas, max_fee_per_gas, max_priority_fee_per_gas, max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x07 base fee with fee == kDefaultMaxPrice") {
    const intx::uint256 base_fee = 0x07;
    const intx::uint256 max_priority_fee_per_gas = kDefaultMaxPrice;
    const intx::uint256 max_fee_per_gas = kDefaultMaxPrice;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas + base_fee, max_fee_per_gas);

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas, max_fee_per_gas, max_priority_fee_per_gas, max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x0 base fee with fee > kDefaultMaxPrice") {
    const intx::uint256 base_fee = 0;
    const intx::uint256 max_priority_fee_per_gas = kDefaultMaxPrice + 0x10;
    const intx::uint256 max_fee_per_gas = kDefaultMaxPrice + 0x10;
    const intx::uint256 expected_price = kDefaultMaxPrice;

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas, max_fee_per_gas, max_priority_fee_per_gas, max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x07 base fee with fee > kDefaultMaxPrice") {
    const intx::uint256 base_fee = 0x07;
    const intx::uint256 max_priority_fee_per_gas = kDefaultMaxPrice + 0x10;
    const intx::uint256 max_fee_per_gas = kDefaultMaxPrice + 0x10;
    const intx::uint256 expected_price = kDefaultMaxPrice;

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas, max_fee_per_gas, max_priority_fee_per_gas, max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x0 base fee and 1 tnx with fee > kDefaultMaxPrice") {
    const intx::uint256 base_fee = 0;
    const intx::uint256 max_priority_fee_per_gas_tx1 = kDefaultMaxPrice + 0x10;
    const intx::uint256 max_fee_per_gas_tx1 = kDefaultMaxPrice + 0x10;
    const intx::uint256 max_priority_fee_per_gas_tx2 = 0x32;
    const intx::uint256 max_fee_per_gas_tx2 = 0x32;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas_tx2 + base_fee, max_fee_per_gas_tx2);

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas_tx1, max_fee_per_gas_tx1, max_priority_fee_per_gas_tx2, max_fee_per_gas_tx2);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x7 base fee and 1 tnx with fee > kDefaultMaxPrice") {
    const intx::uint256 base_fee = 0x7;
    const intx::uint256 max_priority_fee_per_gas_tx1 = kDefaultMaxPrice + 0x10;
    const intx::uint256 max_fee_per_gas_tx1 = kDefaultMaxPrice + 0x10;
    const intx::uint256 max_priority_fee_per_gas_tx2 = 0x32;
    const intx::uint256 max_fee_per_gas_tx2 = 0x32;
    const intx::uint256 expected_price = std::min(max_priority_fee_per_gas_tx2 + base_fee, max_fee_per_gas_tx2);

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas_tx1, max_fee_per_gas_tx1, max_priority_fee_per_gas_tx2, max_fee_per_gas_tx2);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x0 base fee with tnxs with increasing max_priority_fee_per_gas and max_fee_per_gas") {
    const intx::uint256 base_fee = 0x0;
    const intx::uint256 max_priority_fee_per_gas = 0x10;
    const intx::uint256 max_fee_per_gas = 0x10;
    const int delta_max_priority_fee_per_gas = 0x9;
    const int delta_max_fee_per_gas = 0x9;
    const intx::uint256 expected_price = 0x019;

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas, delta_max_priority_fee_per_gas, max_fee_per_gas, delta_max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x7 base fee with tnxs with increasing max_priority_fee_per_gas and max_fee_per_gas") {
    const intx::uint256 base_fee = 0x7;
    const intx::uint256 max_priority_fee_per_gas = 0x10;
    const intx::uint256 max_fee_per_gas = 0x10;
    const int delta_max_priority_fee_per_gas = 0x9;
    const int delta_max_fee_per_gas = 0x9;
    const intx::uint256 expected_price = 0x019;

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas, delta_max_priority_fee_per_gas, max_fee_per_gas, delta_max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x0 base fee with tnxs with decreasing max_priority_fee_per_gas and max_fee_per_gas") {
    const intx::uint256 base_fee = 0x0;
    const intx::uint256 max_priority_fee_per_gas = 0x300;
    const intx::uint256 max_fee_per_gas = 0x300;
    const int delta_max_priority_fee_per_gas = -0x9;
    const int delta_max_fee_per_gas = -0x9;
    const intx::uint256 expected_price = 0x2f7;

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas, delta_max_priority_fee_per_gas, max_fee_per_gas, delta_max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x7 base fee with tnxs with decreasing max_priority_fee_per_gas and max_fee_per_gas") {
    const intx::uint256 base_fee = 0x7;
    const intx::uint256 max_priority_fee_per_gas = 0x200;
    const intx::uint256 max_fee_per_gas = 0x200;
    const int delta_max_priority_fee_per_gas = -0x9;
    const int delta_max_fee_per_gas = -0x9;
    const intx::uint256 expected_price = 0x1f7;

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas, delta_max_priority_fee_per_gas, max_fee_per_gas, delta_max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}

TEST_CASE("60 block with 0x0 base fee with tnxs with increasing max_priority_fee_per_gas and max_fee_per_gas over threshold") {
    const intx::uint256 base_fee = 0x0;
    const intx::uint256 max_priority_fee_per_gas = kDefaultMaxPrice - intx::uint256{0x200};
    const intx::uint256 max_fee_per_gas = kDefaultMaxPrice - intx::uint256{0x200};
    const int delta_max_priority_fee_per_gas = 0x9;
    const int delta_max_fee_per_gas = 0x9;
    const intx::uint256 expected_price = 0x746a528609;

    clear_blocks_vector();
    fill_blocks_vector(60, kBeneficiary, base_fee, max_priority_fee_per_gas, delta_max_priority_fee_per_gas, max_fee_per_gas, delta_max_fee_per_gas);

    GasPriceOracle gas_price_oracle{ block_provider};
    auto result = asio::co_spawn(pool, gas_price_oracle.suggested_price(1), asio::use_future);
    const intx::uint256 &price = result.get();

    CHECK(price == expected_price);
}
} // namespace silkrpc

