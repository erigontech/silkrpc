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

#ifndef SILKRPC_TXPOOL_MINER_HPP_
#define SILKRPC_TXPOOL_MINER_HPP_

#include <memory>
#include <utility>

#include <silkrpc/config.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <evmc/evmc.hpp>
#include <grpcpp/grpcpp.h>
#include <silkworm/common/base.hpp>

#include <silkrpc/common/clock_time.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/awaitables.hpp>
#include <silkrpc/grpc/async_unary_client.hpp>
#include <silkrpc/interfaces/txpool/mining.grpc.pb.h>
#include <silkrpc/interfaces/types/types.pb.h>

namespace silkrpc::txpool {

using GetWorkClient = AsyncUnaryClient<
    ::txpool::Mining::StubInterface,
    ::txpool::GetWorkRequest,
    ::txpool::GetWorkReply,
    &::txpool::Mining::StubInterface::PrepareAsyncGetWork
>;

using GetWorkAwaitable = unary_awaitable<
    boost::asio::io_context::executor_type,
    GetWorkClient,
    ::txpool::Mining::StubInterface,
    ::txpool::GetWorkRequest,
    ::txpool::GetWorkReply
>;

using SubmitWorkClient = AsyncUnaryClient<
    ::txpool::Mining::StubInterface,
    ::txpool::SubmitWorkRequest,
    ::txpool::SubmitWorkReply,
    &::txpool::Mining::StubInterface::PrepareAsyncSubmitWork
>;

using SubmitWorkAwaitable = unary_awaitable<
    boost::asio::io_context::executor_type,
    SubmitWorkClient,
    ::txpool::Mining::StubInterface,
    ::txpool::SubmitWorkRequest,
    ::txpool::SubmitWorkReply
>;

using SubmitHashRateClient = AsyncUnaryClient<
    ::txpool::Mining::StubInterface,
    ::txpool::SubmitHashRateRequest,
    ::txpool::SubmitHashRateReply,
    &::txpool::Mining::StubInterface::PrepareAsyncSubmitHashRate
>;

using SubmitHashRateAwaitable = unary_awaitable<
    boost::asio::io_context::executor_type,
    SubmitHashRateClient,
    ::txpool::Mining::StubInterface,
    ::txpool::SubmitHashRateRequest,
    ::txpool::SubmitHashRateReply
>;

using HashRateClient = AsyncUnaryClient<
    ::txpool::Mining::StubInterface,
    ::txpool::HashRateRequest,
    ::txpool::HashRateReply,
    &::txpool::Mining::StubInterface::PrepareAsyncHashRate
>;

using HashRateAwaitable = unary_awaitable<
    boost::asio::io_context::executor_type,
    HashRateClient,
    ::txpool::Mining::StubInterface,
    ::txpool::HashRateRequest,
    ::txpool::HashRateReply
>;

using MiningClient = AsyncUnaryClient<
    ::txpool::Mining::StubInterface,
    ::txpool::MiningRequest,
    ::txpool::MiningReply,
    &::txpool::Mining::StubInterface::PrepareAsyncMining
>;

using MiningAwaitable = unary_awaitable<
    boost::asio::io_context::executor_type,
    MiningClient,
    ::txpool::Mining::StubInterface,
    ::txpool::MiningRequest,
    ::txpool::MiningReply
>;

struct WorkResult {
    evmc::bytes32 header_hash;
    evmc::bytes32 seed_hash;
    evmc::bytes32 target;
    silkworm::Bytes block_number;
};

struct MiningResult {
    bool enabled;
    bool running;
};

class Miner final {
public:
    explicit Miner(boost::asio::io_context& context, std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue);
    explicit Miner(boost::asio::io_context::executor_type executor, std::unique_ptr<::txpool::Mining::StubInterface> stub, grpc::CompletionQueue* queue);

    ~Miner();

    boost::asio::awaitable<WorkResult> get_work();

    boost::asio::awaitable<bool> submit_work(const silkworm::Bytes& block_nonce, const evmc::bytes32& pow_hash, const evmc::bytes32& digest);

    boost::asio::awaitable<bool> submit_hash_rate(const intx::uint256& rate, const evmc::bytes32& id);

    boost::asio::awaitable<uint64_t> get_hash_rate();

    boost::asio::awaitable<MiningResult> get_mining();

private:
    boost::asio::io_context::executor_type executor_;
    std::unique_ptr<::txpool::Mining::StubInterface> stub_;
    grpc::CompletionQueue* queue_;
};

} // namespace silkrpc::txpool

#endif // SILKRPC_TXPOOL_MINER_HPP_
