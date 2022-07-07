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

#include "remote_database.hpp"

#include <memory>

#include <asio/system_error.hpp>
#include <catch2/catch.hpp>

#include <silkrpc/test/kv_test_base.hpp>
#include <silkrpc/test/grpc_responder.hpp>
#include <silkrpc/test/grpc_actions.hpp>
#include <silkrpc/test/grpc_matcher.hpp>

namespace silkrpc::ethdb::kv {

struct RemoteDatabaseTest : test::KVTestBase {
    // RemoteDatabase holds the KV stub by std::unique_ptr so we cannot rely on mock stub from base class
    StrictMockKVStub* kv_stub_ = new StrictMockKVStub;
    RemoteDatabase remote_db_{grpc_context_, std::unique_ptr<StrictMockKVStub>{kv_stub_}};
};

TEST_CASE_METHOD(RemoteDatabaseTest, "RemoteDatabase::begin", "[silkrpc][ethdb][kv][remote_database]") {
    using namespace testing;  // NOLINT(build/namespaces)

    SECTION("success") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncTxRaw call succeeds
        EXPECT_CALL(*kv_stub_, AsyncTxRaw).WillOnce([&](auto&&, auto&&, void* tag) {
            agrpc::process_grpc_tag(grpc_context_, tag, /*ok=*/true);
            return reader_writer_ptr_.release();
        });
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Read call succeeds
        remote::Pair pair;
        pair.set_txid(4);
        EXPECT_CALL(reader_writer_, Read).WillOnce(test::read_success_with(grpc_context_, pair));

        // Execute the test: RemoteDatabase::begin should return transaction w/ expected tx_id
        const auto txn = spawn_and_wait(remote_db_.begin());
        CHECK(txn->tx_id() == 4);
    }

    SECTION("open failure") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncTxRaw call fails
        EXPECT_CALL(*kv_stub_, AsyncTxRaw).WillOnce([&](auto&&, auto&&, void* tag) {
            agrpc::process_grpc_tag(grpc_context_, tag, /*ok=*/false);
            return reader_writer_ptr_.release();
        });
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Finish call succeeds w/ status cancelled
        EXPECT_CALL(reader_writer_, Finish).WillOnce(test::finish_streaming_with_status(grpc_context_, grpc::Status::CANCELLED));

        // Execute the test: RemoteDatabase::begin should raise an exception w/ expected gRPC status code
        CHECK_THROWS_MATCHES(spawn_and_wait(remote_db_.begin()),
            asio::system_error,
            test::exception_has_cancelled_grpc_status_code());
    }

    SECTION("read failure") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncTxRaw call succeeds
        EXPECT_CALL(*kv_stub_, AsyncTxRaw).WillOnce([&](auto&&, auto&&, void* tag) {
            agrpc::process_grpc_tag(grpc_context_, tag, /*ok=*/true);
            return reader_writer_ptr_.release();
        });
        // 2. AsyncReaderWriter<remote::Cursor, remote::Pair>::Read call fails
        EXPECT_CALL(reader_writer_, Read).WillOnce([&](auto* , void* tag) {
            agrpc::process_grpc_tag(grpc_context_, tag, /*ok=*/false);
        });
        // 3. AsyncReaderWriter<remote::Cursor, remote::Pair>::Finish call succeeds w/ status cancelled
        EXPECT_CALL(reader_writer_, Finish).WillOnce(test::finish_streaming_with_status(grpc_context_, grpc::Status::CANCELLED));

        // Execute the test: RemoteDatabase::begin should raise an exception w/ expected gRPC status code
        CHECK_THROWS_MATCHES(spawn_and_wait(remote_db_.begin()),
            asio::system_error,
            test::exception_has_cancelled_grpc_status_code());
    }
}

} // namespace silkrpc::ethdb::kv
