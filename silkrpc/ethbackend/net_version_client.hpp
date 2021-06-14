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

#ifndef SILKRPC_ETHBACKEND_NET_VERSION_CLIENT_HPP_
#define SILKRPC_ETHBACKEND_NET_VERSION_CLIENT_HPP_

#include <functional>
#include <memory>

#include <grpcpp/grpcpp.h>
#include <silkworm/common/magic_enum.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/async_completion_handler.hpp>
#include <silkrpc/interfaces/remote/ethbackend.grpc.pb.h>

namespace silkrpc::ethbackend {


class NetVersionClient final : public grpc::AsyncCompletionHandler {
    using AsyncResponseReaderPtr = std::unique_ptr<::grpc::ClientAsyncResponseReader<::remote::NetVersionReply>>;

    enum CallStatus { CALL_IDLE, CALL_STARTED, CALL_ENDED };

public:
    explicit NetVersionClient(std::shared_ptr<::grpc::Channel> channel, ::grpc::CompletionQueue* queue)
    : queue_(queue), stub_{remote::ETHBACKEND::NewStub(channel)} {
        SILKRPC_TRACE << "NetVersionClient::ctor " << this << " state: " << magic_enum::enum_name(state_) << "\n";
    }

    ~NetVersionClient() {
        SILKRPC_TRACE << "NetVersionClient::dtor " << this << " state: " << magic_enum::enum_name(state_) << "\n";
    }

    void net_version_call(std::function<void(const ::grpc::Status&, const ::remote::NetVersionReply&)> completed) {
        SILKRPC_TRACE << "NetVersionClient::protocol_version_call " << this << " state: " << magic_enum::enum_name(state_) << " start\n";
        completed_ = completed;
        ::grpc::ClientContext context;
        client_ = stub_->PrepareAsyncNetVersion(&context, ::remote::NetVersionRequest{}, queue_);
        state_ = CALL_STARTED;
        client_->StartCall();
        client_->Finish(&reply_, &result_, grpc::AsyncCompletionHandler::tag(this));
        SILKRPC_TRACE << "NetVersionClient::protocol_version_call " << this << " state: " << magic_enum::enum_name(state_) << " end\n";
    }

    void completed(bool ok) override {
        SILKRPC_TRACE << "NetVersionClient::completed " << this << " state: " << magic_enum::enum_name(state_) << " ok: " << ok << " start\n";
        if (state_ != CALL_STARTED) {
            throw std::runtime_error("NetVersionClient::completed unexpected state");
        }
        SILKRPC_TRACE << "NetVersionClient::completed result: " << result_.ok() << "\n";
        if (!result_.ok()) {
            SILKRPC_ERROR << "NetVersionClient::completed error_code: " << result_.error_code() << "\n";
            SILKRPC_ERROR << "NetVersionClient::completed error_message: " << result_.error_message() << "\n";
            SILKRPC_ERROR << "NetVersionClient::completed error_details: " << result_.error_details() << "\n";
        }
        state_ = CALL_ENDED;
        completed_(result_, reply_);
        SILKRPC_TRACE << "NetVersionClient::completed " << this << " state: " << magic_enum::enum_name(state_) << " end\n";
    }

private:
    ::grpc::CompletionQueue* queue_;
    std::unique_ptr<remote::ETHBACKEND::Stub> stub_;
    AsyncResponseReaderPtr client_;
    ::remote::NetVersionReply reply_;
    ::grpc::Status result_;
    CallStatus state_{CALL_IDLE};
    std::function<void(const ::grpc::Status&, const ::remote::NetVersionReply&)> completed_;
};

} // namespace silkrpc::ethbackend

#endif // SILKRPC_ETHBACKEND_NET_VERSION_CLIENT_HPP_
