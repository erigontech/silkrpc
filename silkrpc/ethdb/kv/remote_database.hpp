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

#ifndef SILKRPC_ETHDB_KV_REMOTE_DATABASE_HPP_
#define SILKRPC_ETHDB_KV_REMOTE_DATABASE_HPP_

#include <cstddef>
#include <memory>
#include <vector>
#include <utility>

#include <asio/io_context.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/ethdb/database.hpp>
#include <silkrpc/ethdb/kv/remote_transaction.hpp>

namespace silkrpc::ethdb::kv {

class RemoteDatabase: public Database {
public:
    RemoteDatabase(asio::io_context& io_context, std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue)
    : io_context_(io_context), channel_(channel), queue_(queue) {
        SILKRPC_TRACE << "RemoteDatabase::ctor " << this << "\n";
    }

    ~RemoteDatabase() {
        SILKRPC_TRACE << "RemoteDatabase::dtor " << this << "\n";
    }

    RemoteDatabase(const RemoteDatabase&) = delete;
    RemoteDatabase& operator=(const RemoteDatabase&) = delete;

    asio::awaitable<std::unique_ptr<Transaction>> begin() override {
        SILKRPC_TRACE << "RemoteDatabase::begin " << this << " start\n";
        auto client = std::make_unique<StreamingClientImpl>(channel_, queue_);
        auto txn = std::make_unique<RemoteTransaction>(io_context_, std::move(client));
        co_await txn->open();
        SILKRPC_TRACE << "RemoteDatabase::begin " << this << " txn: " << txn.get() << " end\n";
        co_return txn;
    }

private:
    asio::io_context& io_context_;
    std::shared_ptr<grpc::Channel> channel_;
    grpc::CompletionQueue* queue_;
};

} // namespace silkrpc::ethdb::kv

#endif  // SILKRPC_ETHDB_KV_REMOTE_DATABASE_HPP_
