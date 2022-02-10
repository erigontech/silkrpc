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

#include <silkrpc/ethbackend/backend_mock.hpp>
#include <silkrpc/json/types.hpp>
#include <silkrpc/http/methods.hpp>
#include <catch2/catch.hpp>
#include <asio/use_future.hpp>
#include <asio/co_spawn.hpp>
#include <utility>

namespace silkrpc::commands {

using Catch::Matchers::Message;

class EngineRpcApiTest : public EngineRpcApi{
public:
    explicit EngineRpcApiTest(std::unique_ptr<ethbackend::BackEnd>& backend): EngineRpcApi(backend) {}

    using EngineRpcApi::handle_engine_get_payload_v1;
};

TEST_CASE("handle_engine_get_payload_v1 succeeds if request is expected payload", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    std::unique_ptr<ethbackend::BackEnd> backend(new ethbackend::BackEndMock());
    ethbackend::test_rpc_call<EngineRpcApiTest, &EngineRpcApiTest::handle_engine_get_payload_v1, std::unique_ptr<ethbackend::BackEnd>>(
        R"({
            "jsonrpc":"2.0",
            "id":1,
            "method":"engine_getPayloadV1",
            "params":["0x0000000000000001"]
        })"_json,
        ethbackend::kGetPayloadTest,
        std::move(backend)
    );
}

TEST_CASE("handle_engine_get_payload_v1 fails with invalid amount of params", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    std::unique_ptr<ethbackend::BackEnd> backend(new ethbackend::BackEndMock());
    ethbackend::test_rpc_call<EngineRpcApiTest, &EngineRpcApiTest::handle_engine_get_payload_v1, std::unique_ptr<ethbackend::BackEnd>>(
        R"({
            "jsonrpc":"2.0",
            "id":1,
            "method":"engine_getPayloadV1",
            "params":[]
        })"_json,
        R"({
            "error":{
                "code":100,
                "message":"invalid engine_getPayloadV1 params: []"
            },
            "id":1,
            "jsonrpc":"2.0" 
        })"_json,
        std::move(backend)
    );
}

} // namespace silkrpc::commands
