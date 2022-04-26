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

#ifndef SILKRPC_GRPC_ASYNC_UNARY_CLIENT_HPP_
#define SILKRPC_GRPC_ASYNC_UNARY_CLIENT_HPP_

#include <functional>
#include <memory>

#include <grpcpp/grpcpp.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/stub_options.h>
#include <magic_enum.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/async_completion_handler.hpp>
#include <silkworm/rpc/completion_tag.hpp>

namespace silkrpc {

template<
    typename StubInterface,
    typename Request,
    typename Reply,
    std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<Reply>>(StubInterface::*PrepareAsync)(grpc::ClientContext*, const Request&, grpc::CompletionQueue*)
>
class AsyncUnaryClient final : public AsyncCompletionHandler {
    using AsyncResponseReaderPtr = std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<Reply>>;

    enum CallStatus { CALL_IDLE, CALL_STARTED, CALL_ENDED };

public:
    explicit AsyncUnaryClient(std::unique_ptr<StubInterface>& stub, grpc::CompletionQueue* queue)
    : stub_(stub), queue_(queue) {
        SILKRPC_TRACE << "AsyncUnaryClient::ctor " << this << " state: " << magic_enum::enum_name(state_) << " start\n";
        finish_processor_ = [this](bool ok) { completed(ok); };
        SILKRPC_TRACE << "AsyncUnaryClient::ctor " << this << " state: " << magic_enum::enum_name(state_) << " end\n";
    }

    ~AsyncUnaryClient() {
        SILKRPC_TRACE << "AsyncUnaryClient::dtor " << this << " state: " << magic_enum::enum_name(state_) << "\n";
    }

    void async_call(const Request& request, std::function<void(const grpc::Status&, const Reply&)> completed) {
        SILKRPC_TRACE << "AsyncUnaryClient::async_call " << this << " state: " << magic_enum::enum_name(state_) << " start\n";
        completed_ = completed;
        client_ = (stub_.get()->*PrepareAsync)(&context_, request, queue_);
        state_ = CALL_STARTED;
        client_->StartCall();
        client_->Finish(&reply_, &result_, &finish_processor_);
        SILKRPC_TRACE << "AsyncUnaryClient::async_call " << this << " state: " << magic_enum::enum_name(state_) << " end\n";
    }

    void completed(bool ok) override {
        SILKRPC_TRACE << "AsyncUnaryClient::completed " << this << " state: " << magic_enum::enum_name(state_) << " ok: " << ok << " start\n";
        if (state_ != CALL_STARTED) {
            throw std::runtime_error("AsyncUnaryClient::completed unexpected state");
        }
        SILKRPC_TRACE << "AsyncUnaryClient::completed result: " << result_.ok() << "\n";
        if (!result_.ok()) {
            SILKRPC_ERROR << "AsyncUnaryClient::completed error_code: " << result_.error_code() << "\n";
            SILKRPC_ERROR << "AsyncUnaryClient::completed error_message: " << result_.error_message() << "\n";
            SILKRPC_ERROR << "AsyncUnaryClient::completed error_details: " << result_.error_details() << "\n";
        }
        state_ = CALL_ENDED;
        completed_(result_, reply_);
        SILKRPC_TRACE << "AsyncUnaryClient::completed " << this << " state: " << magic_enum::enum_name(state_) << " end\n";
    }

private:
    grpc::CompletionQueue* queue_;
    std::unique_ptr<StubInterface>& stub_;
    grpc::ClientContext context_;
    AsyncResponseReaderPtr client_;
    Reply reply_;
    grpc::Status result_;
    CallStatus state_{CALL_IDLE};
    silkworm::rpc::TagProcessor finish_processor_;
    std::function<void(const grpc::Status&, const Reply&)> completed_;
};

} // namespace silkrpc

#endif // SILKRPC_GRPC_ASYNC_UNARY_CLIENT_HPP_
