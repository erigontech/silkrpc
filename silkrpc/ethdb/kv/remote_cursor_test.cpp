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
#include <silkworm/common/util.hpp>

namespace silkrpc::ethdb::kv {

using Catch::Matchers::Message;

class MockBaseStreamingClient : public AsyncTxStreamingClient {
public:
    void start_call(std::function<void(const grpc::Status&)> start_completed) override {}

    void end_call(std::function<void(const grpc::Status&)> end_completed) override {}

    void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {}

    void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}

    void completed(bool ok) override {}
};

TEST_CASE("RemoteCursor::open_cursor", "[silkrpc][ethdb][kv][remote_cursor]") {
    SECTION("success") {
        class MockStreamingClient : public MockBaseStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                auto result = std::async([&]() {
                    remote::Pair pair;
                    pair.set_cursorid(3);
                    read_completed(grpc::Status::OK, pair);
                });
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(grpc::Status::OK);
            }
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
            CHECK(remote_cursor.cursor_id() == 3);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("write_start failure") {
        class MockStreamingClient : public MockBaseStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                auto result = std::async([&]() {
                    remote::Pair pair;
                    pair.set_cursorid(3);
                    read_completed(grpc::Status::OK, pair);
                });
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(grpc::Status::CANCELLED);
            }
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
        class MockStreamingClient : public MockBaseStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                auto result = std::async([&]() {
                    read_completed(grpc::Status::CANCELLED, {});
                });
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                auto result = std::async([&]() {
                    write_completed(grpc::Status::OK);
                });
            }
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

TEST_CASE("RemoteCursor::close_cursor", "[silkrpc][ethdb][kv][remote_cursor]") {
    SECTION("success w/ sync read - sync write") {
        class MockStreamingClient : public MockBaseStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                remote::Pair pair;
                pair.set_cursorid(3);
                read_completed(grpc::Status::OK, pair);
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(grpc::Status::OK);
            }
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient client{channel, &queue};
        KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{asio::co_spawn(io_context, remote_cursor.close_cursor(), asio::use_future)};
            io_context.reset();
            io_context.run();
            result2.get();
            CHECK(remote_cursor.cursor_id() == 0);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("success w/ async read - sync write") {
        class MockStreamingClient : public MockBaseStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                auto result = std::async([&]() {
                    remote::Pair pair;
                    pair.set_cursorid(3);
                    read_completed(grpc::Status::OK, pair);
                });
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(grpc::Status::OK);
            }
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient client{channel, &queue};
        KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{asio::co_spawn(io_context, remote_cursor.close_cursor(), asio::use_future)};
            io_context.reset();
            io_context.run();
            result2.get();
            CHECK(remote_cursor.cursor_id() == 0);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("success w/ sync read - async write") {
        class MockStreamingClient : public MockBaseStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                remote::Pair pair;
                pair.set_cursorid(3);
                read_completed(grpc::Status::OK, pair);
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                auto result = std::async([&]() {
                    write_completed(grpc::Status::OK);
                });
            }
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient client{channel, &queue};
        KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{asio::co_spawn(io_context, remote_cursor.close_cursor(), asio::use_future)};
            io_context.reset();
            io_context.run();
            result2.get();
            CHECK(remote_cursor.cursor_id() == 0);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("success w/ async read - async write") {
        class MockStreamingClient : public MockBaseStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                auto result = std::async([&]() {
                    remote::Pair pair;
                    pair.set_cursorid(3);
                    read_completed(grpc::Status::OK, pair);
                });
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                auto result = std::async([&]() {
                    write_completed(grpc::Status::OK);
                });
            }
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient client{channel, &queue};
        KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{asio::co_spawn(io_context, remote_cursor.close_cursor(), asio::use_future)};
            io_context.reset();
            io_context.run();
            result2.get();
            CHECK(remote_cursor.cursor_id() == 0);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("write_start failure") {
        class MockStreamingClient : public MockBaseStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                auto result = std::async([&]() {
                    remote::Pair pair;
                    pair.set_cursorid(3);
                    read_completed(grpc::Status::OK, pair);
                });
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                auto result = std::async([&]() {
                    write_completed(grpc::Status::CANCELLED);
                });
            }
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient client{channel, &queue};
        KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{asio::co_spawn(io_context, remote_cursor.close_cursor(), asio::use_future)};
            io_context.reset();
            io_context.run();
            result2.get();
            CHECK(remote_cursor.cursor_id() == 0);
            CHECK(false);
        } catch (const std::system_error& e) {
            CHECK(e.code().value() == grpc::StatusCode::CANCELLED);
        }
    }

    SECTION("read_start failure") {
        class MockStreamingClient : public MockBaseStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                auto result = std::async([&]() {
                    read_completed(grpc::Status::CANCELLED, {});
                });
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                auto result = std::async([&]() {
                    write_completed(grpc::Status::OK);
                });
            }
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient client{channel, &queue};
        KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{asio::co_spawn(io_context, remote_cursor.close_cursor(), asio::use_future)};
            io_context.reset();
            io_context.run();
            result2.get();
            CHECK(remote_cursor.cursor_id() == 0);
            CHECK(false);
        } catch (const std::system_error& e) {
            CHECK(e.code().value() == grpc::StatusCode::CANCELLED);
        }
    }
}

TEST_CASE("RemoteCursor::seek", "[silkrpc][ethdb][kv][remote_cursor]") {
    SECTION("success w/ sync read - sync write") {
        class MockStreamingClient : public MockBaseStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                remote::Pair pair;
                pair.set_cursorid(3);
                pair.set_k("6080");
                pair.set_v("6080");
                read_completed(grpc::Status::OK, pair);
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(grpc::Status::OK);
            }
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient client{channel, &queue};
        KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{asio::co_spawn(io_context, remote_cursor.seek(silkworm::Bytes{}), asio::use_future)};
            io_context.reset();
            io_context.run();
            const auto kv_pair = result2.get();
            CHECK(silkworm::to_hex(kv_pair.key) == "36303830");
            CHECK(silkworm::to_hex(kv_pair.value) == "36303830");
            auto result3{asio::co_spawn(io_context, remote_cursor.close_cursor(), asio::use_future)};
            io_context.reset();
            io_context.run();
            result3.get();
            CHECK(remote_cursor.cursor_id() == 0);
        } catch (...) {
            CHECK(false);
        }
    }
}

TEST_CASE("RemoteCursor::seek_exact", "[silkrpc][ethdb][kv][remote_cursor]") {
    SECTION("success w/ sync read - sync write") {
        class MockStreamingClient : public MockBaseStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                remote::Pair pair;
                pair.set_cursorid(3);
                pair.set_v("6080");
                read_completed(grpc::Status::OK, pair);
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(grpc::Status::OK);
            }
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient client{channel, &queue};
        KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{asio::co_spawn(io_context, remote_cursor.seek_exact(silkworm::Bytes{}), asio::use_future)};
            io_context.reset();
            io_context.run();
            const auto kv_pair = result2.get();
            CHECK(kv_pair.key == silkworm::Bytes{});
            CHECK(silkworm::to_hex(kv_pair.value) == "36303830");
            auto result3{asio::co_spawn(io_context, remote_cursor.close_cursor(), asio::use_future)};
            io_context.reset();
            io_context.run();
            result3.get();
            CHECK(remote_cursor.cursor_id() == 0);
        } catch (...) {
            CHECK(false);
        }
    }
}

TEST_CASE("RemoteCursor::next", "[silkrpc][ethdb][kv][remote_cursor]") {
    SECTION("success w/ sync read - sync write") {
        class MockStreamingClient : public MockBaseStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                remote::Pair pair;
                pair.set_cursorid(3);
                pair.set_k("0001");
                pair.set_v("0002");
                read_completed(grpc::Status::OK, pair);
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(grpc::Status::OK);
            }
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient client{channel, &queue};
        KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{asio::co_spawn(io_context, remote_cursor.next(), asio::use_future)};
            io_context.reset();
            io_context.run();
            const auto kv_pair = result2.get();
            CHECK(silkworm::to_hex(kv_pair.key) == "30303031");
            CHECK(silkworm::to_hex(kv_pair.value) == "30303032");
            auto result3{asio::co_spawn(io_context, remote_cursor.close_cursor(), asio::use_future)};
            io_context.reset();
            io_context.run();
            result3.get();
            CHECK(remote_cursor.cursor_id() == 0);
        } catch (...) {
            CHECK(false);
        }
    }
}

TEST_CASE("RemoteCursor::seek_both", "[silkrpc][ethdb][kv][remote_cursor]") {
    SECTION("success w/ sync read - sync write") {
        class MockStreamingClient : public MockBaseStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                remote::Pair pair;
                pair.set_cursorid(3);
                pair.set_v("6080");
                read_completed(grpc::Status::OK, pair);
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(grpc::Status::OK);
            }
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient client{channel, &queue};
        KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{asio::co_spawn(io_context, remote_cursor.seek_both(silkworm::Bytes{}, silkworm::Bytes{}), asio::use_future)};
            io_context.reset();
            io_context.run();
            const auto value = result2.get();
            CHECK(silkworm::to_hex(value) == "36303830");
            auto result3{asio::co_spawn(io_context, remote_cursor.close_cursor(), asio::use_future)};
            io_context.reset();
            io_context.run();
            result3.get();
            CHECK(remote_cursor.cursor_id() == 0);
        } catch (...) {
            CHECK(false);
        }
    }
}

TEST_CASE("RemoteCursor::seek_both_exact", "[silkrpc][ethdb][kv][remote_cursor]") {
    SECTION("success w/ sync read - sync write") {
        class MockStreamingClient : public MockBaseStreamingClient {
        public:
            MockStreamingClient(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                remote::Pair pair;
                pair.set_cursorid(3);
                pair.set_v("6080");
                read_completed(grpc::Status::OK, pair);
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(grpc::Status::OK);
            }
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient client{channel, &queue};
        KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{asio::co_spawn(io_context, remote_cursor.seek_both_exact(silkworm::Bytes{}, silkworm::Bytes{}), asio::use_future)};
            io_context.reset();
            io_context.run();
            const auto kv_pair = result2.get();
            CHECK(kv_pair.key == silkworm::Bytes{});
            CHECK(silkworm::to_hex(kv_pair.value) == "36303830");
            auto result3{asio::co_spawn(io_context, remote_cursor.close_cursor(), asio::use_future)};
            io_context.reset();
            io_context.run();
            result3.get();
            CHECK(remote_cursor.cursor_id() == 0);
        } catch (...) {
            CHECK(false);
        }
    }
}

} // namespace silkrpc::ethdb::kv
