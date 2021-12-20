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

#include "miner.hpp"

#include <functional>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>

#include <asio/io_context.hpp>
#include <asio/co_spawn.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch.hpp>
#include <evmc/evmc.hpp>
#include <gmock/gmock.h>
#include <grpcpp/grpcpp.h>
#include <silkworm/common/base.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/completion_runner.hpp>
#include <silkrpc/interfaces/txpool/mining.grpc.pb.h>
#include <silkrpc/interfaces/txpool/mining_mock_fix24351.grpc.pb.h>

namespace grpc {

inline bool operator==(const Status& lhs, const Status& rhs) {
    return lhs.error_code() == rhs.error_code() &&
        lhs.error_message() == rhs.error_message() &&
        lhs.error_details() == rhs.error_details();
}

} // namespace grpc

namespace txpool {

inline bool operator==(const GetWorkReply& lhs, const GetWorkReply& rhs) {
    if (lhs.headerhash() != rhs.headerhash()) return false;
    if (lhs.seedhash() != rhs.seedhash()) return false;
    if (lhs.target() != rhs.target()) return false;
    if (lhs.blocknumber() != rhs.blocknumber()) return false;
    return true;
}

} // namespace txpool

namespace silkrpc::txpool {

using Catch::Matchers::Message;
using testing::MockFunction;
using testing::Return;
using testing::_;

using evmc::literals::operator""_bytes32;

TEST_CASE("create GetWorkClient", "[silkrpc][txpool][miner]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    class MockClientAsyncGetWorkReader final : public grpc::ClientAsyncResponseReaderInterface<::txpool::GetWorkReply> {
    public:
        MockClientAsyncGetWorkReader(::txpool::GetWorkReply msg, ::grpc::Status status) : msg_(std::move(msg)), status_(std::move(status)) {}
        ~MockClientAsyncGetWorkReader() final = default;
        void StartCall() override {};
        void ReadInitialMetadata(void* tag) override {};
        void Finish(::txpool::GetWorkReply* msg, ::grpc::Status* status, void* tag) override {
            *msg = msg_;
            *status = status_;
        };
    private:
        ::txpool::GetWorkReply msg_;
        grpc::Status status_;
    };

    SECTION("start async GetWork call, get status OK and matching work") {
        std::unique_ptr<::txpool::Mining::StubInterface> stub{std::make_unique<::txpool::FixIssue24351_MockMiningStub>()};
        grpc::CompletionQueue queue;
        txpool::GetWorkClient client{stub, &queue};

        ::txpool::GetWorkReply reply;
        reply.set_headerhash("0x209f062567c161c5f71b3f57a7de277b0e95c3455050b152d785ad7524ef8ee7");
        reply.set_seedhash("0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347");
        reply.set_target("0xe7536c5b61ed0e0ab7f3ce7f085806d40f716689c0c086676757de401b595658");
        reply.set_blocknumber("0x00000000");
        MockClientAsyncGetWorkReader mock_reader{reply, ::grpc::Status::OK};
        EXPECT_CALL(*dynamic_cast<::txpool::FixIssue24351_MockMiningStub*>(stub.get()), PrepareAsyncGetWorkRaw(_, _, _)).WillOnce(Return(&mock_reader));

        MockFunction<void(::grpc::Status, ::txpool::GetWorkReply)> mock_callback;
        EXPECT_CALL(mock_callback, Call(grpc::Status::OK, reply));

        client.async_call(::txpool::GetWorkRequest{}, mock_callback.AsStdFunction());
        client.completed(true);
    }

    /*SECTION("start async Add call, get status OK and import failure") {
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
    }*/
}

} // namespace silkrpc::txpool
