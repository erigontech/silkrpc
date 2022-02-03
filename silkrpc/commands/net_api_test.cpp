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

#include "net_api.hpp"

#include <silkrpc/http/methods.hpp>
#include <silkrpc/ethbackend/test_backend.hpp>
#include <silkrpc/json/types.hpp>
#include <catch2/catch.hpp>
#include <asio/use_future.hpp>
#include <asio/co_spawn.hpp>

namespace silkrpc::commands {

using Catch::Matchers::Message;

class NetRpcApiTest : NetRpcApi {
public:
    explicit NetRpcApiTest(std::unique_ptr<ethbackend::BackEndInterface>& backend): NetRpcApi(backend) {}

    using NetRpcApi::handle_net_peer_count;
    using NetRpcApi::handle_net_version;
};

TEST_CASE("handle_net_peer_count succeeds if request is expected peer count", "[silkrpc][net_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    std::unique_ptr<ethbackend::BackEndInterface> backend(new ethbackend::TestBackEnd());
    NetRpcApiTest rpc(backend);
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto context_pool_thread = std::thread([&]() { cp.run(); });

    // spawn routine
    nlohmann::json reply;
    nlohmann::json request(R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"method",
        "params":[]
    })"_json);
    request["method"] = http::method::k_net_peerCount;
    auto result{asio::co_spawn(cp.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_net_peer_count(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();
    CHECK(reply == R"({
        "id":1,
        "jsonrpc":"2.0",
        "result":"0x5"
    })"_json);
    // Stop context pool
    cp.stop();
    context_pool_thread.join();
}

TEST_CASE("handle_net_version succeeds if request is expected peer count", "[silkrpc][net_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    std::unique_ptr<ethbackend::BackEndInterface> backend(new ethbackend::TestBackEnd());
    NetRpcApiTest rpc(backend);
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto context_pool_thread = std::thread([&]() { cp.run(); });

    // spawn routine
    nlohmann::json reply;
    nlohmann::json request(R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"method",
        "params":[]
    })"_json);
    request["method"] = http::method::k_net_version;
    auto result{asio::co_spawn(cp.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_net_version(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();

    CHECK(reply == R"({
        "id":1,
        "jsonrpc":"2.0",
        "result":"2"
    })"_json);
    // Stop context pool
    cp.stop();
    context_pool_thread.join();
}

using Catch::Matchers::Message;

} // namespace silkrpc::commands

