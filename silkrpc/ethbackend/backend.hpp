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
#include <string>

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
    ::remote::ETHBACKEND::Stub,
    ::remote::ETHBACKEND::NewStub,
    ::remote::EtherbaseRequest,
    ::remote::EtherbaseReply,
    &::remote::ETHBACKEND::Stub::PrepareAsyncEtherbase
>;

using ProtocolVersionClient = AsyncUnaryClient<
    ::remote::ETHBACKEND::Stub,
    ::remote::ETHBACKEND::NewStub,
    ::remote::ProtocolVersionRequest,
    ::remote::ProtocolVersionReply,
    &::remote::ETHBACKEND::Stub::PrepareAsyncProtocolVersion
>;

using NetVersionClient = AsyncUnaryClient<
    ::remote::ETHBACKEND::Stub,
    ::remote::ETHBACKEND::NewStub,
    ::remote::NetVersionRequest,
    ::remote::NetVersionReply,
    &::remote::ETHBACKEND::Stub::PrepareAsyncNetVersion
>;

using ClientVersionClient = AsyncUnaryClient<
    ::remote::ETHBACKEND::Stub,
    ::remote::ETHBACKEND::NewStub,
    ::remote::ClientVersionRequest,
    ::remote::ClientVersionReply,
    &::remote::ETHBACKEND::Stub::PrepareAsyncClientVersion
>;

using EtherbaseAwaitable = unary_awaitable<asio::io_context::executor_type, EtherbaseClient, ::remote::EtherbaseReply>;
using ProtocolVersionAwaitable = unary_awaitable<asio::io_context::executor_type, ProtocolVersionClient, ::remote::ProtocolVersionReply>;
using NetVersionAwaitable = unary_awaitable<asio::io_context::executor_type, NetVersionClient, ::remote::NetVersionReply>;
using ClientVersionAwaitable = unary_awaitable<asio::io_context::executor_type, ClientVersionClient, ::remote::ClientVersionReply>;

class BackEnd final {
public:
    explicit BackEnd(asio::io_context& context, std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue)
    : eb_awaitable_{context.get_executor(), channel, queue},
      pv_awaitable_{context.get_executor(), channel, queue},
      nv_awaitable_{context.get_executor(), channel, queue},
      cv_awaitable_{context.get_executor(), channel, queue} {
        SILKRPC_TRACE << "BackEnd::ctor " << this << "\n";
    }

    ~BackEnd() {
        SILKRPC_TRACE << "BackEnd::dtor " << this << "\n";
    }

    asio::awaitable<evmc::address> etherbase() {
        const auto start_time = clock_time::now();
        const auto reply = co_await eb_awaitable_.async_call(asio::use_awaitable);
        const auto h160_address = reply.address();
        const auto evmc_address{address_from_H160(h160_address)};
        SILKRPC_DEBUG << "BackEnd::etherbase address=" << evmc_address << " t=" << clock_time::since(start_time) << "\n";
        co_return evmc_address;
    }

    asio::awaitable<uint64_t> protocol_version() {
        const auto start_time = clock_time::now();
        const auto reply = co_await pv_awaitable_.async_call(asio::use_awaitable);
        const auto pv = reply.id();
        SILKRPC_DEBUG << "BackEnd::protocol_version version=" << pv << " t=" << clock_time::since(start_time) << "\n";
        co_return pv;
    }

    asio::awaitable<uint64_t> net_version() {
        const auto start_time = clock_time::now();
        const auto reply = co_await nv_awaitable_.async_call(asio::use_awaitable);
        const auto nv = reply.id();
        SILKRPC_DEBUG << "BackEnd::net_version version=" << nv << " t=" << clock_time::since(start_time) << "\n";
        co_return nv;
    }

    asio::awaitable<std::string> client_version() {
        const auto start_time = clock_time::now();
        const auto reply = co_await cv_awaitable_.async_call(asio::use_awaitable);
        const auto cv = reply.nodename();
        SILKRPC_DEBUG << "BackEnd::client_version version=" << cv << " t=" << clock_time::since(start_time) << "\n";
        co_return cv;
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

    EtherbaseAwaitable eb_awaitable_;
    ProtocolVersionAwaitable pv_awaitable_;
    NetVersionAwaitable nv_awaitable_;
    ClientVersionAwaitable cv_awaitable_;
};

} // namespace silkrpc::ethbackend

#endif // SILKRPC_ETHBACKEND_BACKEND_HPP_
