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

#include <silkrpc/config.hpp>

#include <asio/io_context.hpp>
#include <asio/use_awaitable.hpp>
#include <boost/endian/conversion.hpp>
#include <evmc/evmc.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/clock_time.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/ethbackend/awaitables.hpp>
#include <silkrpc/ethbackend/client.hpp>
#include <silkrpc/interfaces/types/types.pb.h>

namespace silkrpc::ethbackend {

class BackEnd final {
public:
    explicit BackEnd(asio::io_context& context, std::shared_ptr<::grpc::Channel> channel, ::grpc::CompletionQueue* queue)
    : context_(context), eb_client_{channel, queue}, eb_awaitable_{context_, eb_client_} {
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

    asio::io_context& context_;
    EtherbaseClient eb_client_;
    EtherbaseAsioAwaitable<asio::io_context::executor_type> eb_awaitable_;
};

} // namespace silkrpc::ethbackend

#endif // SILKRPC_ETHBACKEND_BACKEND_HPP_
