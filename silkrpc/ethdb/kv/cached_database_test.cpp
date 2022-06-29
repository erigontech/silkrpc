/*
   Copyright 2020-2022 The Silkrpc Authors

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

#include "cached_database.hpp"

#include <memory>

#include <asio/co_spawn.hpp>
#include <asio/thread_pool.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch.hpp>
#include <gmock/gmock.h>

#include <silkrpc/ethdb/kv/state_cache.hpp>
#include <silkrpc/ethdb/tables.hpp>
#include <silkrpc/test/dummy_transaction.hpp>
#include <silkrpc/test/mock_cursor.hpp>
#include <silkrpc/test/mock_transaction.hpp>
#include <silkrpc/types/block.hpp>

namespace silkrpc::ethdb::kv {

using Catch::Matchers::Message;
using testing::_;
using testing::InvokeWithoutArgs;

TEST_CASE("CachedDatabase::CachedDatabase", "[silkrpc][ethdb][kv][cached_reader]") {
    BlockNumberOrHash block_id{0};
    test::MockTransaction txn;
    kv::CoherentStateCache cache;
    CHECK_NOTHROW(CachedDatabase{block_id, txn, cache});
}

/*TEST_CASE("CachedDatabase::get_one", "[silkrpc][ethdb][kv][cached_reader]") {
    asio::thread_pool pool{1};
    std::shared_ptr<test::MockCursor> mock_cursor = std::make_shared<test::MockCursor>();
    test::DummyTransaction txn{0, mock_cursor};
    kv::CoherentStateCache cache;

    SECTION("empty key from PlainState in latest block") {
        BlockNumberOrHash block_id{0};
        CachedDatabase cached_db{block_id, txn, cache};
        EXPECT_CALL(*mock_cursor, seek_exact(_)).WillOnce(InvokeWithoutArgs([]() -> asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}};
        }));
        auto result = asio::co_spawn(pool, cached_db.get_one(db::table::kPlainState, silkworm::Bytes{}), asio::use_future);
        const auto value = result.get();
        CHECK(value.empty());
    }
}*/

TEST_CASE("CachedDatabase::get", "[silkrpc][ethdb][kv][cached_reader]") {
    asio::thread_pool pool{1};
    std::shared_ptr<test::MockCursor> mock_cursor = std::make_shared<test::MockCursor>();
    test::DummyTransaction txn{0, mock_cursor};
    kv::CoherentStateCache cache;

    SECTION("empty key from PlainState in latest block") {
        BlockNumberOrHash block_id{0};
        CachedDatabase cached_db{block_id, txn, cache};
        EXPECT_CALL(*mock_cursor, seek(_)).WillOnce(InvokeWithoutArgs([]() -> asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}};
        }));
        auto result = asio::co_spawn(pool, cached_db.get(db::table::kPlainState, silkworm::Bytes{}), asio::use_future);
        const auto kv = result.get();
        CHECK(kv.value.empty());
    }
}

}  // namespace silkrpc::ethdb::kv
