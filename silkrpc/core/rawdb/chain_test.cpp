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

#include "chain.hpp" // NOLINT(build/include)

#include <string>

#include <asio/co_spawn.hpp>
#include <asio/thread_pool.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch.hpp>
#include <evmc/evmc.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>
#include <silkworm/common/rlp_err.hpp>
#include <silkworm/common/util.hpp>

#include <silkrpc/core/blocks.hpp>
#include <silkrpc/ethdb/tables.hpp>

namespace silkrpc::core::rawdb {

using Catch::Matchers::Message;
using testing::InvokeWithoutArgs;
using testing::Return;
using testing::_;
using evmc::literals::operator""_bytes32;

static silkworm::Bytes kNumber{*silkworm::from_hex("00000000003D0900")};
static silkworm::Bytes kBlockHash{*silkworm::from_hex("439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff")};
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
static silkworm::Bytes kBody{*silkworm::from_hex("c68369e45a03c0")};
static silkworm::Bytes kInvalidJsonChainConfig{*silkworm::from_hex("000102")};
static silkworm::Bytes kMissingChainIdConfig{*silkworm::from_hex("7b226265726c696e426c6f636b223a31323234343030302c"
    "2262797a616e7469756d426c6f636b223a343337303030302c22636f6e7374616e74696e6f706c65426c6f636b223a373238303030302"
    "c2264616f466f726b426c6f636b223a313932303030302c22656970313530426c6f636b223a323436333030302c22656970313535426c"
    "6f636b223a323637353030302c22657468617368223a7b7d2c22686f6d657374656164426c6f636b223a313135303030302c226973746"
    "16e62756c426c6f636b223a393036393030302c226c6f6e646f6e426c6f636b223a31323936353030302c226d756972476c6163696572"
    "426c6f636b223a393230303030302c2270657465727362757267426c6f636b223a373238303030307d")};
static silkworm::Bytes kInvalidChainIdConfig{*silkworm::from_hex("7b226265726c696e426c6f636b223a31323234343030302c"
    "2262797a616e7469756d426c6f636b223a343337303030302c22636861696e4964223a22666f6f222c22636f6e7374616e74696e6f706"
    "c65426c6f636b223a373238303030302c2264616f466f726b426c6f636b223a313932303030302c22656970313530426c6f636b223a32"
    "3436333030302c22656970313535426c6f636b223a323637353030302c22657468617368223a7b7d2c22686f6d657374656164426c6f6"
    "36b223a313135303030302c22697374616e62756c426c6f636b223a393036393030302c226c6f6e646f6e426c6f636b223a3132393635"
    "3030302c226d756972476c6163696572426c6f636b223a393230303030302c2270657465727362757267426c6f636b223a37323830303"
    "0307d")};
static silkworm::Bytes kChainConfig{*silkworm::from_hex("7b226265726c696e426c6f636b223a31323234343030302c2262797a6"
    "16e7469756d426c6f636b223a343337303030302c22636861696e4964223a312c22636f6e7374616e74696e6f706c65426c6f636b223a"
    "373238303030302c2264616f466f726b426c6f636b223a313932303030302c22656970313530426c6f636b223a323436333030302c226"
    "56970313535426c6f636b223a323637353030302c22657468617368223a7b7d2c22686f6d657374656164426c6f636b223a3131353030"
    "30302c22697374616e62756c426c6f636b223a393036393030302c226c6f6e646f6e426c6f636b223a31323936353030302c226d75697"
    "2476c6163696572426c6f636b223a393230303030302c2270657465727362757267426c6f636b223a373238303030307d")};

class MockDatabaseReader : public DatabaseReader {
public:
    MOCK_CONST_METHOD2(get, asio::awaitable<KeyValue>(const std::string&, const silkworm::ByteView&));
    MOCK_CONST_METHOD2(get_one, asio::awaitable<silkworm::Bytes>(const std::string&, const silkworm::ByteView&));
    MOCK_CONST_METHOD3(get_both_range, asio::awaitable<std::optional<silkworm::Bytes>>(const std::string&, const silkworm::ByteView&, const silkworm::ByteView&));
    MOCK_CONST_METHOD4(walk, asio::awaitable<void>(const std::string&, const silkworm::ByteView&, uint32_t, Walker));
    MOCK_CONST_METHOD3(for_prefix, asio::awaitable<void>(const std::string&, const silkworm::ByteView&, Walker));
};

void check_expected_block_with_hash(const silkworm::BlockWithHash& bwh) {
    CHECK(bwh.block.header.parent_hash == 0x209f062567c161c5f71b3f57a7de277b0e95c3455050b152d785ad7524ef8ee7_bytes32);
    CHECK(bwh.block.header.ommers_hash == 0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347_bytes32);
    CHECK(bwh.block.header.beneficiary == silkworm::to_address(*silkworm::from_hex("0000000000000000000000000000000000000000")));
    CHECK(bwh.block.header.state_root == 0xe7536c5b61ed0e0ab7f3ce7f085806d40f716689c0c086676757de401b595658_bytes32);
    CHECK(bwh.block.header.transactions_root == 0x40be247314d834a319556d1dcf458e8707cc1aa4a416b6118474ce0c96fccb1a_bytes32);
    CHECK(bwh.block.header.receipts_root == 0x7862fe11d10a9b237ffe9cb660f31e4bc4be66836c9bfc17310d47c60d75671f_bytes32);
    CHECK(bwh.block.header.number == 4000000);
    CHECK(bwh.block.header.gas_limit == 8000000);
    CHECK(bwh.block.header.gas_used == 1996875);
    CHECK(bwh.block.header.timestamp == 1609072811);
    CHECK(bwh.block.header.extra_data == *silkworm::from_hex("d88301091a846765746888676f312e31352e36856c696e757800000000000000be009d0049d6f0ee8ca6764a1d3e"
        "b519bd4d046e167ddcab467d5db31d063f2d58f266fa86c4502aa169d17762090e92b821843de69b41adbb5d86f5d114ba7f01"));
    CHECK(bwh.block.header.mix_hash == 0x0000000000000000000000000000000000000000000000000000000000000000_bytes32);
    CHECK(bwh.hash == 0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32);
}

TEST_CASE("read_header_number") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("existent hash") {
        EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kNumber}; }
        ));
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        auto result = asio::co_spawn(pool, read_header_number(db_reader, block_hash), asio::use_future);
        const auto header_number = result.get();
        CHECK(header_number == 4'000'000);
    }

    SECTION("non-existent hash") {
        EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        const auto block_hash{0x0000000000000000000000000000000000000000000000000000000000000000_bytes32};
        auto result = asio::co_spawn(pool, read_header_number(db_reader, block_hash), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::invalid_argument, Message("empty block number value in read_header_number"));
    }
}

TEST_CASE("read_chain_config") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("empty chain data") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kConfig, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_chain_config(db_reader), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::invalid_argument, Message("empty chain config data in read_chain_config"));
    }

    SECTION("invalid JSON chain data") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kConfig, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kInvalidJsonChainConfig}; }
        ));
        auto result = asio::co_spawn(pool, read_chain_config(db_reader), asio::use_future);
        CHECK_THROWS_AS(result.get(), nlohmann::json::parse_error);
    }

    SECTION("valid JSON chain data") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kConfig, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kChainConfig}; }
        ));
        auto result = asio::co_spawn(pool, read_chain_config(db_reader), asio::use_future);
        const auto chain_config = result.get();
        CHECK(chain_config.genesis_hash == 0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32);
        CHECK(chain_config.config == R"({
            "berlinBlock":12244000,
            "byzantiumBlock":4370000,
            "chainId":1,
            "constantinopleBlock":7280000,
            "daoForkBlock":1920000,
            "eip150Block":2463000,
            "eip155Block":2675000,
            "ethash":{},
            "homesteadBlock":1150000,
            "istanbulBlock":9069000,
            "londonBlock":12965000,
            "muirGlacierBlock":9200000,
            "petersburgBlock":7280000
        })"_json);
    }
}

TEST_CASE("read_chain_id") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("missing chain identifier") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kConfig, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kMissingChainIdConfig}; }
        ));
        auto result = asio::co_spawn(pool, read_chain_id(db_reader), asio::use_future);
        CHECK_THROWS_AS(result.get(), std::runtime_error);
    }

    SECTION("invalid chain identifier") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kConfig, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kInvalidChainIdConfig}; }
        ));
        auto result = asio::co_spawn(pool, read_chain_id(db_reader), asio::use_future);
        CHECK_THROWS_AS(result.get(), nlohmann::json::type_error);
    }

    SECTION("valid chain identifier") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kConfig, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kChainConfig}; }
        ));
        auto result = asio::co_spawn(pool, read_chain_id(db_reader), asio::use_future);
        const auto chain_id = result.get();
        CHECK(chain_id == 1);
    }
}

TEST_CASE("read_canonical_block_hash") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("empty hash bytes") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return silkworm::Bytes{}; }
        ));
        uint64_t block_number{4'000'000};
        auto result = asio::co_spawn(pool, read_canonical_block_hash(db_reader, block_number), asio::use_future);
        CHECK_THROWS_AS(result.get(), std::invalid_argument);
    }

    SECTION("shorter hash bytes") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return *silkworm::from_hex("9816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff"); }
        ));
        uint64_t block_number{4'000'000};
        auto result = asio::co_spawn(pool, read_canonical_block_hash(db_reader, block_number), asio::use_future);
        const auto block_hash = result.get();
        CHECK(block_hash == 0x009816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32);
    }

    SECTION("longer hash bytes") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return *silkworm::from_hex("439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dffabcdef"); }
        ));
        uint64_t block_number{4'000'000};
        auto result = asio::co_spawn(pool, read_canonical_block_hash(db_reader, block_number), asio::use_future);
        const auto block_hash = result.get();
        CHECK(block_hash == 0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32);
    }

    SECTION("valid canonical hash") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        uint64_t block_number{4'000'000};
        auto result = asio::co_spawn(pool, read_canonical_block_hash(db_reader, block_number), asio::use_future);
        const auto block_hash = result.get();
        CHECK(block_hash == 0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32);
    }
}

TEST_CASE("read_total_difficulty") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("empty RLP buffer") {
        EXPECT_CALL(db_reader, get(db::table::kDifficulty, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        evmc::bytes32 block_hash{0xd268bdabee5eab4914d0de9b0e0071364582cfb3c952b19727f1ab429f4ba2a8_bytes32};
        uint64_t block_number{4'000'000};
        auto result = asio::co_spawn(pool, read_total_difficulty(db_reader, block_hash, block_number), asio::use_future);
        CHECK_THROWS_AS(result.get(), std::invalid_argument);
    }

    SECTION("invalid RLP buffer") {
        EXPECT_CALL(db_reader, get(db::table::kDifficulty, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("000102")}; }
        ));
        evmc::bytes32 block_hash{0xd268bdabee5eab4914d0de9b0e0071364582cfb3c952b19727f1ab429f4ba2a8_bytes32};
        uint64_t block_number{4'000'000};
        auto result = asio::co_spawn(pool, read_total_difficulty(db_reader, block_hash, block_number), asio::use_future);
        CHECK_THROWS_AS(result.get(), std::runtime_error);
    }

    SECTION("valid total difficulty") {
        EXPECT_CALL(db_reader, get(db::table::kDifficulty, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("8360c7cc")}; }
        ));
        evmc::bytes32 block_hash{0xd268bdabee5eab4914d0de9b0e0071364582cfb3c952b19727f1ab429f4ba2a8_bytes32};
        uint64_t block_number{4'306'300};
        auto result = asio::co_spawn(pool, read_total_difficulty(db_reader, block_hash, block_number), asio::use_future);
        const auto total_difficulty = result.get();
        CHECK(total_difficulty == 6'342'604 /*0x60c7cc*/);
    }
}

TEST_CASE("read_block_by_number_or_hash") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("using valid number") {
        BlockNumberOrHash bnoh{4'000'000};
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kBody}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kEthTx, _, _, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<void> { co_return; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_number_or_hash(db_reader, bnoh), asio::use_future);
        const silkworm::BlockWithHash bwh = result.get();
        check_expected_block_with_hash(bwh);
    }

    SECTION("using valid hash") {
        BlockNumberOrHash bnoh{"0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff"};
        EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kNumber}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kBody}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kEthTx, _, _, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<void> { co_return; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_number_or_hash(db_reader, bnoh), asio::use_future);
        const silkworm::BlockWithHash bwh = result.get();
        check_expected_block_with_hash(bwh);
    }

    SECTION("using tag kEarliestBlockId") {
        BlockNumberOrHash bnoh{kEarliestBlockId};
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kBody}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kEthTx, _, _, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<void> { co_return; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_number_or_hash(db_reader, bnoh), asio::use_future);
        const silkworm::BlockWithHash bwh = result.get();
        check_expected_block_with_hash(bwh);
    }
}

TEST_CASE("read_block_by_hash") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("block header number not found") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_hash(db_reader, block_hash), asio::use_future);
        CHECK_THROWS_AS(result.get(), std::invalid_argument);
    }

    SECTION("block header not found") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kNumber}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_hash(db_reader, block_hash), asio::use_future);
        CHECK_THROWS_AS(result.get(), std::runtime_error);
    }

    SECTION("invalid block header") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kNumber}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{0x00, 0x01}}; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_hash(db_reader, block_hash), asio::use_future);
        CHECK_THROWS_AS(result.get(), std::runtime_error);
    }

    SECTION("block body not found") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kNumber}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_hash(db_reader, block_hash), asio::use_future);
        CHECK_THROWS_AS(result.get(), std::runtime_error);
    }

    SECTION("invalid block body") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kNumber}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{0x00, 0x01}}; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_hash(db_reader, block_hash), asio::use_future);
        CHECK_THROWS_AS(result.get(), silkworm::rlp::DecodingError);
    }

    SECTION("block found and matching") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kNumber}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kBody}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kEthTx, _, _, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<void> { co_return; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_hash(db_reader, block_hash), asio::use_future);
        const silkworm::BlockWithHash bwh = result.get();
        check_expected_block_with_hash(bwh);
    }
}

TEST_CASE("read_block_by_number") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("block canonical hash not found") {
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return silkworm::Bytes{}; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_number(db_reader, block_number), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::invalid_argument, Message("empty block hash value in read_canonical_block_hash"));
    }

    SECTION("block header not found") {
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_number(db_reader, block_number), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("empty block header RLP in read_header"));
    }

    SECTION("invalid block header") {
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{0x00, 0x01}}; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_number(db_reader, block_number), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("invalid RLP decoding for block header"));
    }

    SECTION("block body not found") {
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_number(db_reader, block_number), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("empty block body RLP in read_body"));
    }

    SECTION("invalid block body") {
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{0x00, 0x01}}; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_number(db_reader, block_number), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), silkworm::rlp::DecodingError, Message("Decoding error : kUnexpectedString"));
    }

    SECTION("block found and matching") {
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kBody}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kEthTx, _, _, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<void> { co_return; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_number(db_reader, block_number), asio::use_future);
        const silkworm::BlockWithHash bwh = result.get();
        check_expected_block_with_hash(bwh);
    }
}

TEST_CASE("read_block_by_transaction_hash") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("block header number not found") {
        const auto transaction_hash{0x18dcb90e76b61fe6f37c9a9cd269a66188c05af5f7a62c50ff3246c6e207dc6d_bytes32};
        EXPECT_CALL(db_reader, get_one(db::table::kTxLookup, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return silkworm::Bytes{}; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_transaction_hash(db_reader, transaction_hash), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::invalid_argument, Message("empty block number value in read_block_by_transaction_hash"));
    }

    SECTION("invalid block header number") {
        const auto transaction_hash{0x18dcb90e76b61fe6f37c9a9cd269a66188c05af5f7a62c50ff3246c6e207dc6d_bytes32};
        EXPECT_CALL(db_reader, get_one(db::table::kTxLookup, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return *silkworm::from_hex("01FFFFFFFFFFFFFFFF"); }
        ));
        auto result = asio::co_spawn(pool, read_block_by_transaction_hash(db_reader, transaction_hash), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::out_of_range, Message("stoul: out of range"));
    }

    SECTION("block canonical hash not found") {
        const auto transaction_hash{0x18dcb90e76b61fe6f37c9a9cd269a66188c05af5f7a62c50ff3246c6e207dc6d_bytes32};
        EXPECT_CALL(db_reader, get_one(db::table::kTxLookup, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return *silkworm::from_hex("3D0900"); }
        ));
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return silkworm::Bytes{}; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_transaction_hash(db_reader, transaction_hash), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::invalid_argument, Message("empty block hash value in read_canonical_block_hash"));
    }

    SECTION("block header not found") {
        const auto transaction_hash{0x18dcb90e76b61fe6f37c9a9cd269a66188c05af5f7a62c50ff3246c6e207dc6d_bytes32};
        EXPECT_CALL(db_reader, get_one(db::table::kTxLookup, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return *silkworm::from_hex("3D0900"); }
        ));
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_transaction_hash(db_reader, transaction_hash), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("empty block header RLP in read_header"));
    }

    SECTION("block body not found") {
        const auto transaction_hash{0x18dcb90e76b61fe6f37c9a9cd269a66188c05af5f7a62c50ff3246c6e207dc6d_bytes32};
        EXPECT_CALL(db_reader, get_one(db::table::kTxLookup, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return *silkworm::from_hex("3D0900"); }
        ));
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_transaction_hash(db_reader, transaction_hash), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("empty block body RLP in read_body"));
    }

    SECTION("block found and matching") {
        const auto transaction_hash{0x18dcb90e76b61fe6f37c9a9cd269a66188c05af5f7a62c50ff3246c6e207dc6d_bytes32};
        EXPECT_CALL(db_reader, get_one(db::table::kTxLookup, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return *silkworm::from_hex("3D0900"); }
        ));
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kBody}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kEthTx, _, _, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<void> { co_return; }
        ));
        auto result = asio::co_spawn(pool, read_block_by_transaction_hash(db_reader, transaction_hash), asio::use_future);
        const silkworm::BlockWithHash bwh = result.get();
        check_expected_block_with_hash(bwh);
    }
}

} // namespace silkrpc::core::rawdb
