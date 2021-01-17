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

#ifndef SILKRPC_KV_REMOTE_TRANSACTION_HPP
#define SILKRPC_KV_REMOTE_TRANSACTION_HPP

#include <memory>

#include <silkrpc/config.hpp>

#include <silkrpc/kv/awaitables.hpp>
#include <silkrpc/kv/cursor.hpp>
#include <silkrpc/kv/remote_cursor.hpp>
#include <silkrpc/kv/transaction.hpp>
#include <silkrpc/kv/client_callback_reactor.hpp>

namespace silkrpc::kv {

using namespace silkworm;

class RemoteTransaction : public Transaction {
public:
    explicit RemoteTransaction(asio::io_context& context, std::shared_ptr<grpc::Channel> channel)
    : context_(context), reactor_{channel}, kv_awaitable_{context_, reactor_} {}

    virtual std::unique_ptr<Cursor> cursor() override {
        return std::make_unique<RemoteCursor>(kv_awaitable_);
    }

    virtual void rollback() override {
        // TODO: trace open cursors, close pending one here (requires cursorId in cursor and making this a coroutine)
    }

private:
    asio::io_context& context_;
    ClientCallbackReactor reactor_;
    KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable_;
};

} // namespace silkrpc::kv

#endif // SILKRPC_KV_REMOTE_TRANSACTION_HPP
