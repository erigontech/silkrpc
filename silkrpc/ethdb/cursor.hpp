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

#include <boost/asio/awaitable.hpp>
#include <silkworm/common/util.hpp>

#include <silkrpc/common/util.hpp>

namespace silkrpc::ethdb {

class Cursor {
public:
    Cursor() = default;
    virtual ~Cursor() = default;

    Cursor(const Cursor&) = delete;
    Cursor& operator=(const Cursor&) = delete;

    virtual uint32_t cursor_id() const = 0;

    virtual boost::asio::awaitable<void> open_cursor(const std::string& table_name) = 0;

    virtual boost::asio::awaitable<KeyValue> seek(silkworm::ByteView key) = 0;

    virtual boost::asio::awaitable<KeyValue> seek_exact(silkworm::ByteView key) = 0;

    virtual boost::asio::awaitable<KeyValue> next() = 0;

    virtual boost::asio::awaitable<void> close_cursor() = 0;
};

class CursorDupSort : public Cursor {
public:
    virtual boost::asio::awaitable<silkworm::Bytes> seek_both(silkworm::ByteView key, silkworm::ByteView value) = 0;

    virtual boost::asio::awaitable<KeyValue> seek_both_exact(silkworm::ByteView key, silkworm::ByteView value) = 0;
};

struct SplittedKeyValue {
    silkworm::Bytes key1;
    silkworm::Bytes key2;
    silkworm::Bytes key3;
    silkworm::Bytes value;
};

class SplitCursor {
public:
    SplitCursor(Cursor& inner_cursor, silkworm::ByteView key, uint64_t match_bits, uint64_t part1_end, uint64_t part2_start, uint64_t part3_start);
    SplitCursor& operator=(const SplitCursor&) = delete;

    boost::asio::awaitable<SplittedKeyValue> seek();

    boost::asio::awaitable<SplittedKeyValue> next();

private:
    Cursor& inner_cursor_;
    silkworm::Bytes key_;
    silkworm::Bytes first_bytes_;
    uint8_t last_bits_;
    uint64_t part1_end_;
    uint64_t part2_start_;
    uint64_t part3_start_;
    uint64_t match_bytes_;
    uint8_t mask_;

    bool match_key(const silkworm::ByteView& key);
    SplittedKeyValue split_key_value(const KeyValue& kv);
};

} // namespace silkrpc::ethdb

#endif  // SILKRPC_ETHDB_CURSOR_HPP_
