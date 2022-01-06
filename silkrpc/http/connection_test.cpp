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

#include "connection.hpp"

#include <asio/thread_pool.hpp>
#include <catch2/catch.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/commands/rpc_api_handler.hpp>
#include <silkrpc/commands/rpc_api_table.hpp>

namespace silkrpc::http {

using Catch::Matchers::Message;

TEST_CASE("connection creation", "[silkrpc][http][connection]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    ChannelFactory create_channel = []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); };

    SECTION("field initialization") {
        ContextPool context_pool{1, create_channel};
        auto context_pool_thread = std::thread([&]() { context_pool.run(); });
        asio::thread_pool workers;
        commands::RpcApiTable handler_table{""};
        auto handler = std::make_unique<commands::RpcApiHandler>(context_pool.get_context(), workers, handler_table);
        // Uncommenting the following lines you got stuck into llvm-cov problem:
        // error: cmd/unit_test: Failed to load coverage: Malformed coverage data
        /*
        Connection conn{context_pool.get_context(), std::move(handler)};
        */
        context_pool.stop();
        CHECK_NOTHROW(context_pool_thread.join());
    }
}

} // namespace silkrpc::http
