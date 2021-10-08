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

#ifndef SILKRPC_ETHDB_KV_REMOTE_TRANSACTION_HPP_
#define SILKRPC_ETHDB_KV_REMOTE_TRANSACTION_HPP_

#include <map>
#include <memory>
#include <string>

#include <silkrpc/config.hpp>

#include <asio/use_awaitable.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/ethdb/kv/awaitables.hpp>
#include <silkrpc/ethdb/cursor.hpp>
#include <silkrpc/ethdb/kv/streaming_client.hpp>
#include <silkrpc/ethdb/transaction.hpp>

namespace silkrpc::ethdb::kv {

class RemoteTransaction : public Transaction {
public:
    explicit RemoteTransaction(asio::io_context& context, std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue)
    : context_(context), client_{channel, queue}, kv_awaitable_{context_, client_} {
        SILKRPC_TRACE << "RemoteTransaction::ctor " << this << " start\n";
        SILKRPC_TRACE << "RemoteTransaction::ctor " << this << " end\n";
    }

    ~RemoteTransaction() {
        SILKRPC_TRACE << "RemoteTransaction::dtor " << this << " start\n";
        SILKRPC_TRACE << "RemoteTransaction::dtor " << this << " end\n";
    }

    asio::awaitable<void> open() override;

    asio::awaitable<std::shared_ptr<Cursor>> cursor(const std::string& table) override;

    asio::awaitable<std::shared_ptr<CursorDupSort>> cursor_dup_sort(const std::string& table) override;

    asio::awaitable<void> close() override;

private:
    asio::awaitable<std::shared_ptr<CursorDupSort>> get_cursor(const std::string& table);
    uint64_t tx_id_;

    asio::io_context& context_;
    StreamingClient client_;
    KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable_;
    std::map<std::string, std::shared_ptr<CursorDupSort>> cursors_;
};

} // namespace silkrpc::ethdb::kv

#endif // SILKRPC_ETHDB_KV_REMOTE_TRANSACTION_HPP_
