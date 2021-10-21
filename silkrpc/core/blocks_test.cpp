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

#include <memory>

#include <asio/co_spawn.hpp>
#include <asio/thread_pool.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

#include <silkworm/common/util.hpp>
#include <silkworm/rlp/encode.hpp>
#include <silkrpc/ethdb/database.hpp>
#include <silkrpc/ethdb/cursor.hpp>
#include <silkrpc/ethdb/transaction.hpp>
#include <silkrpc/ethdb/transaction_database.hpp>
#include <silkrpc/stagedsync/stages.hpp>

namespace silkrpc::core {

using Catch::Matchers::Message;

static const nlohmann::json empty;
static const std::string zeros = "00000000000000000000000000000000000000000000000000000000000000000000000000000000"; // NOLINT

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

TEST_CASE("get_block_number") {
    asio::thread_pool pool{1};
    nlohmann::json json;

    json["SyncStage"] = {
        {silkworm::to_hex(silkrpc::stages::kExecution), "1234567890123456"}
    };

    auto database = DummyDatabase{json};
    auto begin_result = asio::co_spawn(pool, database.begin(), asio::use_future);
    auto tx = begin_result.get();
    ethdb::TransactionDatabase tx_database{*tx};

    SECTION("kEarliestBlockId") {
        auto result = asio::co_spawn(pool, get_block_number(kEarliestBlockId, tx_database), asio::use_future);
        auto block_number = result.get();

        CHECK(block_number == 0);
    }

    SECTION("kLatestBlockId") {
        auto result = asio::co_spawn(pool, get_block_number(kLatestBlockId, tx_database), asio::use_future);
        auto block_number = result.get();

        CHECK(block_number == 0x1234567890123456);
    }

    SECTION("kPendingBlockId") {
        auto result = asio::co_spawn(pool, get_block_number(kPendingBlockId, tx_database), asio::use_future);
        auto block_number = result.get();

        CHECK(block_number == 0x1234567890123456);
    }

    SECTION("number in hex") {
        auto result = asio::co_spawn(pool, get_block_number("0x12345", tx_database), asio::use_future);
        auto block_number = result.get();

        CHECK(block_number == 0x12345);
    }

    SECTION("number in dec") {
        auto result = asio::co_spawn(pool, get_block_number("67890", tx_database), asio::use_future);
        auto block_number = result.get();

        CHECK(block_number == 67890);
    }
}

} // namespace silkrpc::core

