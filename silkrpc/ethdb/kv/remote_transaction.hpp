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
#include <type_traits>

#include <silkrpc/config.hpp>

#include <agrpc/grpcContext.hpp>
#include <asio/use_awaitable.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/ethdb/cursor.hpp>
#include <silkrpc/ethdb/kv/remote_cursor.hpp>
#include <silkrpc/ethdb/transaction.hpp>

namespace silkrpc::ethdb::kv {

/*
// TODO(canepat): concept to force Client implements StreamingClient

template<typename Client>
class RemoteTransaction : public Transaction {
    static_assert(std::is_base_of<AsyncTxStreamingClient, Client>::value && !std::is_same<AsyncTxStreamingClient, Client>::value);

public:
    explicit RemoteTransaction(asio::io_context& context, std::unique_ptr<remote::KV::StubInterface>& stub, grpc::CompletionQueue* queue)
    : context_(context), client_{stub, queue}, kv_awaitable_{context_, client_} {
        SILKRPC_TRACE << "RemoteTransaction::ctor " << this << " start\n";
        SILKRPC_TRACE << "RemoteTransaction::ctor " << this << " end\n";
    }

    ~RemoteTransaction() {
        SILKRPC_TRACE << "RemoteTransaction::dtor " << this << " start\n";
        SILKRPC_TRACE << "RemoteTransaction::dtor " << this << " end\n";
    }

    uint64_t tx_id() const override { return tx_id_; }

    asio::awaitable<void> open() override {
        tx_id_ = co_await kv_awaitable_.async_start(asio::use_awaitable);
        SILKRPC_INFO << "RemoteTransaction::open tx_id=" << tx_id_ << "\n";
        co_return;
    }

    asio::awaitable<std::shared_ptr<Cursor>> cursor(const std::string& table) override {
        co_return co_await get_cursor(table);
    }

    asio::awaitable<std::shared_ptr<CursorDupSort>> cursor_dup_sort(const std::string& table) override {
        co_return co_await get_cursor(table);
    }

    asio::awaitable<void> close() override {
        cursors_.clear();
        co_await kv_awaitable_.async_end(asio::use_awaitable);
        co_return;
    }

private:
    asio::awaitable<std::shared_ptr<CursorDupSort>> get_cursor(const std::string& table) {
        auto cursor_it = cursors_.find(table);
        if (cursor_it != cursors_.end()) {
            co_return cursor_it->second;
        }
        auto cursor = std::make_shared<RemoteCursor>(kv_awaitable_);
        co_await cursor->open_cursor(table);
        cursors_[table] = cursor;
        co_return cursor;
    }

    asio::io_context& context_;
    Client client_;
    KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable_;
    std::map<std::string, std::shared_ptr<CursorDupSort>> cursors_;
    uint64_t tx_id_;
};*/

class RemoteTransaction2 : public Transaction {
public:
    explicit RemoteTransaction2(remote::KV::StubInterface& stub, agrpc::GrpcContext& grpc_context);

    ~RemoteTransaction2();

    uint64_t tx_id() const override { return tx_id_; }

    asio::awaitable<void> open() override;

    asio::awaitable<std::shared_ptr<Cursor>> cursor(const std::string& table) override;

    asio::awaitable<std::shared_ptr<CursorDupSort>> cursor_dup_sort(const std::string& table) override;

    asio::awaitable<void> close() override;

private:
    asio::awaitable<std::shared_ptr<CursorDupSort>> get_cursor(const std::string& table);

    KVTxStreamingRpc tx_rpc_;
    std::map<std::string, std::shared_ptr<CursorDupSort>> cursors_;
    uint64_t tx_id_;
};

} // namespace silkrpc::ethdb::kv

#endif // SILKRPC_ETHDB_KV_REMOTE_TRANSACTION_HPP_
