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

#ifndef SILKRPC_ETHBACKEND_AWAITABLES_HPP_
#define SILKRPC_ETHBACKEND_AWAITABLES_HPP_

#include <silkrpc/config.hpp>

#include <functional>
#include <string>
#include <system_error>
#include <thread>
#include <utility>

#include <asio/async_result.hpp>
#include <asio/detail/non_const_lvalue.hpp>
#include <asio/error.hpp>
#include <asio/io_context.hpp>
#include <grpcpp/grpcpp.h>

#include <silkworm/common/util.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/ethbackend/client.hpp>
#include <silkrpc/ethbackend/async_etherbase.hpp>
#include <silkrpc/ethbackend/async_protocol_version.hpp>
#include <silkrpc/ethbackend/error.hpp>
#include <silkrpc/interfaces/remote/ethbackend.grpc.pb.h>

namespace silkrpc::ethbackend {

template<typename Executor>
struct EtherbaseAsioAwaitable;

template<typename Executor>
struct ProtocolVersionAsioAwaitable;

template<typename Executor>
class initiate_async_etherbase {
public:
    typedef Executor executor_type;

    explicit initiate_async_etherbase(EtherbaseAsioAwaitable<Executor>* self)
    : self_(self) {}

    executor_type get_executor() const noexcept { return self_->get_executor(); }

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        using OP = silkrpc::ethbackend::async_etherbase<WaitHandler, Executor>;
        typename OP::ptr p = {asio::detail::addressof(handler2.value), OP::ptr::allocate(handler2.value), 0};
        wrapper_ = new OP(handler2.value, self_->context_.get_executor());

        self_->client_.finish_call([this](const ::grpc::Status& status, const ::remote::EtherbaseReply& reply) {
            using OP = silkrpc::ethbackend::async_etherbase<WaitHandler, Executor>;
            auto etherbase_op = static_cast<OP*>(wrapper_);
            if (status.ok()) {
                etherbase_op->complete(this, {}, reply);
            } else {
                etherbase_op->complete(this, make_error_code(status.error_code(), status.error_message()), {});
            }
        });
    }

private:
    EtherbaseAsioAwaitable<Executor>* self_;
    void* wrapper_;
};

template<typename Executor>
struct EtherbaseAsioAwaitable {
    typedef Executor executor_type;

    explicit EtherbaseAsioAwaitable(asio::io_context& context, EtherbaseClient& client)
    : context_(context), client_(client) {}

    template<typename WaitHandler>
    auto async_call(WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(asio::error_code, remote::EtherbaseReply)>(initiate_async_etherbase{this}, handler);
    }

    asio::io_context& context_;
    EtherbaseClient& client_;
};

template<typename Executor>
class initiate_async_protocol_version {
public:
    typedef Executor executor_type;

    explicit initiate_async_protocol_version(ProtocolVersionAsioAwaitable<Executor>* self)
    : self_(self) {}

    executor_type get_executor() const noexcept { return self_->get_executor(); }

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        using OP = silkrpc::ethbackend::async_protocolVersion<WaitHandler, Executor>;
        typename OP::ptr p = {asio::detail::addressof(handler2.value), OP::ptr::allocate(handler2.value), 0};
        wrapper_ = new OP(handler2.value, self_->context_.get_executor());

        self_->client_.protocol_version_call([this](const ::grpc::Status& status, const ::remote::ProtocolVersionReply& reply) {
            using OP = silkrpc::ethbackend::async_protocolVersion<WaitHandler, Executor>;
            auto protocol_version_op = static_cast<OP*>(wrapper_);
            if (status.ok()) {
                protocol_version_op->complete(this, {}, reply);
            } else {
                protocol_version_op->complete(this, make_error_code(status.error_code(), status.error_message()), {});
            }
        });
    }

private:
    ProtocolVersionAsioAwaitable<Executor>* self_;
    void* wrapper_;
};

template<typename Executor>
struct ProtocolVersionAsioAwaitable {
    typedef Executor executor_type;

    explicit ProtocolVersionAsioAwaitable(asio::io_context& context, ProtocolVersionClient& client)
    : context_(context), client_(client) {}

    template<typename WaitHandler>
    auto async_call(WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(asio::error_code, remote::ProtocolVersionReply)>(initiate_async_protocol_version{this}, handler);
    }

    asio::io_context& context_;
    ProtocolVersionClient& client_;
};

} // namespace silkrpc::ethbackend

#endif // SILKRPC_ETHBACKEND_AWAITABLES_HPP_
