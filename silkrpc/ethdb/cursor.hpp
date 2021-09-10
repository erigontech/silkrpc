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

    virtual asio::awaitable<KeyValue> seek(const silkworm::ByteView& key) = 0;

    virtual asio::awaitable<KeyValue> seek_exact(const silkworm::ByteView& key) = 0;

    virtual asio::awaitable<KeyValue> next() = 0;

    virtual asio::awaitable<void> close_cursor() = 0;
};

class CursorDupSort : public Cursor {
public:
    virtual asio::awaitable<silkworm::Bytes> seek_both(const silkworm::ByteView& key, const silkworm::ByteView& value) = 0;

    virtual asio::awaitable<KeyValue> seek_both_exact(const silkworm::ByteView& key, const silkworm::ByteView& value) = 0;
};

struct SplittedKeyValue {
    silkworm::Bytes key1;
    silkworm::Bytes key2;
    silkworm::Bytes key3;
    silkworm::Bytes value;
};

class SplitCursor {
public:
    SplitCursor(Cursor& inner_cursor,
        const silkworm::Bytes& key,
        uint64_t match_bits,
        uint64_t length1,
        uint64_t length2);
    SplitCursor& operator=(const SplitCursor&) = delete;

    asio::awaitable<SplittedKeyValue> seek();

    asio::awaitable<SplittedKeyValue> next();

private:
    Cursor& inner_cursor_;
    silkworm::Bytes key_;
    silkworm::Bytes first_bytes_;
    uint8_t last_bits_;
    uint64_t length1_;
    uint64_t length2_;
    uint64_t match_bytes_;
    uint8_t mask_;

    bool match_key(const silkworm::ByteView& key);
    SplittedKeyValue split_key_value(const KeyValue& kv);
};

} // namespace silkrpc::ethdb

#endif  // SILKRPC_ETHDB_CURSOR_HPP_
