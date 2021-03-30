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

#ifndef SILKRPC_KV_CLIENT_CALLBACK_REACTOR_HPP
#define SILKRPC_KV_CLIENT_CALLBACK_REACTOR_HPP

#include <functional>
#include <memory>

#include <grpcpp/grpcpp.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/async_completion_handler.hpp>
#include <silkrpc/ethdb/kv/remote/kv.grpc.pb.h>

namespace silkrpc::ethdb::kv {

typedef std::unique_ptr<::grpc::ClientAsyncReaderWriterInterface<::remote::Cursor, ::remote::Pair>> ClientAsyncReaderWriterPtr;

class ClientCallbackReactor final : public grpc::AsyncCompletionHandler {
    enum CallStatus { CALL_IDLE, CALL_STARTED, READ_STARTED, WRITE_STARTED, CALL_ENDED };

public:
    explicit ClientCallbackReactor(std::shared_ptr<::grpc::Channel> channel, ::grpc::CompletionQueue* queue)
    : stub_{remote::KV::NewStub(channel)}, stream_{stub_->PrepareAsyncTx(&context_, queue)} {
        SILKRPC_TRACE << "ClientCallbackReactor::ctor " << this << " start\n";
        status_ = CALL_IDLE;
        SILKRPC_TRACE << "ClientCallbackReactor::ctor " << this << " end\n";
    }

    ~ClientCallbackReactor() {
        SILKRPC_TRACE << "ClientCallbackReactor::dtor " << this << " start\n";
        SILKRPC_TRACE << "ClientCallbackReactor::dtor " << this << " end\n";
    }

    void start_call(std::function<void(const ::grpc::Status&)> start_completed) {
        SILKRPC_TRACE << "ClientCallbackReactor::start_call " << this << " start\n";
        start_completed_ = start_completed;
        status_ = CALL_STARTED;
        stream_->StartCall(grpc::AsyncCompletionHandler::tag(this));
        SILKRPC_TRACE << "ClientCallbackReactor::start_call " << this << " end\n";
    }

    void end_call(std::function<void(const ::grpc::Status&)> end_completed) {
        SILKRPC_TRACE << "ClientCallbackReactor::end_call " << this << " start\n";
        end_completed_ = end_completed;
        status_ = CALL_ENDED;
        stream_->WritesDone(this);
        stream_->Finish(&result_, grpc::AsyncCompletionHandler::tag(this));
        SILKRPC_TRACE << "ClientCallbackReactor::end_call " << this << " end\n";
    }

    void read_start(std::function<void(const ::grpc::Status&, ::remote::Pair)> read_completed) {
        SILKRPC_TRACE << "ClientCallbackReactor::read_start " << this << " start\n";
        read_completed_ = read_completed;
        status_ = READ_STARTED;
        SILKRPC_TRACE << "ClientCallbackReactor::read_start " << this << " stream: " << stream_ << " BEFORE Read\n";
        stream_->Read(&pair_, grpc::AsyncCompletionHandler::tag(this));
        SILKRPC_TRACE << "ClientCallbackReactor::read_start " << this << " AFTER Read\n";
        SILKRPC_TRACE << "ClientCallbackReactor::read_start " << this << " end\n";
    }

    void write_start(const ::remote::Cursor& cursor, std::function<void(const ::grpc::Status&)> write_completed) {
        SILKRPC_TRACE << "ClientCallbackReactor::write_start " << this << " stream: " << stream_ << " start\n";
        write_completed_ = write_completed;
        status_ = WRITE_STARTED;
        stream_->Write(cursor, grpc::AsyncCompletionHandler::tag(this));
        SILKRPC_TRACE << "ClientCallbackReactor::write_start " << this << " end\n";
    }

    void completed(bool ok) override {
        SILKRPC_TRACE << "ClientCallbackReactor::completed start " << this << " status: " << status_ << " ok: " << ok << "\n";
        if (!ok) {
            stream_->Finish(&result_, grpc::AsyncCompletionHandler::tag(this));
            return;
        }
        SILKRPC_TRACE << "ClientCallbackReactor::completed result: " << result_.ok() << "\n";
        if (!result_.ok()) {
            SILKRPC_TRACE << "ClientCallbackReactor::completed error_code: " << result_.error_code() << "\n";
            SILKRPC_TRACE << "ClientCallbackReactor::completed error_message: " << result_.error_message() << "\n";
            SILKRPC_TRACE << "ClientCallbackReactor::completed error_details: " << result_.error_details() << "\n";
        }
        switch (status_) {
            case CALL_STARTED:
                start_completed_(result_);
            break;
            case WRITE_STARTED:
                write_completed_(result_);
            break;
            case READ_STARTED:
                read_completed_(result_, pair_);
            break;
            case CALL_ENDED:
                end_completed_(result_);
            break;
            default:
            break;
        }
        SILKRPC_TRACE << "ClientCallbackReactor::completed " << this << " end\n";
    }

    void try_cancel() override {
        SILKRPC_TRACE << "ClientCallbackReactor::try_cancel " << this << " start\n";
        context_.TryCancel();
        SILKRPC_TRACE << "ClientCallbackReactor::try_cancel " << this << " end\n";
    }

private:
    std::unique_ptr<remote::KV::Stub> stub_;
    ::grpc::ClientContext context_;
    ClientAsyncReaderWriterPtr stream_;
    ::remote::Pair pair_;
    ::grpc::Status result_;
    CallStatus status_;
    std::function<void(const ::grpc::Status&)> start_completed_;
    std::function<void(const ::grpc::Status&, remote::Pair)> read_completed_;
    std::function<void(const ::grpc::Status&)> write_completed_;
    std::function<void(const ::grpc::Status&)> end_completed_;
};

} // namespace silkrpc::ethdb::kv

#endif // SILKRPC_KV_CLIENT_CALLBACK_REACTOR_HPP
