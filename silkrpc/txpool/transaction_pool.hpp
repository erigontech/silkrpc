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

#ifndef SILKRPC_TXPOOL_TRANSACTION_POOL_HPP_
#define SILKRPC_TXPOOL_TRANSACTION_POOL_HPP_

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <silkrpc/config.hpp>

#include <asio/io_context.hpp>
#include <asio/use_awaitable.hpp>
#include <boost/endian/conversion.hpp>
#include <evmc/evmc.hpp>
#include <grpcpp/grpcpp.h>
#include <silkworm/common/base.hpp>

#include <silkrpc/common/clock_time.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/awaitables.hpp>
#include <silkrpc/grpc/async_unary_client.hpp>
#include <silkrpc/interfaces/txpool/txpool.grpc.pb.h>
#include <silkrpc/interfaces/types/types.pb.h>

namespace silkrpc::txpool {

using AddClient = AsyncUnaryClient<
    ::txpool::Txpool::StubInterface,
    ::txpool::AddRequest,
    ::txpool::AddReply,
    &::txpool::Txpool::StubInterface::PrepareAsyncAdd
>;

using AddAwaitable = unary_awaitable<
    asio::io_context::executor_type,
    AddClient,
    ::txpool::Txpool::StubInterface,
    ::txpool::AddRequest,
    ::txpool::AddReply
>;

using TransactionsClient = AsyncUnaryClient<
    ::txpool::Txpool::StubInterface,
    ::txpool::TransactionsRequest,
    ::txpool::TransactionsReply,
    &::txpool::Txpool::StubInterface::PrepareAsyncTransactions
>;

using TransactionsAwaitable = unary_awaitable<
    asio::io_context::executor_type,
    TransactionsClient,
    ::txpool::Txpool::StubInterface,
    ::txpool::TransactionsRequest,
    ::txpool::TransactionsReply
>;

class TransactionPool final {

public:
    typedef struct {
       bool completed_succesfully;
       std::string error_descr;
    } OperationResult;

    explicit TransactionPool(asio::io_context& context, std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue)
    : TransactionPool(context.get_executor(), ::txpool::Txpool::NewStub(channel, grpc::StubOptions()), queue) {}

    explicit TransactionPool(asio::io_context::executor_type executor, std::unique_ptr<::txpool::Txpool::StubInterface> stub, grpc::CompletionQueue* queue)
    : stub_(std::move(stub)), add_awaitable_{executor, stub_, queue}, transactions_awaitable_{executor, stub_, queue} {
        SILKRPC_TRACE << "TransactionPool::ctor " << this << "\n";
    }

    ~TransactionPool() {
        SILKRPC_TRACE << "TransactionPool::dtor " << this << "\n";
    }

    asio::awaitable<OperationResult> add_transaction(const silkworm::ByteView& rlp_tx) {
        const auto start_time = clock_time::now();
        SILKRPC_DEBUG << "TransactionPool::add_transaction rlp_tx=" << rlp_tx << "\n";
        ::txpool::AddRequest request;
        request.add_rlptxs(rlp_tx.data(), rlp_tx.size());
        const auto reply = co_await add_awaitable_.async_call(request, asio::use_awaitable);
        const auto imported_size = reply.imported_size();
        const auto errors_size = reply.errors_size();
        SILKRPC_DEBUG << "TransactionPool::add_transaction imported_size=" << imported_size << " errors_size=" << errors_size << "\n";
        OperationResult result{true, ""};

        if (imported_size == 1) {
            const auto import_result = reply.imported(0);
            SILKRPC_DEBUG << "TransactionPool::add_transaction import_result=" << import_result << "\n";
            if (import_result != ::txpool::ImportResult::SUCCESS) { 
                result.completed_succesfully = false;
                if (errors_size >= 1) {
                   const auto import_error = reply.errors(0);
                   result.error_descr = import_error;
                   SILKRPC_WARN << "TransactionPool::add_transaction import_result=" << import_result << " error=" << import_error << "\n";
                } else {
                   result.error_descr = "no specific error";
                   SILKRPC_WARN << "TransactionPool::add_transaction import_result=" << import_result << ", no error received\n";
                }
           }
        } else {
            result.completed_succesfully = false;
            result.error_descr = "unexpected imported size";
            SILKRPC_WARN << "TransactionPool::add_transaction unexpected imported_size=" << imported_size << "\n";
        }
        SILKRPC_DEBUG << "TransactionPool::add_transaction t=" << clock_time::since(start_time) << "\n";
        co_return std::move(result);
    }

    asio::awaitable<std::optional<silkworm::Bytes>> get_transaction(const evmc::bytes32& tx_hash) {
        const auto start_time = clock_time::now();
        SILKRPC_DEBUG << "TransactionPool::get_transaction tx_hash=" << tx_hash << "\n";
        auto hi = new ::types::H128{};
        auto lo = new ::types::H128{};
        hi->set_hi(evmc::load64be(tx_hash.bytes + 0));
        hi->set_lo(evmc::load64be(tx_hash.bytes + 8));
        lo->set_hi(evmc::load64be(tx_hash.bytes + 16));
        lo->set_lo(evmc::load64be(tx_hash.bytes + 24));
        ::txpool::TransactionsRequest request;
        ::types::H256* hash_h256{request.add_hashes()};
        hash_h256->set_allocated_hi(hi);  // take ownership
        hash_h256->set_allocated_lo(lo);  // take ownership
        const auto reply = co_await transactions_awaitable_.async_call(request, asio::use_awaitable);
        const auto rlptxs_size = reply.rlptxs_size();
        SILKRPC_DEBUG << "TransactionPool::get_transaction rlptxs_size=" << rlptxs_size << "\n";
        if (rlptxs_size == 1) {
            const auto rlptx = reply.rlptxs(0);
            SILKRPC_DEBUG << "TransactionPool::get_transaction t=" << clock_time::since(start_time) << "\n";
            co_return silkworm::Bytes{rlptx.begin(), rlptx.end()};
        } else {
            SILKRPC_WARN << "TransactionPool::get_transaction unexpected rlptxs_size=" << rlptxs_size << "\n";
            SILKRPC_DEBUG << "TransactionPool::get_transaction t=" << clock_time::since(start_time) << "\n";
            co_return std::nullopt;
        }
    }

private:
    std::unique_ptr<::txpool::Txpool::StubInterface> stub_;
    AddAwaitable add_awaitable_;
    TransactionsAwaitable transactions_awaitable_;
};

} // namespace silkrpc::txpool

#endif // SILKRPC_TXPOOL_TRANSACTION_POOL_HPP_
