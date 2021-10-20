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

#include "remote_cursor.hpp"

#include <future>

#include <asio/co_spawn.hpp>
#include <asio/use_future.hpp>
#include <asio/io_context.hpp>
#include <catch2/catch.hpp>

namespace silkrpc::ethdb::kv {

using Catch::Matchers::Message;

TEST_CASE("remote_cursor::open_cursor", "[silkrpc][ethdb][kv][remote_cursor]") {
    SECTION("success") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
                auto result = std::async([&]() {
                    ::remote::Pair pair;
                    pair.set_cursorid(0x23);
                    read_completed(::grpc::Status::OK, pair);
                });
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(::grpc::Status::OK);
            }
            void completed(bool ok) override {}
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient client{channel, &queue};
        KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result{asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), asio::use_future)};
            io_context.run();
            result.get();
            CHECK(remote_cursor.cursor_id() == 0x23);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("write_start failure") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
                auto result = std::async([&]() {
                    ::remote::Pair pair;
                    pair.set_cursorid(0x23);
                    read_completed(::grpc::Status::OK, pair);
                });
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(::grpc::Status::CANCELLED);
            }
            void completed(bool ok) override {}
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient client{channel, &queue};
        KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result{asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), asio::use_future)};
            io_context.run();
            result.get();
            CHECK(false);
        } catch (const std::system_error& e) {
            CHECK(e.code().value() == grpc::StatusCode::CANCELLED);
        }
    }

    SECTION("read_start failure") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
                auto result = std::async([&]() {
                    read_completed(::grpc::Status::CANCELLED, {});
                });
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                auto result = std::async([&]() {
                    write_completed(::grpc::Status::OK);
                });
            }
            void completed(bool ok) override {}
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient client{channel, &queue};
        KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result{asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), asio::use_future)};
            io_context.run();
            result.get();
            CHECK(false);
        } catch (const std::system_error& e) {
            CHECK(e.code().value() == grpc::StatusCode::CANCELLED);
        }
    }
}

} // namespace silkrpc::ethdb::kv

