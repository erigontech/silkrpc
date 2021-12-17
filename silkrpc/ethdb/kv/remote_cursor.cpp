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

#include "remote_cursor.hpp"

#include <silkrpc/common/clock_time.hpp>

namespace silkrpc::ethdb::kv {

asio::awaitable<void> RemoteCursor::open_cursor(const std::string& table_name) {
    const auto start_time = clock_time::now();
    if (cursor_id_ == 0) {
        SILKRPC_DEBUG << "RemoteCursor::open_cursor opening new cursor for table: " << table_name << "\n";
        cursor_id_ = co_await kv_awaitable_.async_open_cursor(table_name, asio::use_awaitable);
        SILKRPC_DEBUG << "RemoteCursor::open_cursor cursor: " << cursor_id_ << " for table: " << table_name << "\n";
    }
    SILKRPC_DEBUG << "RemoteCursor::open_cursor [" << table_name << "] c=" << cursor_id_ << " t=" << clock_time::since(start_time) << "\n";
    co_return;
}

asio::awaitable<KeyValue> RemoteCursor::seek(const silkworm::ByteView& key) {
    const auto start_time = clock_time::now();
    SILKRPC_DEBUG << "RemoteCursor::seek cursor: " << cursor_id_ << " key: " << key << "\n";
    auto seek_pair = co_await kv_awaitable_.async_seek(cursor_id_, key, asio::use_awaitable);
    const auto k = silkworm::bytes_of_string(seek_pair.k());
    const auto v = silkworm::bytes_of_string(seek_pair.v());
    SILKRPC_DEBUG << "RemoteCursor::seek k: " << k << " v: " << v << " c=" << cursor_id_ << " t=" << clock_time::since(start_time) << "\n";
    co_return KeyValue{k, v};
}

asio::awaitable<KeyValue> RemoteCursor::seek_exact(const silkworm::ByteView& key) {
    const auto start_time = clock_time::now();
    SILKRPC_DEBUG << "RemoteCursor::seek_exact cursor: " << cursor_id_ << " key: " << key << "\n";
    auto seek_pair = co_await kv_awaitable_.async_seek_exact(cursor_id_, key, asio::use_awaitable);
    const auto k = silkworm::bytes_of_string(seek_pair.k());
    const auto v = silkworm::bytes_of_string(seek_pair.v());
    SILKRPC_DEBUG << "RemoteCursor::seek_exact k: " << k << " v: " << v << " c=" << cursor_id_ << " t=" << clock_time::since(start_time) << "\n";
    co_return KeyValue{k, v};
}

asio::awaitable<KeyValue> RemoteCursor::next() {
    const auto start_time = clock_time::now();
    auto next_pair = co_await kv_awaitable_.async_next(cursor_id_, asio::use_awaitable);
    const auto k = silkworm::bytes_of_string(next_pair.k());
    const auto v = silkworm::bytes_of_string(next_pair.v());
    SILKRPC_DEBUG << "RemoteCursor::next k: " << k << " v: " << v << " c=" << cursor_id_ << " t=" << clock_time::since(start_time) << "\n";
    co_return KeyValue{k, v};
}

asio::awaitable<silkworm::Bytes> RemoteCursor::seek_both(const silkworm::ByteView& key, const silkworm::ByteView& value) {
    const auto start_time = clock_time::now();
    SILKRPC_DEBUG << "RemoteCursor::seek_both cursor: " << cursor_id_ << " key: " << key << " subkey: " << value << "\n";
    auto seek_pair = co_await kv_awaitable_.async_seek_both(cursor_id_, key, value, asio::use_awaitable);
    const auto k = silkworm::bytes_of_string(seek_pair.k());
    const auto v = silkworm::bytes_of_string(seek_pair.v());
    SILKRPC_DEBUG << "RemoteCursor::seek_both k: " << k << " v: " << v << " c=" << cursor_id_ << " t=" << clock_time::since(start_time) << "\n";
    co_return v;
}

asio::awaitable<KeyValue> RemoteCursor::seek_both_exact(const silkworm::ByteView& key, const silkworm::ByteView& value) {
    const auto start_time = clock_time::now();
    SILKRPC_DEBUG << "RemoteCursor::seek_both_exact cursor: " << cursor_id_ << " key: " << key << " subkey: " << value << "\n";
    auto seek_pair = co_await kv_awaitable_.async_seek_both_exact(cursor_id_, key, value, asio::use_awaitable);
    const auto k = silkworm::bytes_of_string(seek_pair.k());
    const auto v = silkworm::bytes_of_string(seek_pair.v());
    SILKRPC_DEBUG << "RemoteCursor::seek_both_exact k: " << k << " v: " << v << " c=" << cursor_id_ << " t=" << clock_time::since(start_time) << "\n";
    co_return KeyValue{k, v};
}

asio::awaitable<void> RemoteCursor::close_cursor() {
    const auto start_time = clock_time::now();
    const auto cursor_id = cursor_id_;
    if (cursor_id_ != 0) {
        SILKRPC_DEBUG << "RemoteCursor::close_cursor closing cursor: " << cursor_id_ << "\n";
        co_await kv_awaitable_.async_close_cursor(cursor_id_, asio::use_awaitable); // Can we shoot and forget? Or avoid and delegate to CORE service cleanup?
        SILKRPC_DEBUG << "RemoteCursor::close_cursor cursor: " << cursor_id_ << "\n";
        cursor_id_ = 0;
    }
    SILKRPC_DEBUG << "RemoteCursor::close_cursor c=" << cursor_id << " t=" << clock_time::since(start_time) << "\n";
    co_return;
}

} // namespace silkrpc::ethdb::kv
