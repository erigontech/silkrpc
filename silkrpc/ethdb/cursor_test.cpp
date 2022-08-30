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
#include <catch2/catch.hpp>

namespace silkrpc::ethdb {

using Catch::Matchers::Message;

static silkworm::Bytes value{*silkworm::from_hex("00")};

struct TriString {
    std::string part1;
    std::string part2;
    std::string part3;
};

struct TriBytes {
    silkworm::Bytes part1;
    silkworm::Bytes part2;
    silkworm::Bytes part3;
};

class ArrayCursor : public Cursor {
public:
    explicit ArrayCursor(const std::vector<TriBytes>& vector) : vector_{vector}, index_{0} {}

    ArrayCursor(const ArrayCursor&) = delete;
    ArrayCursor& operator=(const ArrayCursor&) = delete;

    uint32_t cursor_id() const override { return 0; }

    boost::asio::awaitable<void> open_cursor(const std::string& table_name) override { co_return; }

    boost::asio::awaitable<KeyValue> seek(silkworm::ByteView seek_key) override {
        index_ = 0;
        for (; index_ < vector_.size(); index_++) {
            if (vector_[index_].part1 == seek_key) {
                silkworm::Bytes full_key = vector_[index_].part1 + vector_[index_].part2 + vector_[index_].part3;
                co_return KeyValue{full_key, value};
            }
        }
        co_return KeyValue{};
    }

    boost::asio::awaitable<KeyValue> seek_exact(silkworm::ByteView key) override { co_return KeyValue{silkworm::Bytes{key}, value}; }

    boost::asio::awaitable<KeyValue> next() override {
        if (++index_ >= vector_.size()) {
            co_return KeyValue{};
        }
        silkworm::Bytes full_key = vector_[index_].part1 + vector_[index_].part2 + vector_[index_].part3;
        co_return KeyValue{full_key, value};
    }

    boost::asio::awaitable<void> close_cursor() override { co_return; }

    uint32_t index() const { return index_; }

private:
    uint32_t index_;
    const std::vector<TriBytes>& vector_;
};

static silkworm::Bytes to_bytes(const std::string& string) {
    silkworm::Bytes bytes{*silkworm::from_hex(string)};
    return bytes;
}

static TriBytes to_tri_bytes(const TriString& ts) {
    return TriBytes {
        to_bytes(ts.part1),
        to_bytes(ts.part2),
        to_bytes(ts.part3)
    };
}

TEST_CASE("split cursor") {
    boost::asio::thread_pool pool{1};

    std::vector<TriString> hex {
        {"79a4d35bd00b1843ec5292217e71dace5e5a7439", "ffffffffffffffff", "deadbeaf"}, // 0
        {"79a4d418f7887dd4d5123a41b6c8c186686ae8cb", "00000000005151a3", "deadbeaf"},
        {"79a4d418f7887dd4d5123a41b6c8c186686ae8cb", "000000000052a0b3", "deadbeaf"},
        {"79a4d418f7887dd4d5123a41b6c8c186686ae8cb", "000000000052a140", "deadbeaf"},
        {"79a4d418f7887dd4d5123a41b6c8c186686ae8cb", "ffffffffffffffff", "deadbeaf"},
        {"79a4d419a05cfd856ea78962edb543161aa05610", "00000000005151a3", "deadbeaf"}, // 5
        {"79a4d419a05cfd856ea78962edb543161aa05610", "0000000000711143", "deadbeaf"},
        {"79a4d492a05cfd836ea0967edb5943161dd041f7", "ffffffffffffffff", "deadbeaf"},
        {"79a4d706e4bc7fd8ff9d0593a1311386a7a981ea", "ffffffffffffffff", "deadbeaf"},
        {"79a4d7ba9e355258fad372164f2f5184dde5e3e4", "ffffffffffffffff", "deadbeaf"},
        {"79a4ddca4ae487beba98526c7b3cc4ba4d05d9d4", "ffffffffffffffff", "deadbeaf"} // 10
    };

    std::vector<TriBytes> tbv;
    for (TriString entry : hex) {
        auto tb = to_tri_bytes(entry);

        tbv.push_back(tb);
    }
    ArrayCursor ac{tbv};

    SECTION("0 maching bits: seek, key exixts") {
        uint32_t index = 0;
        silkworm::Bytes key = to_bytes(hex[index].part1);

        SplitCursor sc(ac, key, 0, silkworm::kAddressLength, silkworm::kAddressLength, silkworm::kAddressLength + 8);

        auto result = boost::asio::co_spawn(pool, sc.seek(), boost::asio::use_future);
        const SplittedKeyValue &skv = result.get();

        CHECK(skv.key1 == key);
        CHECK(skv.key2 == to_bytes(hex[index].part2));
        CHECK(skv.key3 == to_bytes(hex[index].part3));
    }

    SECTION("0 maching bits: seek & next, key exixts in first position") {
        uint32_t index = 0;
        silkworm::Bytes key = to_bytes(hex[index].part1);

        SplitCursor sc(ac, key, 0, silkworm::kAddressLength, silkworm::kAddressLength, silkworm::kAddressLength + 8);

        auto result = boost::asio::co_spawn(pool, sc.seek(), boost::asio::use_future);
        const SplittedKeyValue &skv = result.get();

        CHECK(skv.key1 == key);
        CHECK(skv.key2 == to_bytes(hex[index].part2));
        CHECK(skv.key3 == to_bytes(hex[index].part3));

        auto count  = 0;
        while (true) {
            auto result = boost::asio::co_spawn(pool, sc.next(), boost::asio::use_future);
            const SplittedKeyValue &skv = result.get();
            if (skv.key1.length() == 0) {
                break;
            }
            count++;

            CHECK(skv.key1 == to_bytes(hex[ac.index()].part1));
            CHECK(skv.key2 == to_bytes(hex[ac.index()].part2));
            CHECK(skv.key3 == to_bytes(hex[ac.index()].part3));
        }
        CHECK(count + 1 == hex.size());
    }

    SECTION("0 maching bits: seek & next, key exixts in 5th position") {
        uint32_t index = 5;
        silkworm::Bytes key = to_bytes(hex[index].part1);

        SplitCursor sc(ac, key, 0, silkworm::kAddressLength, silkworm::kAddressLength, silkworm::kAddressLength + 8);

        auto result = boost::asio::co_spawn(pool, sc.seek(), boost::asio::use_future);
        const SplittedKeyValue &skv = result.get();

        CHECK(skv.key1 == key);
        CHECK(skv.key2 == to_bytes(hex[index].part2));
        CHECK(skv.key3 == to_bytes(hex[index].part3));

        auto count  = 0;
        while (true) {
            auto result = boost::asio::co_spawn(pool, sc.next(), boost::asio::use_future);
            const SplittedKeyValue &skv = result.get();
            if (skv.key1.length() == 0) {
                break;
            }
            count++;

            CHECK(skv.key1 == to_bytes(hex[ac.index()].part1));
            CHECK(skv.key2 == to_bytes(hex[ac.index()].part2));
            CHECK(skv.key3 == to_bytes(hex[ac.index()].part3));
        }
        CHECK(count + 1 == hex.size() - index);
    }

    SECTION("0 maching bits: seek, key does not exixt") {
        silkworm::Bytes key = to_bytes("79a4d75bd00b1843ec5292217e71dace5e5a7438");

        SplitCursor sc(ac, key, 0, silkworm::kAddressLength, silkworm::kAddressLength, silkworm::kAddressLength + 8);

        auto result = boost::asio::co_spawn(pool, sc.seek(), boost::asio::use_future);
        const SplittedKeyValue &skv = result.get();

        CHECK(skv.key1.length() == 0);
        CHECK(skv.key2.length() == 0);
        CHECK(skv.key3.length() == 0);
    }

    SECTION("28 maching bits: seek, key exixts") {
        uint32_t index = 1;
        silkworm::Bytes key = to_bytes(hex[index].part1);

        SplitCursor sc(ac, key, 28, silkworm::kAddressLength, silkworm::kAddressLength, silkworm::kAddressLength + 8);

        auto result = boost::asio::co_spawn(pool, sc.seek(), boost::asio::use_future);
        const SplittedKeyValue &skv = result.get();

        CHECK(skv.key1 == key);
        CHECK(skv.key2 == to_bytes(hex[index].part2));
        CHECK(skv.key3 == to_bytes(hex[index].part3));
    }

    SECTION("28 maching bits: seek & next, key exixts in 5th position") {
        uint32_t index = 1;
        silkworm::Bytes key = to_bytes(hex[index].part1);

        SplitCursor sc(ac, key, 28, silkworm::kAddressLength, silkworm::kAddressLength, silkworm::kAddressLength + 8);

        auto result = boost::asio::co_spawn(pool, sc.seek(), boost::asio::use_future);
        const SplittedKeyValue &skv = result.get();

        CHECK(skv.key1 == key);
        CHECK(skv.key2 == to_bytes(hex[index].part2));
        CHECK(skv.key3 == to_bytes(hex[index].part3));

        auto count  = 0;
        while (true) {
            auto result = boost::asio::co_spawn(pool, sc.next(), boost::asio::use_future);
            const SplittedKeyValue &skv = result.get();
            if (skv.key1.length() == 0) {
                break;
            }
            count++;

            CHECK(skv.key1 == to_bytes(hex[ac.index()].part1));
            CHECK(skv.key2 == to_bytes(hex[ac.index()].part2));
            CHECK(skv.key3 == to_bytes(hex[ac.index()].part3));
        }
        CHECK(count == 5);
    }

    SECTION("0 maching bits: seek, key exixts, length too long") {
        uint32_t index = 1;
        silkworm::Bytes key = to_bytes(hex[index].part1);

        SplitCursor sc(ac, key, 28, silkworm::kAddressLength, silkworm::kAddressLength + 8, silkworm::kAddressLength + 12);

        auto result = boost::asio::co_spawn(pool, sc.seek(), boost::asio::use_future);
        const SplittedKeyValue &skv = result.get();

        CHECK(skv.key1 == key);
        CHECK(skv.key2 == to_bytes(hex[index].part3));
        CHECK(skv.key3.length() == 0);
    }
}

} // namespace silkrpc::ethdb
