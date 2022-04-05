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
#include <silkrpc/ethdb/tables.hpp>
#include <silkworm/common/base.hpp>
#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>
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

class MockDatabase : public ethdb::Database {
public:
    MOCK_METHOD((asio::awaitable<std::unique_ptr<ethdb::Transaction>>), begin, ());
};


class MockTransaction : public ethdb::Transaction {
public:
    MOCK_METHOD((uint64_t), tx_id, (), (const));
    MOCK_METHOD((asio::awaitable<void>), open, ());
    MOCK_METHOD((asio::awaitable<std::shared_ptr<ethdb::Cursor>>), cursor, (const std::string& table));
    MOCK_METHOD((asio::awaitable<std::shared_ptr<ethdb::CursorDupSort>>), cursor_dup_sort, (const std::string& table));
    MOCK_METHOD((asio::awaitable<void>), close, ());
};

class MockCursor : public ethdb::Cursor {
public:
    MOCK_METHOD((uint32_t), cursor_id, (), (const));
    MOCK_METHOD((asio::awaitable<void>), open_cursor, (const std::string& table_name));
    MOCK_METHOD((asio::awaitable<KeyValue>), seek, (silkworm::ByteView key));
    MOCK_METHOD((asio::awaitable<KeyValue>), seek_exact, (silkworm::ByteView key));
    MOCK_METHOD((asio::awaitable<KeyValue>), next, ());
    MOCK_METHOD((asio::awaitable<void>), close_cursor, ());
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
    silkworm::Bytes chain_config_bytes{*silkworm::from_hex(
        "7b22636861696e4964223a313333373330322c22686f6d657374656164426c6f6"
         "36b223a302c22656970313530426c6f636b223a302c22656970313535426c6f63"
         "6b223a302c2262797a616e7469756d426c6f636b223a302c22636f6e7374616e7"
         "4696e6f706c65426c6f636b223a302c2270657465727362757267426c6f636b22"
         "3a302c22697374616e62756c426c6f636b223a302c226265726c696e426c6f636"
         "b223a302c226c6f6e646f6e426c6f636b223a302c227465726d696e616c546f74"
         "616c446966666963756c7479223a2231303030303030222c227465726d696e616"
         "c426c6f636b4e756d626572223a302c227465726d696e616c426c6f636b486173"
         "68223a22307833353539653835313437306636653762626564316462343734393"
         "83036383365386333313562666365393962326136656634376330353763303464"
         "6537383538227d")};
    silkworm::Bytes key(8, '\0');
    silkworm::ByteView block_key = key;

    silkrpc::ContextPool context_pool{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto context_pool_thread = std::thread([&]() { context_pool.run(); });

    MockDatabase mock_database;
    MockTransaction* mock_transaction = new MockTransaction();
    MockCursor* mock_cursor_1 = new MockCursor();
    MockCursor* mock_cursor_2 = new MockCursor();
    std::unique_ptr<ethdb::Database> database_ptr{&mock_database};


    EXPECT_CALL(mock_database, begin()).WillOnce(InvokeWithoutArgs(
        [&]() -> asio::awaitable<std::unique_ptr<ethdb::Transaction>> {
            co_return std::unique_ptr<ethdb::Transaction>{mock_transaction};
        }
    ));

    EXPECT_CALL(*mock_transaction, cursor(testing::_)).WillRepeatedly(InvokeWithoutArgs(
        [&]() -> asio::awaitable<std::shared_ptr<ethdb::Cursor>> {
            co_return std::shared_ptr<ethdb::Cursor>{mock_cursor_1};
        }
    ));

    EXPECT_CALL(*mock_cursor_1, seek_exact(testing::_)).WillOnce(InvokeWithoutArgs(
        [&]() -> asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, key};
        }
    ));

    EXPECT_CALL(*mock_transaction, cursor(db::table::kConfig)).WillOnce(InvokeWithoutArgs(
        [&]() -> asio::awaitable<std::shared_ptr<ethdb::Cursor>> {
            co_return std::shared_ptr<ethdb::Cursor>{mock_cursor_2};
        }
    ));


    EXPECT_CALL(*mock_cursor_2, seek(testing::_)).WillOnce(InvokeWithoutArgs(
        [&]() -> asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, chain_config_bytes};
        }
    ));

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
    CHECK((reply == TransitionConfiguration{
        .terminal_total_difficulty = intx::from_string<intx::uint256>("1000000"),
        .terminal_block_hash = 0x3559e851470f6e7bbed1db474980683e8c315bfce99b2a6ef47c057c04de7858_bytes32,
        .terminal_block_number = 0
    }));
    context_pool.stop();
    context_pool_thread.join();
}
} // namespace silkrpc::commands
