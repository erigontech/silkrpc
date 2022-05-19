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

#ifndef SILKRPC_GRPC_AWAITABLES_HPP_
#define SILKRPC_GRPC_AWAITABLES_HPP_

#include <silkrpc/config.hpp>

#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
#include <utility>

#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/non_const_lvalue.hpp>
#include <boost/asio/error.hpp>
#include <grpcpp/grpcpp.h>

#include <silkworm/common/util.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/grpc/error.hpp>
#include <silkrpc/grpc/async_operation.hpp>

namespace silkrpc {

template<typename Executor, typename UnaryClient, typename StubInterface, typename Request, typename Reply>
struct unary_awaitable;

template<typename Executor, typename UnaryClient, typename StubInterface, typename Request>
struct unary_awaitable<Executor, UnaryClient, StubInterface, Request, void>;

template<typename Executor, typename UnaryClient, template<class, class, class> typename AsyncReplyOperation, typename StubInterface, typename Request, typename Reply>
class initiate_unary_async {
public:
    typedef Executor executor_type;

    explicit initiate_unary_async(unary_awaitable<Executor, UnaryClient, StubInterface, Request, Reply>* self, const Request& request)
    : self_(self), request_(request) {}

    executor_type get_executor() const noexcept { return self_->executor_; }

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        boost::asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        using OP = AsyncReplyOperation<WaitHandler, Executor, Reply>;
        typename OP::ptr p = {boost::asio::detail::addressof(handler2.value), OP::ptr::allocate(handler2.value), 0};
        wrapper_ = new OP(handler2.value, self_->executor_);

        self_->client_.async_call(request_, [this](const grpc::Status& status, const Reply& reply) {
            using OP = AsyncReplyOperation<WaitHandler, Executor, Reply>;
            auto async_reply_op = static_cast<OP*>(wrapper_);
            if (status.ok()) {
                async_reply_op->complete(this, {}, reply);
            } else {
                async_reply_op->complete(this, make_error_code(status.error_code(), status.error_message()), {});
            }
        });
    }

private:
    unary_awaitable<Executor, UnaryClient, StubInterface, Request, Reply>* self_;
    const Request& request_;
    void* wrapper_;
};

template<typename Executor, typename UnaryClient, template<class, class, class> typename AsyncReplyOperation, typename StubInterface, typename Request>
class initiate_unary_async<Executor, UnaryClient, AsyncReplyOperation, StubInterface, Request, void> {
public:
    typedef Executor executor_type;

    explicit initiate_unary_async(unary_awaitable<Executor, UnaryClient, StubInterface, Request, void>* self, const Request& request)
    : self_(self), request_(request) {}

    executor_type get_executor() const noexcept { return self_->executor_; }

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        boost::asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        using OP = AsyncReplyOperation<WaitHandler, Executor, void>;
        typename OP::ptr p = {boost::asio::detail::addressof(handler2.value), OP::ptr::allocate(handler2.value), 0};
        wrapper_ = new OP(handler2.value, self_->executor_);

        self_->client_.async_call(request_, [this](const grpc::Status& status) {
            using OP = AsyncReplyOperation<WaitHandler, Executor, void>;
            auto async_reply_op = static_cast<OP*>(wrapper_);
            if (status.ok()) {
                async_reply_op->complete(this, {});
            } else {
                async_reply_op->complete(this, make_error_code(status.error_code(), status.error_message()));
            }
        });
    }

private:
    unary_awaitable<Executor, UnaryClient, StubInterface, Request, void>* self_;
    const Request& request_;
    void* wrapper_;
};

template<typename Executor, typename UnaryClient, typename StubInterface, typename Request, typename Reply>
struct unary_awaitable {
    typedef Executor executor_type;

    explicit unary_awaitable(const Executor& executor, std::unique_ptr<StubInterface>& stub, grpc::CompletionQueue* queue)
    : executor_(executor), client_{stub, queue} {}

    template<typename WaitHandler>
    auto async_call(const Request& request, WaitHandler&& handler) {
        return boost::asio::async_initiate<WaitHandler, void(boost::system::error_code, Reply)>(
            initiate_unary_async<Executor, UnaryClient, async_reply_operation, StubInterface, Request, Reply>{this, request}, handler);
    }

    const Executor& executor_;
    UnaryClient client_;
};

template<typename Executor, typename UnaryClient, typename StubInterface, typename Request>
struct unary_awaitable<Executor, UnaryClient, StubInterface, Request, void> {
    typedef Executor executor_type;

    explicit unary_awaitable(const Executor& executor, std::unique_ptr<StubInterface>& stub, grpc::CompletionQueue* queue)
    : executor_(executor), client_{stub, queue} {}

    template<typename WaitHandler>
    auto async_call(const Request& request, WaitHandler&& handler) {
        return boost::asio::async_initiate<WaitHandler, void(boost::system::error_code)>(
            initiate_unary_async<Executor, UnaryClient, async_reply_operation, StubInterface, Request, void>{this, request}, handler);
    }

    const Executor& executor_;
    UnaryClient client_;
};

} // namespace silkrpc

#endif // SILKRPC_GRPC_AWAITABLES_HPP_
