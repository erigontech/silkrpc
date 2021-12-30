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

#include "evm_executor.hpp"

#include <optional>
#include <string>
#include <vector>

#include <asio/co_spawn.hpp>
#include <asio/thread_pool.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch.hpp>
#include <evmc/evmc.hpp>
#include <intx/intx.hpp>

namespace silkrpc {

using Catch::Matchers::Message;
using evmc::literals::operator""_address;

TEST_CASE("EVMexecutor") {
    class StubDatabase : public core::rawdb::DatabaseReader {
        asio::awaitable<KeyValue> get(const std::string& table, const silkworm::ByteView& key) const override {
            co_return KeyValue{};
        }
        asio::awaitable<silkworm::Bytes> get_one(const std::string& table, const silkworm::ByteView& key) const override {
            co_return silkworm::Bytes{};
        }
        asio::awaitable<std::optional<silkworm::Bytes>> get_both_range(const std::string& table, const silkworm::ByteView& key, const silkworm::ByteView& subkey) const override {
            co_return silkworm::Bytes{};
        }
        asio::awaitable<void> walk(const std::string& table, const silkworm::ByteView& start_key, uint32_t fixed_bits, core::rawdb::Walker w) const override {
            co_return;
        }
        asio::awaitable<void> for_prefix(const std::string& table, const silkworm::ByteView& prefix, core::rawdb::Walker w) const override {
            co_return;
        }
    };

    SECTION("failed if gas_limit < intrisicgas") {
        StubDatabase tx_database;
        const uint64_t chain_id = 5;
        const auto chain_config_ptr = silkworm::lookup_chain_config(chain_id);

        ChannelFactory my_channel = []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); };
        ContextPool my_pool{1, my_channel};
        asio::thread_pool workers{1};
        auto pool_thread = std::thread([&]() { my_pool.run(); });

        const auto block_number = 10000;
        silkworm::Transaction txn{};
        txn.from = 0xa872626373628737383927236382161739290870_address;
        silkworm::Block block{};
        block.header.number = block_number;

        EVMExecutor executor{my_pool.get_context(), tx_database, *chain_config_ptr, workers, block_number};
        auto execution_result = asio::co_spawn(my_pool.get_io_context().get_executor(), executor.call(block, txn), asio::use_future);
        auto result = execution_result.get();
        my_pool.stop();
        pool_thread.join();
        CHECK(result.error_code == 1000);
        CHECK(result.pre_check_error.value() == "intrinsic gas too low: have 0 want 53000");
    }

    SECTION("failed if base_fee_per_gas > max_fee_per_gas ") {
        StubDatabase tx_database;
        const uint64_t chain_id = 5;
        const auto chain_config_ptr = silkworm::lookup_chain_config(chain_id);

        ChannelFactory my_channel = []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); };
        ContextPool my_pool{1, my_channel};
        asio::thread_pool workers{1};
        auto pool_thread = std::thread([&]() { my_pool.run(); });

        const auto block_number = 6000000;
        silkworm::Block block{};
        block.header.base_fee_per_gas = 0x7;
        block.header.number = block_number;
        silkworm::Transaction txn{};
        txn.max_fee_per_gas = 0x2;
        txn.from = 0xa872626373628737383927236382161739290870_address;

        EVMExecutor executor{my_pool.get_context(), tx_database, *chain_config_ptr, workers, block_number};
        auto execution_result = asio::co_spawn(my_pool.get_io_context().get_executor(), executor.call(block, txn), asio::use_future);
        auto result = execution_result.get();
        my_pool.stop();
        pool_thread.join();
        CHECK(result.error_code == 1000);
        CHECK(result.pre_check_error.value() == "fee cap less than block base fee: address 0xa872626373628737383927236382161739290870, gasFeeCap: 2 baseFee: 7");
    }

    SECTION("failed if  max_priority_fee_per_gas > max_fee_per_gas ") {
        StubDatabase tx_database;
        const uint64_t chain_id = 5;
        const auto chain_config_ptr = silkworm::lookup_chain_config(chain_id);

        ChannelFactory my_channel = []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); };
        ContextPool my_pool{1, my_channel};
        asio::thread_pool workers{1};
        auto pool_thread = std::thread([&]() { my_pool.run(); });

        const auto block_number = 6000000;
        silkworm::Block block{};
        block.header.base_fee_per_gas = 0x1;
        block.header.number = block_number;
        silkworm::Transaction txn{};
        txn.max_fee_per_gas = 0x2;
        txn.from = 0xa872626373628737383927236382161739290870_address;
        txn.max_priority_fee_per_gas = 0x18;

        EVMExecutor executor{my_pool.get_context(), tx_database, *chain_config_ptr, workers, block_number};
        auto execution_result = asio::co_spawn(my_pool.get_io_context().get_executor(), executor.call(block, txn), asio::use_future);
        auto result = execution_result.get();
        my_pool.stop();
        pool_thread.join();
        CHECK(result.error_code == 1000);
        CHECK(result.pre_check_error.value() == "tip higher than fee cap: address 0xa872626373628737383927236382161739290870, tip: 24 gasFeeCap: 2");
    }

    SECTION("failed if transaction cost greater user amount") {
        StubDatabase tx_database;
        const uint64_t chain_id = 5;
        const auto chain_config_ptr = silkworm::lookup_chain_config(chain_id);

        ChannelFactory my_channel = []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); };
        ContextPool my_pool{1, my_channel};
        asio::thread_pool workers{1};
        auto pool_thread = std::thread([&]() { my_pool.run(); });

        const auto block_number = 6000000;
        silkworm::Block block{};
        block.header.base_fee_per_gas = 0x1;
        block.header.number = block_number;
        silkworm::Transaction txn{};
        txn.max_fee_per_gas = 0x2;
        txn.gas_limit = 60000;
        txn.from = 0xa872626373628737383927236382161739290870_address;

        EVMExecutor executor{my_pool.get_context(), tx_database, *chain_config_ptr, workers, block_number};
        auto execution_result = asio::co_spawn(my_pool.get_io_context().get_executor(), executor.call(block, txn), asio::use_future);
        auto result = execution_result.get();
        my_pool.stop();
        pool_thread.join();
        CHECK(result.error_code == 1000);
        CHECK(result.pre_check_error.value() == "insufficient funds for gas * price + value: address 0xa872626373628737383927236382161739290870 have 0 want 60000");
    }

    SECTION("call returns SUCCESS") {
        StubDatabase tx_database;
        const uint64_t chain_id = 5;
        const auto chain_config_ptr = silkworm::lookup_chain_config(chain_id);

        ChannelFactory my_channel = []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); };
        ContextPool my_pool{1, my_channel};
        asio::thread_pool workers{1};
        auto pool_thread = std::thread([&]() { my_pool.run(); });

        const auto block_number = 6000000;
        silkworm::Block block{};
        block.header.number = block_number;
        silkworm::Transaction txn{};
        txn.gas_limit = 60000;
        txn.from = 0xa872626373628737383927236382161739290870_address;

        EVMExecutor executor{my_pool.get_context(), tx_database, *chain_config_ptr, workers, block_number};
        auto execution_result = asio::co_spawn(my_pool.get_io_context().get_executor(), executor.call(block, txn), asio::use_future);
        auto result = execution_result.get();
        my_pool.stop();
        pool_thread.join();
        CHECK(result.error_code == 0);
    }
}

} // namespace silkrpc
