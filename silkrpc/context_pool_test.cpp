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

#include "context_pool.hpp"

#include <stdexcept>
#include <thread>

#include <catch2/catch.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/log.hpp>

namespace silkrpc {

using Catch::Matchers::Message;

ChannelFactory create_channel = []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); };

TEST_CASE("context pool constructor", "[silkrpc][context_pool]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("reject size 0 for context pool") {
        CHECK_THROWS_MATCHES((ContextPool{0, create_channel}), std::logic_error, Message("ContextPool::ContextPool pool_size is 0"));
    }

    SECTION("accept size 1 for context pool") {
        ContextPool cp{1, create_channel};
        CHECK(&cp.get_context() == &cp.get_context());
        CHECK(&cp.get_io_context() == &cp.get_io_context());
    }

    SECTION("accept size greater than 1 for context pool") {
        ContextPool cp{3, create_channel};

        const auto& context1 = cp.get_context();
        const auto& context2 = cp.get_context();
        const auto& context3 = cp.get_context();

        const auto& context4 = cp.get_context();
        const auto& context5 = cp.get_context();
        const auto& context6 = cp.get_context();

        CHECK(&context1 == &context4);
        CHECK(&context2 == &context5);
        CHECK(&context3 == &context6);

        const auto& io_context1 = cp.get_io_context();
        const auto& io_context2 = cp.get_io_context();
        const auto& io_context3 = cp.get_io_context();

        const auto& io_context4 = cp.get_io_context();
        const auto& io_context5 = cp.get_io_context();
        const auto& io_context6 = cp.get_io_context();

        CHECK(&io_context1 == &io_context4);
        CHECK(&io_context2 == &io_context5);
        CHECK(&io_context3 == &io_context6);
    }
}

TEST_CASE("context pool run", "[silkrpc][context_pool]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("context pool running 1 thread") {
        ContextPool cp{1, create_channel};
        auto context_pool_thread = std::thread([&]() { cp.run(); });
        cp.stop();
        CHECK_NOTHROW(context_pool_thread.join());
    }

    SECTION("context pool running 3 thread") {
        ContextPool cp{3, create_channel};
        auto context_pool_thread = std::thread([&]() { cp.run(); });
        cp.stop();
        CHECK_NOTHROW(context_pool_thread.join());
    }
}

} // namespace silkrpc

