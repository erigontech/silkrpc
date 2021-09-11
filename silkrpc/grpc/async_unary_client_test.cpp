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

#include "async_unary_client.hpp"

#include <catch2/catch.hpp>
#include <gmock/gmock.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/interfaces/remote/ethbackend.grpc.pb.h>
#include <silkrpc/interfaces/remote/ethbackend_mock_fix24351.grpc.pb.h>

namespace grpc {

inline bool operator==(const Status& lhs, const Status& rhs) {
    return lhs.error_code() == rhs.error_code() &&
        lhs.error_message() == rhs.error_message() &&
        lhs.error_details() == rhs.error_details();
}

} // namespace grpc

namespace types {

inline bool operator==(const H128& lhs, const H128& rhs) {
    return lhs.hi() == rhs.hi() && lhs.lo() == rhs.lo();
}

inline bool operator==(const H160& lhs, const H160& rhs) {
    return lhs.hi() == rhs.hi() && lhs.lo() == rhs.lo();
}

} // namespace types

namespace remote {

inline bool operator==(const EtherbaseReply& lhs, const EtherbaseReply& rhs) {
    return lhs.address() == rhs.address();
}

} // namespace remote

namespace silkrpc {

using Catch::Matchers::Message;
using testing::AtLeast;
using testing::MockFunction;
using testing::Return;
using testing::_;

TEST_CASE("create async unary client", "[silkrpc][grpc][async_unary_client]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    class MockClientAsyncEtherbaseReader : public grpc::ClientAsyncResponseReaderInterface<remote::EtherbaseReply> {
    public:
        MockClientAsyncEtherbaseReader(remote::EtherbaseReply&& msg, const ::grpc::Status& status) : msg_(std::move(msg)), status_(status) {}
        ~MockClientAsyncEtherbaseReader() override = default;
        void StartCall() override {};
        void ReadInitialMetadata(void* tag) override {};
        void Finish(remote::EtherbaseReply* msg, ::grpc::Status* status, void* tag) override {
            *msg = msg_;
            *status = status_;
        };
    private:
        const remote::EtherbaseReply& msg_;
        const grpc::Status& status_;
    };

    SECTION("start async Etherbase call and get OK result") {
        using EtherbaseClient = AsyncUnaryClient<
            ::remote::ETHBACKEND::StubInterface,
            ::remote::EtherbaseRequest,
            ::remote::EtherbaseReply,
            &::remote::ETHBACKEND::StubInterface::PrepareAsyncEtherbase
        >;
        std::unique_ptr<::remote::ETHBACKEND::StubInterface> stub{std::make_unique<::remote::FixIssue24351_MockETHBACKENDStub>()};
        grpc::CompletionQueue queue;
        EtherbaseClient client{stub, &queue};

        MockClientAsyncEtherbaseReader mock_reader{remote::EtherbaseReply{}, ::grpc::Status::OK};
        EXPECT_CALL(*dynamic_cast<::remote::FixIssue24351_MockETHBACKENDStub*>(stub.get()), PrepareAsyncEtherbaseRaw(_, _, _)).WillOnce(Return(&mock_reader));

        MockFunction<void(::grpc::Status, ::remote::EtherbaseReply)> mock_callback;
        EXPECT_CALL(mock_callback, Call(grpc::Status::OK, remote::EtherbaseReply{}));

        client.async_call(::remote::EtherbaseRequest{}, mock_callback.AsStdFunction());
        client.completed(true);
    }

    SECTION("start async Etherbase call and get KO result") {
        using EtherbaseClient = AsyncUnaryClient<
            ::remote::ETHBACKEND::StubInterface,
            ::remote::EtherbaseRequest,
            ::remote::EtherbaseReply,
            &::remote::ETHBACKEND::StubInterface::PrepareAsyncEtherbase
        >;
        std::unique_ptr<::remote::ETHBACKEND::StubInterface> stub{std::make_unique<::remote::FixIssue24351_MockETHBACKENDStub>()};
        grpc::CompletionQueue queue;
        EtherbaseClient client{stub, &queue};

        MockClientAsyncEtherbaseReader mock_reader{remote::EtherbaseReply{}, ::grpc::Status{::grpc::StatusCode::INTERNAL, "internal error"}};
        EXPECT_CALL(*dynamic_cast<::remote::FixIssue24351_MockETHBACKENDStub*>(stub.get()), PrepareAsyncEtherbaseRaw(_, _, _)).WillOnce(Return(&mock_reader));

        MockFunction<void(::grpc::Status, ::remote::EtherbaseReply)> mock_callback;
        EXPECT_CALL(mock_callback, Call(::grpc::Status{::grpc::StatusCode::INTERNAL, "internal error"}, remote::EtherbaseReply{}));

        client.async_call(::remote::EtherbaseRequest{}, mock_callback.AsStdFunction());
        client.completed(false);
    }
}

} // namespace silkrpc

