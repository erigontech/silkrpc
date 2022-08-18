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

#include <agrpc/grpc_context.hpp>
#include <asio/io_context.hpp>
#include <asio/use_awaitable.hpp>
#include <evmc/evmc.hpp>
#include <intx/intx.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/interfaces/txpool/mining.grpc.pb.h>
#include <silkrpc/interfaces/types/types.pb.h>
#include <silkworm/common/base.hpp>

namespace silkrpc::txpool {

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
    explicit Miner(asio::io_context& context, std::shared_ptr<grpc::Channel> channel, agrpc::GrpcContext& grpc_context);
    explicit Miner(asio::io_context::executor_type executor, std::unique_ptr<::txpool::Mining::StubInterface> stub,
        agrpc::GrpcContext& grpc_context);

    ~Miner();

    asio::awaitable<WorkResult> get_work();

    asio::awaitable<bool> submit_work(const silkworm::Bytes& block_nonce, const evmc::bytes32& pow_hash, const evmc::bytes32& digest);

    asio::awaitable<bool> submit_hash_rate(const intx::uint256& rate, const evmc::bytes32& id);

    asio::awaitable<uint64_t> get_hash_rate();

    asio::awaitable<MiningResult> get_mining();

private:
    asio::io_context::executor_type executor_;
    std::unique_ptr<::txpool::Mining::StubInterface> stub_;
    agrpc::GrpcContext& grpc_context_;
};

} // namespace silkrpc::txpool

#endif // SILKRPC_TXPOOL_MINER_HPP_
