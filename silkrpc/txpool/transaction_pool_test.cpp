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

#include <string>
#include <utility>

#include <agrpc/test.hpp>
#include <catch2/catch.hpp>
#include <evmc/evmc.hpp>
#include <gmock/gmock.h>
#include <grpcpp/grpcpp.h>
#include <silkworm/common/base.hpp>

#include <silkrpc/concurrency/context_pool.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/interfaces/txpool/txpool.grpc.pb.h>
#include <silkrpc/interfaces/txpool/txpool_mock_fix24351.grpc.pb.h>
#include <silkrpc/test/api_test_base.hpp>
#include <silkrpc/test/grpc_actions.hpp>
#include <silkrpc/test/grpc_responder.hpp>

namespace silkrpc::txpool {

using Catch::Matchers::Message;
using testing::AtLeast;
using testing::MockFunction;
using testing::Return;
using testing::_;

using evmc::literals::operator""_address, evmc::literals::operator""_bytes32;
using StrictMockTxpoolStub = testing::StrictMock<::txpool::MockTxpoolStub>;

using TransactionPoolTest = test::GrpcApiTestBase<TransactionPool, StrictMockTxpoolStub>;

TEST_CASE_METHOD(TransactionPoolTest, "TransactionPool::add_transaction", "[silkrpc][txpool][transaction_pool]") {
    test::StrictMockAsyncResponseReader<::txpool::AddReply> reader;
    EXPECT_CALL(*stub_, AsyncAddRaw).WillOnce(testing::Return(&reader));
    const silkworm::Bytes tx_rlp{0x00, 0x01};

    SECTION("call add_transaction and check import success") {
        ::txpool::AddReply response;
        response.add_imported(::txpool::ImportResult::SUCCESS);
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_with(grpc_context_, std::move(response)));
        const auto result = run<&TransactionPool::add_transaction>(tx_rlp);
        CHECK(result.success);
    }

    SECTION("call add_transaction and check import failure [unexpected import size]") {
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_ok(grpc_context_));
        const auto result = run<&TransactionPool::add_transaction>(tx_rlp);
        CHECK(!result.success);
    }

    SECTION("call add_transaction and check import failure [invalid error]") {
        ::txpool::AddReply response;
        response.add_imported(::txpool::ImportResult::INVALID);
        response.add_errors("invalid transaction");
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_with(grpc_context_, std::move(response)));
        const auto result = run<&TransactionPool::add_transaction>(tx_rlp);
        CHECK(!result.success);
    }

    SECTION("call add_transaction and check import failure [internal error]") {
        ::txpool::AddReply response;
        response.add_imported(::txpool::ImportResult::INTERNAL_ERROR);
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_with(grpc_context_, std::move(response)));
        const auto result = run<&TransactionPool::add_transaction>(tx_rlp);
        CHECK(!result.success);
    }

    SECTION("call add_transaction and get error") {
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_cancelled(grpc_context_));
        CHECK_THROWS_AS((run<&TransactionPool::add_transaction>(tx_rlp)), boost::system::system_error);
    }
}

TEST_CASE_METHOD(TransactionPoolTest, "TransactionPool::get_transaction", "[silkrpc][txpool][transaction_pool]") {
    test::StrictMockAsyncResponseReader<::txpool::TransactionsReply> reader;
    EXPECT_CALL(*stub_, AsyncTransactionsRaw).WillOnce(testing::Return(&reader));
    const auto tx_hash{0x3763e4f6e4198413383534c763f3f5dac5c5e939f0a81724e3beb96d6e2ad0d5_bytes32};

    SECTION("call get_transaction and check success") {
        ::txpool::TransactionsReply response;
        response.add_rlptxs("0804");
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_with(grpc_context_, std::move(response)));
        const auto tx_rlp = run<&TransactionPool::get_transaction>(tx_hash);
        CHECK(tx_rlp);
        if (tx_rlp) {
            CHECK(tx_rlp.value() == silkworm::Bytes{0x30, 0x38, 0x30, 0x34});
        }
    }

    SECTION("call get_transaction and check result is null [rlptxs size is 0]") {
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_ok(grpc_context_));
        const auto tx_rlp = run<&TransactionPool::get_transaction>(tx_hash);
        CHECK(!tx_rlp);
    }

    SECTION("call get_transaction and check result is null [rlptxs size is greater than 1]") {
        ::txpool::TransactionsReply response;
        response.add_rlptxs("0804");
        response.add_rlptxs("0905");
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_with(grpc_context_, std::move(response)));
        const auto tx_rlp = run<&TransactionPool::get_transaction>(tx_hash);
        CHECK(!tx_rlp);
    }

    SECTION("call get_transaction and get error") {
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_cancelled(grpc_context_));
        CHECK_THROWS_AS((run<&TransactionPool::get_transaction>(tx_hash)), boost::system::system_error);
    }
}

TEST_CASE_METHOD(TransactionPoolTest, "TransactionPool::nonce", "[silkrpc][txpool][transaction_pool]") {
    test::StrictMockAsyncResponseReader<::txpool::NonceReply> reader;
    EXPECT_CALL(*stub_, AsyncNonceRaw).WillOnce(testing::Return(&reader));
    const auto account{0x99f9b87991262f6ba471f09758cde1c0fc1de734_address};

    SECTION("call nonce and check success") {
        ::txpool::NonceReply response;
        response.set_found(1);
        response.set_nonce(21);
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_with(grpc_context_, std::move(response)));
        const auto nonce = run<&TransactionPool::nonce>(account);
        CHECK(nonce);
        if (nonce) {
            CHECK(nonce.value() == 21);
        }
    }

    SECTION("call nonce and check result is null") {
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_ok(grpc_context_));
        const auto nonce = run<&TransactionPool::nonce>(account);
        CHECK(!nonce);
    }

    SECTION("call nonce and check result is null [not found]") {
        ::txpool::NonceReply response;
        response.set_found(0);
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_with(grpc_context_, std::move(response)));
        const auto nonce = run<&TransactionPool::nonce>(account);
        CHECK(!nonce);
    }

    SECTION("call nonce and get error") {
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_cancelled(grpc_context_));
        CHECK_THROWS_AS((run<&TransactionPool::nonce>(account)), boost::system::system_error);
    }
}

TEST_CASE_METHOD(TransactionPoolTest, "TransactionPool::get_status", "[silkrpc][txpool][transaction_pool]") {
    test::StrictMockAsyncResponseReader<::txpool::StatusReply> reader;
    EXPECT_CALL(*stub_, AsyncStatusRaw).WillOnce(testing::Return(&reader));

    SECTION("call get_status and check success") {
        ::txpool::StatusReply response;
        response.set_queuedcount(0x6);
        response.set_pendingcount(0x5);
        response.set_basefeecount(0x4);
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_with(grpc_context_, std::move(response)));
        const auto status_info = run<&TransactionPool::get_status>();
        CHECK(status_info.queued_count == 0x6);
        CHECK(status_info.pending_count == 0x5);
        CHECK(status_info.base_fee_count == 0x4);
    }

    SECTION("call get_status and check result is empty") {
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_ok(grpc_context_));
        const auto status_info = run<&TransactionPool::get_status>();
        CHECK(status_info.queued_count == 0);
        CHECK(status_info.pending_count == 0);
        CHECK(status_info.base_fee_count == 0);
    }

    SECTION("call get_status and get error") {
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_cancelled(grpc_context_));
        CHECK_THROWS_AS((run<&TransactionPool::get_status>()), boost::system::system_error);
    }
}

TEST_CASE_METHOD(TransactionPoolTest, "TransactionPool::get_transactions", "[silkrpc][txpool][transaction_pool]") {
    test::StrictMockAsyncResponseReader<::txpool::AllReply> reader;
    EXPECT_CALL(*stub_, AsyncAllRaw).WillOnce(testing::Return(&reader));

    SECTION("call get_transactions and check success [one tx]") {
        ::txpool::AllReply response;
        auto tx = response.add_txs();
        tx->set_type(::txpool::AllReply_Type_QUEUED);
        tx->set_sender("99f9b87991262f6ba471f09758cde1c0fc1de734");
        tx->set_rlptx("0804");
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_with(grpc_context_, std::move(response)));
        const auto transactions = run<&TransactionPool::get_transactions>();
        CHECK(transactions.size() == 1);
        if (transactions.size() > 0) {
            CHECK(transactions[0].transaction_type == silkrpc::txpool::TransactionType::QUEUED);
            CHECK(transactions[0].sender == 0x99f9b87991262f6ba471f09758cde1c0fc1de734_address);
            CHECK(transactions[0].rlp == silkworm::Bytes{0x30, 0x38, 0x30, 0x34});
        }
    }

    SECTION("call get_transactions and check success [more than one tx]") {
        ::txpool::AllReply response;
        auto tx = response.add_txs();
        tx->set_type(::txpool::AllReply_Type_QUEUED);
        tx->set_sender("99f9b87991262f6ba471f09758cde1c0fc1de734");
        tx->set_rlptx("0804");
        tx = response.add_txs();
        tx->set_type(::txpool::AllReply_Type_PENDING);
        tx->set_sender("9988b87991262f6ba471f09758cde1c0fc1de735");
        tx->set_rlptx("0806");
        tx = response.add_txs();
        tx->set_type(::txpool::AllReply_Type_BASE_FEE);
        tx->set_sender("9988b87991262f6ba471f09758cde1c0fc1de736");
        tx->set_rlptx("0807");
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_with(grpc_context_, std::move(response)));
        const auto transactions = run<&TransactionPool::get_transactions>();
        CHECK(transactions.size() == 3);
        if (transactions.size() > 2) {
            CHECK(transactions[0].transaction_type == txpool::TransactionType::QUEUED);
            CHECK(transactions[0].sender == 0x99f9b87991262f6ba471f09758cde1c0fc1de734_address);
            CHECK(transactions[0].rlp == silkworm::Bytes{0x30, 0x38, 0x30, 0x34});
            CHECK(transactions[1].transaction_type == txpool::TransactionType::PENDING);
            CHECK(transactions[1].sender == 0x9988b87991262f6ba471f09758cde1c0fc1de735_address);
            CHECK(transactions[1].rlp == silkworm::Bytes{0x30, 0x38, 0x30, 0x36});
            CHECK(transactions[2].transaction_type == txpool::TransactionType::BASE_FEE);
            CHECK(transactions[2].sender == 0x9988b87991262f6ba471f09758cde1c0fc1de736_address);
            CHECK(transactions[2].rlp == silkworm::Bytes{0x30, 0x38, 0x30, 0x37});
        }
    }

    SECTION("call get_transactions and check result is empty") {
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_ok(grpc_context_));
        const auto transactions = run<&TransactionPool::get_transactions>();
        CHECK(transactions.size() == 0);
    }

    SECTION("call get_transactions and get error") {
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_cancelled(grpc_context_));
        CHECK_THROWS_AS((run<&TransactionPool::get_transactions>()), boost::system::system_error);
    }
}

} // namespace silkrpc::txpool
