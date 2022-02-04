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

#include "web3_api.hpp"

#include <silkrpc/http/methods.hpp>
#include <silkrpc/ethbackend/backend_test.hpp>
#include <silkrpc/json/types.hpp>
#include <catch2/catch.hpp>
#include <asio/use_future.hpp>
#include <asio/co_spawn.hpp>

namespace silkrpc::commands {

using Catch::Matchers::Message;

class Web3RpcApiTest : Web3RpcApi {
public:
    explicit Web3RpcApiTest(Context& context): Web3RpcApi(context) {}

    using Web3RpcApi::handle_web3_client_version;
    using Web3RpcApi::handle_web3_sha3;
};

TEST_CASE("handle_web3_client_version succeeds if request is expected version", "[silkrpc][web3_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    Context context;
    context.backend = std::unique_ptr<ethbackend::BackEndInterface>(new ethbackend::TestBackEnd());
    Web3RpcApiTest rpc(context);
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
    request["method"] = http::method::k_web3_clientVersion;
    auto result{asio::co_spawn(cp.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_web3_client_version(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();
    CHECK(reply == R"({
        "id":1,
        "jsonrpc":"2.0",
        "result":"6.0.0"
    })"_json);
    // Stop context pool
    cp.stop();
    context_pool_thread.join();
}

TEST_CASE("handle_web3_sha3 succeeds if request is sha3 of input", "[silkrpc][web3_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    Context context;
    context.backend = std::unique_ptr<ethbackend::BackEndInterface>(new ethbackend::TestBackEnd());
    Web3RpcApiTest rpc(context);
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto context_pool_thread = std::thread([&]() { cp.run(); });

    // spawn routine
    nlohmann::json reply;
    nlohmann::json request(R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"method",
        "params":["0x5"]
    })"_json);
    request["method"] = http::method::k_web3_sha3;
    auto result{asio::co_spawn(cp.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_web3_sha3(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();
    CHECK(reply == R"({
        "id":1,
        "jsonrpc":"2.0",
        "result":"0xdbb8d0f4c497851a5043c6363657698cb1387682cac2f786c731f8936109d795"
    })"_json);
    // Stop context pool
    cp.stop();
    context_pool_thread.join();
}

TEST_CASE("handle_web3_sha3 fails with not enough parameters", "[silkrpc][web3_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    Context context;
    context.backend = std::unique_ptr<ethbackend::BackEndInterface>(new ethbackend::TestBackEnd());
    Web3RpcApiTest rpc(context);
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
    request["method"] = http::method::k_web3_sha3;
    auto result{asio::co_spawn(cp.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_web3_sha3(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();
    CHECK(reply == R"({
        "error":{
            "code":100,
            "message":"invalid web3_sha3 params: []"
        },
        "id":1,
        "jsonrpc":"2.0"
    })"_json);
    // Stop context pool
    cp.stop();
    context_pool_thread.join();
}

TEST_CASE("handle_web3_sha3 fails with not non-hex parameter", "[silkrpc][web3_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    Context context;
    context.backend = std::unique_ptr<ethbackend::BackEndInterface>(new ethbackend::TestBackEnd());
    Web3RpcApiTest rpc(context);
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto context_pool_thread = std::thread([&]() { cp.run(); });

    // spawn routine
    nlohmann::json reply;
    nlohmann::json request(R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"method",
        "params":["buongiorno"]
    })"_json);
    request["method"] = http::method::k_web3_sha3;
    auto result{asio::co_spawn(cp.get_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_web3_sha3(
            request,
            reply
        );
    }, asio::use_future)};
    result.get();
    CHECK(reply == R"({
        "error":{
            "code":100,
            "message":"invalid input: buongiorno"
        },
        "id":1,
        "jsonrpc":"2.0"
    })"_json);
    // Stop context pool
    cp.stop();
    context_pool_thread.join();
}

using Catch::Matchers::Message;

} // namespace silkrpc::commands
