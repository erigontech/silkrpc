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

#ifndef SILKRPC_KV_REMOTE_RemoteDatabase_H_
#define SILKRPC_KV_REMOTE_RemoteDatabase_H_

#include <memory>

#include <asio/io_context.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/ethdb/kv/database.hpp>
#include <silkrpc/ethdb/kv/remote_transaction.hpp>

namespace silkrpc::ethdb::kv {

class RemoteDatabase :public Database {
public:
    RemoteDatabase(asio::io_context& context, std::shared_ptr<::grpc::Channel> channel, ::grpc::CompletionQueue* queue)
    : context_(context), channel_(channel), queue_(queue) {
        SILKRPC_TRACE << "RemoteDatabase::ctor " << this << " start\n";
        SILKRPC_TRACE << "RemoteDatabase::ctor " << this << " end\n";
    }

    ~RemoteDatabase() {
        SILKRPC_TRACE << "RemoteDatabase::dtor " << this << " start\n";
        SILKRPC_TRACE << "RemoteDatabase::dtor " << this << " end\n";
    }

    RemoteDatabase(const RemoteDatabase&) = delete;
    RemoteDatabase& operator=(const RemoteDatabase&) = delete;

    virtual asio::awaitable<std::unique_ptr<Transaction>> begin() override {
        SILKRPC_TRACE << "RemoteDatabase::begin " << this << " start\n";
        auto txn = std::make_unique<RemoteTransaction>(context_, channel_, queue_);
        co_await txn->open();
        SILKRPC_TRACE << "RemoteDatabase::begin " << this << " txn: " << txn.get() << " end\n";
        co_return txn;
    }

private:
    asio::io_context& context_;
    std::shared_ptr<::grpc::Channel> channel_;
    ::grpc::CompletionQueue* queue_;
};

} // namespace silkrpc::ethdb::kv

#endif  // SILKRPC_KV_REMOTE_RemoteDatabase_H_
