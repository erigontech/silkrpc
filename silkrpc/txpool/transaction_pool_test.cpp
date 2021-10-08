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

#include <functional>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>

#include <asio/io_context.hpp>
#include <asio/co_spawn.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch.hpp>
#include <gmock/gmock.h>
#include <grpcpp/grpcpp.h>
#include <silkworm/common/base.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/completion_runner.hpp>
#include <silkrpc/interfaces/txpool/txpool.grpc.pb.h>
#include <silkrpc/interfaces/txpool/txpool_mock_fix24351.grpc.pb.h>

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

TEST_CASE("create AddClient", "[silkrpc][txpool][transaction_pool]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    class MockClientAsyncAddReader final : public grpc::ClientAsyncResponseReaderInterface<::txpool::AddReply> {
    public:
        MockClientAsyncAddReader(::txpool::AddReply msg, ::grpc::Status status) : msg_(std::move(msg)), status_(std::move(status)) {}
        ~MockClientAsyncAddReader() final = default;
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

class EmptyTxpoolService : public ::txpool::Txpool::Service {
public:
    ::grpc::Status Add(::grpc::ServerContext* context, const ::txpool::AddRequest* request, ::txpool::AddReply* response) override {
        return ::grpc::Status::OK;
    }
};

class TestSuccessTxpoolService : public ::txpool::Txpool::Service {
public:
    ::grpc::Status Add(::grpc::ServerContext* context, const ::txpool::AddRequest* request, ::txpool::AddReply* response) override {
        response->add_imported(::txpool::ImportResult::SUCCESS);
        return ::grpc::Status::OK;
    }
};

class TestFailureErrorTxpoolService : public ::txpool::Txpool::Service {
public:
    ::grpc::Status Add(::grpc::ServerContext* context, const ::txpool::AddRequest* request, ::txpool::AddReply* response) override {
        response->add_imported(::txpool::ImportResult::INVALID);
        response->add_errors("invalid transaction");
        return ::grpc::Status::OK;
    }
};

class TestFailureNoErrorTxpoolService : public ::txpool::Txpool::Service {
public:
    ::grpc::Status Add(::grpc::ServerContext* context, const ::txpool::AddRequest* request, ::txpool::AddReply* response) override {
        response->add_imported(::txpool::ImportResult::INTERNAL_ERROR);
        return ::grpc::Status::OK;
    }
};

template<auto mf, typename T>
auto make_method_proxy(T&& obj) {
    return [&obj](auto&&... args) {
        return (std::forward<T>(obj).*mf)(std::forward<decltype(args)>(args)...);
    };
}

template<auto mf, typename R, typename ...Args>
asio::awaitable<R> test_comethod(::txpool::Txpool::Service* service, Args... args) {
    std::ostringstream server_address;
    server_address << "localhost:" << 12345; // TODO(canepat): grpc_pick_unused_port_or_die
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address.str(), grpc::InsecureServerCredentials());
    builder.RegisterService(service);
    const auto server_ptr = builder.BuildAndStart();
    asio::io_context io_context;
    asio::io_context::work work{io_context};
    grpc::CompletionQueue queue;
    CompletionRunner completion_runner{queue, io_context};
    auto io_context_thread = std::thread([&]() { io_context.run(); });
    auto completion_runner_thread = std::thread([&]() { completion_runner.run(); });
    const auto channel = grpc::CreateChannel(server_address.str(), grpc::InsecureChannelCredentials());
    txpool::TransactionPool transaction_pool{io_context, channel, &queue};
    auto method_proxy{make_method_proxy<mf, silkrpc::txpool::TransactionPool>(std::move(transaction_pool))};
    const auto result = co_await method_proxy(args...);
    server_ptr->Shutdown();
    io_context.stop();
    completion_runner.stop();
    if (io_context_thread.joinable()) {
        io_context_thread.join();
    }
    if (completion_runner_thread.joinable()) {
        completion_runner_thread.join();
    }
    co_return result;
}

auto test_add_transaction = test_comethod<&txpool::TransactionPool::add_transaction, bool, silkworm::ByteView>;

TEST_CASE("create TransactionPool", "[silkrpc][txpool][transaction_pool]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("call add_transaction and check import success") {
        TestSuccessTxpoolService service;
        asio::io_context io_context;
        auto success{asio::co_spawn(io_context, test_add_transaction(&service, silkworm::Bytes{0x00, 0x01}), asio::use_future)};
        io_context.run();
        CHECK(success.get() == true);
    }

    SECTION("call add_transaction and check import failure [unexpected import size]") {
        EmptyTxpoolService service;
        asio::io_context io_context;
        auto success{asio::co_spawn(io_context, test_add_transaction(&service, silkworm::Bytes{0x00, 0x01}), asio::use_future)};
        io_context.run();
        CHECK(success.get() == false);
    }

    SECTION("call add_transaction and check import failure [import error]") {
        TestFailureErrorTxpoolService service;
        asio::io_context io_context;
        auto success{asio::co_spawn(io_context, test_add_transaction(&service, silkworm::Bytes{0x00, 0x01}), asio::use_future)};
        io_context.run();
        CHECK(success.get() == false);
    }

    SECTION("call add_transaction and check import failure [import no error]") {
        TestFailureNoErrorTxpoolService service;
        asio::io_context io_context;
        auto success{asio::co_spawn(io_context, test_add_transaction(&service, silkworm::Bytes{0x00, 0x01}), asio::use_future)};
        io_context.run();
        CHECK(success.get() == false);
    }
}

} // namespace silkrpc
