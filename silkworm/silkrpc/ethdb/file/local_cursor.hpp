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

#pragma once

#include <memory>
#include <string>
#include <utility>

#include <silkrpc/config.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/ethdb/cursor.hpp>
#include <silkworm/common/util.hpp>

namespace silkrpc::ethdb::file {

class LocalCursor : public CursorDupSort {
public:
    explicit LocalCursor() : cursor_id_{0} {}

    uint32_t cursor_id() const override { return cursor_id_; };

    boost::asio::awaitable<void> open_cursor(const std::string& table_name, bool is_dup_sorted) override;

    boost::asio::awaitable<KeyValue> seek(silkworm::ByteView key) override;

    boost::asio::awaitable<KeyValue> seek_exact(silkworm::ByteView key) override;

    boost::asio::awaitable<KeyValue> next() override;

    boost::asio::awaitable<KeyValue> next_dup() override;

    boost::asio::awaitable<void> close_cursor() override;

    boost::asio::awaitable<silkworm::Bytes> seek_both(silkworm::ByteView key, silkworm::ByteView value) override;

    boost::asio::awaitable<KeyValue> seek_both_exact(silkworm::ByteView key, silkworm::ByteView value) override;

private:
    uint32_t cursor_id_;
};

} // namespace silkrpc::ethdb::file

