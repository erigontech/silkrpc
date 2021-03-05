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

#include <map>
#include <memory>
#include <string>

#include <silkrpc/config.hpp>

#include <asio/use_awaitable.hpp>

#include <silkrpc/ethdb/kv/awaitables.hpp>
#include <silkrpc/ethdb/kv/cursor.hpp>
#include <silkrpc/ethdb/kv/remote_cursor.hpp>
#include <silkrpc/ethdb/kv/transaction.hpp>
#include <silkrpc/ethdb/kv/client_callback_reactor.hpp>

namespace silkrpc::ethdb::kv {

using namespace silkworm;

class RemoteTransaction : public Transaction {
public:
    explicit RemoteTransaction(asio::io_context& context, std::shared_ptr<grpc::Channel> channel)
    : context_(context), reactor_{channel}, kv_awaitable_{context_, reactor_} {}

    std::unique_ptr<Cursor> cursor() override {
        return std::make_unique<RemoteCursor>(kv_awaitable_);
    }

    asio::awaitable<std::shared_ptr<Cursor>> cursor(const std::string& table) override;

    asio::awaitable<void> close() override;

private:
    asio::io_context& context_;
    ClientCallbackReactor reactor_;
    KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable_;
    std::map<std::string, std::shared_ptr<Cursor>> cursors_;
};

} // namespace silkrpc::ethdb::kv

#endif // SILKRPC_KV_REMOTE_TRANSACTION_HPP
