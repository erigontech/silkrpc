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

#ifndef SILKRPC_GRPC_UNARY_RPC_HPP_
#define SILKRPC_GRPC_UNARY_RPC_HPP_

#include <silkrpc/config.hpp>

#include <memory>
#include <system_error>
#include <utility>

#include <agrpc/rpc.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/experimental/append.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/grpc/error.hpp>
#include <silkrpc/common/log.hpp>

namespace silkrpc {

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

    auto get_executor() const noexcept {
        return grpc_context_.get_executor();
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

#endif // SILKRPC_GRPC_UNARY_RPC_HPP_
