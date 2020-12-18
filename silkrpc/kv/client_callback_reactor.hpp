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

#include <silkrpc/kv/remote/kv.grpc.pb.h>

namespace silkrpc::kv {

class ClientCallbackReactor final : public grpc::experimental::ClientBidiReactor<remote::Cursor, remote::Pair> {
public:
    explicit ClientCallbackReactor(std::shared_ptr<grpc::Channel> channel) : stub_{remote::KV::NewStub(channel)} {
        stub_->experimental_async()->Tx(&context_, this);
        StartCall();
    }

    void read_start(std::function<void(bool,remote::Pair)> read_completed) {
        read_completed_ = read_completed;
        StartRead(&pair_);
    }

    void write_start(remote::Cursor* cursor, std::function<void(bool)> write_completed) {
        write_completed_ = write_completed;
        StartWrite(cursor);
    }

    void OnReadDone(bool ok) override {
        read_completed_(ok, pair_);
    }

    void OnWriteDone(bool ok) override {
        write_completed_(ok);
    }
private:
    std::unique_ptr<remote::KV::Stub> stub_;
    grpc::ClientContext context_;
    remote::Pair pair_;
    std::function<void(bool,remote::Pair)> read_completed_;
    std::function<void(bool)> write_completed_;
};

} // namespace silkrpc::kv

#endif // SILKRPC_KV_CLIENT_CALLBACK_REACTOR_HPP
