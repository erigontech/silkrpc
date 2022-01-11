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

private:
    evmc::address address_from_H160(const types::H160& h160) {
        uint64_t hi_hi = h160.hi().hi();
        uint64_t hi_lo = h160.hi().lo();
        uint32_t lo = h160.lo();
        evmc::address address{};
        boost::endian::store_big_u64(address.bytes +  0, hi_hi);
        boost::endian::store_big_u64(address.bytes +  8, hi_lo);
        boost::endian::store_big_u64(address.bytes + 12, lo);
        return address;
    }

    silkworm::Bytes bytes_from_H128(types::H128& h128) {
        silkworm::Bytes bytes(16, '\0');
        boost::endian::store_big_u64(&bytes[0], h128.hi());
        boost::endian::store_big_u64(&bytes[8], h128.lo());
        return bytes;

    }

    types::H128 H128_from_bytes(const silkworm::Bytes& bytes) {
        types::H128 h128;
        h128.set_hi(boost::endian::load_big_u64(&bytes[0]));
        h128.set_lo(boost::endian::load_big_u64(&bytes[8]));
        return h128;
    }

    types::H160 H160_from_address(const evmc::address& address) {
        types::H160 h160;
        *h160.mutable_hi() = H128_from_bytes(address.bytes);
        h160.set_lo(boost::endian::load_big_u32(address.bytes + 16));
        return h160;
    }

    types::H256 H256_from_bytes(const silkworm::Bytes& bytes) {
        types::H256 h256;
        *h256.mutable_hi() = H128_from_bytes(&bytes[0]);
        *h256.mutable_lo() = H128_from_bytes(&bytes[16]);
        return h256;
    }

    silkworm::Bytes bytes_from_H256(types::H256& h256) {
        silkworm::Bytes bytes(32, '\0');
        auto hi{h256.hi()};
        auto lo{h256.lo()};
        std::memcpy(&bytes[0], bytes_from_H128(hi).data(), 16);
        std::memcpy(&bytes[16], bytes_from_H128(lo).data(), 16);
        return bytes;
    }

    types::H512 H512_from_bytes(const silkworm::Bytes& bytes) {
        types::H512 h512;
        *h512.mutable_hi() = H256_from_bytes(&bytes[0]);
        *h512.mutable_lo() = H256_from_bytes(&bytes[32]);
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

    types::H1024 H1024_from_bytes(const silkworm::Bytes& bytes) {
        types::H1024 h1024;
        *h1024.mutable_hi() = H512_from_bytes(&bytes[0]);
        *h1024.mutable_lo() = H512_from_bytes(&bytes[64]);
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

    types::H2048 H2048_from_bytes(const silkworm::Bytes& bytes) {
        types::H2048 h2048;
        *h2048.mutable_hi() = H1024_from_bytes(&bytes[0]);
        *h2048.mutable_lo() = H1024_from_bytes(&bytes[128]);
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

    asio::io_context::executor_type executor_;
    std::unique_ptr<::remote::ETHBACKEND::StubInterface> stub_;
    grpc::CompletionQueue* queue_;
};

} // namespace silkrpc::ethbackend

#endif // SILKRPC_ETHBACKEND_BACKEND_HPP_
