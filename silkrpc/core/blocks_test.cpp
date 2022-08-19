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

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_future.hpp>
#include <catch2/catch.hpp>
#include <gmock/gmock.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/ethdb/tables.hpp>
#include <silkrpc/stagedsync/stages.hpp>
#include <silkrpc/test/mock_database_reader.hpp>
#include <silkworm/common/base.hpp>

namespace silkrpc::core {

using Catch::Matchers::Message;
using testing::InvokeWithoutArgs;

TEST_CASE("get_block_number", "[silkrpc][core][blocks]") {
    SILKRPC_LOG_STREAMS(null_stream(), null_stream());
    const silkworm::ByteView kExecutionStage{stages::kExecution};
    test::MockDatabaseReader db_reader;
    boost::asio::thread_pool pool{1};

    SECTION("kEarliestBlockId") {
        const std::string EARLIEST_BLOCK_ID = kEarliestBlockId;
        auto result = boost::asio::co_spawn(pool, get_block_number(EARLIEST_BLOCK_ID, db_reader), boost::asio::use_future);
        CHECK(result.get() == kEarliestBlockNumber);
    }

    SECTION("kLatestBlockId") {
        const std::string LATEST_BLOCK_ID = kLatestBlockId;
        EXPECT_CALL(db_reader, get(db::table::kSyncStageProgress, kExecutionStage))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("1234567890123456")};
            }));
        auto result = boost::asio::co_spawn(pool, get_block_number(LATEST_BLOCK_ID, db_reader), boost::asio::use_future);
        CHECK(result.get() == 0x1234567890123456);
    }

    SECTION("kPendingBlockId") {
        const std::string PENDING_BLOCK_ID = kPendingBlockId;
        EXPECT_CALL(db_reader, get(db::table::kSyncStageProgress, kExecutionStage))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("1234567890123456")};
            }));
        auto result = boost::asio::co_spawn(pool, get_block_number(PENDING_BLOCK_ID, db_reader), boost::asio::use_future);
        CHECK(result.get() == 0x1234567890123456);
    }

    SECTION("number in hex") {
        const std::string BLOCK_ID_HEX = "0x12345";
        auto result = boost::asio::co_spawn(pool, get_block_number(BLOCK_ID_HEX, db_reader), boost::asio::use_future);
        CHECK(result.get() == 0x12345);
    }

    SECTION("number in dec") {
        const std::string BLOCK_ID_DEC = "67890";
        auto result = boost::asio::co_spawn(pool, get_block_number(BLOCK_ID_DEC, db_reader), boost::asio::use_future);
        CHECK(result.get() == 67890);
    }
}

TEST_CASE("get_current_block_number", "[silkrpc][core][blocks]") {
    const silkworm::ByteView kFinishStage{stages::kFinish};
    test::MockDatabaseReader db_reader;
    boost::asio::thread_pool pool{1};

    EXPECT_CALL(db_reader, get(db::table::kSyncStageProgress, kFinishStage))
        .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("0000ddff12121212")};
        }));
    auto result = boost::asio::co_spawn(pool, get_current_block_number(db_reader), boost::asio::use_future);
    CHECK(result.get() == 0x0000ddff12121212);
}

TEST_CASE("get_highest_block_number", "[silkrpc][core][blocks]") {
    const silkworm::ByteView kHeadersStage{stages::kHeaders};
    test::MockDatabaseReader db_reader;
    boost::asio::thread_pool pool{1};

    EXPECT_CALL(db_reader, get(db::table::kSyncStageProgress, kHeadersStage))
        .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("0000ddff12345678")};
        }));
    auto result = boost::asio::co_spawn(pool, get_highest_block_number(db_reader), boost::asio::use_future);
    CHECK(result.get() == 0x0000ddff12345678);
}

TEST_CASE("get_latest_block_number", "[silkrpc][core][blocks]") {
    const silkworm::ByteView kExecutionStage{stages::kExecution};
    test::MockDatabaseReader db_reader;
    boost::asio::thread_pool pool{1};

    EXPECT_CALL(db_reader, get(db::table::kSyncStageProgress, kExecutionStage))
        .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("0000ddff12345678")};
        }));
    auto result = boost::asio::co_spawn(pool, get_latest_block_number(db_reader), boost::asio::use_future);
    CHECK(result.get() == 0x0000ddff12345678);
}

TEST_CASE("is_latest_block_number", "[silkrpc][core][blocks]") {
    const silkworm::ByteView kExecutionStage{stages::kExecution};
    test::MockDatabaseReader db_reader;
    boost::asio::thread_pool pool{1};

    SECTION("tag: latest") {
        BlockNumberOrHash bnoh{"latest"};
        auto result = boost::asio::co_spawn(pool, is_latest_block_number(bnoh, db_reader), boost::asio::use_future);
        CHECK(result.get());
    }

    SECTION("tag: pending") {
        BlockNumberOrHash bnoh{"pending"};
        auto result = boost::asio::co_spawn(pool, is_latest_block_number(bnoh, db_reader), boost::asio::use_future);
        CHECK(result.get());
    }

    SECTION("number: latest") {
        BlockNumberOrHash bnoh{1'000'000};
        // Mock reader shall be used to read latest block from Execution stage in table SyncStageProgress
        EXPECT_CALL(db_reader, get(db::table::kSyncStageProgress, kExecutionStage))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("00000000000F4240")};
            }));
        auto result = boost::asio::co_spawn(pool, is_latest_block_number(bnoh, db_reader), boost::asio::use_future);
        CHECK(result.get());
    }

    SECTION("number: not latest") {
        BlockNumberOrHash bnoh{1'000'000};
        // Mock reader shall be used to read latest block from Execution stage in table SyncStageProgress
        EXPECT_CALL(db_reader, get(db::table::kSyncStageProgress, kExecutionStage))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("00000000000F4241")};
            }));
        auto result = boost::asio::co_spawn(pool, is_latest_block_number(bnoh, db_reader), boost::asio::use_future);
        CHECK(!result.get());
    }
}

}  // namespace silkrpc::core
