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
#include <evmc/evmc.hpp>
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

using evmc::literals::operator""_bytes32;

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
    ::grpc::Status Transactions(::grpc::ServerContext* context, const ::txpool::TransactionsRequest* request, ::txpool::TransactionsReply* response) override {
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
auto test_get_transaction = test_comethod<&txpool::TransactionPool::get_transaction, std::optional<silkworm::Bytes>, evmc::bytes32>;

TEST_CASE("create TransactionPool", "[silkrpc][txpool][transaction_pool]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("call add_transaction and check import success") {
        class TestSuccessTxpoolService : public ::txpool::Txpool::Service {
        public:
            ::grpc::Status Add(::grpc::ServerContext* context, const ::txpool::AddRequest* request, ::txpool::AddReply* response) override {
                response->add_imported(::txpool::ImportResult::SUCCESS);
                return ::grpc::Status::OK;
            }
        };
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
        class TestFailureErrorTxpoolService : public ::txpool::Txpool::Service {
        public:
            ::grpc::Status Add(::grpc::ServerContext* context, const ::txpool::AddRequest* request, ::txpool::AddReply* response) override {
                response->add_imported(::txpool::ImportResult::INVALID);
                response->add_errors("invalid transaction");
                return ::grpc::Status::OK;
            }
        };
        TestFailureErrorTxpoolService service;
        asio::io_context io_context;
        auto success{asio::co_spawn(io_context, test_add_transaction(&service, silkworm::Bytes{0x00, 0x01}), asio::use_future)};
        io_context.run();
        CHECK(success.get() == false);
    }

    SECTION("call add_transaction and check import failure [import no error]") {
        class TestFailureNoErrorTxpoolService : public ::txpool::Txpool::Service {
        public:
            ::grpc::Status Add(::grpc::ServerContext* context, const ::txpool::AddRequest* request, ::txpool::AddReply* response) override {
                response->add_imported(::txpool::ImportResult::INTERNAL_ERROR);
                return ::grpc::Status::OK;
            }
        };
        TestFailureNoErrorTxpoolService service;
        asio::io_context io_context;
        auto success{asio::co_spawn(io_context, test_add_transaction(&service, silkworm::Bytes{0x00, 0x01}), asio::use_future)};
        io_context.run();
        CHECK(success.get() == false);
    }

    SECTION("call get_transaction and check result") {
        class TestSuccessTxpoolService : public ::txpool::Txpool::Service {
        public:
            ::grpc::Status Transactions(::grpc::ServerContext* context, const ::txpool::TransactionsRequest* request, ::txpool::TransactionsReply* response) override {
                response->add_rlptxs("0804");
                return ::grpc::Status::OK;
            }
        };
        TestSuccessTxpoolService service;
        asio::io_context io_context;
        const auto tx_hash{0x3763e4f6e4198413383534c763f3f5dac5c5e939f0a81724e3beb96d6e2ad0d5_bytes32};
        auto tx_rlp{asio::co_spawn(io_context, test_get_transaction(&service, tx_hash), asio::use_future)};
        io_context.run();
        CHECK(tx_rlp.get() == silkworm::Bytes{0x30, 0x38, 0x30, 0x34});
    }

    SECTION("call get_transaction and check result is null [rlptxs size is 0]") {
        EmptyTxpoolService service;
        asio::io_context io_context;
        auto tx_rlp{asio::co_spawn(io_context, test_get_transaction(&service, evmc::bytes32{}), asio::use_future)};
        io_context.run();
        CHECK(tx_rlp.get() == std::nullopt);
    }

    SECTION("call get_transaction and check result is null [rlptxs size is greater than 1]") {
        class TestTooManyRlpTxsTxpoolService : public ::txpool::Txpool::Service {
        public:
            ::grpc::Status Transactions(::grpc::ServerContext* context, const ::txpool::TransactionsRequest* request, ::txpool::TransactionsReply* response) override {
                response->add_rlptxs("0804");
                response->add_rlptxs("0905");
                return ::grpc::Status::OK;
            }
        };
        TestTooManyRlpTxsTxpoolService service;
        asio::io_context io_context;
        const auto tx_hash{0x3763e4f6e4198413383534c763f3f5dac5c5e939f0a81724e3beb96d6e2ad0d5_bytes32};
        auto tx_rlp{asio::co_spawn(io_context, test_get_transaction(&service, tx_hash), asio::use_future)};
        io_context.run();
        CHECK(tx_rlp.get() == std::nullopt);
    }
}

} // namespace silkrpc
