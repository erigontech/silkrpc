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
using testing::DoAll;
using testing::Invoke;
using testing::InvokeWithoutArgs;
using testing::Return;
using testing::Unused;
using testing::_;
using evmc::literals::operator""_address;
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
static silkworm::Bytes kNotEmptyBody{*silkworm::from_hex("c683897f2e04c0")};
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

void check_expected_block_header(const silkworm::BlockHeader& header) {
    CHECK(header.parent_hash == 0x209f062567c161c5f71b3f57a7de277b0e95c3455050b152d785ad7524ef8ee7_bytes32);
    CHECK(header.ommers_hash == 0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347_bytes32);
    CHECK(header.beneficiary == silkworm::to_address(*silkworm::from_hex("0000000000000000000000000000000000000000")));
    CHECK(header.state_root == 0xe7536c5b61ed0e0ab7f3ce7f085806d40f716689c0c086676757de401b595658_bytes32);
    CHECK(header.transactions_root == 0x40be247314d834a319556d1dcf458e8707cc1aa4a416b6118474ce0c96fccb1a_bytes32);
    CHECK(header.receipts_root == 0x7862fe11d10a9b237ffe9cb660f31e4bc4be66836c9bfc17310d47c60d75671f_bytes32);
    CHECK(header.number == 4000000);
    CHECK(header.gas_limit == 8000000);
    CHECK(header.gas_used == 1996875);
    CHECK(header.timestamp == 1609072811);
    CHECK(header.extra_data == *silkworm::from_hex("d88301091a846765746888676f312e31352e36856c696e757800000000000000be009d0049d6f0ee8ca6764a1d3e"
        "b519bd4d046e167ddcab467d5db31d063f2d58f266fa86c4502aa169d17762090e92b821843de69b41adbb5d86f5d114ba7f01"));
    CHECK(header.mix_hash == 0x0000000000000000000000000000000000000000000000000000000000000000_bytes32);
}

void check_expected_block_body(const silkworm::BlockBody& body) {
    CHECK(body.transactions.size() == 0);
    CHECK(body.ommers.size() == 0);
}

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

void check_expected_transaction(const Transaction& transaction) {
    const auto eth_hash = hash_of_transaction(transaction);
    const auto tx_hash = silkworm::to_bytes32(silkworm::ByteView{eth_hash.bytes, silkworm::kHashLength});
    CHECK(tx_hash == 0x3ff7b8917f1941784c709d6e54db18500fddc2b4c1a90b5cdec675cd0f9fc042_bytes32);
    CHECK(transaction.access_list.empty());
    CHECK(transaction.block_hash == 0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32);
    CHECK(transaction.block_number == 4'000'000);
    CHECK(transaction.block_base_fee_per_gas == std::nullopt);
    CHECK(transaction.chain_id == 5);
    CHECK(transaction.data == *silkworm::from_hex(
        "f2f0387700000000000000000000000000000000000000000000000000000000000158b09f0270fc889c577c1c64db7c819f921d1b6e8c7e5d3f2ff34f162cf4b324cc05"
    ));
    CHECK(transaction.from == std::nullopt);
    //CHECK(transaction.nonce == 103470);
    CHECK(transaction.max_priority_fee_per_gas == 0x77359400);
    CHECK(transaction.max_fee_per_gas == 0x77359400);
    //CHECK(transaction.gas == 103470);
    CHECK(transaction.gas_limit == 5000000);
    CHECK(transaction.transaction_index == 0);
    CHECK(transaction.type == Transaction::Type::kLegacy);
    
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
        CHECK_THROWS_AS(result.get(), std::runtime_error);
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
        CHECK_THROWS_AS(result.get(), std::runtime_error);
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
        CHECK_THROWS_AS(result.get(), std::out_of_range);
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

TEST_CASE("read_block") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("block header not found") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_block(db_reader, block_hash, block_number), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("empty block header RLP in read_header"));
    }

    SECTION("invalid block header") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{0x00, 0x01}}; }
        ));
        auto result = asio::co_spawn(pool, read_block(db_reader, block_hash, block_number), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("invalid RLP decoding for block header"));
    }

    SECTION("block body not found") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_block(db_reader, block_hash, block_number), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("empty block body RLP in read_body"));
    }

    SECTION("invalid block body") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{0x00, 0x01}}; }
        ));
        auto result = asio::co_spawn(pool, read_block(db_reader, block_hash, block_number), asio::use_future);
        CHECK_THROWS_AS(result.get(), std::runtime_error);
    }

    SECTION("empty block") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("c68369000003c0")}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kEthTx, _, _, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<void> { co_return; }
        ));
        auto result = asio::co_spawn(pool, read_block(db_reader, block_hash, block_number), asio::use_future);
        const silkworm::BlockWithHash bwh = result.get();
        CHECK(bwh.block.transactions.size() == 0);
    }

    SECTION("block found and matching") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kBody}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kEthTx, _, _, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<void> { co_return; }
        ));
        auto result = asio::co_spawn(pool, read_block(db_reader, block_hash, block_number), asio::use_future);
        const silkworm::BlockWithHash bwh = result.get();
        check_expected_block_with_hash(bwh);
    }
}

TEST_CASE("read_header_by_hash") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("block header number not found") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_header_by_hash(db_reader, block_hash), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::invalid_argument, Message("empty block number value in read_header_number"));
    }

    SECTION("block header not found") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kNumber}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_header_by_hash(db_reader, block_hash), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("empty block header RLP in read_header"));
    }

    SECTION("invalid block header") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kNumber}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{0x00, 0x01}}; }
        ));
        auto result = asio::co_spawn(pool, read_header_by_hash(db_reader, block_hash), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("invalid RLP decoding for block header"));
    }

    SECTION("block header found and matching") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        EXPECT_CALL(db_reader, get(db::table::kHeaderNumbers, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kNumber}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        auto result = asio::co_spawn(pool, read_header_by_hash(db_reader, block_hash), asio::use_future);
        const silkworm::BlockHeader bh = result.get();
        check_expected_block_header(bh);
    }
}

TEST_CASE("read_header_by_number") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("block canonical hash not found") {
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return silkworm::Bytes{}; }
        ));
        auto result = asio::co_spawn(pool, read_header_by_number(db_reader, block_number), asio::use_future);
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
        auto result = asio::co_spawn(pool, read_header_by_number(db_reader, block_number), asio::use_future);
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
        auto result = asio::co_spawn(pool, read_header_by_number(db_reader, block_number), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("invalid RLP decoding for block header"));
    }

    SECTION("block header found and matching") {
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kBlockHash; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        auto result = asio::co_spawn(pool, read_header_by_number(db_reader, block_number), asio::use_future);
        const silkworm::BlockHeader bh = result.get();
        check_expected_block_header(bh);
    }
}

TEST_CASE("read_header") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("block header not found") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_header(db_reader, block_hash, block_number), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("empty block header RLP in read_header"));
    }

    SECTION("invalid block header") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{0x00, 0x01}}; }
        ));
        auto result = asio::co_spawn(pool, read_header(db_reader, block_hash, block_number), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("invalid RLP decoding for block header"));
    }

    SECTION("block header found and matching") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        auto result = asio::co_spawn(pool, read_header(db_reader, block_hash, block_number), asio::use_future);
        const silkworm::BlockHeader bh = result.get();
        check_expected_block_header(bh);
    }
}

TEST_CASE("read_body") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("block body not found") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_body(db_reader, block_hash, block_number), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("empty block body RLP in read_body"));
    }

    SECTION("invalid block body") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{0x00, 0x01}}; }
        ));
        auto result = asio::co_spawn(pool, read_body(db_reader, block_hash, block_number), asio::use_future);
        CHECK_THROWS_AS(result.get(), std::runtime_error);
    }

    SECTION("block found and matching") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kBody}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kEthTx, _, _, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<void> { co_return; }
        ));
        auto result = asio::co_spawn(pool, read_body(db_reader, block_hash, block_number), asio::use_future);
        const silkworm::BlockBody body = result.get();
        check_expected_block_body(body);
    }
}

TEST_CASE("read_header_rlp") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("block header RLP not found") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_header_rlp(db_reader, block_hash, block_number), asio::use_future);
        CHECK(result.get() == silkworm::Bytes{});
    }

    SECTION("block header RLP found and matching") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get(db::table::kHeaders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kHeader}; }
        ));
        auto result = asio::co_spawn(pool, read_header_rlp(db_reader, block_hash, block_number), asio::use_future);
        CHECK(result.get() == kHeader);
    }
}

TEST_CASE("read_body_rlp") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("block body RLP not found") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_body_rlp(db_reader, block_hash, block_number), asio::use_future);
        CHECK(result.get() == silkworm::Bytes{});
    }

    SECTION("block body RLP found and matching") {
        const auto block_hash{0x439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff_bytes32};
        const uint64_t block_number{4'000'000};
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kBody}; }
        ));
        auto result = asio::co_spawn(pool, read_body_rlp(db_reader, block_hash, block_number), asio::use_future);
        CHECK(result.get() == kBody);
    }
}

TEST_CASE("read_transaction_by_hash") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("block header number not found") {
        const auto transaction_hash{0x18dcb90e76b61fe6f37c9a9cd269a66188c05af5f7a62c50ff3246c6e207dc6d_bytes32};
        EXPECT_CALL(db_reader, get_one(db::table::kTxLookup, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return silkworm::Bytes{}; }
        ));
        auto result = asio::co_spawn(pool, read_transaction_by_hash(db_reader, transaction_hash), asio::use_future);
        CHECK(result.get() == std::nullopt);
    }

    SECTION("invalid block header number") {
        const auto transaction_hash{0x18dcb90e76b61fe6f37c9a9cd269a66188c05af5f7a62c50ff3246c6e207dc6d_bytes32};
        EXPECT_CALL(db_reader, get_one(db::table::kTxLookup, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return *silkworm::from_hex("01FFFFFFFFFFFFFFFF"); }
        ));
        auto result = asio::co_spawn(pool, read_transaction_by_hash(db_reader, transaction_hash), asio::use_future);
        CHECK_THROWS_AS(result.get(), std::out_of_range);
    }

    SECTION("transaction not found") {
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
        auto result = asio::co_spawn(pool, read_transaction_by_hash(db_reader, transaction_hash), asio::use_future);
        CHECK(result.get() == std::nullopt);
    }

    SECTION("transaction found and matching") {
        const auto transaction_hash{0x3ff7b8917f1941784c709d6e54db18500fddc2b4c1a90b5cdec675cd0f9fc042_bytes32};
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
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kNotEmptyBody}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kEthTx, _, _, _)).WillOnce(Invoke(
            [](Unused, Unused, Unused, Walker w) -> asio::awaitable<void> {
                silkworm::Bytes key{};
                silkworm::Bytes value{*silkworm::from_hex("f8ac8301942e8477359400834c4b40945f62669ba0c6cf41cc162d8157ed71a0b9d6dbaf80b844f2"
                    "f0387700000000000000000000000000000000000000000000000000000000000158b09f0270fc889c577c1c64db7c819f921d"
                    "1b6e8c7e5d3f2ff34f162cf4b324cc052ea0d5494ad16e2233197daa9d54cbbcb1ee534cf9f675fa587c264a4ce01e7d3d23a0"
                    "1421bcf57f4b39eb84a35042dc4675ae167f3e2f50e808252afa23e62e692355")};
                w(key, value);
                co_return;
            }
        ));
        auto result = asio::co_spawn(pool, read_transaction_by_hash(db_reader, transaction_hash), asio::use_future);
        const std::optional<silkrpc::Transaction> transaction = result.get();
        CHECK(transaction.has_value());
        check_expected_transaction(transaction.value());
    }
}

TEST_CASE("read_senders") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("zero senders") { // https://goerli.etherscan.io/block/20000
        const auto block_hash{0x96908d141b3c2727342b48696f97b50845240e3ceda0c86ac3dc2e197eb9675b_bytes32};
        const uint64_t block_number{20'000};
        EXPECT_CALL(db_reader, get(db::table::kSenders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_senders(db_reader, block_hash, block_number), asio::use_future);
        CHECK(result.get() == Addresses{});
    }

    SECTION("one sender") { // https://goerli.etherscan.io/block/3529603
        const auto block_hash{0x8059c265f40cdb2d3b3245847c21ed154eebf299fd0ff01ee3afded43cdadc45_bytes32};
        const uint64_t block_number{3'529'603};
        EXPECT_CALL(db_reader, get(db::table::kSenders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("70A5C9D346416f901826581d423Cd5B92d44Ff5a")};
            }
        ));
        auto result = asio::co_spawn(pool, read_senders(db_reader, block_hash, block_number), asio::use_future);
        CHECK(result.get() == Addresses{0x70A5C9D346416f901826581d423Cd5B92d44Ff5a_address});
    }

    SECTION("many senders") { // https://goerli.etherscan.io/block/3529601
        const auto block_hash{0xc4af12a451cf621a0786b2b240fa4b479ae9110083db4df80d1527dc702e91a6_bytes32};
        const uint64_t block_number{3'529'601};
        EXPECT_CALL(db_reader, get(db::table::kSenders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex(
                    "be188D6641E8b680743A4815dFA0f6208038960F"
                    "0828D0386C1122E565f07DD28c7d1340eD5B3315"
                    "70A5C9D346416f901826581d423Cd5B92d44Ff5a"
                    "Dd74564BC9ff247C23f02cFbA1083c805829D981")};
            }
        ));
        auto result = asio::co_spawn(pool, read_senders(db_reader, block_hash, block_number), asio::use_future);
        CHECK(result.get() == Addresses{
            0xbe188D6641E8b680743A4815dFA0f6208038960F_address,
            0x0828D0386C1122E565f07DD28c7d1340eD5B3315_address,
            0x70A5C9D346416f901826581d423Cd5B92d44Ff5a_address,
            0xDd74564BC9ff247C23f02cFbA1083c805829D981_address});
    }
}

TEST_CASE("read_raw_receipts") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("zero receipts") {
        const auto block_hash{silkworm::kEmptyHash};
        const uint64_t block_number{0};
        EXPECT_CALL(db_reader, get(db::table::kBlockReceipts, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_raw_receipts(db_reader, block_hash, block_number), asio::use_future);
        //CHECK(result.get() == Receipts{}); // TODO(canepat): provide operator== and operator!= for Receipt type
        CHECK(result.get().size() == 0);
    }

    SECTION("one receipt") { // https://goerli.etherscan.io/block/3529600
        const auto block_hash{0x22de8ed177a7b8485d9e26e90dbf523f70cc4a9fa4299bc1daf68791a4386bf3_bytes32};
        const uint64_t block_number{3'529'600};
        EXPECT_CALL(db_reader, get(db::table::kBlockReceipts, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("818400f6011a0004a0c8")}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kLogs, _, _, _)).WillOnce(Invoke(
            [](Unused, Unused, Unused, Walker w) -> asio::awaitable<void> {
                silkworm::Bytes key{*silkworm::from_hex("000000000035db8000000000")};
                silkworm::Bytes value{*silkworm::from_hex(
                    "8683547753cfad258efbc52a9a1452e42ffbce9be486cb835820ddf252ad1be2c89b69c2b068fc"
                    "378daa952ba7f163c4a11628f55a4df523b3ef5820000000000000000000000000ac399a5dfb98"
                    "48d9e83d92d5f7dda9ba1a00132058200000000000000000000000003dd81545f3149538edcb66"
                    "91a4ffee1898bd2ef0582000000000000000000000000000000000000000000000000000000000"
                    "009896808354ac399a5dfb9848d9e83d92d5f7dda9ba1a0013208158209a7def6556351196c74c"
                    "99e1cc8dcd284e9da181ea854c3e6367cc9fad882a515840000000000000000000000000f13c66"
                    "6056048634109c1ecca6893da293c70da40000000000000000000000000214281cf15c1a66b519"
                    "90e2e65e1f7b7c36331883540214281cf15c1a66b51990e2e65e1f7b7c363318815820be2e1f3a"
                    "6197dfd16fa6830c4870364b618b8b288c21cbcfa4fdb5d7c6a5e45b58409f29225dee002d9875"
                    "a2251ca89348cb8db9656b7ff556065eddb16c9f0618a100000000000000000000000000000000"
                    "0000000000000000000000000000000083547753cfad258efbc52a9a1452e42ffbce9be486cb83"
                    "5820ddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef5820000000"
                    "0000000000000000003dd81545f3149538edcb6691a4ffee1898bd2ef058200000000000000000"
                    "000000000828d0386c1122e565f07dd28c7d1340ed5b3315582000000000000000000000000000"
                    "0000000000000000000000000000000098968083543dd81545f3149538edcb6691a4ffee1898bd"
                    "2ef08358202ed7bcf2ff03098102c7003d7ce2a633e4b49b8198b07de5383cdf4c0ab9228b5820"
                    "000000000000000000000000f13c666056048634109c1ecca6893da293c70da458200000000000"
                    "000000000000000214281cf15c1a66b51990e2e65e1f7b7c363318582000000000000000000000"
                    "0000ac399a5dfb9848d9e83d92d5f7dda9ba1a00132083543dd81545f3149538edcb6691a4ffee"
                    "1898bd2ef0835820efaf768237c22e140a862d5d375ad5c153479fac3f8bcf8b580a1651fd62c3"
                    "ef5820000000000000000000000000f13c666056048634109c1ecca6893da293c70da458200000"
                    "000000000000000000000214281cf15c1a66b51990e2e65e1f7b7c363318f6")};
                w(key, value);
                co_return;
            }
        ));
        auto result = asio::co_spawn(pool, read_raw_receipts(db_reader, block_hash, block_number), asio::use_future);
        //CHECK(result.get() == Receipts{Receipt{...}}); // TODO(canepat): provide operator== and operator!= for Receipt type
        CHECK(result.get().size() == Receipts{Receipt{}}.size());
    }

    SECTION("many receipts") { // https://goerli.etherscan.io/block/3529600
        const auto block_hash{0xc9e65d063911aa583e17bbb7070893482203217caf6d9fbb50265c72e7bf73e5_bytes32};
        const uint64_t block_number{3'529'604};
        EXPECT_CALL(db_reader, get(db::table::kBlockReceipts, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex(
                "828400f6011a0003be508400f6011a0008b89a")}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kLogs, _, _, _)).WillOnce(Invoke(
            [](Unused, Unused, Unused, Walker w) -> asio::awaitable<void> {
                silkworm::Bytes key1{*silkworm::from_hex("000000000035db8400000000")};
                silkworm::Bytes value1{*silkworm::from_hex(
                    "8383547977d4f555fbee46303682b17e72e3d94339b4418258206155cfd0fd028b0ca77e8495a6"
                    "0cbe563e8bce8611f0aad6fedbdaafc05d44a258200000000000000000000000004ed7fae4af36"
                    "f11ac28275a98ca1d131e91bb6cd58600000000000000000000000000000000000000000000000"
                    "00015bb9773f49f764000000000000000000000000000000000000000000000000015c2a7b13fd"
                    "0000000000000000000000000000000000000000000000000000000000005f7cd33d8354fa365f"
                    "1384e4eaf6d59f353c782af3ea42feaab98258207aa1a8eb998c779420645fc14513bf058edb34"
                    "7d95c2fc2e6845bdc22f88863158200000000000000000000000004ed7fae4af36f11ac28275a9"
                    "8ca1d131e91bb6cd5840000000000000000000000000000000000000000000000000015c2a7b13"
                    "fd0000000000000000000000000000000000000000000000000000000000005f7cd33d835408f0"
                    "006e549edaef936ac2e3cb0c6f7c45ad5f968258202c7d80ba9bc6395644b4ff4a878353ac20ad"
                    "eed6e23cead48c8cec7a58b6e7195820d76aaac3ecd5ced13bbab3b240a426352f76a6fffd583c"
                    "3b15f4ddae2b754e4e5840000000000000000000000000000000000000000000000000015c2a7b"
                    "13fd0000000000000000000000000000000000000000000000000000000000005f7cd33d")};
                w(key1, value1);
                silkworm::Bytes key2{*silkworm::from_hex("000000000035db8400000001")};
                silkworm::Bytes value2{*silkworm::from_hex(
                    "82835407b39f4fde4a38bace212b546dac87c58dfe3fdc815820649bbc62d0e31342afea4e5cd8"
                    "2d4049e7e1ee912fc0889aa790803be39038c55902400000000000000000000000000000000000"
                    "0000000000000000000000000000a0000000000000000000000000000000000000000000000000"
                    "000000000000010000000000000000000000000000000000000000000000000000000000000001"
                    "400000000000000000000000000000000000000000000000000000000000000180000000000000"
                    "000000000000000000000000000000000000000000000000020000000000000000000000000000"
                    "00000000000000000000000000000000000030a5a151a2320abaab98cfa8366fc326fb6f45cf1c"
                    "93697191ec1370e1caca0fc6237e3bc5328755ae66bc5ddb141f0cb10000000000000000000000"
                    "0000000000000000000000000000000000000000000000000000000000000000000000002000d7"
                    "7be6277f1cdcfce33fdcb127b95fe91e09eec04aecc521dc94866f0055f0000000000000000000"
                    "000000000000000000000000000000000000000000000800405973070000000000000000000000"
                    "000000000000000000000000000000000000000000000000000000000000000000000000000000"
                    "000000000000000060a4dcd35675e049ea5b58d9567f8029669d4cdbe72511d330d96a578e2714"
                    "f1c9db00f6a9babc217b250fc7f217b0261506727657b420d9e05adc73675390ce2eb1e1aef3ba"
                    "c7d1b4b424c9dc07cdcac2729eabdb81c857325e20202ea2476160000000000000000000000000"
                    "0000000000000000000000000000000000000008ac360100000000000000000000000000000000"
                    "00000000000000000000000000835431af35bdfa897cd42b204c003560c385d444707582582026"
                    "725881c2a4290b02cd153d6599fd484f0d4e6062c361e740fbbe39e7ad61425820000000000000"
                    "000000000000000000000000000000000000000000000000000258200000000000000000000000"
                    "00000000000000000000000000000000005f7cd33d")};
                w(key2, value2);
                co_return;
            }
        ));
        auto result = asio::co_spawn(pool, read_raw_receipts(db_reader, block_hash, block_number), asio::use_future);
        //CHECK(result.get() == Receipts{Receipt{...}, Receipt{...}}); // TODO(canepat): provide operator== and operator!= for Receipt type
        CHECK(result.get().size() == Receipts{Receipt{}, Receipt{}}.size());
    }

    SECTION("invalid receipt log") { // https://goerli.etherscan.io/block/3529600
        const auto block_hash{0x22de8ed177a7b8485d9e26e90dbf523f70cc4a9fa4299bc1daf68791a4386bf3_bytes32};
        const uint64_t block_number{3'529'600};
        EXPECT_CALL(db_reader, get(db::table::kBlockReceipts, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("818400f6011a0004a0c8")}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kLogs, _, _, _)).WillOnce(Invoke(
            [](Unused, Unused, Unused, Walker w) -> asio::awaitable<void> {
                silkworm::Bytes key{};
                silkworm::Bytes value{*silkworm::from_hex(
                    "8683547753cfad258efbc52a9a1452e42ffbce9be486cb835820ddf252ad1be2c89b69c2b068fc"
                    "378daa952ba7f163c4a11628f55a4df523b3ef5820000000000000000000000000ac399a5dfb98"
                    "48d9e83d92d5f7dda9ba1a00132058200000000000000000000000003dd81545f3149538edcb66"
                    "91a4ffee1898bd2ef0582000000000000000000000000000000000000000000000000000000000"
                    "009896808354ac399a5dfb9848d9e83d92d5f7dda9ba1a0013208158209a7def6556351196c74c"
                    "99e1cc8dcd284e9da181ea854c3e6367cc9fad882a515840000000000000000000000000f13c66"
                    "6056048634109c1ecca6893da293c70da40000000000000000000000000214281cf15c1a66b519"
                    "90e2e65e1f7b7c36331883540214281cf15c1a66b51990e2e65e1f7b7c363318815820be2e1f3a"
                    "6197dfd16fa6830c4870364b618b8b288c21cbcfa4fdb5d7c6a5e45b58409f29225dee002d9875"
                    "a2251ca89348cb8db9656b7ff556065eddb16c9f0618a100000000000000000000000000000000"
                    "0000000000000000000000000000000083547753cfad258efbc52a9a1452e42ffbce9be486cb83"
                    "5820ddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef5820000000"
                    "0000000000000000003dd81545f3149538edcb6691a4ffee1898bd2ef058200000000000000000"
                    "000000000828d0386c1122e565f07dd28c7d1340ed5b3315582000000000000000000000000000"
                    "0000000000000000000000000000000098968083543dd81545f3149538edcb6691a4ffee1898bd"
                    "2ef08358202ed7bcf2ff03098102c7003d7ce2a633e4b49b8198b07de5383cdf4c0ab9228b5820"
                    "000000000000000000000000f13c666056048634109c1ecca6893da293c70da458200000000000"
                    "000000000000000214281cf15c1a66b51990e2e65e1f7b7c363318582000000000000000000000"
                    "0000ac399a5dfb9848d9e83d92d5f7dda9ba1a00132083543dd81545f3149538edcb6691a4ffee"
                    "1898bd2ef0835820efaf768237c22e140a862d5d375ad5c153479fac3f8bcf8b580a1651fd62c3"
                    "ef5820000000000000000000000000f13c666056048634109c1ecca6893da293c70da458200000"
                    "000000000000000000000214281cf15c1a66b51990e2e65e1f7b7c363318f6")};
                w(key, value);
                co_return;
            }
        ));
        auto result = asio::co_spawn(pool, read_raw_receipts(db_reader, block_hash, block_number), asio::use_future);
        // TODO(canepat): this case should fail instead of providing 1 receipt with 0 logs
        const Receipts receipts = result.get();
        CHECK(receipts.size() == 1);
        CHECK(receipts[0].logs.size() == 0);
    }
}

TEST_CASE("read_receipts") {
    asio::thread_pool pool{1};
    MockDatabaseReader db_reader;

    SECTION("zero receipts w/ zero transactions and zero senders") {
        const auto block_hash{silkworm::kEmptyHash};
        const uint64_t block_number{0};
        EXPECT_CALL(db_reader, get(db::table::kBlockReceipts, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kBody}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kEthTx, _, _, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<void> { co_return; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kSenders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_receipts(db_reader, block_hash, block_number), asio::use_future);
        //CHECK(result.get() == Receipts{}); // TODO(canepat): provide operator== and operator!= for Receipt type
        CHECK(result.get().size() == 0);
    }

    SECTION("zero receipts w/ non-zero transactions and zero senders") {
        const auto block_hash{silkworm::kEmptyHash};
        const uint64_t block_number{0};
        EXPECT_CALL(db_reader, get(db::table::kBlockReceipts, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kNotEmptyBody}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kEthTx, _, _, _)).WillOnce(Invoke(
            [](Unused, Unused, Unused, Walker w) -> asio::awaitable<void> {
                silkworm::Bytes key{};
                silkworm::Bytes value{*silkworm::from_hex("f8ac8301942e8477359400834c4b40945f62669ba0c6cf41cc162d8157ed71a0b9d6dbaf80b844f2"
                    "f0387700000000000000000000000000000000000000000000000000000000000158b09f0270fc889c577c1c64db7c819f921d"
                    "1b6e8c7e5d3f2ff34f162cf4b324cc052ea0d5494ad16e2233197daa9d54cbbcb1ee534cf9f675fa587c264a4ce01e7d3d23a0"
                    "1421bcf57f4b39eb84a35042dc4675ae167f3e2f50e808252afa23e62e692355")};
                w(key, value);
                co_return;
            }
        ));
        EXPECT_CALL(db_reader, get(db::table::kSenders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        auto result = asio::co_spawn(pool, read_receipts(db_reader, block_hash, block_number), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("#transactions and #receipts do not match in read_receipts"));
    }

    SECTION("zero receipts w/ zero transactions and non-zero senders") {
        const auto block_hash{silkworm::kEmptyHash};
        const uint64_t block_number{0};
        EXPECT_CALL(db_reader, get(db::table::kBlockReceipts, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kBody}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kEthTx, _, _, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<void> { co_return; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kSenders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex(
                    "be188D6641E8b680743A4815dFA0f6208038960F"
                    "0828D0386C1122E565f07DD28c7d1340eD5B3315"
                    "70A5C9D346416f901826581d423Cd5B92d44Ff5a"
                    "Dd74564BC9ff247C23f02cFbA1083c805829D981")};
            }
        ));
        auto result = asio::co_spawn(pool, read_receipts(db_reader, block_hash, block_number), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("#senders and #receipts do not match in in read_receipts"));
    }

    SECTION("zero receipts w/ non-zero transactions and non-zero senders") {
        const auto block_hash{silkworm::kEmptyHash};
        const uint64_t block_number{0};
        EXPECT_CALL(db_reader, get(db::table::kBlockReceipts, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}}; }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, kNotEmptyBody}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kEthTx, _, _, _)).WillOnce(Invoke(
            [](Unused, Unused, Unused, Walker w) -> asio::awaitable<void> {
                silkworm::Bytes key{};
                silkworm::Bytes value{*silkworm::from_hex("f8ac8301942e8477359400834c4b40945f62669ba0c6cf41cc162d8157ed71a0b9d6dbaf80b844f2"
                    "f0387700000000000000000000000000000000000000000000000000000000000158b09f0270fc889c577c1c64db7c819f921d"
                    "1b6e8c7e5d3f2ff34f162cf4b324cc052ea0d5494ad16e2233197daa9d54cbbcb1ee534cf9f675fa587c264a4ce01e7d3d23a0"
                    "1421bcf57f4b39eb84a35042dc4675ae167f3e2f50e808252afa23e62e692355")};
                w(key, value);
                co_return;
            }
        ));
        EXPECT_CALL(db_reader, get(db::table::kSenders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex(
                    "be188D6641E8b680743A4815dFA0f6208038960F"
                    "0828D0386C1122E565f07DD28c7d1340eD5B3315"
                    "70A5C9D346416f901826581d423Cd5B92d44Ff5a"
                    "Dd74564BC9ff247C23f02cFbA1083c805829D981")};
            }
        ));
        auto result = asio::co_spawn(pool, read_receipts(db_reader, block_hash, block_number), asio::use_future);
        CHECK_THROWS_MATCHES(result.get(), std::runtime_error, Message("#transactions and #receipts do not match in read_receipts"));
    }

    SECTION("one receipt") { // https://goerli.etherscan.io/block/3529600
        const auto block_hash{silkworm::kEmptyHash};
        const uint64_t block_number{0};
        EXPECT_CALL(db_reader, get(db::table::kBlockReceipts, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("818400f6011a0004a0c8")}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kLogs, _, _, _)).WillOnce(Invoke(
            [](Unused, Unused, Unused, Walker w) -> asio::awaitable<void> {
                silkworm::Bytes key{*silkworm::from_hex("000000000035db8000000000")};
                silkworm::Bytes value{*silkworm::from_hex(
                    "8683547753cfad258efbc52a9a1452e42ffbce9be486cb835820ddf252ad1be2c89b69c2b068fc"
                    "378daa952ba7f163c4a11628f55a4df523b3ef5820000000000000000000000000ac399a5dfb98"
                    "48d9e83d92d5f7dda9ba1a00132058200000000000000000000000003dd81545f3149538edcb66"
                    "91a4ffee1898bd2ef0582000000000000000000000000000000000000000000000000000000000"
                    "009896808354ac399a5dfb9848d9e83d92d5f7dda9ba1a0013208158209a7def6556351196c74c"
                    "99e1cc8dcd284e9da181ea854c3e6367cc9fad882a515840000000000000000000000000f13c66"
                    "6056048634109c1ecca6893da293c70da40000000000000000000000000214281cf15c1a66b519"
                    "90e2e65e1f7b7c36331883540214281cf15c1a66b51990e2e65e1f7b7c363318815820be2e1f3a"
                    "6197dfd16fa6830c4870364b618b8b288c21cbcfa4fdb5d7c6a5e45b58409f29225dee002d9875"
                    "a2251ca89348cb8db9656b7ff556065eddb16c9f0618a100000000000000000000000000000000"
                    "0000000000000000000000000000000083547753cfad258efbc52a9a1452e42ffbce9be486cb83"
                    "5820ddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef5820000000"
                    "0000000000000000003dd81545f3149538edcb6691a4ffee1898bd2ef058200000000000000000"
                    "000000000828d0386c1122e565f07dd28c7d1340ed5b3315582000000000000000000000000000"
                    "0000000000000000000000000000000098968083543dd81545f3149538edcb6691a4ffee1898bd"
                    "2ef08358202ed7bcf2ff03098102c7003d7ce2a633e4b49b8198b07de5383cdf4c0ab9228b5820"
                    "000000000000000000000000f13c666056048634109c1ecca6893da293c70da458200000000000"
                    "000000000000000214281cf15c1a66b51990e2e65e1f7b7c363318582000000000000000000000"
                    "0000ac399a5dfb9848d9e83d92d5f7dda9ba1a00132083543dd81545f3149538edcb6691a4ffee"
                    "1898bd2ef0835820efaf768237c22e140a862d5d375ad5c153479fac3f8bcf8b580a1651fd62c3"
                    "ef5820000000000000000000000000f13c666056048634109c1ecca6893da293c70da458200000"
                    "000000000000000000000214281cf15c1a66b51990e2e65e1f7b7c363318f6")};
                w(key, value);
                co_return;
            }
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("c68341b57401c0")}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kEthTx, _, _, _)).WillOnce(Invoke(
            [](Unused, Unused, Unused, Walker w) -> asio::awaitable<void> {
                silkworm::Bytes key{};
                silkworm::Bytes value{*silkworm::from_hex(
                    "f9018e83065a0c85012a05f200830f4240943dd81545f3149538edcb6691a4ffee1898bd2ef080b90124cf10"
                    "c9690000000000000000000000000214281cf15c1a66b51990e2e65e1f7b7c36331800000000000000000000"
                    "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
                    "00000000000000989680000000000000000000000000ac399a5dfb9848d9e83d92d5f7dda9ba1a0013200000"
                    "0000000000000000000000000000000000000000000000000000000000a00000000000000000000000000000"
                    "00000000000000000000000000000000004182f27f9a01e210e2f3214b036e30229b2ac43e1cf2325bf270ee"
                    "a067e4f8a58a02154776f0dae16f76d1bfc82b9a9d2022039cfb09598954d05b46fc793e731a1c0000000000"
                    "00000000000000000000000000000000000000000000000000001ca0a54794fbc1edb3a2a0d3109091984eeb"
                    "5985b058220fee572147dd99e66b9f34a07dcddb68e3665b6693141c8bd60a12727d29012b7cd6ea452d418c"
                    "43e84d67dc")};
                w(key, value);
                co_return;
            }
        ));
        EXPECT_CALL(db_reader, get(db::table::kSenders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("0828D0386C1122E565f07DD28c7d1340eD5B3315")};
            }
        ));
        auto result = asio::co_spawn(pool, read_receipts(db_reader, block_hash, block_number), asio::use_future);
        //CHECK(result.get() == Receipts{...}); // TODO(canepat): provide operator== and operator!= for Receipt type
        CHECK(result.get().size() == 1);
    }

    SECTION("one contract creation receipt") {
        // TODO(canepat): at least 1 contract creation receipt
    }

    SECTION("many receipts") { // https://goerli.etherscan.io/block/469011
        const auto block_hash{0x608e7102f689c99c027c9f49860212348000eb2e13bff37aa4453605a0a2b9e7_bytes32};
        const uint64_t block_number{469011};
        EXPECT_CALL(db_reader, get(db::table::kBlockReceipts, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("828400f6011a00016e5b8400f6011a0002dc76")}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kLogs, _, _, _)).WillOnce(DoAll(
            Invoke([](Unused, Unused, Unused, Walker w) -> asio::awaitable<void> {
                silkworm::Bytes key{*silkworm::from_hex("000000000007281300000000")};
                silkworm::Bytes value{*silkworm::from_hex(
                    "8183547ef66b77759e12caf3ddb3e4aff524e577c59d8d8358208a22ee899102a366ac8ad0495127319cb1ff2403cfae855f83a89cda126667"
                    "4d5820000000000000000000000000000000000000000000000000000000000000002a58200000000000000000000000000000000000000000"
                    "000000000000000000a4e093582062c0d3e9488c19c1600c863d0ae91981e20ccdf4679813b521851735b306309b")};
                w(key, value);
                co_return;
            }),
            Invoke([](Unused, Unused, Unused, Walker w) -> asio::awaitable<void> {
                silkworm::Bytes key{*silkworm::from_hex("000000000007281300000001")};
                silkworm::Bytes value{*silkworm::from_hex(
                    "8183547ef66b77759e12caf3ddb3e4aff524e577c59d8d8358208a22ee899102a366ac8ad0495127319cb1ff2403cfae855f83a89cda126667"
                    "4d5820000000000000000000000000000000000000000000000000000000000000000458200000000000000000000000000000000000000000"
                    "0000000000000000004100fa58203ce6ba2fb2eb7fa648ad0970b9f8eecfd4c511bf7499c971c10743c555ed2496")};
                w(key, value);
                co_return;
            })
        ));
        EXPECT_CALL(db_reader, get(db::table::kBlockBodies, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex("c68301875702c0")}; }
        ));
        EXPECT_CALL(db_reader, walk(db::table::kEthTx, _, _, _)).WillOnce(Invoke(
            [](Unused, Unused, Unused, Walker w) -> asio::awaitable<void> {
                silkworm::Bytes key1{*silkworm::from_hex("0000000000018757")};
                silkworm::Bytes value1{*silkworm::from_hex(
                    "f8cb823392843b9aca008303d090947ef66b77759e12caf3ddb3e4aff524e577c59d8d80b864e9c6c176000000000000000000000000000000"
                    "000000000000000000000000000000002a0000000000000000000000000000000000000000000000000000000000a4e09362c0d3e9488c19c1"
                    "600c863d0ae91981e20ccdf4679813b521851735b306309b1ba03aaa1d392769f655b7a751d60239ef9a52a70772eb8135e94abc9bc06ea283"
                    "23a067d93fbedbb12048fc8d70c5b99dddaaf04a109894671a57f1285f48a9e3b3e9")};
                w(key1, value1);
                silkworm::Bytes key2{*silkworm::from_hex("0000000000018758")};
                silkworm::Bytes value2{*silkworm::from_hex(
                    "f8cb823393843b9aca008303d090947ef66b77759e12caf3ddb3e4aff524e577c59d8d80b864e9c6c176000000000000000000000000000000"
                    "000000000000000000000000000000000400000000000000000000000000000000000000000000000000000000004100fa3ce6ba2fb2eb7fa6"
                    "48ad0970b9f8eecfd4c511bf7499c971c10743c555ed24961ba0752f02b1438be7f67ebf0e71310db3514b162fb169cdb95ad15dde38eff771"
                    "9ba01033638bf86024fe2750ace6f79ea444703f6920979ad1fd495f9167d197a436")};
                w(key2, value2);
                co_return;
            }
        ));
        EXPECT_CALL(db_reader, get(db::table::kSenders, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, *silkworm::from_hex(
                    "79047abf3af2a1061b108d71d6dc7bdb0647479079047abf3af2a1061b108d71d6dc7bdb06474790")
                };
            }
        ));
        auto result = asio::co_spawn(pool, read_receipts(db_reader, block_hash, block_number), asio::use_future);
        //CHECK(result.get() == Receipts{Receipt{...}, Receipt{...}}); // TODO(canepat): provide operator== and operator!= for Receipt type
        CHECK(result.get().size() == Receipts{Receipt{}, Receipt{}}.size());
    }
}

} // namespace silkrpc::core::rawdb
