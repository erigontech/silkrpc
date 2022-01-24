/*
    Copyright 2020 The Silkrpc Authors

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

#ifndef SILKRPC_ETHBACKEND_BACKEND_HPP_
#define SILKRPC_ETHBACKEND_BACKEND_HPP_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <silkrpc/config.hpp>

#include <asio/io_context.hpp>
#include <asio/use_awaitable.hpp>
#include <boost/endian/conversion.hpp>
#include <evmc/evmc.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/clock_time.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/awaitables.hpp>
#include <silkrpc/grpc/async_unary_client.hpp>
#include <silkrpc/interfaces/remote/ethbackend.grpc.pb.h>
#include <silkrpc/interfaces/types/types.pb.h>
#include <silkrpc/types/execution_payload.hpp>

namespace silkrpc::ethbackend {

using EtherbaseClient = AsyncUnaryClient<
    ::remote::ETHBACKEND::StubInterface,
    ::remote::EtherbaseRequest,
    ::remote::EtherbaseReply,
    &::remote::ETHBACKEND::StubInterface::PrepareAsyncEtherbase
>;

using ProtocolVersionClient = AsyncUnaryClient<
    ::remote::ETHBACKEND::StubInterface,
    ::remote::ProtocolVersionRequest,
    ::remote::ProtocolVersionReply,
    &::remote::ETHBACKEND::StubInterface::PrepareAsyncProtocolVersion
>;

using NetVersionClient = AsyncUnaryClient<
    ::remote::ETHBACKEND::StubInterface,
    ::remote::NetVersionRequest,
    ::remote::NetVersionReply,
    &::remote::ETHBACKEND::StubInterface::PrepareAsyncNetVersion
>;

using ClientVersionClient = AsyncUnaryClient<
    ::remote::ETHBACKEND::StubInterface,
    ::remote::ClientVersionRequest,
    ::remote::ClientVersionReply,
    &::remote::ETHBACKEND::StubInterface::PrepareAsyncClientVersion
>;

using NetPeerCountClient = AsyncUnaryClient<
    ::remote::ETHBACKEND::StubInterface,
    ::remote::NetPeerCountRequest,
    ::remote::NetPeerCountReply,
    &::remote::ETHBACKEND::StubInterface::PrepareAsyncNetPeerCount
>;

using EngineGetPayloadV1Client = AsyncUnaryClient<
    ::remote::ETHBACKEND::StubInterface,
    ::remote::EngineGetPayloadRequest,
    ::types::ExecutionPayload,
    &::remote::ETHBACKEND::StubInterface::PrepareAsyncEngineGetPayloadV1
>;

using EtherbaseAwaitable = unary_awaitable<
    asio::io_context::executor_type,
    EtherbaseClient,
    ::remote::ETHBACKEND::StubInterface,
    ::remote::EtherbaseRequest,
    ::remote::EtherbaseReply
>;

using ProtocolVersionAwaitable = unary_awaitable<
    asio::io_context::executor_type,
    ProtocolVersionClient,
    ::remote::ETHBACKEND::StubInterface,
    ::remote::ProtocolVersionRequest,
    ::remote::ProtocolVersionReply
>;

using NetVersionAwaitable = unary_awaitable<
    asio::io_context::executor_type,
    NetVersionClient,
    ::remote::ETHBACKEND::StubInterface,
    ::remote::NetVersionRequest,
    ::remote::NetVersionReply
>;

using ClientVersionAwaitable = unary_awaitable<
    asio::io_context::executor_type,
    ClientVersionClient,
    ::remote::ETHBACKEND::StubInterface,
    ::remote::ClientVersionRequest,
    ::remote::ClientVersionReply
>;

using NetPeerCountAwaitable = unary_awaitable<
    asio::io_context::executor_type,
    NetPeerCountClient,
    ::remote::ETHBACKEND::StubInterface,
    ::remote::NetPeerCountRequest,
    ::remote::NetPeerCountReply
>;

using EngineGetPayloadV1Awaitable = unary_awaitable<
    asio::io_context::executor_type,
    EngineGetPayloadV1Client,
    ::remote::ETHBACKEND::StubInterface,
    ::remote::EngineGetPayloadRequest,
    ::types::ExecutionPayload
>;

class BackEnd final {
public:
    explicit BackEnd(asio::io_context& context, std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue)
    : BackEnd(context.get_executor(), ::remote::ETHBACKEND::NewStub(channel), queue) {}

    explicit BackEnd(asio::io_context::executor_type executor, std::unique_ptr<::remote::ETHBACKEND::StubInterface> stub, grpc::CompletionQueue* queue)
    : executor_(executor), stub_(std::move(stub)), queue_(queue) {
        SILKRPC_TRACE << "BackEnd::ctor " << this << "\n";
    }

    ~BackEnd() {
        SILKRPC_TRACE << "BackEnd::dtor " << this << "\n";
    }

    asio::awaitable<evmc::address> etherbase() {
        const auto start_time = clock_time::now();
        EtherbaseAwaitable eb_awaitable{executor_, stub_, queue_};
        const auto reply = co_await eb_awaitable.async_call(::remote::EtherbaseRequest{}, asio::use_awaitable);
        evmc::address evmc_address;
        if (reply.has_address()) {
            const auto h160_address = reply.address();
            evmc_address = address_from_H160(h160_address);
        }
        SILKRPC_DEBUG << "BackEnd::etherbase address=" << evmc_address << " t=" << clock_time::since(start_time) << "\n";
        co_return evmc_address;
    }

    asio::awaitable<uint64_t> protocol_version() {
        const auto start_time = clock_time::now();
        ProtocolVersionAwaitable pv_awaitable{executor_, stub_, queue_};
        const auto reply = co_await pv_awaitable.async_call(::remote::ProtocolVersionRequest{}, asio::use_awaitable);
        const auto pv = reply.id();
        SILKRPC_DEBUG << "BackEnd::protocol_version version=" << pv << " t=" << clock_time::since(start_time) << "\n";
        co_return pv;
    }

    asio::awaitable<uint64_t> net_version() {
        const auto start_time = clock_time::now();
        NetVersionAwaitable nv_awaitable{executor_, stub_, queue_};
        const auto reply = co_await nv_awaitable.async_call(::remote::NetVersionRequest{}, asio::use_awaitable);
        const auto nv = reply.id();
        SILKRPC_DEBUG << "BackEnd::net_version version=" << nv << " t=" << clock_time::since(start_time) << "\n";
        co_return nv;
    }

    asio::awaitable<std::string> client_version() {
        const auto start_time = clock_time::now();
        ClientVersionAwaitable cv_awaitable{executor_, stub_, queue_};
        const auto reply = co_await cv_awaitable.async_call(::remote::ClientVersionRequest{}, asio::use_awaitable);
        const auto cv = reply.nodename();
        SILKRPC_DEBUG << "BackEnd::client_version version=" << cv << " t=" << clock_time::since(start_time) << "\n";
        co_return cv;
    }

    asio::awaitable<uint64_t> net_peer_count() {
        const auto start_time = clock_time::now();
        NetPeerCountAwaitable npc_awaitable{executor_, stub_, queue_};
        const auto reply = co_await npc_awaitable.async_call(::remote::NetPeerCountRequest{}, asio::use_awaitable);
        const auto count = reply.count();
        SILKRPC_DEBUG << "BackEnd::net_peer_count count=" << count << " t=" << clock_time::since(start_time) << "\n";
        co_return count;
    }

    asio::awaitable<ExecutionPayload> engine_get_payload_v1(uint64_t payload_id) {
        const auto start_time = clock_time::now();
        EngineGetPayloadV1Awaitable npc_awaitable{executor_, stub_, queue_};
        ::remote::EngineGetPayloadRequest req;
        req.set_payloadid(payload_id);
        const auto reply = co_await npc_awaitable.async_call(req, asio::use_awaitable);
        auto execution_payload{decode_execution_payload(reply)};
        SILKRPC_DEBUG << "BackEnd::engine_get_payload_v1 data=" << execution_payload << " t=" << clock_time::since(start_time) << "\n";
        co_return execution_payload;
    }

    // just for testing
    asio::awaitable<::types::ExecutionPayload> execution_payload_to_proto(ExecutionPayload payload) {
        co_return encode_execution_payload(payload);
    }

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

    silkworm::Bytes bytes_from_H128(const types::H128& h128) {
        silkworm::Bytes bytes(16, '\0');
        boost::endian::store_big_u64(&bytes[0], h128.hi());
        boost::endian::store_big_u64(&bytes[8], h128.lo());
        return bytes;
    }

    types::H128* H128_from_bytes(const uint8_t* bytes) {
        auto h128{new types::H128()};
        h128->set_hi(boost::endian::load_big_u64(bytes));
        h128->set_lo(boost::endian::load_big_u64(bytes + 8));
        return h128;
    }

    types::H160* H160_from_address(const evmc::address& address) {
        auto h160{new types::H160()};
        auto hi{H128_from_bytes(address.bytes)};
        h160->set_allocated_hi(hi);
        h160->set_lo(boost::endian::load_big_u32(address.bytes + 16));
        return h160;
    }

    types::H256* H256_from_bytes(const uint8_t* bytes) {
        auto h256{new types::H256()};
        auto hi{H128_from_bytes(bytes)};
        auto lo{H128_from_bytes(bytes + 16)};
        h256->set_allocated_hi(hi);
        h256->set_allocated_lo(lo);
        return h256;
    }

    silkworm::Bytes bytes_from_H256(const types::H256& h256) {
        silkworm::Bytes bytes(32, '\0');
        auto hi{h256.hi()};
        auto lo{h256.lo()};
        std::memcpy(&bytes[0], bytes_from_H128(hi).data(), 16);
        std::memcpy(&bytes[16], bytes_from_H128(lo).data(), 16);
        return bytes;
    }

    intx::uint256 uint256_from_H256(const types::H256& h256) {
        intx::uint256 n;
        n[3] = h256.hi().hi();
        n[2] = h256.hi().lo();
        n[1] = h256.lo().hi();
        n[0] = h256.lo().lo();
        return n;
    }

    types::H256* H256_from_uint256(const intx::uint256& n) {
        auto h256{new types::H256()};
        auto hi{new types::H128()};
        auto lo{new types::H128()};

        hi->set_hi(n[3]);
        hi->set_lo(n[2]);
        lo->set_hi(n[1]);
        lo->set_lo(n[0]);

        h256->set_allocated_hi(hi);
        h256->set_allocated_lo(lo);
        return h256;
    }

    evmc::bytes32 bytes32_from_H256(const types::H256& h256) {
        evmc::bytes32 bytes32;
        std::memcpy(bytes32.bytes, bytes_from_H256(h256).data(), 32);
        return bytes32;
    }

    types::H512* H512_from_bytes(const uint8_t* bytes) {
        auto h512{new types::H512()};
        auto hi{H256_from_bytes(bytes)};
        auto lo{H256_from_bytes(bytes + 32)};
        h512->set_allocated_hi(hi);
        h512->set_allocated_lo(lo);
        return h512;
    }

    silkworm::Bytes bytes_from_H512(types::H512& h512) {
        silkworm::Bytes bytes(64, '\0');
        auto hi{h512.hi()};
        auto lo{h512.lo()};
        std::memcpy(&bytes[0], bytes_from_H256(hi).data(), 32);
        std::memcpy(&bytes[32], bytes_from_H256(lo).data(), 32);
        return bytes;
    }

    types::H1024* H1024_from_bytes(const uint8_t* bytes) {
        auto h1024{new types::H1024()};
        auto hi{H512_from_bytes(bytes)};
        auto lo{H512_from_bytes(bytes + 64)};
        h1024->set_allocated_hi(hi);
        h1024->set_allocated_lo(lo);
        return h1024;
    }

    silkworm::Bytes bytes_from_H1024(types::H1024& h1024) {
        silkworm::Bytes bytes(128, '\0');
        auto hi{h1024.hi()};
        auto lo{h1024.lo()};
        std::memcpy(&bytes[0], bytes_from_H512(hi).data(), 64);
        std::memcpy(&bytes[64], bytes_from_H512(lo).data(), 64);
        return bytes;
    }

    types::H2048* H2048_from_bytes(const uint8_t* bytes) {
        auto h2048{new types::H2048()};
        auto hi{H1024_from_bytes(bytes)};
        auto lo{H1024_from_bytes(bytes + 128)};
        h2048->set_allocated_hi(hi);
        h2048->set_allocated_lo(lo);
        return h2048;
    }

    silkworm::Bytes bytes_from_H2048(types::H2048& h2048) {
        silkworm::Bytes bytes(256, '\0');
        auto hi{h2048.hi()};
        auto lo{h2048.lo()};
        std::memcpy(&bytes[0], bytes_from_H1024(hi).data(), 128);
        std::memcpy(&bytes[128], bytes_from_H1024(lo).data(), 128);
        return bytes;
    }

    ExecutionPayload decode_execution_payload(const types::ExecutionPayload& execution_payload_grpc) {
        auto state_root_h256{execution_payload_grpc.stateroot()};
        auto receipts_root_h256{execution_payload_grpc.receiptroot()};
        auto block_hash_h256{execution_payload_grpc.blockhash()};
        auto parent_hash_h256{execution_payload_grpc.parenthash()};
        auto random_h256{execution_payload_grpc.random()};
        auto base_fee_h256{execution_payload_grpc.basefeepergas()};
        auto logs_bloom_h2048{execution_payload_grpc.logsbloom()};
        auto extra_data_string{execution_payload_grpc.extradata()}; // []byte becomes std::string in silkrpc protobuf
        // Convert h2048 to a bloom
        silkworm::Bloom bloom;
        std::memcpy(&bloom[0], bytes_from_H2048(logs_bloom_h2048).data(), 256);
        // Convert transactions in std::string to silkworm::Bytes
        std::vector<silkworm::Bytes> transactions;
        for (const auto& transaction_string : execution_payload_grpc.transactions()) {
            transactions.push_back(silkworm::bytes_of_string(transaction_string));
        }

        // Assembling the execution_payload data structure
        return ExecutionPayload{
            .number = execution_payload_grpc.blocknumber(),
            .timestamp = execution_payload_grpc.timestamp(),
            .gas_limit = execution_payload_grpc.gaslimit(),
            .gas_used = execution_payload_grpc.gasused(),
            .suggested_fee_recipient = address_from_H160(execution_payload_grpc.coinbase()),
            .state_root = bytes32_from_H256(state_root_h256),
            .receipts_root = bytes32_from_H256(receipts_root_h256),
            .parent_hash = bytes32_from_H256(parent_hash_h256),
            .block_hash = bytes32_from_H256(block_hash_h256),
            .random = bytes32_from_H256(random_h256),
            .base_fee = uint256_from_H256(base_fee_h256),
            .logs_bloom = bloom,
            .extra_data = silkworm::bytes_of_string(extra_data_string),
            .transactions = transactions
        };
    }

    types::ExecutionPayload encode_execution_payload(const ExecutionPayload& execution_payload) {
        types::ExecutionPayload execution_payload_grpc;
        // Numerical parameters
        execution_payload_grpc.set_blocknumber(execution_payload.number);
        execution_payload_grpc.set_timestamp(execution_payload.timestamp);
        execution_payload_grpc.set_gaslimit(execution_payload.gas_limit);
        execution_payload_grpc.set_gasused(execution_payload.gas_used);
        // coinbase
        execution_payload_grpc.set_allocated_coinbase(H160_from_address(execution_payload.suggested_fee_recipient));
        // 32-bytes parameters
        execution_payload_grpc.set_allocated_receiptroot(H256_from_bytes(execution_payload.receipts_root.bytes));
        execution_payload_grpc.set_allocated_stateroot(H256_from_bytes(execution_payload.state_root.bytes));
        execution_payload_grpc.set_allocated_parenthash(H256_from_bytes(execution_payload.parent_hash.bytes));
        execution_payload_grpc.set_allocated_random(H256_from_bytes(execution_payload.random.bytes));
        execution_payload_grpc.set_allocated_basefeepergas(H256_from_uint256(execution_payload.base_fee));
        // Logs Bloom
        execution_payload_grpc.set_allocated_logsbloom(H2048_from_bytes(&execution_payload.logs_bloom[0]));
        // String-like parameters
        for (auto transaction_bytes : execution_payload.transactions) {
            execution_payload_grpc.add_transactions(std::string(reinterpret_cast<char*>(&transaction_bytes[0]), transaction_bytes.size()));
        }
        execution_payload_grpc.set_extradata(std::string(execution_payload.extra_data.begin(), execution_payload.extra_data.end()));
        return execution_payload_grpc;
    }

    asio::io_context::executor_type executor_;
    std::unique_ptr<::remote::ETHBACKEND::StubInterface> stub_;
    grpc::CompletionQueue* queue_;
};

} // namespace silkrpc::ethbackend

#endif // SILKRPC_ETHBACKEND_BACKEND_HPP_
