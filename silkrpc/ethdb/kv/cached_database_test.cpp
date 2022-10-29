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

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_future.hpp>
#include <catch2/catch.hpp>
#include <gmock/gmock.h>

#include <silkrpc/core/rawdb/accessors.hpp>
#include <silkrpc/ethdb/kv/state_cache.hpp>
#include <silkrpc/ethdb/tables.hpp>
#include <silkrpc/test/dummy_transaction.hpp>
#include <silkrpc/test/mock_cursor.hpp>
#include <silkrpc/test/mock_state_cache.hpp>
#include <silkrpc/test/mock_transaction.hpp>
#include <silkrpc/types/block.hpp>
#include <silkworm/common/util.hpp>

namespace silkrpc::ethdb::kv {

using Catch::Matchers::Message;
using testing::_;
using testing::InvokeWithoutArgs;
using testing::Return;

static constexpr auto kTestBlockNumber{1'000'000};
static const auto kTestBlockNumberBytes{*silkworm::from_hex("00000000000F4240")};

static const auto kTestData{*silkworm::from_hex("600035600055")};
static const silkworm::Bytes kZeroBytes{};

TEST_CASE("CachedDatabase::CachedDatabase", "[silkrpc][ethdb][kv][cached_database]") {
    BlockNumberOrHash block_id{0};
    test::MockTransaction mock_txn;
    test::MockStateCache mock_cache;
    CHECK_NOTHROW(CachedDatabase{block_id, mock_txn, mock_cache});
}

TEST_CASE("CachedDatabase::get_one", "[silkrpc][ethdb][kv][cached_database]") {
    boost::asio::thread_pool pool{1};
    std::shared_ptr<test::MockCursor> mock_cursor = std::make_shared<test::MockCursor>();
    test::MockStateCache mock_cache;

    SECTION("cache miss: empty key from PlainState in latest block") {
        BlockNumberOrHash block_id{0};
        test::DummyTransaction fake_txn{0, mock_cursor};
        CachedDatabase cached_db{block_id, fake_txn, mock_cache};
        // Mock cursor shall be used to read latest block from Execution state in table SyncStageProgress
        EXPECT_CALL(*mock_cursor, seek(_)).WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{kZeroBytes, kTestBlockNumberBytes};
        }));
        // Mock cursor shall be used to read from table PlainState
        EXPECT_CALL(*mock_cursor, seek_exact(_)).WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{kZeroBytes, kTestData};
        }));
        auto result = boost::asio::co_spawn(pool, cached_db.get_one(db::table::kPlainState, kZeroBytes), boost::asio::use_future);
        const auto value = result.get();
        CHECK(value == kTestData);
    }

    SECTION("cache hit: empty key from PlainState in latest block") {
        BlockNumberOrHash block_id{kTestBlockNumber};
        test::DummyTransaction fake_txn{0, mock_cursor};
        test::MockStateView* mock_view = new test::MockStateView;
        CachedDatabase cached_db{block_id, fake_txn, mock_cache};
        // Mock cursor shall be used to read latest block from Execution stage in table SyncStageProgress
        EXPECT_CALL(*mock_cursor, seek(_)).WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{kZeroBytes, kTestBlockNumberBytes};
        }));
        // Mock cache shall return the mock view instance
        EXPECT_CALL(mock_cache, get_view(_)).WillOnce(InvokeWithoutArgs([=]() -> std::unique_ptr<StateView> {
            return std::unique_ptr<test::MockStateView>{mock_view};
        }));
        // Mock view shall be used to read value from data cache
        EXPECT_CALL(*mock_view, get(_)).WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
            co_return kTestData;
        }));
        auto result = boost::asio::co_spawn(pool, cached_db.get_one(db::table::kPlainState, kZeroBytes), boost::asio::use_future);
        const auto value = result.get();
        CHECK(value == kTestData);
    }

    SECTION("cache hit: empty key from Code in latest block") {
        BlockNumberOrHash block_id{kTestBlockNumber};
        test::DummyTransaction fake_txn{0, mock_cursor};
        test::MockStateView* mock_view = new test::MockStateView;
        CachedDatabase cached_db{block_id, fake_txn, mock_cache};
        // Mock cursor shall be used to read latest block from Execution stage in table SyncStageProgress
        EXPECT_CALL(*mock_cursor, seek(_)).WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{kZeroBytes, kTestBlockNumberBytes};
        }));
        // Mock cache shall return the mock view instance
        EXPECT_CALL(mock_cache, get_view(_)).WillOnce(InvokeWithoutArgs([=]() -> std::unique_ptr<StateView> {
            return std::unique_ptr<test::MockStateView>{mock_view};
        }));
        // Mock view shall be used to read value from code cache
        EXPECT_CALL(*mock_view, get_code(_)).WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
            co_return kTestData;
        }));
        auto result = boost::asio::co_spawn(pool, cached_db.get_one(db::table::kCode, kZeroBytes), boost::asio::use_future);
        const auto value = result.get();
        CHECK(value == kTestData);
    }
}

TEST_CASE("CachedDatabase::get", "[silkrpc][ethdb][kv][cached_database]") {
    boost::asio::thread_pool pool{1};
    std::shared_ptr<test::MockCursor> mock_cursor = std::make_shared<test::MockCursor>();
    test::DummyTransaction fake_txn{0, mock_cursor};
    test::MockStateCache mock_cache;
    BlockNumberOrHash block_id{kTestBlockNumber};
    CachedDatabase cached_db{block_id, fake_txn, mock_cache};
    // Mock cursor shall provide the value returned by get
    EXPECT_CALL(*mock_cursor, seek(_)).WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
        co_return KeyValue{kZeroBytes, kZeroBytes};
    }));
    auto result = boost::asio::co_spawn(pool, cached_db.get(db::table::kPlainState, kZeroBytes), boost::asio::use_future);
    const auto kv = result.get();
    CHECK(kv.value.empty());
}

TEST_CASE("CachedDatabase::get_both_range", "[silkrpc][ethdb][kv][cached_database]") {
    boost::asio::thread_pool pool{1};
    std::shared_ptr<test::MockCursor> mock_cursor = std::make_shared<test::MockCursor>();
    test::DummyTransaction fake_txn{0, mock_cursor};
    test::MockStateCache mock_cache;
    BlockNumberOrHash block_id{kTestBlockNumber};
    CachedDatabase cached_db{block_id, fake_txn, mock_cache};
    // Mock cursor shall provide the value returned by get_both_range
    EXPECT_CALL(*mock_cursor, seek_both(_, _)).WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
        co_return kZeroBytes;
    }));
    auto result = boost::asio::co_spawn(pool, cached_db.get_both_range(db::table::kCode, kZeroBytes, kZeroBytes), boost::asio::use_future);
    const auto value = result.get();
    CHECK(value);
    if (value) {
        CHECK((*value).empty());
    }
}

TEST_CASE("CachedDatabase::walk", "[silkrpc][ethdb][kv][cached_database]") {
    boost::asio::thread_pool pool{1};
    std::shared_ptr<test::MockCursor> mock_cursor = std::make_shared<test::MockCursor>();
    test::DummyTransaction fake_txn{0, mock_cursor};
    test::MockStateCache mock_cache;
    BlockNumberOrHash block_id{kTestBlockNumber};
    CachedDatabase cached_db{block_id, fake_txn, mock_cache};
    // Mock cursor shall provide the starting key-value pair for the walk
    EXPECT_CALL(*mock_cursor, seek(_)).WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
        co_return KeyValue{*silkworm::from_hex("00"), kZeroBytes};
    }));
    core::rawdb::Walker walker = [&](const silkworm::Bytes& k, const silkworm::Bytes& v) -> bool {
        return false;
    };
    auto result = boost::asio::co_spawn(pool, cached_db.walk(db::table::kCode, kZeroBytes, 0, walker), boost::asio::use_future);
    CHECK_NOTHROW(result.get());
}

TEST_CASE("CachedDatabase::for_prefix", "[silkrpc][ethdb][kv][cached_database]") {
    boost::asio::thread_pool pool{1};
    std::shared_ptr<test::MockCursor> mock_cursor = std::make_shared<test::MockCursor>();
    test::DummyTransaction fake_txn{0, mock_cursor};
    test::MockStateCache mock_cache;
    BlockNumberOrHash block_id{kTestBlockNumber};
    CachedDatabase cached_db{block_id, fake_txn, mock_cache};
    // Mock cursor shall provide the starting key-value pair for the iteration
    EXPECT_CALL(*mock_cursor, seek(_)).WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
        co_return KeyValue{*silkworm::from_hex("00"), kZeroBytes};
    }));
    core::rawdb::Walker walker = [&](const silkworm::Bytes& k, const silkworm::Bytes& v) -> bool {
        return false;
    };
    auto result = boost::asio::co_spawn(pool, cached_db.for_prefix(db::table::kCode, kZeroBytes, walker), boost::asio::use_future);
    CHECK_NOTHROW(result.get());
}

}  // namespace silkrpc::ethdb::kv
