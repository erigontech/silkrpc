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

#include <agrpc/test.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/asio/io_context.hpp>
#include <catch2/catch.hpp>
#include <gmock/gmock.h>
#include <silkworm/common/util.hpp>
#include <silkrpc/test/kv_test_base.hpp>
#include <silkrpc/test/grpc_responder.hpp>
#include <silkrpc/test/grpc_actions.hpp>
#include <silkrpc/test/grpc_matcher.hpp>

namespace silkrpc::ethdb::kv {

using Catch::Matchers::Message;

class MockBaseStreamingClient : public AsyncTxStreamingClient {
public:
    void start_call(std::function<void(const grpc::Status&)> start_completed) override {}

    void end_call(std::function<void(const grpc::Status&)> end_completed) override {}

    void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {}

    void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
};

TEST_CASE("RemoteCursor::open_cursor", "[silkrpc][ethdb][kv][remote_cursor]") {
    SECTION("success") {
        class MockStreamingClient1 : public MockBaseStreamingClient {
        public:
            MockStreamingClient1(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
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
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient1 client{channel, &queue};
        KvAsioAwaitable<boost::asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result{boost::asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), boost::asio::use_future)};
            io_context.run();
            result.get();
            CHECK(remote_cursor.cursor_id() == 3);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("write_start failure") {
        class MockStreamingClient2 : public MockBaseStreamingClient {
        public:
            MockStreamingClient2(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
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
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient2 client{channel, &queue};
        KvAsioAwaitable<boost::asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result{boost::asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), boost::asio::use_future)};
            io_context.run();
            result.get();
            CHECK(false);
        } catch (const boost::system::system_error& e) {
            CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
        }
    }

    SECTION("read_start failure") {
        class MockStreamingClient3 : public MockBaseStreamingClient {
        public:
            MockStreamingClient3(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
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
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient3 client{channel, &queue};
        KvAsioAwaitable<boost::asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result{boost::asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), boost::asio::use_future)};
            io_context.run();
            result.get();
            CHECK(false);
        } catch (const boost::system::system_error& e) {
            CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
        }
    }
}

TEST_CASE("RemoteCursor::close_cursor", "[silkrpc][ethdb][kv][remote_cursor]") {
    SECTION("success w/ sync read - sync write") {
        class MockStreamingClient4 : public MockBaseStreamingClient {
        public:
            MockStreamingClient4(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                remote::Pair pair;
                pair.set_cursorid(3);
                read_completed(grpc::Status::OK, pair);
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(grpc::Status::OK);
            }
        };
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient4 client{channel, &queue};
        KvAsioAwaitable<boost::asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{boost::asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), boost::asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{boost::asio::co_spawn(io_context, remote_cursor.close_cursor(), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            result2.get();
            CHECK(remote_cursor.cursor_id() == 0);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("success w/ async read - sync write") {
        class MockStreamingClient5 : public MockBaseStreamingClient {
        public:
            MockStreamingClient5(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
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
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient5 client{channel, &queue};
        KvAsioAwaitable<boost::asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{boost::asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), boost::asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{boost::asio::co_spawn(io_context, remote_cursor.close_cursor(), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            result2.get();
            CHECK(remote_cursor.cursor_id() == 0);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("success w/ sync read - async write") {
        class MockStreamingClient6 : public MockBaseStreamingClient {
        public:
            MockStreamingClient6(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
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
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient6 client{channel, &queue};
        KvAsioAwaitable<boost::asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{boost::asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), boost::asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{boost::asio::co_spawn(io_context, remote_cursor.close_cursor(), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            result2.get();
            CHECK(remote_cursor.cursor_id() == 0);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("success w/ async read - async write") {
        class MockStreamingClient7 : public MockBaseStreamingClient {
        public:
            MockStreamingClient7(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
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
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient7 client{channel, &queue};
        KvAsioAwaitable<boost::asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{boost::asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), boost::asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{boost::asio::co_spawn(io_context, remote_cursor.close_cursor(), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            result2.get();
            CHECK(remote_cursor.cursor_id() == 0);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("write_start failure") {
        class MockStreamingClient8 : public MockBaseStreamingClient {
        public:
            MockStreamingClient8(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
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
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient8 client{channel, &queue};
        KvAsioAwaitable<boost::asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{boost::asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), boost::asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{boost::asio::co_spawn(io_context, remote_cursor.close_cursor(), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            result2.get();
            CHECK(remote_cursor.cursor_id() == 0);
            CHECK(false);
        } catch (const boost::system::system_error& e) {
            CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
        }
    }

    SECTION("read_start failure") {
        class MockStreamingClient9 : public MockBaseStreamingClient {
        public:
            MockStreamingClient9(std::shared_ptr<grpc::Channel> /*channel*/, grpc::CompletionQueue* /*queue*/) {}
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
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient9 client{channel, &queue};
        KvAsioAwaitable<boost::asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{boost::asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), boost::asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{boost::asio::co_spawn(io_context, remote_cursor.close_cursor(), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            result2.get();
            CHECK(remote_cursor.cursor_id() == 0);
            CHECK(false);
        } catch (const boost::system::system_error& e) {
            CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
        }
    }
}

TEST_CASE("RemoteCursor::seek", "[silkrpc][ethdb][kv][remote_cursor]") {
    SECTION("success w/ sync read - sync write") {
        class MockStreamingClient10 : public MockBaseStreamingClient {
        public:
            MockStreamingClient10(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue) {}
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
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient10 client{channel, &queue};
        KvAsioAwaitable<boost::asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{boost::asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), boost::asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{boost::asio::co_spawn(io_context, remote_cursor.seek(silkworm::Bytes{}), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            const auto kv_pair = result2.get();
            CHECK(silkworm::to_hex(kv_pair.key) == "36303830");
            CHECK(silkworm::to_hex(kv_pair.value) == "36303830");
            auto result3{boost::asio::co_spawn(io_context, remote_cursor.close_cursor(), boost::asio::use_future)};
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
        class MockStreamingClient11 : public MockBaseStreamingClient {
        public:
            MockStreamingClient11(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue) {}
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
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient11 client{channel, &queue};
        KvAsioAwaitable<boost::asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{boost::asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), boost::asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{boost::asio::co_spawn(io_context, remote_cursor.seek_exact(silkworm::Bytes{}), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            const auto kv_pair = result2.get();
            CHECK(kv_pair.key == silkworm::Bytes{});
            CHECK(silkworm::to_hex(kv_pair.value) == "36303830");
            auto result3{boost::asio::co_spawn(io_context, remote_cursor.close_cursor(), boost::asio::use_future)};
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
        class MockStreamingClient12 : public MockBaseStreamingClient {
        public:
            MockStreamingClient12(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue) {}
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
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient12 client{channel, &queue};
        KvAsioAwaitable<boost::asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{boost::asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), boost::asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{boost::asio::co_spawn(io_context, remote_cursor.next(), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            const auto kv_pair = result2.get();
            CHECK(silkworm::to_hex(kv_pair.key) == "30303031");
            CHECK(silkworm::to_hex(kv_pair.value) == "30303032");
            auto result3{boost::asio::co_spawn(io_context, remote_cursor.close_cursor(), boost::asio::use_future)};
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
        class MockStreamingClient13 : public MockBaseStreamingClient {
        public:
            MockStreamingClient13(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue) {}
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
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient13 client{channel, &queue};
        KvAsioAwaitable<boost::asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{boost::asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), boost::asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{boost::asio::co_spawn(io_context, remote_cursor.seek_both(silkworm::Bytes{}, silkworm::Bytes{}), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            const auto value = result2.get();
            CHECK(silkworm::to_hex(value) == "36303830");
            auto result3{boost::asio::co_spawn(io_context, remote_cursor.close_cursor(), boost::asio::use_future)};
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
        class MockStreamingClient14 : public MockBaseStreamingClient {
        public:
            MockStreamingClient14(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue) {}
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
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient14 client{channel, &queue};
        KvAsioAwaitable<boost::asio::io_context::executor_type> kv_awaitable{io_context, client};
        RemoteCursor remote_cursor{kv_awaitable};
        try {
            auto result1{boost::asio::co_spawn(io_context, remote_cursor.open_cursor("table1"), boost::asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_cursor.cursor_id() == 3);
            auto result2{boost::asio::co_spawn(io_context, remote_cursor.seek_both_exact(silkworm::Bytes{}, silkworm::Bytes{}), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            const auto kv_pair = result2.get();
            CHECK(kv_pair.key == silkworm::Bytes{});
            CHECK(silkworm::to_hex(kv_pair.value) == "36303830");
            auto result3{boost::asio::co_spawn(io_context, remote_cursor.close_cursor(), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            result3.get();
            CHECK(remote_cursor.cursor_id() == 0);
        } catch (...) {
            CHECK(false);
        }
    }
}

struct RemoteCursorTest : test::KVTestBase {
    RemoteCursorTest() {
        this->expect_request_async_tx();
        EXPECT_CALL(reader_writer_, Read).WillOnce(test::read_success_with(grpc_context_, remote::Pair{}));
        kv_streaming_rpc_.request_and_read(boost::asio::use_future).get();
    }

    KVStreamingRpc kv_streaming_rpc_{*stub_, grpc_context_};
    RemoteCursor2 remote_cursor_{kv_streaming_rpc_};
};

TEST_CASE_METHOD(RemoteCursorTest, "RemoteCursor2::open_cursor", "[silkrpc][ethdb][kv][remote_cursor]") {
    using namespace testing;
    SECTION("success") {
        EXPECT_CALL(reader_writer_, Write(
                AllOf(Property(&remote::Cursor::op, Eq(remote::Op::OPEN)), Property(&remote::Cursor::bucketname, Eq("table1"))), _))
            .WillOnce(test::write_success(grpc_context_));
        remote::Pair pair;
        pair.set_cursorid(3);
        EXPECT_CALL(reader_writer_, Read).WillOnce(test::read_success_with(grpc_context_, pair));        
        CHECK_NOTHROW(spawn_and_wait(remote_cursor_.open_cursor("table1")));
        CHECK(remote_cursor_.cursor_id() == 3);
    }
    SECTION("write failure") {
        EXPECT_CALL(reader_writer_, Write(_, _)).WillOnce(test::write_failure(grpc_context_));
        EXPECT_CALL(reader_writer_, Finish).WillOnce(test::finish_streaming_with_status(grpc_context_, grpc::Status::CANCELLED));
        CHECK_THROWS_MATCHES(spawn_and_wait(remote_cursor_.open_cursor("table1")), 
            boost::system::system_error, 
            test::exception_has_cancelled_grpc_status_code());
    }    
    SECTION("read failure") {
        EXPECT_CALL(reader_writer_, Write(_, _)).WillOnce(test::write_success(grpc_context_));
        EXPECT_CALL(reader_writer_, Read).WillOnce([&](auto* , void* tag) {
            agrpc::process_grpc_tag(grpc_context_, tag, false);
        });        
        EXPECT_CALL(reader_writer_, Finish).WillOnce(test::finish_streaming_with_status(grpc_context_, grpc::Status::CANCELLED));
        CHECK_THROWS_MATCHES(spawn_and_wait(remote_cursor_.open_cursor("table1")), 
            boost::system::system_error, 
            test::exception_has_cancelled_grpc_status_code());
    }
}

TEST_CASE_METHOD(RemoteCursorTest, "RemoteCursor2::close_cursor", "[silkrpc][ethdb][kv][remote_cursor]") {
    using namespace testing;
    SECTION("success") {
        Expectation open = EXPECT_CALL(reader_writer_, Write(_, _)).WillOnce(test::write_success(grpc_context_));
        EXPECT_CALL(reader_writer_, Write(
                AllOf(Property(&remote::Cursor::op, Eq(remote::Op::CLOSE)), Property(&remote::Cursor::cursor, Eq(3))), _))
            .After(open)
            .WillOnce(test::write_success(grpc_context_));
        remote::Pair pair;
        pair.set_cursorid(3);
        EXPECT_CALL(reader_writer_, Read).Times(2).WillRepeatedly(test::read_success_with(grpc_context_, pair)); 
        CHECK_NOTHROW(spawn_and_wait(remote_cursor_.open_cursor("table1")));
        CHECK_NOTHROW(spawn_and_wait(remote_cursor_.close_cursor()));
        CHECK(remote_cursor_.cursor_id() == 0);
    }
}

} // namespace silkrpc::ethdb::kv
