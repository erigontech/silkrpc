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
#include <catch2/catch.hpp>
#include <gmock/gmock.h>
#include <asio/awaitable.hpp>
#include <asio/use_future.hpp>
#include <asio/co_spawn.hpp>
#include <utility>
#include <string>

namespace silkrpc::commands {

using Catch::Matchers::Message;

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
    explicit EngineRpcApiTest(std::unique_ptr<ethbackend::BackEnd>& backend): EngineRpcApi(backend) {}

    using EngineRpcApi::handle_engine_get_payload_v1;
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
    // Initialise components
    EngineRpcApiTest rpc(backend_ptr);

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
    EngineRpcApiTest rpc(backend_ptr);

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

} // namespace silkrpc::commands
