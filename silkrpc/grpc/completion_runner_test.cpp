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

#include "completion_runner.hpp"

#include <future>
#include <thread>

#include <asio/io_context.hpp>
#include <catch2/catch.hpp>
#include <grpcpp/alarm.h>
#include <grpcpp/support/time.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/async_completion_handler.hpp>

namespace silkrpc {

using Catch::Matchers::Message;

int64_t g_fixture_slowdown_factor = 1;
int64_t g_poller_slowdown_factor = 1;

bool built_under_valgrind() {
#ifdef RUNNING_ON_VALGRIND
  return true;
#else
  return false;
#endif
}

bool built_under_tsan() {
#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
  return true;
#else
  return false;
#endif
#else
#ifdef THREAD_SANITIZER
  return true;
#else
  return false;
#endif
#endif
}

bool built_under_asan() {
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
  return true;
#else
  return false;
#endif
#else
#ifdef ADDRESS_SANITIZER
  return true;
#else
  return false;
#endif
#endif
}

bool built_under_msan() {
#if defined(__has_feature)
#if __has_feature(memory_sanitizer)
  return true;
#else
  return false;
#endif
#else
#ifdef MEMORY_SANITIZER
  return true;
#else
  return false;
#endif
#endif
}

bool built_under_ubsan() {
#ifdef GRPC_UBSAN
  return true;
#else
  return false;
#endif
}

int64_t grpc_test_sanitizer_slowdown_factor() {
    int64_t sanitizer_multiplier = 1;
    if (built_under_valgrind()) {
        sanitizer_multiplier = 20;
    } else if (built_under_tsan()) {
        sanitizer_multiplier = 5;
    } else if (built_under_asan()) {
        sanitizer_multiplier = 3;
    } else if (built_under_msan()) {
        sanitizer_multiplier = 4;
    } else if (built_under_ubsan()) {
        sanitizer_multiplier = 5;
    }
    return sanitizer_multiplier;
}

int64_t grpc_test_slowdown_factor() {
    return grpc_test_sanitizer_slowdown_factor() * g_fixture_slowdown_factor * g_poller_slowdown_factor;
}

gpr_timespec grpc_timeout_milliseconds_to_deadline(int64_t time_ms) {
    return gpr_time_add(
        gpr_now(GPR_CLOCK_MONOTONIC),
        gpr_time_from_micros(
            grpc_test_slowdown_factor() * static_cast<int64_t>(1e3) * time_ms,
            GPR_TIMESPAN));
}

TEST_CASE("start completion runner", "[silkrpc][grpc][completion_runner]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("waiting on empty completion queue") {
        grpc::CompletionQueue queue;
        asio::io_context io_context;
        CompletionRunner completion_runner{queue, io_context};
        auto completion_runner_thread = std::thread([&]() { completion_runner.run(); });
        std::this_thread::yield();
        completion_runner.stop();
        CHECK_NOTHROW(completion_runner_thread.join());
    }

    SECTION("posting handler completion to I/O execution context") {
        grpc::CompletionQueue queue;
        asio::io_context io_context;
        asio::io_context::work work{io_context};
        CompletionRunner completion_runner{queue, io_context};
        auto io_context_thread = std::thread([&]() { io_context.run(); });
        auto completion_runner_thread = std::thread([&]() { completion_runner.run(); });
        std::this_thread::yield();
        std::promise<void> p;
        std::future<void> f = p.get_future();
        class ACH : public AsyncCompletionHandler {
        public:
            explicit ACH(std::promise<void>& p) : p_(p) {}
            void completed(bool ok) override { p_.set_value(); };
        private:
            std::promise<void>& p_;
        };
        ACH handler{p};
        AsyncCompletionHandler* handler_ptr = &handler;
        grpc::Alarm alarm;
        alarm.Set(&queue, grpc_timeout_milliseconds_to_deadline(10), AsyncCompletionHandler::tag(handler_ptr));
        f.get();
        completion_runner.stop();
        io_context.stop();
        CHECK_NOTHROW(io_context_thread.join());
        CHECK_NOTHROW(completion_runner_thread.join());
    }

    SECTION("exiting on completion queue already shutdown") {
        grpc::CompletionQueue queue;
        asio::io_context io_context;
        CompletionRunner completion_runner{queue, io_context};
        completion_runner.stop();
        auto completion_runner_thread = std::thread([&]() { completion_runner.run(); });
        std::this_thread::yield();
        CHECK_NOTHROW(completion_runner_thread.join());
    }

    SECTION("stopping again after already stopped") {
        grpc::CompletionQueue queue;
        asio::io_context io_context;
        CompletionRunner completion_runner{queue, io_context};
        auto completion_runner_thread = std::thread([&]() { completion_runner.run(); });
        std::this_thread::yield();
        completion_runner.stop();
        CHECK_NOTHROW(completion_runner_thread.join());
        CHECK_NOTHROW(completion_runner.stop());
    }
}

} // namespace silkrpc

