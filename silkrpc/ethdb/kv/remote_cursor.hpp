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

#ifndef SILKRPC_KV_REMOTE_CURSOR_H_
#define SILKRPC_KV_REMOTE_CURSOR_H_

#include <silkrpc/config.hpp>

#include <memory>

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <asio/use_awaitable.hpp>

#include <silkworm/common/util.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/ethdb/kv/awaitables.hpp>
#include <silkrpc/ethdb/kv/cursor.hpp>

namespace silkrpc::ethdb::kv {

class RemoteCursor : public Cursor {
public:
    RemoteCursor(KvAsioAwaitable<asio::io_context::executor_type>& kv_awaitable)
    : kv_awaitable_(kv_awaitable) {}

    RemoteCursor(const RemoteCursor&) = delete;
    RemoteCursor& operator=(const RemoteCursor&) = delete;

    asio::awaitable<common::KeyValue> seek(const std::string& table_name, const silkworm::Bytes& seek_key) override {
        auto cursor_id = co_await open_cursor(table_name);
        auto kv_pair = co_await seek(cursor_id, seek_key);
        co_await close_cursor(cursor_id); // Can we shoot and forget?
        co_return kv_pair;
    }

    asio::awaitable<uint32_t> open_cursor(const std::string& table_name) override {
        auto cursor_id = co_await kv_awaitable_.async_open_cursor(table_name, asio::use_awaitable);
        co_return cursor_id;
    }

    asio::awaitable<silkrpc::common::KeyValue> seek(uint32_t cursor_id, const silkworm::Bytes& seek_key) override {
        auto seek_pair = co_await kv_awaitable_.async_seek(cursor_id, seek_key, asio::use_awaitable);
        const auto k = silkworm::bytes_of_string(seek_pair.k());
        const auto v = silkworm::bytes_of_string(seek_pair.v());
        co_return silkrpc::common::KeyValue{k, v};
    }

    asio::awaitable<silkrpc::common::KeyValue> next(uint32_t cursor_id) override {
        auto next_pair = co_await kv_awaitable_.async_next(cursor_id, asio::use_awaitable);
        const auto k = silkworm::bytes_of_string(next_pair.k());
        const auto v = silkworm::bytes_of_string(next_pair.v());
        co_return silkrpc::common::KeyValue{k, v};
    }

    asio::awaitable<void> close_cursor(uint32_t cursor_id) override {
        co_await kv_awaitable_.async_close_cursor(cursor_id, asio::use_awaitable); // Can we shoot and forget?
        co_return;
    }

private:
    KvAsioAwaitable<asio::io_context::executor_type>& kv_awaitable_;
};

} // namespace silkrpc::ethdb::kv

#endif  // SILKRPC_KV_REMOTE_CURSOR_H_
