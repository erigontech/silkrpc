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

#include "storage_walker.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <string>

#include <asio/co_spawn.hpp>
#include <asio/thread_pool.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch.hpp>

#include <silkworm/common/util.hpp>
#include <silkworm/node/silkworm/common/log.hpp>
#include <silkrpc/ethdb/database.hpp>
#include <silkrpc/ethdb/cursor.hpp>
#include <silkrpc/ethdb/transaction.hpp>
#include <silkworm/rlp/encode.hpp>

namespace silkrpc {

using evmc::literals::operator""_address;
using evmc::literals::operator""_bytes32;

const nlohmann::json empty;
const std::string zeros = "00000000000000000000000000000000000000000000000000000000000000000000000000000000"; // NOLINT
const evmc::bytes32 zero_hash = 0x0000000000000000000000000000000000000000000000000000000000000000_bytes32;

class DummyCursor : public silkrpc::ethdb::CursorDupSort {
public:
    explicit DummyCursor(const nlohmann::json& json) : json_{json} {}

    uint32_t cursor_id() const override {
        return 0;
    }

    asio::awaitable<void> open_cursor(const std::string& table_name) override {
        table_name_ = table_name;
        table_ = json_.value(table_name_, empty);
        itr_ = table_.end();

        co_return;
    }

    asio::awaitable<void> close_cursor() override {
        table_name_ = "";
        co_return;
    }

    asio::awaitable<KeyValue> seek(const silkworm::ByteView& key) override {
        const auto key_ = silkworm::to_hex(key);

        KeyValue out;
        for (itr_ = table_.begin(); itr_ != table_.end(); itr_++) {
            auto actual = key_;
            auto delta = itr_.key().size() - actual.size();
            if (delta > 0) {
                actual += zeros.substr(0, delta);
            }
            if (itr_.key() >= actual) {
                auto kk{*silkworm::from_hex(itr_.key())};
                auto value{*silkworm::from_hex(itr_.value().get<std::string>())};
                out = KeyValue{kk, value};
                break;
            }
        }

        co_return out;
    }

    asio::awaitable<KeyValue> seek_exact(const silkworm::ByteView& key) override {
        const nlohmann::json table = json_.value(table_name_, empty);
        const auto& entry = table.value(silkworm::to_hex(key), "");
        auto value{*silkworm::from_hex(entry)};

        auto kv = KeyValue{silkworm::Bytes{key}, value};

        co_return kv;
    }

    asio::awaitable<KeyValue> next() override {
        KeyValue out;

        if (++itr_ != table_.end()) {
            auto key{*silkworm::from_hex(itr_.key())};
            auto value{*silkworm::from_hex(itr_.value().get<std::string>())};
            out = KeyValue{key, value};
        }

        co_return out;
    }

    asio::awaitable<silkworm::Bytes> seek_both(const silkworm::ByteView& key, const silkworm::ByteView& value) override {
        silkworm::Bytes key_{key};
        key_ += value;

        const nlohmann::json table = json_.value(table_name_, empty);
        const auto& entry = table.value(silkworm::to_hex(key_), "");
        auto out{*silkworm::from_hex(entry)};

        co_return out;
    }

    asio::awaitable<KeyValue> seek_both_exact(const silkworm::ByteView& key, const silkworm::ByteView& value) override {
        silkworm::Bytes key_{key};
        key_ += value;

        const nlohmann::json table = json_.value(table_name_, empty);
        const auto& entry = table.value(silkworm::to_hex(key_), "");
        auto out{*silkworm::from_hex(entry)};
        auto kv = KeyValue{silkworm::Bytes{}, out};

        co_return kv;
    }

private:
    std::string table_name_;
    const nlohmann::json& json_;
    nlohmann::json table_;
    nlohmann::json::iterator itr_;
};

class DummyTransaction: public silkrpc::ethdb::Transaction {
public:
    explicit DummyTransaction(const nlohmann::json& json) : json_{json} {}

    uint64_t tx_id() const override { return 0; }

    asio::awaitable<void> open() override {
        co_return;
    }

    asio::awaitable<std::shared_ptr<silkrpc::ethdb::Cursor>> cursor(const std::string& table) override {
        auto cursor = std::make_unique<DummyCursor>(json_);
        co_await cursor->open_cursor(table);

        co_return cursor;
    }

    asio::awaitable<std::shared_ptr<silkrpc::ethdb::CursorDupSort>> cursor_dup_sort(const std::string& table) override {
        auto cursor = std::make_unique<DummyCursor>(json_);
        co_await cursor->open_cursor(table);

        co_return cursor;
    }

    asio::awaitable<void> close() override {
        co_return;
    }

private:
    const nlohmann::json& json_;
};

class DummyDatabase: public silkrpc::ethdb::Database {
public:
    explicit DummyDatabase(const nlohmann::json& json) : json_{json} {}

    asio::awaitable<std::unique_ptr<silkrpc::ethdb::Transaction>> begin() override {
        auto txn = std::make_unique<DummyTransaction>(json_);
        co_return txn;
    }

private:
    const nlohmann::json& json_;
};

TEST_CASE("Storage walker") {
    asio::thread_pool pool{1};
    nlohmann::json json;

    json["PlainState"] = {
        {"79a4d418f7887dd4d5123a41b6c8c186686ae8cb", "030207fc08107ee3bbb7bf3a70"},
        {"79a4d492a05cfd836ea0967edb5943161dd041f7", "0d0101010120d6ea9698de278dad2f31566cd744dd75c4e09925b4bb8f041d265012a940797c"},
        {"79a4d492a05cfd836ea0967edb5943161dd041f700000000000000010000000000000000000000000000000000000000000000000000000000000001", "2ac3c1d3e24b45c6c310534bc2dd84b5ed576335"},
        {"79a4d492a05cfd836ea0967edb5943161dd041f700000000000000010000000000000000000000000000000000000000000000000000000000000006", "335a9b3f79dcfefda3295be6f7c7c47f077dbcd9"},
        {"79a4d706e4bc7fd8ff9d0593a1311386a7a981ea", "0d0101010120925fa7384049febb1eddca32821f1f1d709687628c1cf77ef40ca5013d04bdef"},
        {"79a4d706e4bc7fd8ff9d0593a1311386a7a981ea00000000000000010000000000000000000000000000000000000000000000000000000000000001", "2ac3c1d3e24b45c6c310534bc2dd84b5ed576335"},
        {"79a4d706e4bc7fd8ff9d0593a1311386a7a981ea00000000000000010000000000000000000000000000000000000000000000000000000000000003", "1f6ea08600"},
        {"79a4d706e4bc7fd8ff9d0593a1311386a7a981ea00000000000000010000000000000000000000000000000000000000000000000000000000000006", "9d5a08e7551951a3ca73cd84a6409ef1e77f5abe"},
        {"79a4d706e4bc7fd8ff9d0593a1311386a7a981ea00000000000000010178b166a1bcfd299a6ce6918f016c8d0c52788988d89f65f5727c2fa97be6e9", "1e80355e00"},
        {"79a4d706e4bc7fd8ff9d0593a1311386a7a981ea0000000000000001b797965b738ad51ddbf643b315d0421c26972862ca2e64304783dc8930a2b6e8", "ee6b2800"},
        {"79a4d75bd00b1843ec5292217e71dace5e5a7439", "03010107181855facbc200"}
    };
    json["StorageHistory"] = {
        {"79a4d492a05cfd836ea0967edb5943161dd041f70000000000000000000000000000000000000000000000000000000000000001ffffffffffffffff", "0100000000000000000000003a300000010000004b00000010000000019b"}, // NOLINT
        {"79a4d492a05cfd836ea0967edb5943161dd041f70000000000000000000000000000000000000000000000000000000000000006ffffffffffffffff", "0100000000000000000000003a300000010000004b00000010000000019b"}, // NOLINT
        {"79a4d706e4bc7fd8ff9d0593a1311386a7a981ea0000000000000000000000000000000000000000000000000000000000000001ffffffffffffffff", "0100000000000000000000003a300000010000004800000010000000b9e0"}, // NOLINT
        {"79a4d706e4bc7fd8ff9d0593a1311386a7a981ea0000000000000000000000000000000000000000000000000000000000000003ffffffffffffffff", "0100000000000000000000003a300000010000004b00010010000000d505c5c5"}, // NOLINT
        {"79a4d706e4bc7fd8ff9d0593a1311386a7a981ea0000000000000000000000000000000000000000000000000000000000000006ffffffffffffffff", "0100000000000000000000003a300000010000004800000010000000b9e0"}, // NOLINT
        {"79a4d706e4bc7fd8ff9d0593a1311386a7a981ea0178b166a1bcfd299a6ce6918f016c8d0c52788988d89f65f5727c2fa97be6e9ffffffffffffffff", "0100000000000000000000003a300000010000004b00000010000000c5c5"}, // NOLINT
        {"79a4d706e4bc7fd8ff9d0593a1311386a7a981eab797965b738ad51ddbf643b315d0421c26972862ca2e64304783dc8930a2b6e8ffffffffffffffff", "0100000000000000000000003a300000010000004b00000010000000d505"}, // NOLINT
        {"79a4e7d68b82799b9d52609756b86bd18193f2b20000000000000000000000000000000000000000000000000000000000000000ffffffffffffffff", "0100000000000000000000003a300000010000004d0000001000000052ca"} // NOLINT
    };

    auto database = DummyDatabase{json};
    auto result = asio::co_spawn(pool, database.begin(), asio::use_future);
    auto tx = result.get();
    StorageWalker walker{*tx};

    const uint64_t block_number{0x52a0b3};
    const evmc::bytes32 start_location{};

    std::map<std::string, std::string> storage;
    StorageWalker::Collector collector = [&](const evmc::address& address, const silkworm::Bytes& loc, const silkworm::Bytes& data) {
        storage["0x" + silkworm::to_hex(loc)] = silkworm::to_hex(data);
        return true;
    };

    SECTION("collect storage 1") {
        const evmc::address start_address{0x79a4d418f7887dd4d5123a41b6c8c186686ae8cb_address};
        const uint64_t incarnation{0};

        auto result = asio::co_spawn(pool, walker.walk_of_storages(block_number, start_address, start_location, incarnation, collector), asio::use_future);
        result.get();

        CHECK(storage.size() == 0);
    }

    SECTION("collect storage 2") {
        const evmc::address start_address{0x79a4d492a05cfd836ea0967edb5943161dd041f7_address};
        const uint64_t incarnation{1};

        auto result = asio::co_spawn(pool, walker.walk_of_storages(block_number, start_address, start_location, incarnation, collector), asio::use_future);
        result.get();

        CHECK(storage.size() == 2);
        CHECK(storage["0x0000000000000000000000000000000000000000000000000000000000000001"] == "2ac3c1d3e24b45c6c310534bc2dd84b5ed576335");
        CHECK(storage["0x0000000000000000000000000000000000000000000000000000000000000006"] == "335a9b3f79dcfefda3295be6f7c7c47f077dbcd9");
    }

    SECTION("collect storage 3") {
        const evmc::address start_address{0x79a4d706e4bc7fd8ff9d0593a1311386a7a981ea_address};
        const uint64_t incarnation{1};

        auto result = asio::co_spawn(pool, walker.walk_of_storages(block_number, start_address, start_location, incarnation, collector), asio::use_future);
        result.get();

        CHECK(storage.size() == 5);
        CHECK(storage["0x0000000000000000000000000000000000000000000000000000000000000001"] == "2ac3c1d3e24b45c6c310534bc2dd84b5ed576335");
        CHECK(storage["0x0000000000000000000000000000000000000000000000000000000000000003"] == "1f6ea08600");
        CHECK(storage["0x0000000000000000000000000000000000000000000000000000000000000006"] == "9d5a08e7551951a3ca73cd84a6409ef1e77f5abe");
        CHECK(storage["0x0178b166a1bcfd299a6ce6918f016c8d0c52788988d89f65f5727c2fa97be6e9"] == "1e80355e00");
        CHECK(storage["0xb797965b738ad51ddbf643b315d0421c26972862ca2e64304783dc8930a2b6e8"] == "ee6b2800");
    }
}

TEST_CASE("make key for address and location") {
    evmc::address address = 0x79a4d418f7887dd4d5123a41b6c8c186686ae8cb_address;
    evmc::bytes32 location = 0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421_bytes32;

    auto key = make_key(address, location);
    CHECK(silkworm::to_hex(key) == "79a4d418f7887dd4d5123a41b6c8c186686ae8cb56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
}

TEST_CASE("make key for address, incarnation and location") {
    evmc::address address = 0x79a4d418f7887dd4d5123a41b6c8c186686ae8cb_address;
    evmc::bytes32 location = 0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421_bytes32;
    uint64_t incarnation = 1;

    auto key = make_key(address, incarnation, location);
    CHECK(silkworm::to_hex(key) == "79a4d418f7887dd4d5123a41b6c8c186686ae8cb000000000000000156e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
}

}  // namespace silkrpc
