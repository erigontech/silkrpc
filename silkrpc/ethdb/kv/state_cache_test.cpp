/*
   Copyright 2022 The Silkrpc Authors

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

#include "state_cache.hpp"

#include <memory>
#include <string>
#include <vector>

#include <asio/co_spawn.hpp>
#include <asio/thread_pool.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch.hpp>
#include <evmc/evmc.hpp>
#include <gmock/gmock.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/core/rawdb/util.hpp>
#include <silkrpc/test/dummy_transaction.hpp>
#include <silkrpc/test/mock_transaction.hpp>
#include <silkworm/common/base.hpp>
#include <silkworm/rpc/conversion.hpp>

namespace silkrpc::ethdb::kv {

using namespace evmc::literals;  // NOLINT(build/namespaces_literals)

using Catch::Matchers::Message;
using testing::_;
using testing::InvokeWithoutArgs;
using testing::Return;

static constexpr auto kTestBlockNumber{1'000'000};
static constexpr auto kTestBlockHash{0x8e38b4dbf6b11fcc3b9dee84fb7986e29ca0a02cecd8977c161ff7333329681e_bytes32};

static constexpr auto kTestAddress{0x0a6bb546b9208cfab9e8fa2b9b2c042b18df7030_address};
static constexpr uint64_t kTestIncarnation{3};

static const silkworm::Bytes kTestData1{*silkworm::from_hex("600035600055")};
static const silkworm::Bytes kTestData2{*silkworm::from_hex("6000356000550055")};

static const silkworm::Bytes kTestCode1{*silkworm::from_hex("602a6000556101c960015560068060166000396000f3600035600055")};
static const silkworm::Bytes kTestCode2{*silkworm::from_hex("602a5f556101c960015560048060135f395ff35f355f55")};

static const auto kTestHashedLocation1{0x6677907ab33937e392b9be983b30818f29d594039c9e1e7490bf7b3698888fb1_bytes32};
static const auto kTestHashedLocation2{0xe046602dcccb1a2f1d176718c8e709a42bba57af2da2379ba7130e2f916c95cd_bytes32};

class MockCursor : public Cursor {
public:
    MOCK_CONST_METHOD0(cursor_id, uint32_t());
    MOCK_METHOD1(open_cursor, asio::awaitable<void>(const std::string&));
    MOCK_METHOD1(seek, asio::awaitable<KeyValue>(silkworm::ByteView));
    MOCK_METHOD1(seek_exact, asio::awaitable<KeyValue>(silkworm::ByteView));
    MOCK_METHOD0(next, asio::awaitable<KeyValue>());
    MOCK_METHOD0(close_cursor, asio::awaitable<void>());
};

TEST_CASE("CoherentStateRoot", "[silkrpc][ethdb][kv][state_cache]") {
    SECTION("CoherentStateRoot::CoherentStateRoot") {
        CoherentStateRoot root;
        CHECK(root.cache.empty());
        CHECK(root.code_cache.empty());
        CHECK(!root.ready);
        CHECK(!root.canonical);
    }
}

TEST_CASE("CoherentCacheConfig", "[silkrpc][ethdb][kv][state_cache]") {
    SECTION("CoherentCacheConfig::CoherentCacheConfig") {
        CoherentCacheConfig config;
        CHECK(config.max_views == kDefaultMaxViews);
        CHECK(config.with_storage);
        CHECK(config.max_state_keys == kDefaultMaxStateKeys);
        CHECK(config.max_code_keys == kDefaultMaxCodeKeys);
    }
}

remote::StateChangeBatch new_batch(silkworm::BlockNum block_height, const evmc::bytes32& block_hash,
                                   const std::vector<silkworm::Bytes>&& tx_rlps, bool unwind) {
    remote::StateChangeBatch state_changes;

    remote::StateChange* latest_change = state_changes.add_changebatch();
    latest_change->set_blockheight(block_height);
    latest_change->set_allocated_blockhash(silkworm::rpc::H256_from_bytes32(block_hash).release());
    latest_change->set_direction(unwind ? remote::Direction::UNWIND : remote::Direction::FORWARD);
    for (auto& tx_rlp : tx_rlps) {
        latest_change->add_txs(silkworm::to_hex(tx_rlp));
    }

    return state_changes;
}

TEST_CASE("CoherentStateCache", "[silkrpc][ethdb][kv][state_cache]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::Debug);
    asio::thread_pool pool{1};

    SECTION("CoherentStateCache::CoherentStateCache default config") {
        CoherentStateCache cache;
        CHECK(cache.size() == 0);
        CHECK(cache.state_hit_count() == 0);
        CHECK(cache.state_miss_count() == 0);
        CHECK(cache.state_key_count() == 0);
        CHECK(cache.state_eviction_count() == 0);
    }

    SECTION("CoherentStateCache::CoherentStateCache wrong config") {
        CoherentCacheConfig config{0, true, kDefaultMaxStateKeys, kDefaultMaxCodeKeys};
        CHECK_THROWS_AS(CoherentStateCache{config}, std::invalid_argument);
    }

    SECTION("CoherentStateCache::get_view: empty cache => view invalid") {
        CoherentStateCache cache;
        test::MockTransaction txn;
        EXPECT_CALL(txn, tx_id());
        std::unique_ptr<StateView> view = cache.get_view(txn);
        CHECK(view == nullptr);
        CHECK(cache.state_hit_count() == 0);
        CHECK(cache.state_miss_count() == 0);
        CHECK(cache.state_key_count() == 0);
        CHECK(cache.state_eviction_count() == 0);
    }

    SECTION("CoherentStateCache::get_view: empty batch => view invalid") {
        CoherentStateCache cache;
        cache.on_new_block(remote::StateChangeBatch{});
        CHECK(cache.size() == 0);
        test::MockTransaction txn;
        EXPECT_CALL(txn, tx_id());
        std::unique_ptr<StateView> view = cache.get_view(txn);
        CHECK(view == nullptr);
        CHECK(cache.state_hit_count() == 0);
        CHECK(cache.state_miss_count() == 0);
        CHECK(cache.state_key_count() == 0);
        CHECK(cache.state_eviction_count() == 0);
    }

    SECTION("CoherentStateCache::get_view: single change batch => view valid, search hit") {
        CoherentStateCache cache;

        auto batch = new_batch(kTestBlockNumber, kTestBlockHash, std::vector<silkworm::Bytes>{}, /*unwind=*/false);
        remote::StateChange* state_change = batch.mutable_changebatch(0);
        remote::AccountChange* account_change = state_change->add_changes();
        account_change->set_allocated_address(silkworm::rpc::H160_from_address(kTestAddress).release());
        account_change->set_action(remote::Action::STORAGE);
        account_change->set_incarnation(kTestIncarnation);
        remote::StorageChange* storage_change1 = account_change->add_storagechanges();
        storage_change1->set_data(kTestData1.data(), kTestData1.size());
        storage_change1->set_allocated_location(silkworm::rpc::H256_from_bytes32(kTestHashedLocation1).release());
        cache.on_new_block(batch);
        CHECK(cache.size() == 1);

        test::MockTransaction txn;
        EXPECT_CALL(txn, tx_id()).Times(2);

        std::unique_ptr<StateView> view = cache.get_view(txn);
        CHECK(view != nullptr);
        if (view) {
            const auto storage_key1 = composite_storage_key(kTestAddress, kTestIncarnation, kTestHashedLocation1.bytes);
            auto result = asio::co_spawn(pool, view->get(storage_key1), asio::use_future);
            const auto value = result.get();
            CHECK(value.has_value());
            if (value) {
                CHECK(*value == kTestData1);
            }
            CHECK(cache.state_hit_count() == 1);
            CHECK(cache.state_miss_count() == 0);
            CHECK(cache.state_key_count() == 1);
            CHECK(cache.state_eviction_count() == 0);
        }
    }

    SECTION("CoherentStateCache::get_view: single change batch => view valid, search miss") {
        CoherentStateCache cache;

        auto batch = new_batch(kTestBlockNumber, kTestBlockHash, std::vector<silkworm::Bytes>{}, /*unwind=*/false);
        remote::StateChange* state_change = batch.mutable_changebatch(0);
        remote::AccountChange* account_change = state_change->add_changes();
        account_change->set_allocated_address(silkworm::rpc::H160_from_address(kTestAddress).release());
        account_change->set_action(remote::Action::STORAGE);
        account_change->set_incarnation(kTestIncarnation);
        remote::StorageChange* storage_change1 = account_change->add_storagechanges();
        storage_change1->set_data(kTestData1.data(), kTestData1.size());
        storage_change1->set_allocated_location(silkworm::rpc::H256_from_bytes32(kTestHashedLocation1).release());
        cache.on_new_block(batch);
        CHECK(cache.size() == 1);

        std::shared_ptr<MockCursor> mock_cursor = std::make_shared<MockCursor>();
        test::DummyTransaction txn{mock_cursor};

        std::unique_ptr<StateView> view = cache.get_view(txn);
        CHECK(view != nullptr);
        if (view) {
            EXPECT_CALL(*mock_cursor, seek_exact(_)).WillOnce(InvokeWithoutArgs([]() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, kTestData2};
            }));

            const auto storage_key2 = composite_storage_key(kTestAddress, kTestIncarnation, kTestHashedLocation2.bytes);
            auto result = asio::co_spawn(pool, view->get(storage_key2), asio::use_future);
            const auto value = result.get();
            CHECK(value.has_value());
            if (value) {
                CHECK(*value == kTestData2);
            }
            CHECK(cache.state_hit_count() == 0);
            CHECK(cache.state_miss_count() == 1);
            CHECK(cache.state_key_count() == 1);
            CHECK(cache.state_eviction_count() == 0);
        }
    }

    SECTION("CoherentStateCache::get_view: double change batch => double search hit") {
        CoherentStateCache cache;

        auto batch = new_batch(kTestBlockNumber, kTestBlockHash, std::vector<silkworm::Bytes>{}, /*unwind=*/false);
        remote::StateChange* state_change = batch.mutable_changebatch(0);
        remote::AccountChange* account_change = state_change->add_changes();
        account_change->set_allocated_address(silkworm::rpc::H160_from_address(kTestAddress).release());
        account_change->set_action(remote::Action::STORAGE);
        account_change->set_incarnation(kTestIncarnation);
        remote::StorageChange* storage_change1 = account_change->add_storagechanges();
        storage_change1->set_data(kTestData1.data(), kTestData1.size());
        storage_change1->set_allocated_location(silkworm::rpc::H256_from_bytes32(kTestHashedLocation1).release());
        remote::StorageChange* storage_change2 = account_change->add_storagechanges();
        storage_change2->set_data(kTestData2.data(), kTestData2.size());
        storage_change2->set_allocated_location(silkworm::rpc::H256_from_bytes32(kTestHashedLocation2).release());
        cache.on_new_block(batch);
        CHECK(cache.size() == 2);

        test::MockTransaction txn;
        EXPECT_CALL(txn, tx_id()).Times(3);
        std::unique_ptr<StateView> view = cache.get_view(txn);

        CHECK(view != nullptr);
        if (view) {
            const auto storage_key1 = composite_storage_key(kTestAddress, kTestIncarnation, kTestHashedLocation1.bytes);
            auto result1 = asio::co_spawn(pool, view->get(storage_key1), asio::use_future);
            const auto value1 = result1.get();
            CHECK(value1.has_value());
            if (value1) {
                CHECK(*value1 == kTestData1);
            }
            const auto storage_key2 = composite_storage_key(kTestAddress, kTestIncarnation, kTestHashedLocation2.bytes);
            auto result2 = asio::co_spawn(pool, view->get(storage_key2), asio::use_future);
            const auto value2 = result2.get();
            CHECK(value2.has_value());
            if (value2) {
                CHECK(*value2 == kTestData2);
            }
            CHECK(cache.state_hit_count() == 2);
            CHECK(cache.state_miss_count() == 0);
            CHECK(cache.state_key_count() == 2);
            CHECK(cache.state_eviction_count() == 0);
        }
    }
}

}  // namespace silkrpc::ethdb::kv
