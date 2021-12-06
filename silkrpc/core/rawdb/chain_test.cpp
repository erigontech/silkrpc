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
#include <nlohmann/json.hpp>
#include <silkworm/common/util.hpp>

#include <silkrpc/ethdb/tables.hpp>

namespace silkrpc::core::rawdb {

using Catch::Matchers::Message;
using evmc::literals::operator""_address, evmc::literals::operator""_bytes32;

static silkworm::Bytes kKey{*silkworm::from_hex("00")};
static silkworm::Bytes kValue{*silkworm::from_hex("00")};

static silkworm::Bytes kHash{*silkworm::from_hex("439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff")};
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

class DummyDatabaseReader : public DatabaseReader {
public:
    DummyDatabaseReader() = default;
    DummyDatabaseReader(const DummyDatabaseReader&) = delete;
    DummyDatabaseReader& operator=(const DummyDatabaseReader&) = delete;

    asio::awaitable<KeyValue> get(const std::string& table, const silkworm::ByteView& key) const override {
        silkworm::Bytes bk{key};
        silkworm::Bytes bv = co_await get_one(table, key);
        co_return KeyValue{bk, bv};
    }

    asio::awaitable<silkworm::Bytes> get_one(const std::string& table, const silkworm::ByteView& key) const override {
        if (table == silkrpc::db::table::kHeaders) {
            co_return kHeader;
        } else if (table == silkrpc::db::table::kBlockBodies) {
            co_return kBody;
        } else if (table == silkrpc::db::table::kCanonicalHashes) {
            co_return kHash;
        }
        co_return kValue;
    }

    asio::awaitable<std::optional<silkworm::Bytes>> get_both_range(const std::string& table, const silkworm::ByteView& key, const silkworm::ByteView& subkey) const override {
        std::optional<silkworm::Bytes> opt = kValue;
        co_return opt;
    }

    asio::awaitable<void> walk(const std::string& table, const silkworm::ByteView& start_key, uint32_t fixed_bits, Walker w) const override {
        co_return;
    }

    asio::awaitable<void> for_prefix(const std::string& table, const silkworm::ByteView& prefix, Walker w) const override {
        co_return;
    }
};

TEST_CASE("read_block") {
    asio::thread_pool pool{1};

    DummyDatabaseReader reader;

    SECTION("read_block_by_number_or_hash by number") {
        BlockNumberOrHash bnoh{4000000};

        auto result = asio::co_spawn(pool, read_block_by_number_or_hash(reader, bnoh), asio::use_future);
        const silkworm::BlockWithHash &bwh = result.get();

        CHECK(bwh.block.header.parent_hash == silkworm::to_bytes32(*silkworm::from_hex("209f062567c161c5f71b3f57a7de277b0e95c3455050b152d785ad7524ef8ee7")));
        CHECK(bwh.block.header.ommers_hash == silkworm::to_bytes32(*silkworm::from_hex("1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347")));
        CHECK(bwh.block.header.beneficiary == silkworm::to_address(*silkworm::from_hex("0000000000000000000000000000000000000000")));
        CHECK(bwh.block.header.state_root == silkworm::to_bytes32(*silkworm::from_hex("e7536c5b61ed0e0ab7f3ce7f085806d40f716689c0c086676757de401b595658")));
        CHECK(bwh.block.header.transactions_root == silkworm::to_bytes32(*silkworm::from_hex("40be247314d834a319556d1dcf458e8707cc1aa4a416b6118474ce0c96fccb1a")));
        CHECK(bwh.block.header.receipts_root == silkworm::to_bytes32(*silkworm::from_hex("7862fe11d10a9b237ffe9cb660f31e4bc4be66836c9bfc17310d47c60d75671f")));
        CHECK(bwh.block.header.number == 4000000);
        CHECK(bwh.block.header.gas_limit == 8000000);
        CHECK(bwh.block.header.gas_used == 1996875);
        CHECK(bwh.block.header.timestamp == 1609072811);
        CHECK(bwh.block.header.extra_data == *silkworm::from_hex("d88301091a846765746888676f312e31352e36856c696e757800000000000000be009d0049d6f0ee8ca6764a1d3e"
            "b519bd4d046e167ddcab467d5db31d063f2d58f266fa86c4502aa169d17762090e92b821843de69b41adbb5d86f5d114ba7f01"));
        CHECK(bwh.block.header.mix_hash == silkworm::to_bytes32(*silkworm::from_hex("0000000000000000000000000000000000000000000000000000000000000000")));
        CHECK(bwh.hash == silkworm::to_bytes32(*silkworm::from_hex("439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff")));
    }

    SECTION("read_block_by_number") {
        uint64_t bnoh{4000000};

        auto result = asio::co_spawn(pool, read_block_by_number(reader, bnoh), asio::use_future);
        const silkworm::BlockWithHash &bwh = result.get();

        CHECK(bwh.block.header.parent_hash == silkworm::to_bytes32(*silkworm::from_hex("209f062567c161c5f71b3f57a7de277b0e95c3455050b152d785ad7524ef8ee7")));
        CHECK(bwh.block.header.ommers_hash == silkworm::to_bytes32(*silkworm::from_hex("1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347")));
        CHECK(bwh.block.header.beneficiary == silkworm::to_address(*silkworm::from_hex("0000000000000000000000000000000000000000")));
        CHECK(bwh.block.header.state_root == silkworm::to_bytes32(*silkworm::from_hex("e7536c5b61ed0e0ab7f3ce7f085806d40f716689c0c086676757de401b595658")));
        CHECK(bwh.block.header.transactions_root == silkworm::to_bytes32(*silkworm::from_hex("40be247314d834a319556d1dcf458e8707cc1aa4a416b6118474ce0c96fccb1a")));
        CHECK(bwh.block.header.receipts_root == silkworm::to_bytes32(*silkworm::from_hex("7862fe11d10a9b237ffe9cb660f31e4bc4be66836c9bfc17310d47c60d75671f")));
        CHECK(bwh.block.header.number == 4000000);
        CHECK(bwh.block.header.gas_limit == 8000000);
        CHECK(bwh.block.header.gas_used == 1996875);
        CHECK(bwh.block.header.timestamp == 1609072811);
        CHECK(bwh.block.header.extra_data == *silkworm::from_hex("d88301091a846765746888676f312e31352e36856c696e757800000000000000be009d0049d6f0ee8ca6764a1d3e"
            "b519bd4d046e167ddcab467d5db31d063f2d58f266fa86c4502aa169d17762090e92b821843de69b41adbb5d86f5d114ba7f01"));
        CHECK(bwh.block.header.mix_hash == silkworm::to_bytes32(*silkworm::from_hex("0000000000000000000000000000000000000000000000000000000000000000")));
        CHECK(bwh.hash == silkworm::to_bytes32(*silkworm::from_hex("439816753229fc0736bf86a5048de4bc9fcdede8c91dadf88c828c76b2281dff")));
    }
}

} // namespace silkrpc::core::rawdb
