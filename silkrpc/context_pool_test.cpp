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

#include <atomic>
#include <stdexcept>
#include <thread>

#include <catch2/catch.hpp>
#include <grpcpp/grpcpp.h>
#include <silkworm/common/log.hpp>

#include <silkrpc/common/log.hpp>

namespace silkrpc {

using Catch::Matchers::Message;

TEST_CASE("SleepingWaitStrategy", "[silkrpc][context_pool]") {
    SleepingWaitStrategy wait_strategy;
    CHECK_NOTHROW(wait_strategy.wait_once(0));
    CHECK_NOTHROW(wait_strategy.wait_once(1));
}

TEST_CASE("YieldingWaitStrategy", "[silkrpc][context_pool]") {
    YieldingWaitStrategy wait_strategy;
    CHECK_NOTHROW(wait_strategy.wait_once(0));
    CHECK_NOTHROW(wait_strategy.wait_once(1));
}

TEST_CASE("SpinWaitWaitStrategy", "[silkrpc][context_pool]") {
    SpinWaitWaitStrategy wait_strategy;
    CHECK_NOTHROW(wait_strategy.wait_once(0));
    CHECK_NOTHROW(wait_strategy.wait_once(1));
}

TEST_CASE("BusySpinWaitStrategy", "[silkrpc][context_pool]") {
    BusySpinWaitStrategy wait_strategy;
    CHECK_NOTHROW(wait_strategy.wait_once(0));
    CHECK_NOTHROW(wait_strategy.wait_once(1));
}

TEST_CASE("make_wait_strategy", "[silkrpc][context_pool]") {
    CHECK(dynamic_cast<SleepingWaitStrategy*>(make_wait_strategy(WaitMode::sleeping).get()) != nullptr);
    CHECK(dynamic_cast<YieldingWaitStrategy*>(make_wait_strategy(WaitMode::yielding).get()) != nullptr);
    CHECK(dynamic_cast<SpinWaitWaitStrategy*>(make_wait_strategy(WaitMode::spin_wait).get()) != nullptr);
    CHECK(dynamic_cast<BusySpinWaitStrategy*>(make_wait_strategy(WaitMode::busy_spin).get()) != nullptr);
    CHECK(make_wait_strategy(WaitMode::blocking).get() == nullptr);
}

ChannelFactory create_channel = []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); };

TEST_CASE("Context", "[silkrpc][context_pool]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);
    Context context{create_channel, std::make_shared<BlockCache>()};

    SECTION("Context::Context", "[silkrpc][context_pool]") {
        CHECK_NOTHROW(context.io_context() != nullptr);
        CHECK_NOTHROW(context.rpc_end_point() != nullptr);
        CHECK_NOTHROW(context.backend() != nullptr);
        CHECK_NOTHROW(context.miner() != nullptr);
        CHECK_NOTHROW(context.block_cache() != nullptr);
    }

    SECTION("Context::execution_loop", "[silkrpc][context_pool]") {
        std::atomic_bool processed{false};
        auto io_context = context.io_context();
        io_context->post([&]() {
            processed = true;
            io_context->stop();
        });
        auto context_thread = std::thread([&]() { context.execute_loop(); });
        CHECK_NOTHROW(context_thread.join());
        CHECK(processed);
    }

    SECTION("Context::stop", "[silkrpc][context_pool]") {
        std::atomic_bool processed{false};
        context.io_context()->post([&]() {
            processed = true;
        });
        auto context_thread = std::thread([&]() { context.execute_loop(); });
        CHECK_NOTHROW(context.stop());
        CHECK_NOTHROW(context_thread.join());
    }
}

TEST_CASE("create context pool", "[silkrpc][context_pool]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("reject size 0") {
        CHECK_THROWS_MATCHES((ContextPool{0, create_channel}), std::logic_error, Message("ContextPool::ContextPool pool_size is 0"));
    }

    SECTION("accept size 1") {
        ContextPool cp{1, create_channel};
        CHECK(&cp.next_context() == &cp.next_context());
        CHECK(&cp.next_io_context() == &cp.next_io_context());
    }

    SECTION("accept size greater than 1") {
        ContextPool cp{3, create_channel};

        const auto& context1 = cp.next_context();
        const auto& context2 = cp.next_context();
        const auto& context3 = cp.next_context();

        const auto& context4 = cp.next_context();
        const auto& context5 = cp.next_context();
        const auto& context6 = cp.next_context();

        CHECK(&context1 == &context4);
        CHECK(&context2 == &context5);
        CHECK(&context3 == &context6);

        const auto& io_context1 = cp.next_io_context();
        const auto& io_context2 = cp.next_io_context();
        const auto& io_context3 = cp.next_io_context();

        const auto& io_context4 = cp.next_io_context();
        const auto& io_context5 = cp.next_io_context();
        const auto& io_context6 = cp.next_io_context();

        CHECK(&io_context1 == &io_context4);
        CHECK(&io_context2 == &io_context5);
        CHECK(&io_context3 == &io_context6);
    }
}

TEST_CASE("start context pool", "[silkrpc][context_pool]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("running 1 thread") {
        ContextPool cp{1, create_channel};
        auto context_pool_thread = std::thread([&]() { cp.run(); });
        cp.stop();
        CHECK_NOTHROW(context_pool_thread.join());
    }

    SECTION("running 3 thread") {
        ContextPool cp{3, create_channel};
        auto context_pool_thread = std::thread([&]() { cp.run(); });
        cp.stop();
        CHECK_NOTHROW(context_pool_thread.join());
    }

    SECTION("multiple runners require multiple pools") {
        ContextPool cp1{3, create_channel};
        ContextPool cp2{3, create_channel};
        auto context_pool_thread1 = std::thread([&]() { cp1.run(); });
        auto context_pool_thread2 = std::thread([&]() { cp2.run(); });
        cp1.stop();
        cp2.stop();
        CHECK_NOTHROW(context_pool_thread1.join());
        CHECK_NOTHROW(context_pool_thread2.join());
    }
}

TEST_CASE("stop context pool", "[silkrpc][context_pool]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("not yet running") {
        ContextPool cp{3, create_channel};
        CHECK_NOTHROW(cp.stop());
    }

    SECTION("already stopped") {
        ContextPool cp{3, create_channel};
        auto context_pool_thread = std::thread([&]() { cp.run(); });
        cp.stop();
        CHECK_NOTHROW(cp.stop());
        context_pool_thread.join();
        CHECK_NOTHROW(cp.stop());
    }
}

TEST_CASE("restart context pool", "[silkrpc][context_pool]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("running 1 thread") {
        ContextPool cp{1, create_channel};
        auto context_pool_thread = std::thread([&]() { cp.run(); });
        cp.stop();
        CHECK_NOTHROW(context_pool_thread.join());
        context_pool_thread = std::thread([&]() { cp.run(); });
        cp.stop();
        CHECK_NOTHROW(context_pool_thread.join());
    }

    SECTION("running 3 thread") {
        ContextPool cp{3, create_channel};
        auto context_pool_thread = std::thread([&]() { cp.run(); });
        cp.stop();
        CHECK_NOTHROW(context_pool_thread.join());
        context_pool_thread = std::thread([&]() { cp.run(); });
        cp.stop();
        CHECK_NOTHROW(context_pool_thread.join());
    }
}

TEST_CASE("print context pool", "[silkrpc][context_pool]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);
    ContextPool cp{1, create_channel};
    CHECK_NOTHROW(null_stream() << cp.next_context());
}

} // namespace silkrpc
