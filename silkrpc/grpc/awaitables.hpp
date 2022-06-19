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
#include <utility>

#include <agrpc/rpc.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/experimental/append.hpp>
#include <boost/asio/detail/non_const_lvalue.hpp>
#include <grpcpp/grpcpp.h>

#include <silkworm/common/util.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/log.hpp>
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

namespace detail {
struct DoneTag {
};

template<typename Executor>
struct ExecutorDispatcher {
    Executor executor_;

    template<typename CompletionToken, typename... Args>
    void dispatch(CompletionToken&& token, Args&&... args){
                boost::asio::dispatch(boost::asio::bind_executor(executor_, boost::asio::experimental::append(std::forward<CompletionToken>(token), std::forward<Args>(args)...)));

    }
};   
    
struct InlineDispatcher {
    template<typename CompletionToken, typename... Args>
    void dispatch(CompletionToken&& token, Args&&... args){
            std::forward<CompletionToken>(token)(std::forward<Args>(args)...);

    }
};
}

template<auto Rpc>
class UnaryRpc;

template<
    typename Stub,
    typename Request,
    template<typename> typename Reader,
    typename Reply,
    std::unique_ptr<Reader<Reply>>(Stub::*Async)(grpc::ClientContext*, const Request&, grpc::CompletionQueue*)
>
class UnaryRpc<Async> {
private:
    template<typename Dispatcher>
    struct Call {
        UnaryRpc& self_;
        const Request& request_;
        [[no_unique_address]] Dispatcher dispatcher_;

        template<typename Op>
        void operator()(Op& op) {
            SILKRPC_TRACE << "UnaryRpc::initiate " << this << "\n";
            self_.reader_ = agrpc::request(Async, self_.stub_, self_.context_, request_, self_.grpc_context_);
            agrpc::finish(self_.reader_, self_.reply_, self_.status_, 
                boost::asio::bind_executor(self_.grpc_context_, std::move(op)));
        }

        template<typename Op>
        void operator()(Op& op, bool /*ok*/) {
            dispatcher_.dispatch(std::move(op), detail::DoneTag{});
        }

        template<typename Op>
        void operator()(Op& op, detail::DoneTag) {
            auto& status = self_.status_;
            SILKRPC_TRACE << "UnaryRpc::completed result: " << status.ok() << "\n";
            if (status.ok()) {
                op.complete({}, std::move(self_.reply_));
            } else {
                SILKRPC_ERROR << "UnaryRpc::completed error_code: " << status.error_code() << "\n";
                SILKRPC_ERROR << "UnaryRpc::completed error_message: " << status.error_message() << "\n";
                SILKRPC_ERROR << "UnaryRpc::completed error_details: " << status.error_details() << "\n";
                op.complete(make_error_code(status.error_code(), status.error_message()), {});
            }
        }
    };

public:
    explicit UnaryRpc(Stub& stub, agrpc::GrpcContext& grpc_context)
    : stub_(stub), grpc_context_(grpc_context) {}

    template<typename CompletionToken = agrpc::DefaultCompletionToken>
    auto finish(const Request& request, CompletionToken&& token = {}) {
        return boost::asio::async_compose<CompletionToken, void(boost::system::error_code, Reply)>(
            Call<detail::InlineDispatcher>{*this, request}, token);
    }

    template<typename Executor, typename CompletionToken = agrpc::DefaultCompletionToken>
    auto finish_on(const Executor& executor, const Request& request, CompletionToken&& token = {}) {
        return boost::asio::async_compose<CompletionToken, void(boost::system::error_code, Reply)>(
            Call<detail::ExecutorDispatcher<Executor>>{*this, request, executor}, token, executor);
    }

private:
    Stub& stub_;
    agrpc::GrpcContext& grpc_context_;
    grpc::ClientContext context_;
    std::unique_ptr<Reader<Reply>> reader_;
    Reply reply_;
    grpc::Status status_;
};

} // namespace silkrpc

#endif // SILKRPC_GRPC_AWAITABLES_HPP_
