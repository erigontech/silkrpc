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

#include "erigon_api.hpp"

#include <stdexcept>
#include <thread>

#include <asio/co_spawn.hpp>
#include <asio/io_context.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch.hpp>

namespace silkrpc {

using Catch::Matchers::Message;

class ErigonRpcApiTest : public commands::ErigonRpcApi {
public:
    explicit ErigonRpcApiTest(Context& context) : commands::ErigonRpcApi{context} {}

    using commands::ErigonRpcApi::handle_erigon_get_header_by_hash;
    using commands::ErigonRpcApi::handle_erigon_get_header_by_number;
    using commands::ErigonRpcApi::handle_erigon_get_logs_by_hash;
    using commands::ErigonRpcApi::handle_erigon_forks;
    using commands::ErigonRpcApi::handle_erigon_issuance;
};

using TestHandleMethod = asio::awaitable<void>(ErigonRpcApiTest::*)(const nlohmann::json&, nlohmann::json&);

void check_api(TestHandleMethod method, const nlohmann::json& request, nlohmann::json& reply, bool success) {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto context_pool_thread = std::thread([&]() { cp.run(); });
    try {
        ErigonRpcApiTest erigon_api{cp.next_context()};
        auto result{asio::co_spawn(cp.next_io_context(), [&]() {
            return (&erigon_api->*method)(request, reply);
        }, asio::use_future)};
        result.get();
        CHECK(success);
    } catch (...) {
        CHECK(!success);
    }
    cp.stop();
    context_pool_thread.join();
}

void check_api_ok(TestHandleMethod method, const nlohmann::json& request, nlohmann::json& reply) {
    check_api(method, request, reply, /*success=*/true);
}

void check_api_ko(TestHandleMethod method, const nlohmann::json& request, nlohmann::json& reply) {
    check_api(method, request, reply, /*success=*/false);
}

TEST_CASE("handle_erigon_get_header_by_hash", "[silkrpc][erigon_api]") {
    nlohmann::json reply;

    SECTION("request params is empty: success and return error") {
        check_api_ok(&ErigonRpcApiTest::handle_erigon_get_header_by_hash, R"({
            "jsonrpc":"2.0",
            "id":1,
            "method":"erigon_getHeaderByHash",
            "params":[]
        })"_json, reply);
        CHECK(reply == R"({
            "jsonrpc":"2.0",
            "id":1,
            "error":{"code":100,"message":"invalid erigon_getHeaderByHash params: []"}
        })"_json);
    }
}

} // namespace silkrpc

