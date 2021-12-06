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

#ifndef SILKRPC_ETHDB_KV_TX_STREAMING_CLIENT_HPP_
#define SILKRPC_ETHDB_KV_TX_STREAMING_CLIENT_HPP_

#include <functional>
#include <memory>

#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/async_streaming_client.hpp>
#include <silkrpc/interfaces/remote/kv.grpc.pb.h>

namespace silkrpc::ethdb::kv {

using ClientAsyncReaderWriterPtr = std::unique_ptr<grpc::ClientAsyncReaderWriterInterface<remote::Cursor, remote::Pair>>;
using AsyncTxStreamingClient = AsyncStreamingClient<remote::Cursor, remote::Pair>;

class TxStreamingClient final : public AsyncTxStreamingClient {
    enum CallStatus { CALL_IDLE, CALL_STARTED, READ_STARTED, WRITE_STARTED, DONE_STARTED, CALL_ENDED };

public:
    explicit TxStreamingClient(std::unique_ptr<remote::KV::StubInterface>& stub, grpc::CompletionQueue* queue)
    : stub_(stub), stream_{stub_->PrepareAsyncTx(&context_, queue)} {
        SILKRPC_TRACE << "TxStreamingClient::ctor " << this << " start\n";
        status_ = CALL_IDLE;
        SILKRPC_TRACE << "TxStreamingClient::ctor " << this << " status: " << status_ << " end\n";
    }

    ~TxStreamingClient() {
        SILKRPC_TRACE << "TxStreamingClient::dtor " << this << " status: " << status_ << "\n";
    }

    void start_call(std::function<void(const grpc::Status&)> start_completed) override {
        SILKRPC_TRACE << "TxStreamingClient::start_call " << this << " status: " << status_ << " start\n";
        start_completed_ = start_completed;
        status_ = CALL_STARTED;
        stream_->StartCall(AsyncCompletionHandler::tag(this));
        SILKRPC_TRACE << "TxStreamingClient::start_call " << this << " status: " << status_ << " end\n";
    }

    void end_call(std::function<void(const grpc::Status&)> end_completed) override {
        SILKRPC_TRACE << "TxStreamingClient::end_call " << this << " status: " << status_ << " start\n";
        end_completed_ = end_completed;
        status_ = DONE_STARTED;
        stream_->WritesDone(this);
        SILKRPC_TRACE << "TxStreamingClient::end_call " << this << " status: " << status_ << " end\n";
    }

    void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed)  override {
        SILKRPC_TRACE << "TxStreamingClient::read_start " << this << " status: " << status_ << " start\n";
        read_completed_ = read_completed;
        status_ = READ_STARTED;
        SILKRPC_TRACE << "TxStreamingClient::read_start " << this << " stream: " << stream_.get() << " BEFORE Read\n";
        stream_->Read(&pair_, AsyncCompletionHandler::tag(this));
        SILKRPC_TRACE << "TxStreamingClient::read_start " << this << " AFTER Read\n";
        SILKRPC_TRACE << "TxStreamingClient::read_start " << this << " status: " << status_ << " end\n";
    }

    void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
        SILKRPC_TRACE << "TxStreamingClient::write_start " << this << " stream: " << stream_.get() << " status: " << status_ << " start\n";
        write_completed_ = write_completed;
        status_ = WRITE_STARTED;
        stream_->Write(cursor, AsyncCompletionHandler::tag(this));
        SILKRPC_TRACE << "TxStreamingClient::write_start " << this << " status: " << status_ << " end\n";
    }

    void completed(bool ok) override {
        SILKRPC_TRACE << "TxStreamingClient::completed " << this << " status: " << status_ << " ok: " << ok << " start\n";
        if (!ok && !finishing_) {
            finishing_ = true;
            stream_->Finish(&result_, AsyncCompletionHandler::tag(this));
            return;
        }
        SILKRPC_TRACE << "TxStreamingClient::completed result: " << result_.ok() << "\n";
        if (!result_.ok()) {
            SILKRPC_ERROR << "TxStreamingClient::completed error_code: " << result_.error_code() << "\n";
            SILKRPC_ERROR << "TxStreamingClient::completed error_message: " << result_.error_message() << "\n";
            SILKRPC_ERROR << "TxStreamingClient::completed error_details: " << result_.error_details() << "\n";
        }
        switch (status_) {
            case CALL_STARTED:
                start_completed_(result_);
            break;
            case WRITE_STARTED:
                write_completed_(result_);
            break;
            case READ_STARTED:
                SILKRPC_TRACE << "TxStreamingClient::completed pair cursorid: " << pair_.cursorid() << "\n";
                read_completed_(result_, pair_);
            break;
            case DONE_STARTED:
                status_ = CALL_ENDED;
                if (!finishing_) {
                    finishing_ = true;
                    stream_->Finish(&result_, AsyncCompletionHandler::tag(this));
                } else {
                    end_completed_(result_);
                }
            break;
            case CALL_ENDED:
                end_completed_(result_);
            break;
            default:
            break;
        }
        SILKRPC_TRACE << "TxStreamingClient::completed " << this << " status: " << status_ << " end\n";
    }

private:
    std::unique_ptr<remote::KV::StubInterface>& stub_;
    grpc::ClientContext context_;
    ClientAsyncReaderWriterPtr stream_;
    remote::Pair pair_;
    grpc::Status result_;
    CallStatus status_;
    bool finishing_{false};
    std::function<void(const grpc::Status&)> start_completed_;
    std::function<void(const grpc::Status&, const remote::Pair&)> read_completed_;
    std::function<void(const grpc::Status&)> write_completed_;
    std::function<void(const grpc::Status&)> end_completed_;
};

} // namespace silkrpc::ethdb::kv

#endif // SILKRPC_ETHDB_KV_TX_STREAMING_CLIENT_HPP_
