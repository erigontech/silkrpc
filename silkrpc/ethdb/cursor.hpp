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

#ifndef SILKRPC_ETHDB_CURSOR_HPP_
#define SILKRPC_ETHDB_CURSOR_HPP_

#include <silkrpc/config.hpp>

#include <memory>
#include <string>

#include <asio/awaitable.hpp>

#include <silkworm/common/util.hpp>
#include <silkrpc/common/util.hpp>

namespace silkrpc::ethdb {

class Cursor {
public:
    Cursor() = default;

    Cursor(const Cursor&) = delete;
    Cursor& operator=(const Cursor&) = delete;

    virtual uint32_t cursor_id() const = 0;

    virtual asio::awaitable<void> open_cursor(const std::string& table_name) = 0;

    virtual asio::awaitable<KeyValue> seek(const silkworm::ByteView& seek_key) = 0;

    virtual asio::awaitable<KeyValue> next() = 0;

    virtual asio::awaitable<void> close_cursor() = 0;
};

} // namespace silkrpc::ethdb

#endif  // SILKRPC_ETHDB_CURSOR_HPP_
