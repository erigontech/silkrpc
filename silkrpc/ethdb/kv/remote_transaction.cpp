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

#include "remote_transaction.hpp"

namespace silkrpc::ethdb::kv {

asio::awaitable<std::shared_ptr<Cursor>> RemoteTransaction::cursor(const std::string& table) {
    auto cursor_it = cursors_.find(table);
    if (cursor_it != cursors_.end()) {
        co_return cursor_it->second;
    }
    auto cursor = std::make_shared<RemoteCursor>(kv_awaitable_);
    co_await cursor->open_cursor(table);
    cursors_[table] = cursor;
    co_return cursor;
}

asio::awaitable<void> RemoteTransaction::close() {
    for (const auto& [table, cursor] : cursors_) {
        co_await cursor->close_cursor();
    }
    cursors_.clear();
    co_await kv_awaitable_.async_close(asio::use_awaitable);
    co_return;
}

} // namespace silkrpc::ethdb::kv
