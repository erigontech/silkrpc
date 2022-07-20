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

#include <asio/co_spawn.hpp>
#include <asio/use_future.hpp>
#include <asio/io_context.hpp>
#include <catch2/catch.hpp>

#include <silkrpc/test/grpc_actions.hpp>
#include <silkrpc/test/grpc_matcher.hpp>
#include <silkrpc/test/kv_test_base.hpp>

namespace silkrpc::ethdb::kv {

/*
using Catch::Matchers::Message;

TEST_CASE("RemoteTransaction::cursor_dup_sort", "[silkrpc][ethdb][kv][remote_transaction]") {
    SECTION("success") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& stub, grpc::CompletionQueue* queue) {}
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
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result{asio::co_spawn(io_context, remote_tx.cursor_dup_sort("table1"), asio::use_future)};
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
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& stub, grpc::CompletionQueue* queue) {}
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
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result1{asio::co_spawn(io_context, remote_tx.cursor_dup_sort("table1"), asio::use_future)};
            io_context.run();
            auto cursor1 = result1.get();
            CHECK(cursor1->cursor_id() == 0x23);
            auto result2{asio::co_spawn(io_context, remote_tx.cursor_dup_sort("table1"), asio::use_future)};
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
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& stub, grpc::CompletionQueue* queue) {}
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
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result{asio::co_spawn(io_context, remote_tx.cursor_dup_sort("table1"), asio::use_future)};
            io_context.run();
            result.get();
            CHECK(false);
        } catch (const std::system_error& e) {
            CHECK(e.code().value() == grpc::StatusCode::CANCELLED);
        }
    }

    SECTION("fail read_start") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& stub, grpc::CompletionQueue* queue) {}
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
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        std::unique_ptr<remote::KV::StubInterface> stub{remote::KV::NewStub(channel)};
        grpc::CompletionQueue queue;
        RemoteTransaction<MockStreamingClient> remote_tx(io_context, stub, &queue);
        try {
            auto result{asio::co_spawn(io_context, remote_tx.cursor_dup_sort("table1"), asio::use_future)};
            io_context.run();
            result.get();
            CHECK(false);
        } catch (const std::system_error& e) {
            CHECK(e.code().value() == grpc::StatusCode::CANCELLED);
        }
    }
}
*/

using testing::_;

struct RemoteTransactionTest : test::KVTestBase {
    RemoteTransaction2 remote_tx_{*stub_, grpc_context_};
};

TEST_CASE_METHOD(RemoteTransactionTest, "RemoteTransaction2::open", "[silkrpc][ethdb][kv][remote_transaction]") {
    SECTION("success") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncTxRaw call succeeds
        expect_request_async_tx(/*ok=*/true);
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Read call succeeds setting the specified transaction ID
        remote::Pair pair;
        pair.set_txid(4);
        EXPECT_CALL(reader_writer_, Read).WillOnce(test::read_success_with(grpc_context_, pair));

        // Execute the test: opening a transaction should succeed and transaction should have expected transaction ID
        CHECK_NOTHROW(spawn_and_wait(remote_tx_.open()));
        CHECK(remote_tx_.tx_id() == 4);
    }
    SECTION("request failure") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncTxRaw call fails
        expect_request_async_tx(/*ok=*/false);
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Finish call succeeds w/ status cancelled
        EXPECT_CALL(reader_writer_, Finish).WillOnce(test::finish_streaming_cancelled(grpc_context_));

        // Execute the test: opening a transaction should raise an exception w/ expected gRPC status code
        CHECK_THROWS_MATCHES(spawn_and_wait(remote_tx_.open()), asio::system_error, test::exception_has_cancelled_grpc_status_code());
    }
    SECTION("read failure") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncTxRaw call succeeds
        expect_request_async_tx(/*ok=*/true);
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Read call fails
        EXPECT_CALL(reader_writer_, Read).WillOnce(test::read_failure(grpc_context_));
        // 3. AsyncReaderWriter<remote::Cursor, remote::Pair>::Finish call succeeds w/ status cancelled
        EXPECT_CALL(reader_writer_, Finish).WillOnce(test::finish_streaming_cancelled(grpc_context_));

        // Execute the test: opening a transaction should raise an exception w/ expected gRPC status code
        CHECK_THROWS_MATCHES(spawn_and_wait(remote_tx_.open()), asio::system_error, test::exception_has_cancelled_grpc_status_code());
    }
}

TEST_CASE_METHOD(RemoteTransactionTest, "RemoteTransaction2::close", "[silkrpc][ethdb][kv][remote_transaction]") {
    //TODO(canepat) waiting for PrepareAsync in asio-grpc
    /*SECTION("success w/o open") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncTxRaw call succeeds
        expect_request_async_tx(.ok=true);
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::WritesDone call succeeds
        EXPECT_CALL(reader_writer_, WritesDone).WillOnce(test::writes_done_success(grpc_context_));
        // 3. AsyncReaderWriter<remote::Cursor, remote::Pair>::Finish call succeeds w/ status OK
        EXPECT_CALL(reader_writer_, Finish).WillOnce(test::finish_streaming_ok(grpc_context_));

        // Execute the test: closing the transaction should succeed and transaction should have zero transaction ID
        CHECK_NOTHROW(spawn_and_wait(remote_tx_.close()));
        CHECK(remote_tx_.tx_id() == 0);
    }*/
    SECTION("success w/ open w/o cursor in table") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncTxRaw call succeeds
        expect_request_async_tx(/*ok=*/true);
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Read call succeeds w/ expected transaction ID set in pair
        remote::Pair pair;
        pair.set_txid(4);
        EXPECT_CALL(reader_writer_, Read).WillOnce(test::read_success_with(grpc_context_, pair));
        // 3. AsyncReaderWriter<remote::Cursor, remote::Pair>::WritesDone call succeeds
        EXPECT_CALL(reader_writer_, WritesDone).WillOnce(test::writes_done_success(grpc_context_));
        // 4. AsyncReaderWriter<remote::Cursor, remote::Pair>::Finish call succeeds w/ status OK
        EXPECT_CALL(reader_writer_, Finish).WillOnce(test::finish_streaming_ok(grpc_context_));

        // Execute the test preconditions:
        // open a new transaction w/ expected tx_id
        REQUIRE_NOTHROW(spawn_and_wait(remote_tx_.open()));
        REQUIRE(remote_tx_.tx_id() == 4);

        // Execute the test: closing the transaction should succeed and transaction should have zero transaction ID
        CHECK_NOTHROW(spawn_and_wait(remote_tx_.close()));
        CHECK(remote_tx_.tx_id() == 0);
    }
    SECTION("success w/ open w/ cursor in table") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncTxRaw call succeeds
        expect_request_async_tx(/*ok=*/true);
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Read call succeeds w/ expected transaction ID set in pair
        remote::Pair pair;
        pair.set_txid(4);
        EXPECT_CALL(reader_writer_, Read).Times(2).WillRepeatedly(test::read_success_with(grpc_context_, pair));
        // 3. AsyncReaderWriter<remote::Cursor, remote::Pair>::Write call succeeds
        EXPECT_CALL(reader_writer_, Write(_, _)).WillOnce(test::write_success(grpc_context_));
        // 4. AsyncReaderWriter<remote::Cursor, remote::Pair>::WritesDone call succeeds
        EXPECT_CALL(reader_writer_, WritesDone).WillOnce(test::writes_done_success(grpc_context_));
        // 5. AsyncReaderWriter<remote::Cursor, remote::Pair>::Finish call succeeds w/ status OK
        EXPECT_CALL(reader_writer_, Finish).WillOnce(test::finish_streaming_ok(grpc_context_));

        // Execute the test preconditions:
        // open a new transaction w/ expected transaction ID
        REQUIRE_NOTHROW(spawn_and_wait(remote_tx_.open()));
        REQUIRE(remote_tx_.tx_id() == 4);
        // open a cursor within such transaction
        const auto cursor = spawn_and_wait(remote_tx_.cursor("table1"));
        REQUIRE(cursor != nullptr);

        // Execute the test: closing the transaction should succeed and transaction should have zero transaction ID
        CHECK_NOTHROW(spawn_and_wait(remote_tx_.close()));
        CHECK(remote_tx_.tx_id() == 0);
    }
    SECTION("failure in write") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncTxRaw call succeeds
        expect_request_async_tx(/*ok=*/true);
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Read call succeeds w/ expected transaction ID set in pair
        remote::Pair pair;
        pair.set_txid(4);
        EXPECT_CALL(reader_writer_, Read).WillOnce(test::read_success_with(grpc_context_, pair));
        // 3. AsyncReaderWriter<remote::Cursor, remote::Pair>::WritesDone call fails
        EXPECT_CALL(reader_writer_, WritesDone).WillOnce(test::writes_done_failure(grpc_context_));
        // 4. AsyncReaderWriter<remote::Cursor, remote::Pair>::Finish call succeeds w/ status cancelled
        EXPECT_CALL(reader_writer_, Finish).WillOnce(test::finish_streaming_cancelled(grpc_context_));

        // Execute the test preconditions:
        // open a new transaction w/ expected transaction ID
        REQUIRE_NOTHROW(spawn_and_wait(remote_tx_.open()));
        REQUIRE(remote_tx_.tx_id() == 4);

        // Execute the test: closing the transaction should raise an exception w/ expected gRPC status code
        CHECK_THROWS_MATCHES(spawn_and_wait(remote_tx_.close()), asio::system_error, test::exception_has_cancelled_grpc_status_code());
    }
    SECTION("failure in finish") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncTxRaw call succeeds
        expect_request_async_tx(/*ok=*/true);
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Read call succeeds w/ expected transaction ID set in pair
        remote::Pair pair;
        pair.set_txid(4);
        EXPECT_CALL(reader_writer_, Read).WillOnce(test::read_success_with(grpc_context_, pair));
        // 4. AsyncReaderWriter<remote::Cursor, remote::Pair>::WritesDone call succeeds
        EXPECT_CALL(reader_writer_, WritesDone).WillOnce(test::writes_done_success(grpc_context_));
        // 4. AsyncReaderWriter<remote::Cursor, remote::Pair>::Finish call fails
        EXPECT_CALL(reader_writer_, Finish).WillOnce(test::finish_streaming_error(grpc_context_));

        // Execute the test preconditions:
        // open a new transaction w/ expected transaction ID
        REQUIRE_NOTHROW(spawn_and_wait(remote_tx_.open()));
        REQUIRE(remote_tx_.tx_id() == 4);

        // Execute the test: closing the transaction should raise an exception w/ expected gRPC status code
        CHECK_THROWS_MATCHES(spawn_and_wait(remote_tx_.close()), asio::system_error, test::exception_has_unknown_grpc_status_code());
    }
}

TEST_CASE_METHOD(RemoteTransactionTest, "RemoteTransaction2::cursor", "[silkrpc][ethdb][kv][remote_transaction]") {
    SECTION("success") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncTxRaw call succeeds
        expect_request_async_tx(/*ok=*/true);
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Read calls succeed w/ specified transaction and cursor IDs
        remote::Pair txid_pair;
        txid_pair.set_txid(4);
        remote::Pair cursorid_pair;
        cursorid_pair.set_cursorid(0x23);
        EXPECT_CALL(reader_writer_, Read)
            .WillOnce(test::read_success_with(grpc_context_, txid_pair))
            .WillOnce(test::read_success_with(grpc_context_, cursorid_pair));
        // 3. AsyncReaderWriter<remote::Cursor, remote::Pair>::Write call succeeds
        EXPECT_CALL(reader_writer_, Write(_, _)).WillOnce(test::write_success(grpc_context_));

        // Execute the test preconditions:
        // open a new transaction w/ expected transaction ID
        REQUIRE_NOTHROW(spawn_and_wait(remote_tx_.open()));
        REQUIRE(remote_tx_.tx_id() == 4);

        // Execute the test: opening a cursor should succeed and cursor should have expected cursor ID
        std::shared_ptr<Cursor> cursor;
        CHECK_NOTHROW(cursor = spawn_and_wait(remote_tx_.cursor("table1")));
        CHECK(cursor->cursor_id() == 0x23);
    }
    SECTION("failure in read") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncTxRaw call succeeds
        expect_request_async_tx(/*ok=*/true);
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Read 1st call succeeds w/ specified transaction ID, 2nd call fails
        remote::Pair txid_pair;
        txid_pair.set_txid(4);
        EXPECT_CALL(reader_writer_, Read)
            .WillOnce(test::read_success_with(grpc_context_, txid_pair))
            .WillOnce(test::read_failure(grpc_context_));
        // 3. AsyncReaderWriter<remote::Cursor, remote::Pair>::Write call succeeds
        EXPECT_CALL(reader_writer_, Write(_, _)).WillOnce(test::write_success(grpc_context_));
        // 4. AsyncReaderWriter<remote::Cursor, remote::Pair>::Finish call succeeds w/ status cancelled
        EXPECT_CALL(reader_writer_, Finish).WillOnce(test::finish_streaming_cancelled(grpc_context_));

        // Execute the test preconditions:
        // open a new transaction w/ expected transaction ID
        REQUIRE_NOTHROW(spawn_and_wait(remote_tx_.open()));
        REQUIRE(remote_tx_.tx_id() == 4);

        // Execute the test: opening a cursor should raise an exception w/ expected gRPC status code
        CHECK_THROWS_MATCHES(spawn_and_wait(remote_tx_.cursor("table1")), asio::system_error, test::exception_has_cancelled_grpc_status_code());
    }
    SECTION("failure in write") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncTxRaw call succeeds
        expect_request_async_tx(/*ok=*/true);
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Read call succeeds w/ specified transaction ID
        remote::Pair txid_pair;
        txid_pair.set_txid(4);
        EXPECT_CALL(reader_writer_, Read).WillOnce(test::read_success_with(grpc_context_, txid_pair));
        // 3. AsyncReaderWriter<remote::Cursor, remote::Pair>::Write call fails
        EXPECT_CALL(reader_writer_, Write(_, _)).WillOnce(test::write_failure(grpc_context_));
        // 4. AsyncReaderWriter<remote::Cursor, remote::Pair>::Finish call succeeds w/ status cancelled
        EXPECT_CALL(reader_writer_, Finish).WillOnce(test::finish_streaming_cancelled(grpc_context_));

        // Execute the test preconditions:
        // open a new transaction w/ expected transaction ID
        REQUIRE_NOTHROW(spawn_and_wait(remote_tx_.open()));
        REQUIRE(remote_tx_.tx_id() == 4);

        // Execute the test: opening a cursor should raise an exception w/ expected gRPC status code
        CHECK_THROWS_MATCHES(spawn_and_wait(remote_tx_.cursor("table1")), asio::system_error, test::exception_has_cancelled_grpc_status_code());
    }
}

} // namespace silkrpc::ethdb::kv
