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

#ifndef SILKRPC_GRPC_BIDI_STREAMING_RPC_HPP_
#define SILKRPC_GRPC_BIDI_STREAMING_RPC_HPP_

#include <silkrpc/config.hpp>

#include <memory>
#include <system_error>
#include <utility>

#include <agrpc/rpc.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/experimental/append.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/error.hpp>

namespace silkrpc {

namespace detail {
struct ReadDoneTag {
};
}

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
            if (ok) {
                agrpc::read(self_.reader_writer_, self_.reply_, 
                    boost::asio::bind_executor(self_.grpc_context_, boost::asio::experimental::append(std::move(op), detail::ReadDoneTag{})));
            } else{
                self_.finish(std::move(op));
            }
        }

        template<typename Op>
        void operator()(Op& op, bool ok, detail::ReadDoneTag) {
            if (ok) {
                op.complete({}, self_.reply_);
            } else {
                self_.finish(std::move(op));
            }
        }

        template<typename Op>
        void operator()(Op& op, const boost::system::error_code& ec) {
            op.complete(ec, self_.reply_);
        }
    }; 

    struct RequestAndRead : ReadNext {
        template<typename Op>
        void operator()(Op& op) {
            SILKRPC_TRACE << "BidiStreamingRpc::RequestAndRead::initiate " << this << "\n";
            agrpc::request(Async, this->self_.stub_, this->self_.context_, this->self_.reader_writer_, 
                boost::asio::bind_executor(this->self_.grpc_context_, std::move(op)));
        }

        using ReadNext::operator();
    };    
    
    struct WriteAndRead : ReadNext {
        const Request& request;

        template<typename Op>
        void operator()(Op& op) {
            SILKRPC_TRACE << "BidiStreamingRpc::WriteAndRead::initiate " << this << "\n";
            agrpc::write(this->self_.reader_writer_, request, boost::asio::bind_executor(this->self_.grpc_context_, std::move(op)));
        }

        using ReadNext::operator();
    };

    struct WritesDoneAndFinish {
        BidiStreamingRpc& self_;

        template<typename Op>
        void operator()(Op& op) {
            SILKRPC_TRACE << "BidiStreamingRpc::WritesDoneAndFinish::initiate " << this << "\n";
            agrpc::writes_done(self_.reader_writer_, boost::asio::bind_executor(self_.grpc_context_, std::move(op)));
        }

        template<typename Op>
        void operator()(Op& op, bool /*ok*/) {
            self_.finish(std::move(op));
        }

        template<typename Op>
        void operator()(Op& op, const boost::system::error_code& ec) {
            op.complete(ec);
        }
    };

    struct Finish {
        BidiStreamingRpc& self_;

        template<typename Op>
        void operator()(Op& op) {
            SILKRPC_TRACE << "BidiStreamingRpc::Finish::initiate " << this << "\n";
            agrpc::finish(self_.reader_writer_, self_.status_, boost::asio::bind_executor(self_.grpc_context_, std::move(op)));
        }

        template<typename Op>
        void operator()(Op& op, bool /*ok*/) {
            auto& status = self_.status_;
            if (status.ok()) {
                op.complete({});
            } else {
                SILKRPC_ERROR << "BidiStreamingRpc::Finish::completed error_code: " << status.error_code() << "\n";
                SILKRPC_ERROR << "BidiStreamingRpc::Finish::completed error_message: " << status.error_message() << "\n";
                SILKRPC_ERROR << "BidiStreamingRpc::Finish::completed error_details: " << status.error_details() << "\n";
                op.complete(make_error_code(status.error_code(), status.error_message()));
            }
        }
    };

public:
    explicit BidiStreamingRpc(Stub& stub, agrpc::GrpcContext& grpc_context)
    : stub_(stub), grpc_context_(grpc_context) {}

    template<typename CompletionToken = agrpc::DefaultCompletionToken>
    auto request_and_read(CompletionToken&& token = {}) {
        return boost::asio::async_compose<CompletionToken, void(boost::system::error_code, Reply&)>(
            RequestAndRead{*this}, token);
    }

    template<typename CompletionToken = agrpc::DefaultCompletionToken>
    auto write_and_read(const Request& request, CompletionToken&& token = {}) {
        return boost::asio::async_compose<CompletionToken, void(boost::system::error_code, Reply&)>(
            WriteAndRead{*this, request}, token);
    }

    template<typename CompletionToken = agrpc::DefaultCompletionToken>
    auto writes_done_and_finish(CompletionToken&& token = {}) {
        return boost::asio::async_compose<CompletionToken, void(boost::system::error_code)>(
            WritesDoneAndFinish{*this}, token);
    }

    auto get_executor() const noexcept {
        return grpc_context_.get_executor();
    }

private:
    template<typename CompletionToken = agrpc::DefaultCompletionToken>
    auto finish(CompletionToken&& token = {}) {
        return boost::asio::async_compose<CompletionToken, void(boost::system::error_code)>(
            Finish{*this}, token);
    }

    Stub& stub_;
    agrpc::GrpcContext& grpc_context_;
    grpc::ClientContext context_;
    std::unique_ptr<Responder<Request, Reply>> reader_writer_;
    Reply reply_;
    grpc::Status status_;
};

} // namespace silkrpc::ethdb::kv

#endif // SILKRPC_GRPC_BIDI_STREAMING_RPC_HPP_
