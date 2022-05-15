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

#include "eth_api.hpp"

#include <memory>
#include <thread>

#include <asio/co_spawn.hpp>
#include <asio/thread_pool.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch.hpp>
#include <grpcpp/grpcpp.h>
#include <nlohmann/json.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/context_pool.hpp>
#include <silkrpc/ethdb/cursor.hpp>
#include <silkrpc/ethdb/database.hpp>
#include <silkrpc/ethdb/transaction.hpp>

namespace silkrpc {

using Catch::Matchers::Message;

class EthereumRpcApiTest : public commands::EthereumRpcApi {
public:
    explicit EthereumRpcApiTest(Context& context, asio::thread_pool& workers) : commands::EthereumRpcApi{context, workers} {}

    using commands::EthereumRpcApi::handle_eth_block_number;
    using commands::EthereumRpcApi::handle_eth_send_raw_transaction;
};

typedef asio::awaitable<void> (EthereumRpcApiTest::*HandleTestMethod)(const nlohmann::json&, nlohmann::json&);

void test_eth_api(HandleTestMethod test_handle_method, const nlohmann::json& request, nlohmann::json& reply) {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    auto context_pool_thread = std::thread([&]() { cp.run(); });
    asio::thread_pool workers{1};
    try {
        EthereumRpcApiTest eth_api{cp.next_context(), workers};
        auto result{asio::co_spawn(cp.next_io_context(), [&]() {
            return (&eth_api->*test_handle_method)(request, reply);
        }, asio::use_future)};
        result.get();
    } catch (...) {
        CHECK(false);
    }
    cp.stop();
    context_pool_thread.join();
}

TEST_CASE("handle_eth_block_number succeeds if request well-formed", "[silkrpc][eth_api]") {
    nlohmann::json reply;
    /*
     test_eth_api(&EthereumRpcApiTest::handle_eth_block_number, R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"eth_blockNumber",
        "params":[]
    })"_json, reply);
   */
}

TEST_CASE("handle_eth_block_number fails if request empty", "[silkrpc][eth_api]") {
    nlohmann::json reply;
    //test_eth_api(&EthereumRpcApiTest::handle_eth_block_number, R"({})"_json, reply);
}

TEST_CASE("handle_eth_send_raw_transaction fails rlp parsing", "[silkrpc][eth_api]") {
/*
    nlohmann::json reply;
    test_eth_api(&EthereumRpcApiTest::handle_eth_send_raw_transaction, R"({
        "jsonrpc": "2.0", 
        "id":1,
        "method": "eth_sendRawTransaction", 
        "params": ["0xd46ed67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f0724456"]     
     })"_json, reply);
     //CHECK (reply.content == "rlp: element is larger than containing list");
*/
}

TEST_CASE("handle_eth_send_raw_transaction fails wrong number digit", "[silkrpc][eth_api]") {
/*
    nlohmann::json reply;
    test_eth_api(&EthereumRpcApiTest::handle_eth_send_raw_transaction, R"({
        "jsonrpc": "2.0", 
        "id":1,
        "method": "eth_sendRawTransaction", 
        "params": ["0xd46ed67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445"]     
     })"_json, reply);
     //CHECK (reply.content == "cannot unmarshal hex string");

*/
}

} // namespace silkrpc
