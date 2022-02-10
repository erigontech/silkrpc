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
#include <silkrpc/ethbackend/backend_mock.hpp>
#include <silkrpc/json/types.hpp>
#include <catch2/catch.hpp>
#include <asio/use_future.hpp>
#include <asio/co_spawn.hpp>

namespace silkrpc::commands {

using Catch::Matchers::Message;

class Web3RpcApiTest : public Web3RpcApi {
public:
    explicit Web3RpcApiTest(Context& context): Web3RpcApi(context) {}

    using Web3RpcApi::handle_web3_client_version;
    using Web3RpcApi::handle_web3_sha3;
};

TEST_CASE("handle_web3_client_version succeeds if request is expected version", "[silkrpc][web3_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    Context context;
    context.backend = std::unique_ptr<ethbackend::BackEnd>(new ethbackend::BackEndMock());
    ethbackend::test_rpc_call<Web3RpcApiTest, &Web3RpcApiTest::handle_web3_client_version, Context&>(
        R"({
            "jsonrpc":"2.0",
            "id":1,
            "method":"web3_clientVersion",
            "params":[]
        })"_json,
        R"({
            "id":1,
            "jsonrpc":"2.0",
            "result":"6.0.0"
        })"_json,
        context
    );
}

TEST_CASE("handle_web3_sha3 succeeds if request is sha3 of input", "[silkrpc][web3_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    Context context;
    context.backend = std::unique_ptr<ethbackend::BackEnd>(new ethbackend::BackEndMock());
    ethbackend::test_rpc_call<Web3RpcApiTest, &Web3RpcApiTest::handle_web3_sha3, Context&>(
        R"({
            "jsonrpc":"2.0",
            "id":1,
            "method":"handle_web3_sha3",
            "params":["0x1"]
        })"_json,
        R"({
            "id":1,
            "jsonrpc":"2.0",
            "result":"0x5fe7f977e71dba2ea1a68e21057beebb9be2ac30c6410aa38d4f3fbe41dcffd2"
        })"_json,
        context
    );
}

TEST_CASE("handle_web3_sha3 fails with not enough parameters", "[silkrpc][web3_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    Context context;
    context.backend = std::unique_ptr<ethbackend::BackEnd>(new ethbackend::BackEndMock());
    ethbackend::test_rpc_call<Web3RpcApiTest, &Web3RpcApiTest::handle_web3_sha3, Context&>(
        R"({
            "jsonrpc":"2.0",
            "id":1,
            "method":"web3_sha3",
            "params":[]
        })"_json,
        R"({
            "error":{
                "code":100,
                "message":"invalid web3_sha3 params: []"
            },
            "id":1,
            "jsonrpc":"2.0" 
        })"_json,
        context
    );
}

TEST_CASE("handle_web3_sha3 fails with not non-hex parameter", "[silkrpc][web3_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    Context context;
    context.backend = std::unique_ptr<ethbackend::BackEnd>(new ethbackend::BackEndMock());
    ethbackend::test_rpc_call<Web3RpcApiTest, &Web3RpcApiTest::handle_web3_sha3, Context&>(
        R"({
            "jsonrpc":"2.0",
            "id":1,
            "method":"handle_web3_sha3",
            "params":["buongiorno"]
        })"_json,
        R"({
            "error":{
                "code":100,
                "message":"invalid input: buongiorno"
            },
            "id":1,
            "jsonrpc":"2.0" 
        })"_json,
        context
    );
}

using Catch::Matchers::Message;

} // namespace silkrpc::commands
