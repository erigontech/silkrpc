/*
   Copyright 2020-2021 The Silkrpc Authors

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

#include "transaction_pool.hpp"

#include <memory>

#include <catch2/catch.hpp>
#include <gmock/gmock.h>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/interfaces/txpool/txpool.grpc.pb.h>
#include <silkrpc/interfaces/txpool/txpool_mock_fix24351.grpc.pb.h>

//extern inline bool operator==(const grpc::Status& lhs, const grpc::Status& rhs);
namespace grpc {

inline bool operator==(const Status& lhs, const Status& rhs) {
    return lhs.error_code() == rhs.error_code() &&
        lhs.error_message() == rhs.error_message() &&
        lhs.error_details() == rhs.error_details();
}

} // namespace grpc

namespace txpool {

inline bool operator==(const AddReply& lhs, const AddReply& rhs) {
    if (lhs.imported_size() != rhs.imported_size()) return false;
    for (auto i{0}; i < lhs.imported_size(); i++) {
        if (lhs.imported(i) != rhs.imported(i)) return false;
    }
    if (lhs.errors_size() != rhs.errors_size()) return false;
    for (auto i{0}; i < lhs.errors_size(); i++) {
        if (lhs.errors(i) != rhs.errors(i)) return false;
    }
    return true;
}

} // namespace txpool

namespace silkrpc {

using Catch::Matchers::Message;
using testing::AtLeast;
using testing::MockFunction;
using testing::Return;
using testing::_;

TEST_CASE("create transaction pool", "[silkrpc][txpool][transaction_pool]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    uint64_t g_fixture_slowdown_factor = 1;
    uint64_t g_poller_slowdown_factor = 1;

    auto grpc_test_slowdown_factor = [&]() {
        return 4/*grpc_test_sanitizer_slowdown_factor()*/ * g_fixture_slowdown_factor * g_poller_slowdown_factor;
    };

    auto grpc_timeout_milliseconds_to_deadline = [&](int64_t time_ms) {
        return gpr_time_add(
            gpr_now(GPR_CLOCK_MONOTONIC),
            gpr_time_from_micros(
                grpc_test_slowdown_factor() * static_cast<int64_t>(1e3) * time_ms,
                GPR_TIMESPAN));
    };

    class MockClientAsyncAddReader : public grpc::ClientAsyncResponseReaderInterface<::txpool::AddReply> {
    public:
        MockClientAsyncAddReader(::txpool::AddReply msg, ::grpc::Status status) : msg_(msg), status_(status) {}
        ~MockClientAsyncAddReader() {}
        void StartCall() override {};
        void ReadInitialMetadata(void* tag) override {};
        void Finish(::txpool::AddReply* msg, ::grpc::Status* status, void* tag) override {
            *msg = msg_;
            *status = status_;
        };
    private:
        ::txpool::AddReply msg_;
        grpc::Status status_;
    };

    SECTION("start async Add call, get status OK and import success") {
        std::unique_ptr<::txpool::Txpool::StubInterface> stub{std::make_unique<::txpool::FixIssue24351_MockTxpoolStub>()};
        grpc::CompletionQueue queue;
        txpool::AddClient client{stub, &queue};

        ::txpool::AddReply reply;
        reply.add_imported(::txpool::ImportResult::SUCCESS);
        MockClientAsyncAddReader mock_reader{reply, ::grpc::Status::OK};
        EXPECT_CALL(*dynamic_cast<::txpool::FixIssue24351_MockTxpoolStub*>(stub.get()), PrepareAsyncAddRaw(_, _, _)).WillOnce(Return(&mock_reader));

        MockFunction<void(::grpc::Status, ::txpool::AddReply)> mock_callback;
        EXPECT_CALL(mock_callback, Call(grpc::Status::OK, reply));

        client.async_call(::txpool::AddRequest{}, mock_callback.AsStdFunction());
        client.completed(true);
    }

    SECTION("start async Add call, get status OK and import failure") {
        std::unique_ptr<::txpool::Txpool::StubInterface> stub{std::make_unique<::txpool::FixIssue24351_MockTxpoolStub>()};
        grpc::CompletionQueue queue;
        txpool::AddClient client{stub, &queue};

        ::txpool::AddReply reply;
        reply.add_imported(::txpool::ImportResult::INVALID);
        reply.add_errors("empty txn is invalid");
        MockClientAsyncAddReader mock_reader{reply, ::grpc::Status::OK};
        EXPECT_CALL(*dynamic_cast<::txpool::FixIssue24351_MockTxpoolStub*>(stub.get()), PrepareAsyncAddRaw(_, _, _)).WillOnce(Return(&mock_reader));

        MockFunction<void(::grpc::Status, ::txpool::AddReply)> mock_callback;
        EXPECT_CALL(mock_callback, Call(grpc::Status::OK, reply));

        client.async_call(::txpool::AddRequest{}, mock_callback.AsStdFunction());
        client.completed(true);
    }

    SECTION("start async Add call and get status KO") {
        std::unique_ptr<::txpool::Txpool::StubInterface> stub{std::make_unique<::txpool::FixIssue24351_MockTxpoolStub>()};
        grpc::CompletionQueue queue;
        txpool::AddClient client{stub, &queue};

        MockClientAsyncAddReader mock_reader{::txpool::AddReply{}, ::grpc::Status{::grpc::StatusCode::INTERNAL, "internal error"}};
        EXPECT_CALL(*dynamic_cast<::txpool::FixIssue24351_MockTxpoolStub*>(stub.get()), PrepareAsyncAddRaw(_, _, _)).WillOnce(Return(&mock_reader));

        MockFunction<void(::grpc::Status, ::txpool::AddReply)> mock_callback;
        EXPECT_CALL(mock_callback, Call(::grpc::Status{::grpc::StatusCode::INTERNAL, "internal error"}, ::txpool::AddReply{}));

        client.async_call(::txpool::AddRequest{}, mock_callback.AsStdFunction());
        client.completed(false);
    }
}

} // namespace silkrpc
