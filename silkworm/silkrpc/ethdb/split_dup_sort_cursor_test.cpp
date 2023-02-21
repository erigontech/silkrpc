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

#include "cursor.hpp"

#include <iostream>
#include <string>
#include <vector>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_future.hpp>
#include <gmock/gmock.h>

#include <silkworm/silkrpc/test/mock_cursor.hpp>

#include <catch2/catch.hpp>

namespace silkrpc::ethdb {

using Catch::Matchers::Message;
using evmc::literals::operator""_bytes32;
using evmc::literals::operator""_address;
using testing::_;
using testing::InvokeWithoutArgs;

static const silkworm::Bytes value{*silkworm::from_hex("0x000000000000000000000000000000000000000000000000000000000000001134567")};
static const silkworm::Bytes short_key{*silkworm::from_hex("0x00")};
static const silkworm::Bytes empty_key{};
static const silkworm::Bytes empty_value{};
static const silkworm::Bytes wrong_key{*silkworm::from_hex("0x79a4d35bd00b1843ec5292217e71dace5e5a7430")};

TEST_CASE("split dup sort cursor") {
    boost::asio::thread_pool pool{1};
    test::MockCursor  csdp;

    SECTION("0 maching bits: seek_both, key not exists") {
        const evmc::address key = 0x79a4d35bd00b1843ec5292217e71dace5e5a7439_address;
        const evmc::bytes32 location = 0x0000000000000000000000000000000000000000000000000000000000000001_bytes32;

        SplitDupSortCursor sc(csdp, key, location, 0, silkworm::kAddressLength, silkworm::kAddressLength, 0);

        EXPECT_CALL(csdp, seek_both(_, _))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return silkworm::Bytes{};
            }));
        auto result = boost::asio::co_spawn(pool, sc.seek_both(), boost::asio::use_future);
        const SplittedKeyValue &skv = result.get();

        CHECK(silkworm::to_hex(skv.key1) == silkworm::to_hex(key));
        CHECK(silkworm::to_hex(skv.key2) == "");
        CHECK(silkworm::to_hex(skv.key3) == "");
        CHECK(silkworm::to_hex(skv.value) == "");
    }

    SECTION("evmc:.address maching bits: seek_both, key not exists") {
        const evmc::address key = 0x79a4d35bd00b1843ec5292217e71dace5e5a7439_address;
        const evmc::bytes32 location = 0x0000000000000000000000000000000000000000000000000000000000000001_bytes32;

        SplitDupSortCursor sc(csdp, key, location, 8 * silkworm::kAddressLength, silkworm::kAddressLength, 
                              silkworm::kAddressLength,  silkworm::kHashLength);

        EXPECT_CALL(csdp, seek_both(_, _))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return silkworm::Bytes{};
            }));
        auto result = boost::asio::co_spawn(pool, sc.seek_both(), boost::asio::use_future);
        const SplittedKeyValue &skv = result.get();

        CHECK(silkworm::to_hex(skv.key1) == "");
        CHECK(silkworm::to_hex(skv.key2) == "");
        CHECK(silkworm::to_hex(skv.key3) == "");
        CHECK(silkworm::to_hex(skv.value) == "");
    }

    SECTION("evmc:.address maching bits: seek_both, key exists") {
        const evmc::address key = 0x79a4d35bd00b1843ec5292217e71dace5e5a7439_address;
        const evmc::bytes32 location = 0x0000000000000000000000000000000000000000000000000000000000000001_bytes32;

        SplitDupSortCursor sc(csdp, key, location, 8 * silkworm::kAddressLength, silkworm::kAddressLength, 
                              silkworm::kAddressLength,  silkworm::kHashLength);

        EXPECT_CALL(csdp, seek_both(_, _))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return value;
            }));
        auto result = boost::asio::co_spawn(pool, sc.seek_both(), boost::asio::use_future);
        const SplittedKeyValue &skv = result.get();

        CHECK(silkworm::to_hex(skv.key1) == silkworm::to_hex(key));
        CHECK(silkworm::to_hex(skv.key2) == silkworm::to_hex(location));
        CHECK(silkworm::to_hex(skv.key3) == "");
        CHECK(silkworm::to_hex(skv.value) == "134567");
    }

    SECTION("evmc:.address maching bits: next_dup, key exists short key") {
        const evmc::address key = 0x79a4d35bd00b1843ec5292217e71dace5e5a7439_address;
        const evmc::bytes32 location = 0x0000000000000000000000000000000000000000000000000000000000000001_bytes32;

        SplitDupSortCursor sc(csdp, key, location, 8 * silkworm::kAddressLength, silkworm::kAddressLength, 
                              silkworm::kAddressLength,  silkworm::kHashLength);

        EXPECT_CALL(csdp, next_dup())
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{short_key, value};
            }));
        auto result = boost::asio::co_spawn(pool, sc.next_dup(), boost::asio::use_future);
        const SplittedKeyValue &skv = result.get();

        CHECK(silkworm::to_hex(skv.key1) == "");
        CHECK(silkworm::to_hex(skv.key2) == "");
        CHECK(silkworm::to_hex(skv.key3) == "");
        CHECK(silkworm::to_hex(skv.value) == "");
    }

    SECTION("evmc:.address maching bits: next_dup, key exists empty key") {
        const evmc::address key = 0x79a4d35bd00b1843ec5292217e71dace5e5a7439_address;
        const evmc::bytes32 location = 0x0000000000000000000000000000000000000000000000000000000000000001_bytes32;

        SplitDupSortCursor sc(csdp, key, location, 8 * silkworm::kAddressLength, silkworm::kAddressLength, 
                              silkworm::kAddressLength,  silkworm::kHashLength);

        EXPECT_CALL(csdp, next_dup())
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{empty_key, value};
            }));
        auto result = boost::asio::co_spawn(pool, sc.next_dup(), boost::asio::use_future);
        const SplittedKeyValue &skv = result.get();

        CHECK(silkworm::to_hex(skv.key1) == "");
        CHECK(silkworm::to_hex(skv.key2) == "");
        CHECK(silkworm::to_hex(skv.key3) == "");
        CHECK(silkworm::to_hex(skv.value) == "");
    }

    SECTION("evmc:.address maching bits: next_dup, key exists wrong key") {
        const evmc::address key = 0x79a4d35bd00b1843ec5292217e71dace5e5a7439_address;
        const evmc::bytes32 location = 0x0000000000000000000000000000000000000000000000000000000000000001_bytes32;

        SplitDupSortCursor sc(csdp, key, location, 8 * silkworm::kAddressLength, silkworm::kAddressLength, 
                              silkworm::kAddressLength,  silkworm::kHashLength);

        EXPECT_CALL(csdp, next_dup())
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{wrong_key, value};
            }));
        auto result = boost::asio::co_spawn(pool, sc.next_dup(), boost::asio::use_future);
        const SplittedKeyValue &skv = result.get();

        CHECK(silkworm::to_hex(skv.key1) == "");
        CHECK(silkworm::to_hex(skv.key2) == "");
        CHECK(silkworm::to_hex(skv.key3) == "");
        CHECK(silkworm::to_hex(skv.value) == "");
    }
}

} // namespace silkrpc::ethdb
