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

#include "local_cursor.hpp"

#include <silkrpc/common/clock_time.hpp>

namespace silkrpc::ethdb::file {

boost::asio::awaitable<void> LocalCursor::open_cursor(const std::string& table_name, bool is_dup_sorted) {
    const auto start_time = clock_time::now();
    if (cursor_id_ == 0) {
        SILKRPC_DEBUG << "LocalCursor::open_cursor opening new cursor for table: " << table_name << "\n";

        SILKRPC_DEBUG << "LocalCursor::open_cursor cursor: " << cursor_id_ << " for table: " << table_name << "\n";
    }
    SILKRPC_DEBUG << "LocalCursor::open_cursor [" << table_name << "] c=" << cursor_id_ << " t=" << clock_time::since(start_time) << "\n";
    co_return;
}

boost::asio::awaitable<KeyValue> LocalCursor::seek(silkworm::ByteView key) {
    const auto start_time = clock_time::now();
    SILKRPC_DEBUG << "LocalCursor::seek cursor: " << cursor_id_ << " key: " << key << "\n";
    co_return KeyValue{};
}

boost::asio::awaitable<KeyValue> LocalCursor::seek_exact(silkworm::ByteView key) {
    const auto start_time = clock_time::now();
    SILKRPC_DEBUG << "LocalCursor::seek_exact cursor: " << cursor_id_ << " key: " << key << "\n";
    co_return KeyValue{};
}

boost::asio::awaitable<KeyValue> LocalCursor::next() {
    const auto start_time = clock_time::now();
    co_return KeyValue{};
}

boost::asio::awaitable<KeyValue> LocalCursor::next_dup() {
    const auto start_time = clock_time::now();
    co_return KeyValue{};
}

boost::asio::awaitable<silkworm::Bytes> LocalCursor::seek_both(silkworm::ByteView key, silkworm::ByteView value) {
    const auto start_time = clock_time::now();
    SILKRPC_DEBUG << "LocalCursor::seek_both cursor: " << cursor_id_ << " key: " << key << " subkey: " << value << "\n";
    silkworm::Bytes ret_value{};
    co_return ret_value;
}

boost::asio::awaitable<KeyValue> LocalCursor::seek_both_exact(silkworm::ByteView key, silkworm::ByteView value) {
    const auto start_time = clock_time::now();
    SILKRPC_DEBUG << "LocalCursor::seek_both_exact cursor: " << cursor_id_ << " key: " << key << " subkey: " << value << "\n";
    co_return KeyValue{};
}

boost::asio::awaitable<void> LocalCursor::close_cursor() {
    const auto start_time = clock_time::now();
    const auto cursor_id = cursor_id_;
    if (cursor_id_ != 0) {
        SILKRPC_DEBUG << "LocalCursor::close_cursor closing cursor: " << cursor_id_ << "\n";
        SILKRPC_DEBUG << "LocalCursor::close_cursor cursor: " << cursor_id_ << "\n";
        cursor_id_ = 0;
    }
    SILKRPC_DEBUG << "LocalCursor::close_cursor c=" << cursor_id << " t=" << clock_time::since(start_time) << "\n";
    co_return;
}

} // namespace silkrpc::ethdb::file
