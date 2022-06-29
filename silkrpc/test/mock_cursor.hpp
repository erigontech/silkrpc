/*
   Copyright 2022 The Silkrpc Authors

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

#ifndef SILKRPC_TEST_MOCK_CURSOR_HPP_
#define SILKRPC_TEST_MOCK_CURSOR_HPP_

#include <string>

#include <asio/awaitable.hpp>
#include <gmock/gmock.h>
#include <silkworm/common/base.hpp>

#include <silkrpc/common/util.hpp>
#include <silkrpc/ethdb/cursor.hpp>

namespace silkrpc::test {

class MockCursor : public ethdb::Cursor {
public:
    uint32_t cursor_id() const override { return 0; }

    MOCK_METHOD((asio::awaitable<void>), open_cursor, (const std::string& table_name));
    MOCK_METHOD((asio::awaitable<KeyValue>), seek, (silkworm::ByteView key));
    MOCK_METHOD((asio::awaitable<KeyValue>), seek_exact, (silkworm::ByteView key));
    MOCK_METHOD((asio::awaitable<KeyValue>), next, ());
    MOCK_METHOD((asio::awaitable<void>), close_cursor, ());
};

class MockCursorDupSort : public ethdb::CursorDupSort, MockCursor {
public:
    uint32_t cursor_id() const override { return 0; }
    //MOCK_METHOD((uint32_t), cursor_id, ());
    MOCK_METHOD((asio::awaitable<void>), open_cursor, (const std::string& table_name));
    MOCK_METHOD((asio::awaitable<KeyValue>), seek, (silkworm::ByteView key));
    MOCK_METHOD((asio::awaitable<KeyValue>), seek_exact, (silkworm::ByteView key));
    MOCK_METHOD((asio::awaitable<KeyValue>), next, ());
    MOCK_METHOD((asio::awaitable<void>), close_cursor, ());
    MOCK_METHOD((asio::awaitable<silkworm::Bytes>), seek_both, (silkworm::ByteView, silkworm::ByteView));
    MOCK_METHOD((asio::awaitable<KeyValue>), seek_both_exact, (silkworm::ByteView, silkworm::ByteView));
};

}  // namespace silkrpc::test

#endif  // SILKRPC_TEST_MOCK_CURSOR_HPP_
