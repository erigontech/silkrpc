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
using evmc::literals::operator""_bytes32;

static silkworm::Bytes kNumber{*silkworm::from_hex("00000000003D0900")};
static silkworm::Bytes block_hash = silkworm::bytes_of_string(std::string("0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff"));
static silkworm::Bytes kHeader{*silkworm::from_hex("f9025ca0209f062567c161c5f71b3f57a7de277b0e95c3455050b152d785ad"
    "7524ef8ee7a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347940000000000000000000000000000000"
    "000000000a0e7536c5b61ed0e0ab7f3ce7f085806d40f716689c0c086676757de401b595658a040be247314d834a319556d1dcf458e87"
    "07cc1aa4a416b6118474ce0c96fccb1aa07862fe11d10a9b237ffe9cb660f31e4bc4be66836c9bfc17310d47c60d75671fb9010000000"
    "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
    "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
    "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
    "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
    "0000000000000000000000000000000000000000000000000000000000000000000000001833d0900837a1200831e784b845fe880abb8"
    "61d88301091a846765746888676f312e31352e36856c696e757800000000000000be009d0049d6f0ee8ca6764a1d3eb519bd4d046e167"
    "ddcab467d5db31d063f2d58f266fa86c4502aa169d17762090e92b821843de69b41adbb5d86f5d114ba7f01a000000000000000000000"
    "00000000000000000000000000000000000000000000880000000000000000")};

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
        const std::string EARLIEST_BLOCK_ID = kEarliestBlockId;
        auto result = asio::co_spawn(pool, get_block_number(EARLIEST_BLOCK_ID, db_reader), asio::use_future);
        CHECK(result.get() == kEarliestBlockNumber);
    }

    SECTION("kLatestBlockId") {
        const std::string LATEST_BLOCK_ID = kLatestBlockId;

        EXPECT_CALL(db_reader, get(db::table::kLastForkchoice, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));

        EXPECT_CALL(db_reader, get(db::table::kSyncStageProgress, kExecutionStage)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("1234567890123456")};
                 }
        ));
        auto result = asio::co_spawn(pool, get_block_number(LATEST_BLOCK_ID, db_reader), asio::use_future);
        CHECK(result.get() == 0x1234567890123456);
    }

    SECTION("kPendingBlockId") {
        const std::string PENDING_BLOCK_ID = kPendingBlockId;
        EXPECT_CALL(db_reader, get(db::table::kLastForkchoice, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));

        EXPECT_CALL(db_reader, get(db::table::kSyncStageProgress, kExecutionStage)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("1234567890123456")}; }
        ));
        auto result = asio::co_spawn(pool, get_block_number(PENDING_BLOCK_ID, db_reader), asio::use_future);
        CHECK(result.get() == 0x1234567890123456);
    }


    SECTION("kFinalizedBlockId") {
        const std::string FINALIZED_FORKCHOICE_BLOCK_ID = kFinalizedBlockId;
        EXPECT_CALL(db_reader, get(db::table::kLastForkchoice, _)).WillOnce(InvokeWithoutArgs(
            [&]() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, block_hash};
            }
        ));

        EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, kNumber};
            }
        ));

        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));

        auto result = asio::co_spawn(pool, get_block_number(FINALIZED_FORKCHOICE_BLOCK_ID, db_reader), asio::use_future);
        CHECK(result.get() == 0x3d0900);
    }

    SECTION("kSafeBlockId") {
        const std::string SAFE_FORKCHOICE_BLOCK_ID = kSafeBlockId;
        EXPECT_CALL(db_reader, get(db::table::kLastForkchoice, _)).WillOnce(InvokeWithoutArgs(
            [&]() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, block_hash};
            }
        ));

        EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, kNumber};
            }
        ));

        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));

        auto result = asio::co_spawn(pool, get_block_number(SAFE_FORKCHOICE_BLOCK_ID, db_reader), asio::use_future);
        CHECK(result.get() == 0x3d0900);
    }

    SECTION("number in hex") {
        const std::string BLOCK_ID_HEX = "0x12345";
        auto result = asio::co_spawn(pool, get_block_number(BLOCK_ID_HEX, db_reader), asio::use_future);
        CHECK(result.get() == 0x12345);
    }

    SECTION("number in dec") {
        const std::string BLOCK_ID_DEC = "67890";
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

    EXPECT_CALL(db_reader, get(db::table::kLastForkchoice, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));

    EXPECT_CALL(db_reader, get(db::table::kSyncStageProgress, kExecutionStage)).WillOnce(InvokeWithoutArgs(
        []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("0000ddff12345678")}; }
    ));
    auto result = asio::co_spawn(pool, get_latest_block_number(db_reader), asio::use_future);
    CHECK(result.get() == 0x0000ddff12345678);
}

TEST_CASE("get_latest_block_number with head forkchoice number", "[silkrpc][core][blocks]") {
    const silkworm::ByteView kExecutionStage{stages::kExecution};
    MockDatabaseReader db_reader;
    asio::thread_pool pool{1};

    EXPECT_CALL(db_reader, get(db::table::kLastForkchoice, _)).WillOnce(InvokeWithoutArgs(
            [&]() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, block_hash};
            }
    ));

    EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, kNumber};
            }
        ));

    EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
        []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
    ));

    auto result = asio::co_spawn(pool, get_latest_block_number(db_reader), asio::use_future);
    CHECK(result.get() == 0x3d0900);
}

TEST_CASE("get_finalized_forkchoice_number with finalized block in db", "[silkrpc][core][blocks]") {
    MockDatabaseReader db_reader;
    asio::thread_pool pool{1};
    const std::string FINALIZED_FORKCHOICE_BLOCK_ID = kFinalizedBlockId;
    EXPECT_CALL(db_reader, get(db::table::kLastForkchoice, _)).WillOnce(InvokeWithoutArgs(
        [&]() -> asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, block_hash};
        }
    ));

    EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
        []() -> asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, kNumber};
        }
    ));

    EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
        []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
    ));

    auto result = asio::co_spawn(pool, get_block_number(FINALIZED_FORKCHOICE_BLOCK_ID, db_reader), asio::use_future);
    CHECK(result.get() == 0x3d0900);
}

TEST_CASE("get_finalized_forkchoice_number with no finalized block we return genesis number", "[silkrpc][core][blocks]") {
    MockDatabaseReader db_reader;
    asio::thread_pool pool{1};

    EXPECT_CALL(db_reader, get(db::table::kLastForkchoice, _)).WillOnce(InvokeWithoutArgs(
        [&]() -> asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}};
        }
    ));

    auto result = asio::co_spawn(pool, get_forkchoice_finalized_block_number(db_reader), asio::use_future);
    CHECK(result.get() == 0x0);
}

TEST_CASE("get_safe_forkchoice_number with safe block in db", "[silkrpc][core][blocks]") {
    MockDatabaseReader db_reader;
    asio::thread_pool pool{1};
    const std::string SAFE_FORKCHOICE_BLOCK_ID = kSafeBlockId;
    EXPECT_CALL(db_reader, get(db::table::kLastForkchoice, _)).WillOnce(InvokeWithoutArgs(
        [&]() -> asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, block_hash};
        }
    ));

    EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
        []() -> asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, kNumber};
        }
    ));

    EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
        []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
    ));

    auto result = asio::co_spawn(pool, get_block_number(SAFE_FORKCHOICE_BLOCK_ID, db_reader), asio::use_future);
    CHECK(result.get() == 0x3d0900);
}

TEST_CASE("get_safe_forkchoice_number with no safe block we return genesis number", "[silkrpc][core][blocks]") {
    MockDatabaseReader db_reader;
    asio::thread_pool pool{1};

    EXPECT_CALL(db_reader, get(db::table::kLastForkchoice, _)).WillOnce(InvokeWithoutArgs(
        [&]() -> asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}};
        }
    ));

    auto result = asio::co_spawn(pool, get_forkchoice_safe_block_number(db_reader), asio::use_future);
    CHECK(result.get() == 0x0);
}


} // namespace silkrpc::core

