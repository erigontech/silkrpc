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

#include "tx_streaming_client.hpp"

#include <future>
#include <system_error>

#include <catch2/catch.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/interfaces/remote/kv.grpc.pb.h>
#include <silkrpc/interfaces/remote/kv_mock_fix24351.grpc.pb.h>

namespace grpc {

inline bool operator==(const Status& lhs, const Status& rhs) {
    return lhs.error_code() == rhs.error_code() &&
        lhs.error_message() == rhs.error_message() &&
        lhs.error_details() == rhs.error_details();
}

} // namespace grpc

namespace remote {

inline bool operator==(const Pair& lhs, const Pair& rhs) {
    return lhs.k() == rhs.k() && lhs.v() == rhs.v() ||
        lhs.txid() == rhs.txid() ||
        lhs.cursorid() == rhs.cursorid();
}

} // namespace remote

namespace silkrpc::ethdb::kv {

using Catch::Matchers::Message;
using testing::MockFunction;
using testing::Return;
using testing::_;

class MockClientAsyncRW_OK : public grpc::ClientAsyncReaderWriterInterface<remote::Cursor, remote::Pair> {
public:
    void StartCall(void* tag) override {}
    void ReadInitialMetadata(void* tag) override {}
    void Read(remote::Pair* pair, void* tag) override {
        pair->set_k("0001");
        pair->set_v("0002");
    }
    void Write(const remote::Cursor& msg, void* tag) override {}
    void Write(const remote::Cursor& msg, grpc::WriteOptions options, void* tag) override {}
    void WritesDone(void* tag) override {}
    void Finish(grpc::Status* status, void* tag) override {
        *status = grpc::Status::OK;
    }
};

class MockClientAsyncRW_KO : public grpc::ClientAsyncReaderWriterInterface<remote::Cursor, remote::Pair> {
public:
    void StartCall(void* tag) override {}
    void ReadInitialMetadata(void* tag) override {}
    void Read(remote::Pair* pair, void* tag) override {}
    void Write(const remote::Cursor& msg, void* tag) override {}
    void Write(const remote::Cursor& msg, grpc::WriteOptions options, void* tag) override {}
    void WritesDone(void* tag) override {}
    void Finish(grpc::Status* status, void* tag) override {
        *status = grpc::Status::CANCELLED;
    }
};

TEST_CASE("TxStreamingClient::start_call", "[silkrpc][ethdb][kv][tx_streaming_client]") {
    SECTION("success") {
        std::unique_ptr<remote::KV::StubInterface> stub{std::make_unique<remote::FixIssue24351_MockKVStub>()};

        EXPECT_CALL(
            *dynamic_cast<remote::FixIssue24351_MockKVStub*>(stub.get()),
            PrepareAsyncTxRaw(_, _)).WillOnce(Return(new MockClientAsyncRW_OK);
        grpc::CompletionQueue queue;
        TxStreamingClient client{stub, &queue};

        MockFunction<void(grpc::Status)> mock_start_callback;
        EXPECT_CALL(mock_start_callback, Call(grpc::Status::OK));
        client.start_call(mock_start_callback.AsStdFunction());
        client.completed(true); // successful completion of StartCall API
    }

    SECTION("failure") {
        std::unique_ptr<remote::KV::StubInterface> stub{std::make_unique<remote::FixIssue24351_MockKVStub>()};

        EXPECT_CALL(
            *dynamic_cast<remote::FixIssue24351_MockKVStub*>(stub.get()),
            PrepareAsyncTxRaw(_, _)).WillOnce(Return(new MockClientAsyncRW_KO));
        grpc::CompletionQueue queue;
        TxStreamingClient client{stub, &queue};

        MockFunction<void(grpc::Status)> mock_start_callback;
        EXPECT_CALL(mock_start_callback, Call(grpc::Status::CANCELLED));
        client.start_call(mock_start_callback.AsStdFunction());
        client.completed(false); // failed completion of StartCall API
        client.completed(true); // successful completion of Finish API
    }
}

TEST_CASE("TxStreamingClient::read_start", "[silkrpc][ethdb][kv][tx_streaming_client]") {
    SECTION("success") {
        std::unique_ptr<remote::KV::StubInterface> stub{std::make_unique<remote::FixIssue24351_MockKVStub>()};

        EXPECT_CALL(
            *dynamic_cast<remote::FixIssue24351_MockKVStub*>(stub.get()),
            PrepareAsyncTxRaw(_, _)).WillOnce(Return(new MockClientAsyncRW_OK));
        grpc::CompletionQueue queue;
        TxStreamingClient client{stub, &queue};

        MockFunction<void(grpc::Status)> mock_start_callback;
        EXPECT_CALL(mock_start_callback, Call(grpc::Status::OK));
        client.start_call(mock_start_callback.AsStdFunction());
        client.completed(true); // successful completion of StartCall API

        MockFunction<void(grpc::Status, remote::Pair)> mock_read_callback;
        remote::Pair kv_pair{};
        kv_pair.set_k("0001");
        kv_pair.set_v("0002");
        EXPECT_CALL(mock_read_callback, Call(grpc::Status::OK, kv_pair));
        client.read_start(mock_read_callback.AsStdFunction());
        client.completed(true); // successful completion of Read API
    }

    SECTION("failure") {
        std::unique_ptr<remote::KV::StubInterface> stub{std::make_unique<remote::FixIssue24351_MockKVStub>()};

        EXPECT_CALL(
            *dynamic_cast<remote::FixIssue24351_MockKVStub*>(stub.get()),
            PrepareAsyncTxRaw(_, _)).WillOnce(Return(new MockClientAsyncRW_KO));
        grpc::CompletionQueue queue;
        TxStreamingClient client{stub, &queue};

        MockFunction<void(grpc::Status)> mock_start_callback;
        EXPECT_CALL(mock_start_callback, Call(grpc::Status::OK));
        client.start_call(mock_start_callback.AsStdFunction());
        client.completed(true); // successful completion of StartCall API

        MockFunction<void(grpc::Status, remote::Pair)> mock_read_callback;
        EXPECT_CALL(mock_read_callback, Call(grpc::Status::CANCELLED, remote::Pair{}));
        client.read_start(mock_read_callback.AsStdFunction());
        client.completed(false); // failed completion of Read API
        client.completed(true); // successful completion of Finish API
    }
}

TEST_CASE("TxStreamingClient::write_start", "[silkrpc][ethdb][kv][tx_streaming_client]") {
    SECTION("success") {
        std::unique_ptr<remote::KV::StubInterface> stub{std::make_unique<remote::FixIssue24351_MockKVStub>()};

        EXPECT_CALL(
            *dynamic_cast<remote::FixIssue24351_MockKVStub*>(stub.get()),
            PrepareAsyncTxRaw(_, _)).WillOnce(Return(new MockClientAsyncRW_OK));
        grpc::CompletionQueue queue;
        TxStreamingClient client{stub, &queue};

        MockFunction<void(grpc::Status)> mock_start_callback;
        EXPECT_CALL(mock_start_callback, Call(grpc::Status::OK));
        client.start_call(mock_start_callback.AsStdFunction());
        client.completed(true); // successful completion of StartCall API

        MockFunction<void(grpc::Status)> mock_write_callback;
        remote::Cursor cursor{};
        cursor.set_k("0001");
        cursor.set_v("0002");
        EXPECT_CALL(mock_write_callback, Call(grpc::Status::OK));
        client.write_start(cursor, mock_write_callback.AsStdFunction());
        client.completed(true); // successful completion of Write API
    }

    SECTION("failure") {
        std::unique_ptr<remote::KV::StubInterface> stub{std::make_unique<remote::FixIssue24351_MockKVStub>()};

        EXPECT_CALL(
            *dynamic_cast<remote::FixIssue24351_MockKVStub*>(stub.get()),
            PrepareAsyncTxRaw(_, _)).WillOnce(Return(new MockClientAsyncRW_KO));
        grpc::CompletionQueue queue;
        TxStreamingClient client{stub, &queue};

        MockFunction<void(grpc::Status)> mock_start_callback;
        EXPECT_CALL(mock_start_callback, Call(grpc::Status::OK));
        client.start_call(mock_start_callback.AsStdFunction());
        client.completed(true); // successful completion of StartCall API

        MockFunction<void(grpc::Status)> mock_write_callback;
        remote::Cursor cursor{};
        cursor.set_k("0001");
        cursor.set_v("0002");
        EXPECT_CALL(mock_write_callback, Call(grpc::Status::CANCELLED));
        client.write_start(cursor, mock_write_callback.AsStdFunction());
        client.completed(false); // failed completion of Write API
        client.completed(true); // successful completion of Finish API
    }
}

TEST_CASE("TxStreamingClient::end_call", "[silkrpc][ethdb][kv][tx_streaming_client]") {
    SECTION("success") {
        std::unique_ptr<remote::KV::StubInterface> stub{std::make_unique<remote::FixIssue24351_MockKVStub>()};

        EXPECT_CALL(
            *dynamic_cast<remote::FixIssue24351_MockKVStub*>(stub.get()),
            PrepareAsyncTxRaw(_, _)).WillOnce(Return(new MockClientAsyncRW_OK));
        grpc::CompletionQueue queue;
        TxStreamingClient client{stub, &queue};

        MockFunction<void(grpc::Status)> mock_start_callback;
        EXPECT_CALL(mock_start_callback, Call(grpc::Status::OK));
        client.start_call(mock_start_callback.AsStdFunction());
        client.completed(true); // successful completion of StartCall API

        MockFunction<void(grpc::Status)> mock_end_callback;
        EXPECT_CALL(mock_end_callback, Call(grpc::Status::OK));
        client.end_call(mock_end_callback.AsStdFunction());
        client.completed(true); // successful completion of WritesDone API
        client.completed(true); // successful completion of Finish API
    }

    SECTION("failure") {
        std::unique_ptr<remote::KV::StubInterface> stub{std::make_unique<remote::FixIssue24351_MockKVStub>()};

        EXPECT_CALL(
            *dynamic_cast<remote::FixIssue24351_MockKVStub*>(stub.get()),
            PrepareAsyncTxRaw(_, _)).WillOnce(Return(new MockClientAsyncRW_KO));
        grpc::CompletionQueue queue;
        TxStreamingClient client{stub, &queue};

        MockFunction<void(grpc::Status)> mock_start_callback;
        EXPECT_CALL(mock_start_callback, Call(grpc::Status::OK));
        client.start_call(mock_start_callback.AsStdFunction());
        client.completed(true); // successful completion of StartCall API

        MockFunction<void(grpc::Status)> mock_end_callback;
        EXPECT_CALL(mock_end_callback, Call(grpc::Status::CANCELLED));
        client.end_call(mock_end_callback.AsStdFunction());
        client.completed(false); // failed completion of WritesDone API
        client.completed(true); // successful completion of Finish API
    }
}

} // namespace silkrpc::ethdb::kv
