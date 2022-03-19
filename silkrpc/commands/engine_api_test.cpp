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

#include "engine_api.hpp"

#include <silkrpc/json/types.hpp>
#include <silkrpc/http/methods.hpp>
#include <silkrpc/ethdb/transaction_database.hpp>
#include <silkrpc/core/rawdb/chain.hpp>
#include <catch2/catch.hpp>
#include <gmock/gmock.h>
#include <asio/awaitable.hpp>
#include <asio/use_future.hpp>
#include <asio/co_spawn.hpp>
#include <utility>
#include <string>

namespace silkrpc::commands {

using Catch::Matchers::Message;
using evmc::literals::operator""_bytes32;

namespace {
class BackEndMock : public ethbackend::BackEnd {
public:
    MOCK_METHOD((asio::awaitable<evmc::address>), etherbase, ());
    MOCK_METHOD((asio::awaitable<uint64_t>), protocol_version, ());
    MOCK_METHOD((asio::awaitable<uint64_t>), net_version, ());
    MOCK_METHOD((asio::awaitable<std::string>), client_version, ());
    MOCK_METHOD((asio::awaitable<uint64_t>), net_peer_count, ());
    MOCK_METHOD((asio::awaitable<ExecutionPayload>), engine_get_payload_v1, (uint64_t payload_id));
    MOCK_METHOD((asio::awaitable<PayloadStatus>), engine_new_payload_v1, (ExecutionPayload payload));
};

} // namespace

class EngineRpcApiTest : public EngineRpcApi{
public:
    explicit EngineRpcApiTest(std::unique_ptr<ethdb::Database>& database, std::unique_ptr<ethbackend::BackEnd>& backend): EngineRpcApi(database, backend) {}

    using EngineRpcApi::handle_engine_get_payload_v1;
    using EngineRpcApi::handle_engine_new_payload_v1;
    using EngineRpcApi::handle_engine_exchange_transition_configuration_v1;
};

using testing::InvokeWithoutArgs;

TEST_CASE("handle_engine_get_payload_v1 succeeds if request is expected payload", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    BackEndMock backend;
    EXPECT_CALL(backend, engine_get_payload_v1(1)).WillOnce(InvokeWithoutArgs(
        []() -> asio::awaitable<ExecutionPayload> {
            co_return ExecutionPayload{1};
        }
    ));

    std::unique_ptr<ethbackend::BackEnd> backend_ptr(&backend);

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_getPayloadV1",
        "params":["0x0000000000000001"]
    })"_json;
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto context_pool_thread = std::thread([&]() { cp.run(); });
    std::unique_ptr<ethdb::Database> database;
    // Initialise components
    EngineRpcApiTest rpc(database, backend_ptr);

    // spawn routine
    auto result{asio::co_spawn(cp.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_get_payload_v1(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();

    CHECK(reply == ExecutionPayload{1});

    cp.stop();
    context_pool_thread.join();
    backend_ptr.release();
}

TEST_CASE("handle_engine_get_payload_v1 fails with invalid amount of params", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_getPayloadV1",
        "params":[]
    })"_json;
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto context_pool_thread = std::thread([&]() { cp.run(); });
    // Initialise components
    std::unique_ptr<ethbackend::BackEnd> backend_ptr(new BackEndMock);
    std::unique_ptr<ethdb::Database> database;
    EngineRpcApiTest rpc(database, backend_ptr);

    // spawn routine
    auto result{asio::co_spawn(cp.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_get_payload_v1(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();

    CHECK(reply ==  R"({
        "error":{
            "code":100,
            "message":"invalid engine_getPayloadV1 params: []"
        },
        "id":1,
        "jsonrpc":"2.0" 
    })"_json);

    cp.stop();
    context_pool_thread.join();
}

TEST_CASE("handle_engine_new_payload_v1 succeeds if request is expected payload status", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    BackEndMock backend;
    EXPECT_CALL(backend, engine_new_payload_v1(testing::_)).WillOnce(InvokeWithoutArgs(
        []() -> asio::awaitable<PayloadStatus> {
            co_return PayloadStatus{
                .status = "INVALID",
                .latest_valid_hash = 0x0000000000000000000000000000000000000000000000000000000000000040_bytes32,
                .validation_error = "some error"
            };
        }
    ));

    std::unique_ptr<ethbackend::BackEnd> backend_ptr(&backend);

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_newPayloadV1",
        "params":[{
            "parentHash":"0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a",
            "suggestedFeeRecipient":"0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b",
            "stateRoot":"0xca3149fa9e37db08d1cd49c9061db1002ef1cd58db2210f2115c8c989b2bdf45",
            "receiptsRoot":"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
            "logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "prevRandao":"0x0000000000000000000000000000000000000000000000000000000000000001",
            "blockNumber":"0x1",
            "gasLimit":"0x1c9c380",
            "gasUsed":"0x0",
            "timestamp":"0x5",
            "extraData":"0x",
            "baseFeePerGas":"0x7",
            "blockHash":"0x3559e851470f6e7bbed1db474980683e8c315bfce99b2a6ef47c057c04de7858",
            "transactions":["0xf92ebdeab45d368f6354e8c5a8ac586c"]
        }]
    })"_json;
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto context_pool_thread = std::thread([&]() { cp.run(); });
    std::unique_ptr<ethdb::Database> database;
    // Initialise components
    EngineRpcApiTest rpc(database, backend_ptr);

    // spawn routine
    auto result{asio::co_spawn(cp.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_new_payload_v1(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();

    CHECK(reply == PayloadStatus{
            .status = "INVALID",
            .latest_valid_hash = 0x0000000000000000000000000000000000000000000000000000000000000040_bytes32,
            .validation_error = "some error"
        });

    cp.stop();
    context_pool_thread.join();
    backend_ptr.release();
}

TEST_CASE("handle_engine_new_payload_v1 fails with invalid amount of params", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_newPayloadV1",
        "params":[]
    })"_json;
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto context_pool_thread = std::thread([&]() { cp.run(); });
    // Initialise components
    std::unique_ptr<ethbackend::BackEnd> backend_ptr(new BackEndMock);
    std::unique_ptr<ethdb::Database> database;
    EngineRpcApiTest rpc(database, backend_ptr);

    // spawn routine
    auto result{asio::co_spawn(cp.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_new_payload_v1(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();

    CHECK(reply ==  R"({
        "error":{
            "code":100,
            "message":"invalid engine_newPayloadV1 params: []"
        },
        "id":1,
        "jsonrpc":"2.0" 
    })"_json);

    cp.stop();
    context_pool_thread.join();
}

TEST_CASE("handle_engine_transition_configuration_v1 succeeds if EL configurations has the same request configuration", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    silkrpc::ContextPool context_pool{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto& context = context_pool.get_context();
    auto& database_ptr = context.database;
    auto result_tx{asio::co_spawn(context_pool.get_io_context(), [&database_ptr]() { return database_ptr->begin(); }, asio::use_future)};
    auto tx = result_tx.get();
    ethdb::TransactionDatabase tx_database(*tx);

    auto chain_config_result{asio::co_spawn(context_pool.get_io_context(), [&tx_database]() { return core::rawdb::read_chain_config(tx_database); }, asio::use_future)};
    const auto& chain_config = chain_config_result.get();

    auto config = silkworm::ChainConfig::from_json(chain_config.config);
    config->*terminal_total_difficulty = intx::from_string<intx::uint256>("1000000");
    config->*terminal_block_hash = 0x3559e851470f6e7bbed1db474980683e8c315bfce99b2a6ef47c057c04de7858_bytes32;
    config->*terminal_block_number = 0;

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_transitionConfigurationV1",
        "params": [{
            "terminalTotalDifficulty":"0xf4240",
            "terminalBlockHash":"0x3559e851470f6e7bbed1db474980683e8c315bfce99b2a6ef47c057c04de7858",
            "terminalBlockNumber":"0x0"
        }]
    })"_json;
    std::unique_ptr<ethbackend::BackEnd> backend_ptr;
    EngineRpcApiTest rpc(database_ptr, backend_ptr);
    auto result{asio::co_spawn(context_pool.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_exchange_transition_configuration_v1(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();
    CHECK(reply == TransitionConfiguration{
        .terminal_total_difficulty = intx::from_string<intx::uint256>("1000000"),
        .terminal_block_hash = 0x3559e851470f6e7bbed1db474980683e8c315bfce99b2a6ef47c057c04de7858_bytes32,
        .terminal_block_number = 0
    });
    context_pool.stop();
}

TEST_CASE("handle_engine_transition_configuration_v1 fails if EL configurations has different TTD", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    silkrpc::ContextPool context_pool{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto& context = context_pool.get_context();
    auto& database_ptr = context.database;
    auto result_tx{asio::co_spawn(context_pool.get_io_context(), [&database_ptr]() { return database_ptr->begin(); }, asio::use_future)};
    auto tx = result_tx.get();
    ethdb::TransactionDatabase tx_database(*tx);

    auto chain_config_result{asio::co_spawn(context_pool.get_io_context(), [&tx_database]() { return core::rawdb::read_chain_config(tx_database); }, asio::use_future)};
    const auto& chain_config = chain_config_result.get();

    auto config = silkworm::ChainConfig::from_json(chain_config.config);
    config->*terminal_total_difficulty = intx::from_string<intx::uint256>("1000000");
    config->*terminal_block_hash = 0x3559e851470f6e7bbed1db474980683e8c315bfce99b2a6ef47c057c04de7858_bytes32;
    config->*terminal_block_number = 0;

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_transitionConfigurationV1",
        "params": [{
            "terminalTotalDifficulty":"0xf4240", 
            "terminalBlockHash":"0x3559e851470f6e7bbed1db474980683e8c315bfce99b2a6ef47c057c04de7858", 
            "terminalBlockNumber":"0x0"
        }]
    })"_json;

    std::unique_ptr<ethbackend::BackEnd> backend_ptr;
    EngineRpcApiTest rpc(database_ptr, backend_ptr);
    auto result{asio::co_spawn(context_pool.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_exchange_transition_configuration_v1(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();

    CHECK(reply != R"({
        "error":{
            "code":100,
            "message":"incorrect terminal total difficulty"
        },
        "id":1,
        "jsonrpc":"2.0" 
    })"_json);
    context_pool.stop();
}

TEST_CASE("handle_engine_transition_configuration_v1 fails if EL configurations has different terminal block hash", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    silkrpc::ContextPool context_pool{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto& context = context_pool.get_context();
    auto& database_ptr = context.database;
    auto result_tx{asio::co_spawn(context_pool.get_io_context(), [&database_ptr]() { return database_ptr->begin(); }, asio::use_future)};
    auto tx = result_tx.get();
    ethdb::TransactionDatabase tx_database(*tx);

    auto chain_config_result{asio::co_spawn(context_pool.get_io_context(), [&tx_database]() { return core::rawdb::read_chain_config(tx_database); }, asio::use_future)};
    const auto& chain_config = chain_config_result.get();

    auto config = silkworm::ChainConfig::from_json(chain_config.config);
    config->*terminal_total_difficulty = intx::from_string<intx::uint256>("1000000");
    config->*terminal_block_hash = 0x3559e851470f6e7bbed1db474980683e8c315bfce99b2a6ef47c057c04de0000_bytes32;
    config->*terminal_block_number = 0;

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_transitionConfigurationV1",
        "params": [{
            "terminalTotalDifficulty":"0xf4240", 
            "terminalBlockHash":"0x3559e851470f6e7bbed1db474980683e8c315bfce99b2a6ef47c057c04de7858", 
            "terminalBlockNumber":"0x0"
        }]
    })"_json;

    std::unique_ptr<ethbackend::BackEnd> backend_ptr;
    EngineRpcApiTest rpc(database_ptr, backend_ptr);
    auto result{asio::co_spawn(context_pool.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_exchange_transition_configuration_v1(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();

    CHECK(reply != R"({
        "error":{
            "code":100,
            "message":"incorrect terminal block hash"
        },
        "id":1,
        "jsonrpc":"2.0" 
    })"_json);
    context_pool.stop();
}

TEST_CASE("handle_engine_transition_configuration_v1 fails if EL configurations has no TTD", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    silkrpc::ContextPool context_pool{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto& context = context_pool.get_context();
    auto& database_ptr = context.database;
    auto result_tx{asio::co_spawn(context_pool.get_io_context(), [&database_ptr]() { return database_ptr->begin(); }, asio::use_future)};
    auto tx = result_tx.get();
    ethdb::TransactionDatabase tx_database(*tx);

    auto chain_config_result{asio::co_spawn(context_pool.get_io_context(), [&tx_database]() { return core::rawdb::read_chain_config(tx_database); }, asio::use_future)};
    const auto& chain_config = chain_config_result.get();

    auto config = silkworm::ChainConfig::from_json(chain_config.config);
    config->*terminal_total_difficulty = std::nullopt;
    config->*terminal_block_hash = 0x3559e851470f6e7bbed1db474980683e8c315bfce99b2a6ef47c057c04de0000_bytes32;
    config->*terminal_block_number = 0;
    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_transitionConfigurationV1",
        "params": [{
            "terminalTotalDifficulty":"0xf4240", 
            "terminalBlockHash":"0x3559e851470f6e7bbed1db474980683e8c315bfce99b2a6ef47c057c04de7858", 
            "terminalBlockNumber":"0x0"
        }]
    })"_json;

    std::unique_ptr<ethbackend::BackEnd> backend_ptr;
    EngineRpcApiTest rpc(database_ptr, backend_ptr);
    auto result{asio::co_spawn(context_pool.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_exchange_transition_configuration_v1(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();

    CHECK(reply != R"({
        "error":{
            "code":100,
            "message":"execution layer does not have terminal total difficulty"
        },
        "id":1,
        "jsonrpc":"2.0" 
    })"_json);
    context_pool.stop();
}

TEST_CASE("handle_engine_transition_configuration_v1 fails if request has wrong params", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);
    silkrpc::ContextPool context_pool{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto& context = context_pool.get_context();
    auto& database_ptr = context.database;
    auto result_tx{asio::co_spawn(context_pool.get_io_context(), [&database_ptr]() { return database_ptr->begin(); }, asio::use_future)};
    auto tx = result_tx.get();
    ethdb::TransactionDatabase tx_database(*tx);

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_transitionConfigurationV1",
        "params": []
    })"_json;

    std::unique_ptr<ethbackend::BackEnd> backend_ptr;
    EngineRpcApiTest rpc(database_ptr, backend_ptr);
    auto result{asio::co_spawn(context_pool.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_exchange_transition_configuration_v1(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();

    CHECK(reply != R"({
        "error":{
            "code":100,
            "message":"invalid engine_transitionConfigurationV1 params: []"
        },
        "id":1,
        "jsonrpc":"2.0" 
    })"_json);
    context_pool.stop();
}
} // namespace silkrpc::commands
