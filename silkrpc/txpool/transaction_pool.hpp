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
#include <vector>

#include <silkrpc/config.hpp>

#include <agrpc/grpc_context.hpp>
#include <asio/io_context.hpp>
#include <asio/use_awaitable.hpp>
#include <boost/endian/conversion.hpp>
#include <evmc/evmc.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/clock_time.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/interfaces/txpool/txpool.grpc.pb.h>
#include <silkrpc/interfaces/types/types.pb.h>
#include <silkworm/common/base.hpp>

namespace silkrpc::txpool {

struct OperationResult {
    bool success;
    std::string error_descr;
};

struct StatusInfo {
    unsigned int queued_count;
    unsigned int pending_count;
    unsigned int base_fee_count;
};

enum TransactionType {
    QUEUED,
    PENDING,
    BASE_FEE
};

struct TransactionInfo {
    TransactionType transaction_type;
    evmc::address sender;
    silkworm::Bytes rlp;
};

using TransactionsInPool = std::vector<TransactionInfo>;

class TransactionPool final {
public:
    explicit TransactionPool(asio::io_context& context, std::shared_ptr<grpc::Channel> channel, agrpc::GrpcContext& grpc_context);

    explicit TransactionPool(asio::io_context::executor_type executor, std::unique_ptr<::txpool::Txpool::StubInterface> stub,
        agrpc::GrpcContext& grpc_context);

    ~TransactionPool();

    asio::awaitable<OperationResult> add_transaction(const silkworm::ByteView& rlp_tx);

    asio::awaitable<std::optional<silkworm::Bytes>> get_transaction(const evmc::bytes32& tx_hash);

    asio::awaitable<std::optional<uint64_t>> nonce(const evmc::address& address);

    asio::awaitable<StatusInfo> get_status();

    /*asio::awaitable<TransactionsInPool> get_transactions() {
        const auto start_time = clock_time::now();
        SILKRPC_DEBUG << "TransactionPool::get_transactions\n";
        TransactionsInPool transactions_in_pool{};
        ::txpool::AllRequest request{};
        AllAwaitable all_awaitable{executor_, stub_, queue_};
        const auto reply = co_await all_awaitable.async_call(request, asio::use_awaitable);
        const auto txs_size = reply.txs_size();
        for (int i = 0; i < txs_size; i++) {
            TransactionInfo element{};
            const auto tx = reply.txs(i);
            element.sender = address_from_H160(tx.sender());
            const auto rlp = tx.rlptx();
            element.rlp = silkworm::Bytes{rlp.begin(), rlp.end()};
            if (tx.txntype() == ::txpool::AllReply_TxnType_PENDING) {
                element.transaction_type = PENDING;
            } else if (tx.txntype() == ::txpool::AllReply_TxnType_QUEUED) {
                element.transaction_type = QUEUED;
            } else {
                element.transaction_type = BASE_FEE;
            }
            transactions_in_pool.push_back(element);
        }
        SILKRPC_DEBUG << "TransactionPool::get_transactions t=" << clock_time::since(start_time) << "\n";
        co_return transactions_in_pool;
    }*/
    asio::awaitable<TransactionsInPool> get_transactions();

private:
    evmc::address address_from_H160(const types::H160& h160) {
        uint64_t hi_hi = h160.hi().hi();
        uint64_t hi_lo = h160.hi().lo();
        uint32_t lo = h160.lo();
        evmc::address address{};
        boost::endian::store_big_u64(address.bytes +  0, hi_hi);
        boost::endian::store_big_u64(address.bytes +  8, hi_lo);
        boost::endian::store_big_u32(address.bytes + 16, lo);
        return address;
    }
    types::H160* H160_from_address(const evmc::address& address);
    types::H128* H128_from_bytes(const uint8_t* bytes);

    asio::io_context::executor_type executor_;
    std::unique_ptr<::txpool::Txpool::StubInterface> stub_;
    agrpc::GrpcContext& grpc_context_;
};

} // namespace silkrpc::txpool

#endif // SILKRPC_TXPOOL_TRANSACTION_POOL_HPP_
