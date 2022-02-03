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

#include <silkrpc/ethbackend/backend_test.hpp>
#include <silkrpc/json/types.hpp>
#include <silkrpc/http/methods.hpp>
#include <catch2/catch.hpp>
#include <asio/use_future.hpp>
#include <asio/co_spawn.hpp>

namespace silkrpc::commands {

using Catch::Matchers::Message;

class EngineRpcApiTest : EngineRpcApi{
public:
    explicit EngineRpcApiTest(std::unique_ptr<ethbackend::BackEndInterface>& backend): EngineRpcApi(backend) {}

    using EngineRpcApi::handle_engine_get_payload_v1;
};

TEST_CASE("handle_engine_get_payload_v1 succeeds if request is expected payload", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto context_pool_thread = std::thread([&]() { cp.run(); });
    asio::thread_pool workers{1};
    // Initialise components
    std::unique_ptr<ethbackend::BackEndInterface> backend(new ethbackend::TestBackEnd());
    EngineRpcApiTest rpc(backend);

    // spawn routine
    nlohmann::json reply;
    nlohmann::json request(R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"method",
        "params":["0x0000000000000001"]
    })"_json);
    request["method"] = http::method::k_engine_getPayloadV1;

    auto result{asio::co_spawn(cp.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_get_payload_v1(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();
    ExecutionPayload response_payload = reply;
    CHECK(response_payload.number == 1);
    // Stop context pool
    cp.stop();
    context_pool_thread.join();
}

TEST_CASE("handle_engine_get_payload_v1 fails with invalid amount of params", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto context_pool_thread = std::thread([&]() { cp.run(); });
    asio::thread_pool workers{1};
    // Initialise components
    std::unique_ptr<ethbackend::BackEndInterface> backend(new ethbackend::TestBackEnd());
    EngineRpcApiTest rpc(backend);

    // spawn routine
    nlohmann::json reply;
    nlohmann::json request(R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"method",
        "params":[]
    })"_json);
    request["method"] = http::method::k_engine_getPayloadV1;

    auto result{asio::co_spawn(cp.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_get_payload_v1(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();

    CHECK(reply == R"({
        "error":{
            "code":100,
            "message":"invalid engine_getPayloadV1 params: []"
        },
        "id":1,
        "jsonrpc":"2.0"
    })"_json);
    // Stop context pool
    cp.stop();
    context_pool_thread.join();
}

} // namespace silkrpc::commands
