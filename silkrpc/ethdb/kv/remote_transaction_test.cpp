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

#include "remote_transaction.hpp"

#include <future>
#include <system_error>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/asio/io_context.hpp>
#include <catch2/catch.hpp>

#include <silkrpc/ethdb/kv/tx_streaming_client.hpp>
#include <silkrpc/test/grpc_actions.hpp>
#include <silkrpc/test/grpc_matcher.hpp>
#include <silkrpc/test/kv_test_base.hpp>

namespace silkrpc::ethdb::kv {

TEST_CASE("RemoteTransaction::open", "[silkrpc][ethdb][kv][remote_transaction]") {
    SECTION("success") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {
                auto result = std::async([&]() {
                    std::this_thread::yield();
                    start_completed(::grpc::Status::OK);
                });
            }
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
                auto result = std::async([&]() {
                    ::remote::Pair pair;
                    pair.set_txid(4);
                    read_completed(::grpc::Status::OK, pair);
                });
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
        };
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result{boost::asio::co_spawn(io_context, remote_tx.open(), boost::asio::use_future)};
            io_context.run();
            result.get();
            CHECK(remote_tx.tx_id() == 4);
            CHECK(true);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("fail start_call") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {
                start_completed(::grpc::Status::CANCELLED);
            }
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
                auto result = std::async([&]() {
                    ::remote::Pair pair;
                    pair.set_txid(4);
                    read_completed(::grpc::Status::OK, pair);
                });
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
        };
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result{boost::asio::co_spawn(io_context, remote_tx.open(), boost::asio::use_future)};
            io_context.run();
            result.get();
            CHECK(false);
        } catch (const boost::system::system_error& e) {
            CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
        }
    }

    SECTION("fail read_start") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {
                start_completed(::grpc::Status::OK);
            }
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
                auto result = std::async([&]() {
                    ::remote::Pair pair;
                    pair.set_txid(4);
                    read_completed(::grpc::Status::CANCELLED, pair);
                });
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
        };
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result{boost::asio::co_spawn(io_context, remote_tx.open(), boost::asio::use_future)};
            io_context.run();
            result.get();
            CHECK(false);
        } catch (const boost::system::system_error& e) {
            CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
        }
    }
}

TEST_CASE("RemoteTransaction::close", "[silkrpc][ethdb][kv][remote_transaction]") {
    SECTION("success open and no cursor in table") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {
                start_completed(::grpc::Status::OK);
            }
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {
                end_completed(::grpc::Status::OK);
            }
            void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
                ::remote::Pair pair;
                pair.set_txid(4);
                read_completed(::grpc::Status::OK, pair);
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
        };
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result1{boost::asio::co_spawn(io_context, remote_tx.open(), boost::asio::use_future)};
            io_context.run();
            result1.get();
            CHECK(remote_tx.tx_id() == 4);
            auto result2{boost::asio::co_spawn(io_context, remote_tx.close(), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            result2.get();
            CHECK(true);
        } catch (...) {
           CHECK(false);
        }
    }

    SECTION("success no open and no cursor in table") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {
                end_completed(::grpc::Status::OK);
            }
            void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {}
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
        };
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result{boost::asio::co_spawn(io_context, remote_tx.close(), boost::asio::use_future)};
            io_context.run();
            result.get();
            CHECK(true);
        } catch (...) {
           CHECK(false);
        }
    }

    SECTION("success with cursor in table") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {
                start_completed(::grpc::Status::OK);
            }
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {
                end_completed(::grpc::Status::OK);
            }
            void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
                ::remote::Pair pair;
                pair.set_txid(4);
                read_completed(::grpc::Status::OK, pair);
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(::grpc::Status::OK);
            }
        };
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result1{boost::asio::co_spawn(io_context, remote_tx.open(), boost::asio::use_future)};
            io_context.run();
            result1.get();
            auto result2{boost::asio::co_spawn(io_context, remote_tx.cursor("table1"), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            auto cursor = result2.get();
            CHECK(cursor != nullptr);
            auto result3{boost::asio::co_spawn(io_context, remote_tx.close(), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            result3.get();
            CHECK(cursor->cursor_id() == 0);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("fail end_call") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {
                start_completed(::grpc::Status::OK);
            }
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {
                end_completed(::grpc::Status::CANCELLED);
            }
            void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
                ::remote::Pair pair;
                pair.set_txid(4);
                read_completed(::grpc::Status::OK, pair);
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(::grpc::Status::OK);
            }
        };
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result{boost::asio::co_spawn(io_context, remote_tx.close(), boost::asio::use_future)};
            io_context.run();
            result.get();
            CHECK(false);
        } catch (const boost::system::system_error& e) {
            CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
        }
    }
}

TEST_CASE("RemoteTransaction::cursor", "[silkrpc][ethdb][kv][remote_transaction]") {
    SECTION("success") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, const ::remote::Pair &)> read_completed) override {
               ::remote::Pair pair;
               pair.set_cursorid(0x23);
               read_completed(::grpc::Status::OK, pair);
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::OK);
            }
        };
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result{boost::asio::co_spawn(io_context, remote_tx.cursor("table1"), boost::asio::use_future)};
            io_context.run();
            auto cursor = result.get();
            CHECK(cursor->cursor_id() == 0x23);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("success 2 cursor") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, const ::remote::Pair &)> read_completed) override {
                ::remote::Pair pair;
                pair.set_cursorid(0x23);
                read_completed(::grpc::Status::OK, pair);
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(::grpc::Status::OK);
            }
        };
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result1{boost::asio::co_spawn(io_context, remote_tx.cursor("table1"), boost::asio::use_future)};
            io_context.run();
            auto cursor1 = result1.get();
            CHECK(cursor1->cursor_id() == 0x23);
            auto result2{boost::asio::co_spawn(io_context, remote_tx.cursor("table2"), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            auto cursor2 = result2.get();
            CHECK(cursor2->cursor_id() == 0x23);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("fail write_start") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
                ::remote::Pair pair;
                pair.set_cursorid(0x23);
                read_completed(::grpc::Status::OK, pair);
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(::grpc::Status::CANCELLED);
            }
        };
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result{boost::asio::co_spawn(io_context, remote_tx.cursor("table1"), boost::asio::use_future)};
            io_context.run();
            result.get();
            CHECK(false);
        } catch (const boost::system::system_error& e) {
            CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
        }
    }

    SECTION("fail read_start") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
                ::remote::Pair pair;
                pair.set_cursorid(0x23);
                read_completed(::grpc::Status::CANCELLED, pair);
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(::grpc::Status::OK);
            }
        };
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result{boost::asio::co_spawn(io_context, remote_tx.cursor("table1"), boost::asio::use_future)};
            io_context.run();
            result.get();
            CHECK(false);
        } catch (const boost::system::system_error& e) {
            CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
        }
    }
}

TEST_CASE("RemoteTransaction::cursor_dup_sort", "[silkrpc][ethdb][kv][remote_transaction]") {
    SECTION("success") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
                ::remote::Pair pair;
                pair.set_cursorid(0x23);
                read_completed(::grpc::Status::OK, pair);
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(::grpc::Status::OK);
            }
        };
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result{boost::asio::co_spawn(io_context, remote_tx.cursor_dup_sort("table1"), boost::asio::use_future)};
            io_context.run();
            auto cursor = result.get();
            CHECK(cursor->cursor_id() == 0x23);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("success 2 cursor") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
                ::remote::Pair pair;
                pair.set_cursorid(0x23);
                read_completed(::grpc::Status::OK, pair);
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(::grpc::Status::OK);
            }
        };
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result1{boost::asio::co_spawn(io_context, remote_tx.cursor_dup_sort("table1"), boost::asio::use_future)};
            io_context.run();
            auto cursor1 = result1.get();
            CHECK(cursor1->cursor_id() == 0x23);
            auto result2{boost::asio::co_spawn(io_context, remote_tx.cursor_dup_sort("table1"), boost::asio::use_future)};
            io_context.reset();
            io_context.run();
            auto cursor2 = result2.get();
            CHECK(cursor2->cursor_id() == 0x23);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("fail write_start") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
                ::remote::Pair pair;
                pair.set_cursorid(0x23);
                read_completed(::grpc::Status::OK, pair);
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(::grpc::Status::CANCELLED);
            }
        };
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result{boost::asio::co_spawn(io_context, remote_tx.cursor_dup_sort("table1"), boost::asio::use_future)};
            io_context.run();
            result.get();
            CHECK(false);
        } catch (const boost::system::system_error& e) {
            CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
        }
    }

    SECTION("fail read_start") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
                ::remote::Pair pair;
                pair.set_cursorid(0x23);
                read_completed(::grpc::Status::CANCELLED, pair);
            }
            void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
                write_completed(::grpc::Status::OK);
            }
        };
        boost::asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result{boost::asio::co_spawn(io_context, remote_tx.cursor_dup_sort("table1"), boost::asio::use_future)};
            io_context.run();
            result.get();
            CHECK(false);
        } catch (const boost::system::system_error& e) {
            CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
        }
    }
}

struct RemoteTransactionTest : test::KVTestBase {
    RemoteTransaction2 remote_tx_{*stub_, grpc_context_};
};

TEST_CASE_METHOD(RemoteTransactionTest, "RemoteTransaction2::open", "[silkrpc][ethdb][kv][remote_transaction]") {
    SECTION("request fails") {
        EXPECT_CALL(*stub_, AsyncTxRaw).WillOnce([&](auto&&, auto&&, void* tag) {
            agrpc::process_grpc_tag(grpc_context_, tag, false);
            return reader_writer_ptr_.release();
        });
        EXPECT_CALL(reader_writer_, Finish).WillOnce([&](grpc::Status* status, void* tag) {
            *status = grpc::Status::CANCELLED;
            agrpc::process_grpc_tag(grpc_context_, tag, true);
        });
        CHECK_THROWS_MATCHES(spawn_and_wait(remote_tx_.open()), boost::system::system_error, test::exception_has_cancelled_grpc_status_code());
    }
    SECTION("success") {
        this->expect_request_async_tx();
        remote::Pair pair;
        pair.set_txid(4);
        EXPECT_CALL(reader_writer_, Read).WillOnce(test::read_success_with(grpc_context_, pair));
        CHECK_NOTHROW(spawn_and_wait(remote_tx_.open()));
        CHECK(remote_tx_.tx_id() == 4);
    }
}

TEST_CASE_METHOD(RemoteTransactionTest, "RemoteTransaction2::close", "[silkrpc][ethdb][kv][remote_transaction]") {
    using namespace testing;
    this->expect_request_async_tx();
    SECTION("success with cursor in table") {
        remote::Pair pair;
        pair.set_txid(4);
        EXPECT_CALL(reader_writer_, Read).Times(2).WillRepeatedly(test::read_success_with(grpc_context_, pair));
        EXPECT_CALL(reader_writer_, Write(_, _)).WillOnce(test::write_success(grpc_context_));
        EXPECT_CALL(reader_writer_, WritesDone).WillOnce(test::writes_done_success(grpc_context_));
        EXPECT_CALL(reader_writer_, Finish).WillOnce(test::finish_streaming_with_status(grpc_context_, grpc::Status::OK));
        CHECK_NOTHROW(spawn_and_wait(remote_tx_.open()));
        const auto cursor = spawn_and_wait(remote_tx_.cursor("table1"));
        CHECK(cursor != nullptr);
        CHECK_NOTHROW(spawn_and_wait(remote_tx_.close()));
        CHECK(remote_tx_.tx_id() == 0);
    }
}

} // namespace silkrpc::ethdb::kv
