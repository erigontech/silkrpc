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
#include <asio/co_spawn.hpp>
#include <asio/use_future.hpp>
#include <asio/io_context.hpp>
#include <catch2/catch.hpp>
#include <gmock/gmock.h>

#include <silkrpc/test/kv_test_base.hpp>
#include <silkrpc/test/grpc_responder.hpp>
#include <silkrpc/test/grpc_actions.hpp>
#include <silkrpc/test/grpc_matcher.hpp>
#include <silkworm/common/util.hpp>

namespace silkrpc::ethdb::kv {

/*
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
            MockStreamingClient1(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue) {}
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
        MockStreamingClient1 client{channel, &queue};
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
        class MockStreamingClient2 : public MockBaseStreamingClient {
        public:
            MockStreamingClient2(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue) {}
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
        MockStreamingClient2 client{channel, &queue};
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
        class MockStreamingClient3 : public MockBaseStreamingClient {
        public:
            MockStreamingClient3(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue) {}
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
        MockStreamingClient3 client{channel, &queue};
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
        class MockStreamingClient4 : public MockBaseStreamingClient {
        public:
            MockStreamingClient4(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue) {}
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
        MockStreamingClient4 client{channel, &queue};
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
        class MockStreamingClient5 : public MockBaseStreamingClient {
        public:
            MockStreamingClient5(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue) {}
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
        MockStreamingClient5 client{channel, &queue};
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
        class MockStreamingClient6 : public MockBaseStreamingClient {
        public:
            MockStreamingClient6(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue) {}
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
        MockStreamingClient6 client{channel, &queue};
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
        class MockStreamingClient7 : public MockBaseStreamingClient {
        public:
            MockStreamingClient7(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue) {}
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
        MockStreamingClient7 client{channel, &queue};
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
        class MockStreamingClient8 : public MockBaseStreamingClient {
        public:
            MockStreamingClient8(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue) {}
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
        MockStreamingClient8 client{channel, &queue};
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
        class MockStreamingClient9 : public MockBaseStreamingClient {
        public:
            MockStreamingClient9(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue) {}
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
        MockStreamingClient9 client{channel, &queue};
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
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient10 client{channel, &queue};
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
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient11 client{channel, &queue};
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
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient12 client{channel, &queue};
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
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient13 client{channel, &queue};
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
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        MockStreamingClient14 client{channel, &queue};
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
}*/

struct RemoteCursorTest : test::KVTestBase {
    RemoteCursorTest() {
        // Set the call expectations common to all RemoteCursor tests:
        // remote::KV::StubInterface::AsyncTxRaw call succeeds
        expect_request_async_tx(true);
        // AsyncReaderWriter<remote::Cursor, remote::Pair>::Read call succeeds w/ tx_id set in pair ignored
        EXPECT_CALL(reader_writer_, Read).WillOnce(test::read_success_with(grpc_context_, remote::Pair{}));

        // Execute the test preconditions: start a new Tx RPC and read first incoming message (tx_id)
        REQUIRE_NOTHROW(tx_rpc_.request_and_read(asio::use_future).get());
    }

    TxRpc tx_rpc_{*stub_, grpc_context_};
    RemoteCursor2 remote_cursor_{tx_rpc_};
};

TEST_CASE_METHOD(RemoteCursorTest, "RemoteCursor2::open_cursor", "[silkrpc][ethdb][kv][remote_cursor]") {
    using namespace testing;  // NOLINT(build/namespaces)

    SECTION("success") {
        // Set the call expectations:
        // 1. AsyncReaderWriter<remote::Cursor, remote::Pair>::Write call to open cursor on specified table succeeds
        EXPECT_CALL(reader_writer_, Write(
                AllOf(Property(&remote::Cursor::op, Eq(remote::Op::OPEN)), Property(&remote::Cursor::bucketname, Eq("table1"))), _))
            .WillOnce(test::write_success(grpc_context_));
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Read call succeeds setting the specified cursor ID
        remote::Pair pair;
        pair.set_cursorid(3);
        EXPECT_CALL(reader_writer_, Read).WillOnce(test::read_success_with(grpc_context_, pair));

        // Execute the test: opening a cursor on specified table should succeed and cursor should have expected cursor ID
        CHECK_NOTHROW(spawn_and_wait(remote_cursor_.open_cursor("table1")));
        CHECK(remote_cursor_.cursor_id() == 3);
    }
    SECTION("write failure") {
        // Set the call expectations:
        // 1. AsyncReaderWriter<remote::Cursor, remote::Pair>::Write call to open cursor on specified table fails
        EXPECT_CALL(reader_writer_, Write(_, _)).WillOnce(test::write_failure(grpc_context_));
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Finish call succeeds w/ status cancelled
        EXPECT_CALL(reader_writer_, Finish).WillOnce(test::finish_streaming_cancelled(grpc_context_));

        // Execute the test: opening a cursor should raise an exception w/ expected gRPC status code
        CHECK_THROWS_MATCHES(spawn_and_wait(remote_cursor_.open_cursor("table1")),
            asio::system_error,
            test::exception_has_cancelled_grpc_status_code());
    }
    SECTION("read failure") {
        // Set the call expectations:
        // 1. AsyncReaderWriter<remote::Cursor, remote::Pair>::Write call to open cursor on specified table succeeds
        EXPECT_CALL(reader_writer_, Write(_, _)).WillOnce(test::write_success(grpc_context_));
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Read call fails
        EXPECT_CALL(reader_writer_, Read).WillOnce(test::read_failure(grpc_context_));
        // 3. AsyncReaderWriter<remote::Cursor, remote::Pair>::Finish call succeeds w/ status cancelled
        EXPECT_CALL(reader_writer_, Finish).WillOnce(test::finish_streaming_cancelled(grpc_context_));

        // Execute the test: opening a cursor should raise an exception w/ expected gRPC status code
        CHECK_THROWS_MATCHES(spawn_and_wait(remote_cursor_.open_cursor("table1")),
            asio::system_error,
            test::exception_has_cancelled_grpc_status_code());
    }
}

TEST_CASE_METHOD(RemoteCursorTest, "RemoteCursor2::close_cursor", "[silkrpc][ethdb][kv][remote_cursor]") {
    using namespace testing;  // NOLINT(build/namespaces)

    SECTION("success") {
        // Set the call expectations:
        // 1. AsyncReaderWriter<remote::Cursor, remote::Pair>::Write call to open cursor succeeds
        Expectation open = EXPECT_CALL(reader_writer_, Write(_, _)).WillOnce(test::write_success(grpc_context_));
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Write call to close cursor w/ specified cursor ID succeeds
        EXPECT_CALL(reader_writer_, Write(
                AllOf(Property(&remote::Cursor::op, Eq(remote::Op::CLOSE)), Property(&remote::Cursor::cursor, Eq(3))), _))
            .After(open)
            .WillOnce(test::write_success(grpc_context_));
        // 3. AsyncReaderWriter<remote::Cursor, remote::Pair>::Read calls succeed setting the specified cursor ID
        remote::Pair pair;
        pair.set_cursorid(3);
        EXPECT_CALL(reader_writer_, Read).Times(2).WillRepeatedly(test::read_success_with(grpc_context_, pair));

        // Execute the test preconditions: open a new cursor on specified table
        REQUIRE_NOTHROW(spawn_and_wait(remote_cursor_.open_cursor("table1")));

        // Execute the test: closing a cursor should succeed and reset the cursor ID
        CHECK_NOTHROW(spawn_and_wait(remote_cursor_.close_cursor()));
        CHECK(remote_cursor_.cursor_id() == 0);
    }
}

} // namespace silkrpc::ethdb::kv
