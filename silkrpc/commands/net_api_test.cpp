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
#include <silkrpc/ethbackend/backend_mock.hpp>
#include <silkrpc/json/types.hpp>
#include <catch2/catch.hpp>
#include <asio/use_future.hpp>
#include <asio/co_spawn.hpp>
#include <utility>

namespace silkrpc::commands {

using Catch::Matchers::Message;

class NetRpcApiTest : public NetRpcApi {
public:
    explicit NetRpcApiTest(std::unique_ptr<ethbackend::BackEnd>& backend): NetRpcApi(backend) {}

    using NetRpcApi::handle_net_peer_count;
    using NetRpcApi::handle_net_version;
};

TEST_CASE("handle_net_peer_count succeeds if request is expected peer count", "[silkrpc][net_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    std::unique_ptr<ethbackend::BackEnd> backend(new ethbackend::BackEndMock());
    ethbackend::test_rpc_call<NetRpcApiTest, &NetRpcApiTest::handle_net_peer_count, std::unique_ptr<ethbackend::BackEnd>>(
        R"({
            "jsonrpc":"2.0",
            "id":1,
            "method":"net_peerCount",
            "params":[]
        })"_json,
        R"({
            "id":1,
            "jsonrpc":"2.0",
            "result":"0x5"
        })"_json,
        std::move(backend)
    );
}

TEST_CASE("handle_net_version succeeds if request is expected version", "[silkrpc][net_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    std::unique_ptr<ethbackend::BackEnd> backend(new ethbackend::BackEndMock());
    ethbackend::test_rpc_call<NetRpcApiTest, &NetRpcApiTest::handle_net_version, std::unique_ptr<ethbackend::BackEnd>>(
        R"({
            "jsonrpc":"2.0",
            "id":1,
            "method":"net_version",
            "params":[]
        })"_json,
        R"({
            "id":1,
            "jsonrpc":"2.0",
            "result":"2"
        })"_json,
        std::move(backend)
    );
}

using Catch::Matchers::Message;

} // namespace silkrpc::commands

