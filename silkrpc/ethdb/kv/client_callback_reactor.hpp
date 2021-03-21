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
#include <silkrpc/ethdb/kv/remote/kv.grpc.pb.h>

namespace silkrpc::ethdb::kv {

class ClientCallbackReactor final : public grpc::experimental::ClientBidiReactor<remote::Cursor, remote::Pair> {
    enum CallStatus { CALL_IDLE, READ_STARTED, WRITE_STARTED, CALL_ENDED };

public:
    explicit ClientCallbackReactor(std::shared_ptr<grpc::Channel> channel) : stub_{remote::KV::NewStub(channel)} {
        SILKRPC_TRACE << "ClientCallbackReactor::ctor " << this << " start\n";
        status_ = CALL_IDLE;
        stub_->experimental_async()->Tx(&context_, this);
        SILKRPC_TRACE << "ClientCallbackReactor::ctor " << this << " end\n";
    }

    void end_call(std::function<void(const grpc::Status&)> end_completed) {
        SILKRPC_TRACE << "ClientCallbackReactor::end_call " << this << " start\n";
        end_completed_ = end_completed;
        status_ = CALL_ENDED;
        StartWritesDone();
        SILKRPC_TRACE << "ClientCallbackReactor::end_call " << this << " end\n";
    }

    void read_start(std::function<void(const grpc::Status&, remote::Pair)> read_completed) {
        SILKRPC_TRACE << "ClientCallbackReactor::read_start " << this << " start\n";
        read_completed_ = read_completed;
        status_ = READ_STARTED;
        StartRead(&pair_);
        SILKRPC_TRACE << "ClientCallbackReactor::read_start " << this << " end\n";
    }

    void write_start(remote::Cursor* cursor, std::function<void(const grpc::Status&)> write_completed) {
        SILKRPC_TRACE << "ClientCallbackReactor::write_start " << this << " start\n";
        write_completed_ = write_completed;
        bool not_started = status_ == CALL_IDLE;
        status_ = WRITE_STARTED;
        StartWrite(cursor);
        if (not_started) {
            SILKRPC_TRACE << "ClientCallbackReactor::write_start StartCall start\n";
            StartCall();
            SILKRPC_TRACE << "ClientCallbackReactor::write_start StartCall end\n";
        }
        SILKRPC_TRACE << "ClientCallbackReactor::write_start " << this << " end\n";
    }

    void OnReadDone(bool ok) override {
        SILKRPC_TRACE << "ClientCallbackReactor::OnReadDone " << this << " start ok: " << ok << "\n";
        if (ok) {
            read_completed_(grpc::Status{}, pair_);
        }
        SILKRPC_TRACE << "ClientCallbackReactor::OnReadDone " << this << " end\n";
    }

    void OnWriteDone(bool ok) override {
        SILKRPC_TRACE << "ClientCallbackReactor::OnWriteDone " << this << " start ok: " << ok << "\n";
        if (ok) {
            write_completed_(grpc::Status{});
        }
        SILKRPC_TRACE << "ClientCallbackReactor::OnWriteDone " << this << " end\n";
    }

    void OnDone(const grpc::Status& status) override {
        SILKRPC_TRACE << "ClientCallbackReactor::OnDone start " << this << " ok: " << status.ok() << "\n";
        if (!status.ok()) {
            SILKRPC_TRACE << "ClientCallbackReactor::OnDone error_code: " << status.error_code() << "\n";
            SILKRPC_TRACE << "ClientCallbackReactor::OnDone error_message: " << status.error_message() << "\n";
            SILKRPC_TRACE << "ClientCallbackReactor::OnDone error_details: " << status.error_details() << "\n";
        }
        SILKRPC_TRACE << "ClientCallbackReactor::OnDone status: " << status_ << "\n";
        switch (status_) {
            case WRITE_STARTED:
                write_completed_(status);
            break;
            case READ_STARTED:
                read_completed_(status, pair_);
            break;
            case CALL_ENDED:
                end_completed_(status);
            break;
            default:
            break;
        }
        SILKRPC_TRACE << "ClientCallbackReactor::OnDone " << this << " end\n";
    }

private:
    std::unique_ptr<remote::KV::Stub> stub_;
    grpc::ClientContext context_;
    remote::Pair pair_;
    CallStatus status_;
    std::function<void(const grpc::Status&, remote::Pair)> read_completed_;
    std::function<void(const grpc::Status&)> write_completed_;
    std::function<void(const grpc::Status&)> end_completed_;
};

} // namespace silkrpc::ethdb::kv

#endif // SILKRPC_KV_CLIENT_CALLBACK_REACTOR_HPP
