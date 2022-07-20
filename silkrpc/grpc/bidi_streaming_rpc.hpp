/*
    Copyright 2022 The Silkrpc Authors

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

#ifndef SILKRPC_GRPC_BIDI_STREAMING_RPC_HPP_
#define SILKRPC_GRPC_BIDI_STREAMING_RPC_HPP_

#include <memory>
#include <system_error>
#include <utility>

#include <silkrpc/config.hpp>

#include <agrpc/rpc.hpp>
#include <asio/compose.hpp>
#include <asio/dispatch.hpp>
#include <asio/experimental/append.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/error.hpp>
#include <silkrpc/grpc/util.hpp>

namespace silkrpc {

namespace detail {
struct ReadDoneTag {
};
} // namespace detail

template<auto Rpc>
class BidiStreamingRpc;

template<
    typename Stub,
    typename Request,
    template<typename, typename> typename Responder,
    typename Reply,
    std::unique_ptr<Responder<Request, Reply>>(Stub::*Async)(grpc::ClientContext*, grpc::CompletionQueue*, void*)
>
class BidiStreamingRpc<Async> {
private:
     struct ReadNext {
        BidiStreamingRpc& self_;

        template<typename Op>
        void operator()(Op& op, bool ok) {
            SILKRPC_TRACE << "BidiStreamingRpc::ReadNext(op, ok): " << this << " START\n";
            if (ok) {
                SILKRPC_DEBUG << "BidiStreamingRpc::ReadNext(op, ok): rw=" << self_.reader_writer_.get() << " before read\n";
                agrpc::read(self_.reader_writer_, self_.reply_,
                    asio::bind_executor(self_.grpc_context_, asio::experimental::append(std::move(op), detail::ReadDoneTag{})));
                SILKRPC_DEBUG << "BidiStreamingRpc::ReadNext(op, ok): rw=" << self_.reader_writer_.get() << " after read\n";
            } else {
                self_.finish(std::move(op));
            }
        }

        template<typename Op>
        void operator()(Op& op, bool ok, detail::ReadDoneTag) {
            SILKRPC_TRACE << "BidiStreamingRpc::ReadNext(op, ok, ReadDoneTag): " << this << " ok=" << ok << "\n";
            if (ok) {
                op.complete({}, self_.reply_);
            } else {
                self_.finish(std::move(op));
            }
        }

        template<typename Op>
        void operator()(Op& op, const asio::error_code& ec) {
            SILKRPC_TRACE << "BidiStreamingRpc::ReadNext(op, ec): " << this << " ec=" << ec << "\n";
            op.complete(ec, self_.reply_);
        }
    };

    struct RequestAndRead : ReadNext {
        template<typename Op>
        void operator()(Op& op) {
            SILKRPC_TRACE << "BidiStreamingRpc::RequestAndRead::initiate rw=" << this->self_.reader_writer_.get() << " START\n";
            agrpc::request(Async, this->self_.stub_, this->self_.context_, this->self_.reader_writer_,
                asio::bind_executor(this->self_.grpc_context_, std::move(op)));
            SILKRPC_TRACE << "BidiStreamingRpc::RequestAndRead::initiate rw=" << this->self_.reader_writer_.get() << " END\n";
        }

        using ReadNext::operator();
    };

    struct WriteAndRead : ReadNext {
        const Request& request;

        template<typename Op>
        void operator()(Op& op) {
            SILKRPC_TRACE << "BidiStreamingRpc::WriteAndRead::initiate " << this << "\n";
            agrpc::write(this->self_.reader_writer_, request, asio::bind_executor(this->self_.grpc_context_, std::move(op)));
        }

        using ReadNext::operator();
    };

    struct WritesDoneAndFinish {
        BidiStreamingRpc& self_;

        template<typename Op>
        void operator()(Op& op) {
            SILKRPC_TRACE << "BidiStreamingRpc::WritesDoneAndFinish::initiate " << this << "\n";
            agrpc::writes_done(self_.reader_writer_, asio::bind_executor(self_.grpc_context_, std::move(op)));
        }

        template<typename Op>
        void operator()(Op& op, bool /*ok*/) {
            self_.finish(std::move(op));
        }

        template<typename Op>
        void operator()(Op& op, const asio::error_code& ec) {
            op.complete(ec);
        }
    };

    struct Finish {
        BidiStreamingRpc& self_;

        template<typename Op>
        void operator()(Op& op) {
            SILKRPC_TRACE << "BidiStreamingRpc::Finish::initiate " << this << "\n";
            agrpc::finish(self_.reader_writer_, self_.status_, asio::bind_executor(self_.grpc_context_, std::move(op)));
        }

        template<typename Op>
        void operator()(Op& op, bool ok) {
            // Check Finish result to treat any unknown error as such (strict)
            if (!ok) {
                self_.status_ = grpc::Status{grpc::StatusCode::UNKNOWN, "unknown error in finish"};
            }

            SILKRPC_DEBUG << "BidiStreamingRpc::Finish::completed ok=" << ok << " " << self_.status_ << "\n";
            if (self_.status_.ok()) {
                op.complete({});
            } else {
                op.complete(make_error_code(self_.status_.error_code(), self_.status_.error_message()));
            }
        }
    };

public:
    explicit BidiStreamingRpc(Stub& stub, agrpc::GrpcContext& grpc_context)
        : stub_(stub), grpc_context_(grpc_context) {}

    template<typename CompletionToken = agrpc::DefaultCompletionToken>
    auto request_and_read(CompletionToken&& token = {}) {
        return asio::async_compose<CompletionToken, void(asio::error_code, Reply&)>(RequestAndRead{*this}, token);
    }

    template<typename CompletionToken = agrpc::DefaultCompletionToken>
    auto write_and_read(const Request& request, CompletionToken&& token = {}) {
        return asio::async_compose<CompletionToken, void(asio::error_code, Reply&)>(WriteAndRead{*this, request}, token);
    }

    template<typename CompletionToken = agrpc::DefaultCompletionToken>
    auto writes_done_and_finish(CompletionToken&& token = {}) {
        return asio::async_compose<CompletionToken, void(asio::error_code)>(WritesDoneAndFinish{*this}, token);
    }

    auto get_executor() const noexcept {
        return grpc_context_.get_executor();
    }

private:
    template<typename CompletionToken = agrpc::DefaultCompletionToken>
    auto finish(CompletionToken&& token = {}) {
        return asio::async_compose<CompletionToken, void(asio::error_code)>(Finish{*this}, token);
    }

    Stub& stub_;
    agrpc::GrpcContext& grpc_context_;
    grpc::ClientContext context_;
    std::unique_ptr<Responder<Request, Reply>> reader_writer_;
    Reply reply_;
    grpc::Status status_;
};

} // namespace silkrpc

#endif // SILKRPC_GRPC_BIDI_STREAMING_RPC_HPP_
