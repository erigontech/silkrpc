/*
   Copyright 2021 The Silkrpc Authors

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

#include "blocks.hpp"

#include <string>

#include <asio/co_spawn.hpp>
#include <asio/thread_pool.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch.hpp>
#include <gmock/gmock.h>
#include <silkworm/common/base.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/stagedsync/stages.hpp>
#include <silkrpc/ethdb/tables.hpp>

namespace silkrpc::core {

using Catch::Matchers::Message;
using testing::InvokeWithoutArgs;
using testing::_;

class MockDatabaseReader : public core::rawdb::DatabaseReader {
public:
    MOCK_CONST_METHOD2(get, asio::awaitable<KeyValue>(const std::string&, const silkworm::ByteView&));
    MOCK_CONST_METHOD2(get_one, asio::awaitable<silkworm::Bytes>(const std::string&, const silkworm::ByteView&));
    MOCK_CONST_METHOD3(get_both_range, asio::awaitable<std::optional<silkworm::Bytes>>(const std::string&, const silkworm::ByteView&, const silkworm::ByteView&));
    MOCK_CONST_METHOD4(walk, asio::awaitable<void>(const std::string&, const silkworm::ByteView&, uint32_t, core::rawdb::Walker));
    MOCK_CONST_METHOD3(for_prefix, asio::awaitable<void>(const std::string&, const silkworm::ByteView&, core::rawdb::Walker));
};

TEST_CASE("get_block_number", "[silkrpc][core][blocks]") {
    SILKRPC_LOG_STREAMS(null_stream(), null_stream());
    const silkworm::ByteView kExecutionStage{stages::kExecution};
    MockDatabaseReader db_reader;
    asio::thread_pool pool{1};

    SECTION("kEarliestBlockId") {
        static const std::string EARLIEST_BLOCK_ID = kEarliestBlockId;
        auto result = asio::co_spawn(pool, get_block_number(EARLIEST_BLOCK_ID, db_reader), asio::use_future);
        CHECK(result.get() == kEarliestBlockNumber);
    }

    SECTION("kLatestBlockId") {
        static const std::string LATEST_BLOCK_ID = kLatestBlockId;
        EXPECT_CALL(db_reader, get(db::table::kSyncStageProgress, kExecutionStage)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("1234567890123456")}; }
        ));
        auto result = asio::co_spawn(pool, get_block_number(LATEST_BLOCK_ID, db_reader), asio::use_future);
        CHECK(result.get() == 0x1234567890123456);
    }

    SECTION("kPendingBlockId") {
        static const std::string PENDING_BLOCK_ID = kPendingBlockId;
        EXPECT_CALL(db_reader, get(db::table::kSyncStageProgress, kExecutionStage)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("1234567890123456")}; }
        ));
        auto result = asio::co_spawn(pool, get_block_number(PENDING_BLOCK_ID, db_reader), asio::use_future);
        CHECK(result.get() == 0x1234567890123456);
    }

    SECTION("number in hex") {
        static const std::string BLOCK_ID_HEX = "0x12345";
        auto result = asio::co_spawn(pool, get_block_number(BLOCK_ID_HEX, db_reader), asio::use_future);
        CHECK(result.get() == 0x12345);
    }

    SECTION("number in dec") {
        static const std::string BLOCK_ID_DEC = "67890";
        auto result = asio::co_spawn(pool, get_block_number(BLOCK_ID_DEC, db_reader), asio::use_future);
        CHECK(result.get() == 67890);
    }
}

TEST_CASE("get_current_block_number", "[silkrpc][core][blocks]") {
    const silkworm::ByteView kFinishStage{stages::kFinish};
    MockDatabaseReader db_reader;
    asio::thread_pool pool{1};

    EXPECT_CALL(db_reader, get(db::table::kSyncStageProgress, kFinishStage)).WillOnce(InvokeWithoutArgs(
        []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("0000ddff12121212")}; }
    ));
    auto result = asio::co_spawn(pool, get_current_block_number(db_reader), asio::use_future);
    CHECK(result.get() == 0x0000ddff12121212);
}

TEST_CASE("get_highest_block_number", "[silkrpc][core][blocks]") {
    const silkworm::ByteView kHeadersStage{stages::kHeaders};
    MockDatabaseReader db_reader;
    asio::thread_pool pool{1};

    EXPECT_CALL(db_reader, get(db::table::kSyncStageProgress, kHeadersStage)).WillOnce(InvokeWithoutArgs(
        []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("0000ddff12345678")}; }
    ));
    auto result = asio::co_spawn(pool, get_highest_block_number(db_reader), asio::use_future);
    CHECK(result.get() == 0x0000ddff12345678);
}

TEST_CASE("get_latest_block_number", "[silkrpc][core][blocks]") {
    const silkworm::ByteView kExecutionStage{stages::kExecution};
    MockDatabaseReader db_reader;
    asio::thread_pool pool{1};

    EXPECT_CALL(db_reader, get(db::table::kSyncStageProgress, kExecutionStage)).WillOnce(InvokeWithoutArgs(
        []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("0000ddff12345678")}; }
    ));
    auto result = asio::co_spawn(pool, get_latest_block_number(db_reader), asio::use_future);
    CHECK(result.get() == 0x0000ddff12345678);
}

} // namespace silkrpc::core

