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

namespace silkrpc::txpool {

Miner::Miner(asio::io_context& context, std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue)
: Miner(context.get_executor(), ::txpool::Mining::NewStub(channel), queue) {}

Miner::Miner(asio::io_context::executor_type executor, std::unique_ptr<::txpool::Mining::StubInterface> stub, grpc::CompletionQueue* queue)
: executor_(executor), stub_(std::move(stub)), queue_(queue) {
    SILKRPC_TRACE << "Miner::ctor " << this << "\n";
}

Miner::~Miner() {
    SILKRPC_TRACE << "Miner::dtor " << this << "\n";
}

asio::awaitable<WorkResult> Miner::get_work() {
    const auto start_time = clock_time::now();
    SILKRPC_DEBUG << "Miner::get_work\n";
    GetWorkAwaitable get_work_awaitable{executor_, stub_, queue_};
    const auto reply = co_await get_work_awaitable.async_call(::txpool::GetWorkRequest{}, asio::use_awaitable);
    const auto header_hash = silkworm::bytes32_from_hex(reply.headerhash());
    SILKRPC_DEBUG << "Miner::get_work header_hash=" << header_hash << "\n";
    const auto seed_hash = silkworm::bytes32_from_hex(reply.seedhash());
    SILKRPC_DEBUG << "Miner::get_work seed_hash=" << seed_hash << "\n";
    const auto target = silkworm::bytes32_from_hex(reply.target());
    SILKRPC_DEBUG << "Miner::get_work target=" << target << "\n";
    const auto block_number = silkworm::from_hex(reply.blocknumber()).value_or(silkworm::Bytes{});
    SILKRPC_DEBUG << "Miner::get_work block_number=" << block_number << "\n";
    WorkResult result{header_hash, seed_hash, target, block_number};
    SILKRPC_DEBUG << "Miner::get_work t=" << clock_time::since(start_time) << "\n";
    co_return result;
}

asio::awaitable<bool> Miner::submit_work(const silkworm::Bytes& block_nonce, const evmc::bytes32& pow_hash, const evmc::bytes32& digest) {
    const auto start_time = clock_time::now();
    SILKRPC_DEBUG << "Miner::submit_work block_nonce=" << block_nonce << " pow_hash=" << pow_hash << " digest=" << digest << "\n";
    ::txpool::SubmitWorkRequest submit_work_request;
    submit_work_request.set_blocknonce(block_nonce.data(), block_nonce.size());
    submit_work_request.set_powhash(pow_hash.bytes, silkworm::kHashLength);
    submit_work_request.set_digest(digest.bytes, silkworm::kHashLength);
    SubmitWorkAwaitable submit_work_awaitable{executor_, stub_, queue_};
    const auto reply = co_await submit_work_awaitable.async_call(submit_work_request, asio::use_awaitable);
    const auto ok = reply.ok();
    SILKRPC_DEBUG << "Miner::submit_work ok=" << std::boolalpha << ok << " t=" << clock_time::since(start_time) << "\n";
    co_return ok;
}

asio::awaitable<bool> Miner::submit_hash_rate(const intx::uint256& rate, const evmc::bytes32& id) {
    const auto start_time = clock_time::now();
    SILKRPC_DEBUG << "Miner::submit_hash_rate rate=" << rate << " id=" << id << "\n";
    ::txpool::SubmitHashRateRequest submit_hashrate_request;
    submit_hashrate_request.set_rate(uint64_t(rate));
    submit_hashrate_request.set_id(id.bytes, silkworm::kHashLength);
    SubmitHashRateAwaitable submit_hashrate_awaitable{executor_, stub_, queue_};
    const auto reply = co_await submit_hashrate_awaitable.async_call(submit_hashrate_request, asio::use_awaitable);
    const auto ok = reply.ok();
    SILKRPC_DEBUG << "Miner::submit_hash_rate ok=" << std::boolalpha << ok << " t=" << clock_time::since(start_time) << "\n";
    co_return ok;
}

asio::awaitable<uint64_t> Miner::get_hash_rate() {
    const auto start_time = clock_time::now();
    SILKRPC_DEBUG << "Miner::hash_rate\n";
    HashRateAwaitable hashrate_awaitable{executor_, stub_, queue_};
    const auto reply = co_await hashrate_awaitable.async_call(::txpool::HashRateRequest{}, asio::use_awaitable);
    const auto hashrate = reply.hashrate();
    SILKRPC_DEBUG << "Miner::hash_rate hashrate=" << hashrate << " t=" << clock_time::since(start_time) << "\n";
    co_return 0;
}

asio::awaitable<MiningResult> Miner::get_mining() {
    const auto start_time = clock_time::now();
    SILKRPC_DEBUG << "Miner::get_mining\n";
    MiningAwaitable mining_awaitable{executor_, stub_, queue_};
    const auto reply = co_await mining_awaitable.async_call(::txpool::MiningRequest{}, asio::use_awaitable);
    const auto enabled = reply.enabled();
    SILKRPC_DEBUG << "Miner::get_mining enabled=" << std::boolalpha << enabled << "\n";
    const auto running = reply.running();
    SILKRPC_DEBUG << "Miner::get_mining running=" << std::boolalpha << running << "\n";
    MiningResult result{enabled, running};
    SILKRPC_DEBUG << "Miner::get_mining t=" << clock_time::since(start_time) << "\n";
    co_return result;
}

} // namespace silkrpc::txpool
