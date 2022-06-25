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
#include <utility>
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
#include <silkworm/common/assert.hpp>
#include <silkworm/common/base.hpp>
#include <silkworm/common/util.hpp>
#include <silkworm/rpc/conversion.hpp>

namespace silkrpc::ethdb::kv {

using namespace evmc::literals;  // NOLINT(build/namespaces_literals)

using Catch::Matchers::Message;
using testing::_;
using testing::InvokeWithoutArgs;
using testing::Return;

static constexpr uint64_t kTestViewId0{3'000'000};
static constexpr uint64_t kTestViewId1{3'000'001};
static constexpr uint64_t kTestViewId2{3'000'002};

static constexpr auto kTestBlockNumber{1'000'000};
static constexpr auto kTestBlockHash{0x8e38b4dbf6b11fcc3b9dee84fb7986e29ca0a02cecd8977c161ff7333329681e_bytes32};

static constexpr auto kTestAddress{0x0a6bb546b9208cfab9e8fa2b9b2c042b18df7030_address};
static constexpr uint64_t kTestIncarnation{3};

static const silkworm::Bytes kTestAccountData{*silkworm::from_hex("600035600055")};

static const silkworm::Bytes kTestStorageData1{*silkworm::from_hex("600035600055")};
static const silkworm::Bytes kTestStorageData2{*silkworm::from_hex("6000356000550055")};
static const std::vector<silkworm::Bytes> kTestStorageData{kTestStorageData1, kTestStorageData2};

static const silkworm::Bytes kTestCode1{*silkworm::from_hex("602a6000556101c960015560068060166000396000f3600035600055")};
static const silkworm::Bytes kTestCode2{*silkworm::from_hex("602a5f556101c960015560048060135f395ff35f355f55")};
static const std::vector<silkworm::Bytes> kTestCodes{kTestCode1, kTestCode2};

static const auto kTestHashedLocation1{0x6677907ab33937e392b9be983b30818f29d594039c9e1e7490bf7b3698888fb1_bytes32};
static const auto kTestHashedLocation2{0xe046602dcccb1a2f1d176718c8e709a42bba57af2da2379ba7130e2f916c95cd_bytes32};
static const std::vector<evmc::bytes32> kTestHashedLocations{kTestHashedLocation1, kTestHashedLocation2};

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

remote::StateChangeBatch new_batch(uint64_t view_id, silkworm::BlockNum block_height, const evmc::bytes32& block_hash,
                                   const std::vector<silkworm::Bytes>&& tx_rlps, bool unwind) {
    remote::StateChangeBatch state_changes;
    state_changes.set_databaseviewid(view_id);

    remote::StateChange* latest_change = state_changes.add_changebatch();
    latest_change->set_blockheight(block_height);
    latest_change->set_allocated_blockhash(silkworm::rpc::H256_from_bytes32(block_hash).release());
    latest_change->set_direction(unwind ? remote::Direction::UNWIND : remote::Direction::FORWARD);
    for (auto& tx_rlp : tx_rlps) {
        latest_change->add_txs(silkworm::to_hex(tx_rlp));
    }

    return state_changes;
}

remote::StateChangeBatch new_batch_with_upsert(uint64_t view_id, silkworm::BlockNum block_height, const evmc::bytes32& block_hash,
                                               const std::vector<silkworm::Bytes>&& tx_rlps, bool unwind) {
    remote::StateChangeBatch state_changes = new_batch(view_id, block_height, block_hash, std::move(tx_rlps), unwind);
    remote::StateChange* latest_change = state_changes.mutable_changebatch(0);

    remote::AccountChange* account_change = latest_change->add_changes();
    account_change->set_allocated_address(silkworm::rpc::H160_from_address(kTestAddress).release());
    account_change->set_action(remote::Action::UPSERT);
    account_change->set_incarnation(kTestIncarnation);
    account_change->set_data(kTestAccountData.data(), kTestAccountData.size());

    return state_changes;
}

remote::StateChangeBatch new_batch_with_upsert_code(uint64_t view_id, silkworm::BlockNum block_height,
                                                    const evmc::bytes32& block_hash, const std::vector<silkworm::Bytes>&& tx_rlps,
                                                    bool unwind, int num_code_changes) {
    remote::StateChangeBatch state_changes = new_batch(view_id, block_height, block_hash, std::move(tx_rlps), unwind);
    remote::StateChange* latest_change = state_changes.mutable_changebatch(0);

    SILKWORM_ASSERT(num_code_changes <= kTestCodes.size());
    for (int i{0}; i < num_code_changes; ++i) {
        remote::AccountChange* account_change = latest_change->add_changes();
        account_change->set_allocated_address(silkworm::rpc::H160_from_address(kTestAddress).release());
        account_change->set_action(remote::Action::UPSERT_CODE);
        account_change->set_incarnation(kTestIncarnation);
        account_change->set_data(kTestAccountData.data(), kTestAccountData.size());
        account_change->set_code(kTestCodes[i].data(), kTestCodes[i].size());
    }

    return state_changes;
}

remote::StateChangeBatch new_batch_with_delete(uint64_t view_id, silkworm::BlockNum block_height, const evmc::bytes32& block_hash,
                                               const std::vector<silkworm::Bytes>&& tx_rlps, bool unwind) {
    remote::StateChangeBatch state_changes = new_batch(view_id, block_height, block_hash, std::move(tx_rlps), unwind);
    remote::StateChange* latest_change = state_changes.mutable_changebatch(0);

    remote::AccountChange* account_change = latest_change->add_changes();
    account_change->set_allocated_address(silkworm::rpc::H160_from_address(kTestAddress).release());
    account_change->set_action(remote::Action::DELETE);

    return state_changes;
}

remote::StateChangeBatch new_batch_with_storage(uint64_t view_id, silkworm::BlockNum block_height,
                                                const evmc::bytes32& block_hash, const std::vector<silkworm::Bytes>&& tx_rlps,
                                                bool unwind, int num_storage_changes) {
    remote::StateChangeBatch state_changes = new_batch(view_id, block_height, block_hash, std::move(tx_rlps), unwind);
    remote::StateChange* latest_change = state_changes.mutable_changebatch(0);

    remote::AccountChange* account_change = latest_change->add_changes();
    account_change->set_allocated_address(silkworm::rpc::H160_from_address(kTestAddress).release());
    account_change->set_action(remote::Action::STORAGE);
    account_change->set_incarnation(kTestIncarnation);
    SILKWORM_ASSERT(num_storage_changes <= kTestHashedLocations.size());
    SILKWORM_ASSERT(num_storage_changes <= kTestStorageData.size());
    for (int i{0}; i < num_storage_changes; ++i) {
        remote::StorageChange* storage_change = account_change->add_storagechanges();
        storage_change->set_allocated_location(silkworm::rpc::H256_from_bytes32(kTestHashedLocations[i]).release());
        storage_change->set_data(kTestStorageData[i].data(), kTestStorageData[i].size());
    }

    return state_changes;
}

remote::StateChangeBatch new_batch_with_code(uint64_t view_id, silkworm::BlockNum block_height, const evmc::bytes32& block_hash,
                                             const std::vector<silkworm::Bytes>&& tx_rlps, bool unwind, int num_code_changes) {
    remote::StateChangeBatch state_changes = new_batch(view_id, block_height, block_hash, std::move(tx_rlps), unwind);
    remote::StateChange* latest_change = state_changes.mutable_changebatch(0);

    SILKWORM_ASSERT(num_code_changes <= kTestCodes.size());
    for (int i{0}; i < num_code_changes; ++i) {
        remote::AccountChange* account_change = latest_change->add_changes();
        account_change->set_allocated_address(silkworm::rpc::H160_from_address(kTestAddress).release());
        account_change->set_action(remote::Action::CODE);
        account_change->set_code(kTestCodes[i].data(), kTestCodes[i].size());
    }

    return state_changes;
}

void check_upsert(CoherentStateCache& cache, Transaction& txn, const evmc::address& address, const silkworm::Bytes& data) {
    std::unique_ptr<StateView> view = cache.get_view(txn);
    CHECK(view != nullptr);
    if (view) {
        asio::thread_pool pool{1};
        const silkworm::Bytes address_key{address.bytes, silkworm::kAddressLength};
        auto result = asio::co_spawn(pool, view->get(address_key), asio::use_future);
        const auto value = result.get();
        CHECK(value.has_value());
        if (value) {
            CHECK(*value == data);
        }
    }
}

void check_code(CoherentStateCache& cache, Transaction& txn, silkworm::ByteView code) {
    std::unique_ptr<StateView> view = cache.get_view(txn);
    CHECK(view != nullptr);
    if (view) {
        asio::thread_pool pool{1};
        const ethash::hash256 code_hash{silkworm::keccak256(kTestCode1)};
        const silkworm::Bytes code_hash_key{code_hash.bytes, silkworm::kHashLength};
        auto result = asio::co_spawn(pool, view->get_code(code_hash_key), asio::use_future);
        const auto value = result.get();
        CHECK(value.has_value());
        if (value) {
            CHECK(*value == kTestCode1);
        }
    }
}

TEST_CASE("CoherentStateCache::CoherentStateCache", "[silkrpc][ethdb][kv][state_cache]") {
    SECTION("default config") {
        CoherentStateCache cache;
        CHECK(cache.latest_data_size() == 0);
        CHECK(cache.latest_code_size() == 0);
        CHECK(cache.state_hit_count() == 0);
        CHECK(cache.state_miss_count() == 0);
        CHECK(cache.state_key_count() == 0);
        CHECK(cache.state_eviction_count() == 0);
    }

    SECTION("wrong config") {
        CoherentCacheConfig config{0, true, kDefaultMaxStateKeys, kDefaultMaxCodeKeys};
        CHECK_THROWS_AS(CoherentStateCache{config}, std::invalid_argument);
    }
}

TEST_CASE("CoherentStateCache::get_view returns no view", "[silkrpc][ethdb][kv][state_cache]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);
    asio::thread_pool pool{1};

    SECTION("no batch") {
        CoherentStateCache cache;
        test::MockTransaction txn;
        EXPECT_CALL(txn, tx_id()).WillOnce(Return(kTestViewId0));
        std::unique_ptr<StateView> view = cache.get_view(txn);
        CHECK(view == nullptr);
        CHECK(cache.state_hit_count() == 0);
        CHECK(cache.state_miss_count() == 0);
        CHECK(cache.state_key_count() == 0);
        CHECK(cache.state_eviction_count() == 0);
    }

    SECTION("empty batch") {
        CoherentStateCache cache;
        cache.on_new_block(remote::StateChangeBatch{});
        CHECK(cache.latest_data_size() == 0);
        test::MockTransaction txn;
        EXPECT_CALL(txn, tx_id()).WillOnce(Return(kTestViewId0));
        std::unique_ptr<StateView> view = cache.get_view(txn);
        CHECK(view == nullptr);
        CHECK(cache.state_hit_count() == 0);
        CHECK(cache.state_miss_count() == 0);
        CHECK(cache.state_key_count() == 0);
        CHECK(cache.state_eviction_count() == 0);
    }
}

TEST_CASE("CoherentStateCache::get_view returns one view", "[silkrpc][ethdb][kv][state_cache]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);
    CoherentStateCache cache;
    asio::thread_pool pool{1};

    SECTION("single upsert change batch => search hit") {
        auto batch = new_batch_with_upsert(kTestViewId0, kTestBlockNumber, kTestBlockHash, std::vector<silkworm::Bytes>{},
                                           /*unwind=*/false);
        cache.on_new_block(batch);
        CHECK(cache.latest_data_size() == 1);

        test::MockTransaction txn;
        EXPECT_CALL(txn, tx_id()).Times(2).WillRepeatedly(Return(kTestViewId0));

        check_upsert(cache, txn, kTestAddress, kTestAccountData);
        CHECK(cache.state_hit_count() == 1);
        CHECK(cache.state_miss_count() == 0);
        CHECK(cache.state_key_count() == 1);
        CHECK(cache.state_eviction_count() == 0);
    }

    SECTION("single upsert+code change batch => double search hit") {
        auto batch = new_batch_with_upsert_code(kTestViewId0, kTestBlockNumber, kTestBlockHash, std::vector<silkworm::Bytes>{},
                                                /*unwind=*/false, /*num_code_changes=*/1);
        cache.on_new_block(batch);
        CHECK(cache.latest_data_size() == 1);
        CHECK(cache.latest_code_size() == 1);

        test::MockTransaction txn;
        EXPECT_CALL(txn, tx_id()).Times(4).WillRepeatedly(Return(kTestViewId0));

        check_upsert(cache, txn, kTestAddress, kTestAccountData);
        CHECK(cache.state_hit_count() == 1);
        CHECK(cache.state_miss_count() == 0);
        CHECK(cache.state_key_count() == 1);
        CHECK(cache.state_eviction_count() == 0);

        check_code(cache, txn, kTestCode1);
        CHECK(cache.code_hit_count() == 1);
        CHECK(cache.code_miss_count() == 0);
        CHECK(cache.code_key_count() == 1);
        CHECK(cache.code_eviction_count() == 0);
    }

    SECTION("single delete change batch => search hit") {
        auto batch = new_batch_with_delete(kTestViewId0, kTestBlockNumber, kTestBlockHash, std::vector<silkworm::Bytes>{},
                                           /*unwind=*/false);
        cache.on_new_block(batch);
        CHECK(cache.latest_data_size() == 1);

        test::MockTransaction txn;
        EXPECT_CALL(txn, tx_id()).Times(2).WillRepeatedly(Return(kTestViewId0));

        std::unique_ptr<StateView> view = cache.get_view(txn);
        CHECK(view != nullptr);
        if (view) {
            const silkworm::Bytes address_key{kTestAddress.bytes, silkworm::kAddressLength};
            auto result1 = asio::co_spawn(pool, view->get(address_key), asio::use_future);
            const auto value1 = result1.get();
            CHECK(value1.has_value());
            if (value1) {
                CHECK(*value1 == silkworm::Bytes{});
            }
            CHECK(cache.state_hit_count() == 1);
            CHECK(cache.state_miss_count() == 0);
            CHECK(cache.state_key_count() == 1);
            CHECK(cache.state_eviction_count() == 0);
        }
    }

    SECTION("single storage change batch => search hit") {
        auto batch = new_batch_with_storage(kTestViewId0, kTestBlockNumber, kTestBlockHash, std::vector<silkworm::Bytes>{},
                                            /*unwind=*/false, /*num_storage_changes=*/1);
        cache.on_new_block(batch);
        CHECK(cache.latest_data_size() == 1);

        test::MockTransaction txn;
        EXPECT_CALL(txn, tx_id()).Times(2).WillRepeatedly(Return(kTestViewId0));

        std::unique_ptr<StateView> view = cache.get_view(txn);
        CHECK(view != nullptr);
        if (view) {
            const auto storage_key1 = composite_storage_key(kTestAddress, kTestIncarnation, kTestHashedLocation1.bytes);
            auto result = asio::co_spawn(pool, view->get(storage_key1), asio::use_future);
            const auto value = result.get();
            CHECK(value.has_value());
            if (value) {
                CHECK(*value == kTestStorageData1);
            }
            CHECK(cache.state_hit_count() == 1);
            CHECK(cache.state_miss_count() == 0);
            CHECK(cache.state_key_count() == 1);
            CHECK(cache.state_eviction_count() == 0);
        }
    }

    SECTION("single storage change batch => search miss") {
        auto batch = new_batch_with_storage(kTestViewId0, kTestBlockNumber, kTestBlockHash, std::vector<silkworm::Bytes>{},
                                            /*unwind=*/false, /*num_storage_changes=*/1);
        cache.on_new_block(batch);
        CHECK(cache.latest_data_size() == 1);

        std::shared_ptr<MockCursor> mock_cursor = std::make_shared<MockCursor>();
        test::DummyTransaction txn{kTestViewId0, mock_cursor};

        std::unique_ptr<StateView> view = cache.get_view(txn);
        CHECK(view != nullptr);
        if (view) {
            EXPECT_CALL(*mock_cursor, seek_exact(_)).WillOnce(InvokeWithoutArgs([]() -> asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, kTestStorageData2};
            }));

            const auto storage_key2 = composite_storage_key(kTestAddress, kTestIncarnation, kTestHashedLocation2.bytes);
            auto result = asio::co_spawn(pool, view->get(storage_key2), asio::use_future);
            const auto value = result.get();
            CHECK(value.has_value());
            if (value) {
                CHECK(*value == kTestStorageData2);
            }
            CHECK(cache.state_hit_count() == 0);
            CHECK(cache.state_miss_count() == 1);
            CHECK(cache.state_key_count() == 1);
            CHECK(cache.state_eviction_count() == 0);
        }
    }

    SECTION("double storage change batch => double search hit") {
        auto batch = new_batch_with_storage(kTestViewId0, kTestBlockNumber, kTestBlockHash, std::vector<silkworm::Bytes>{},
                                            /*unwind=*/false, /*num_storage_changes=*/2);
        cache.on_new_block(batch);
        CHECK(cache.latest_data_size() == 2);

        test::MockTransaction txn;
        EXPECT_CALL(txn, tx_id()).Times(3).WillRepeatedly(Return(kTestViewId0));
        std::unique_ptr<StateView> view = cache.get_view(txn);

        CHECK(view != nullptr);
        if (view) {
            const auto storage_key1 = composite_storage_key(kTestAddress, kTestIncarnation, kTestHashedLocation1.bytes);
            auto result1 = asio::co_spawn(pool, view->get(storage_key1), asio::use_future);
            const auto value1 = result1.get();
            CHECK(value1.has_value());
            if (value1) {
                CHECK(*value1 == kTestStorageData1);
            }
            const auto storage_key2 = composite_storage_key(kTestAddress, kTestIncarnation, kTestHashedLocation2.bytes);
            auto result2 = asio::co_spawn(pool, view->get(storage_key2), asio::use_future);
            const auto value2 = result2.get();
            CHECK(value2.has_value());
            if (value2) {
                CHECK(*value2 == kTestStorageData2);
            }
            CHECK(cache.state_hit_count() == 2);
            CHECK(cache.state_miss_count() == 0);
            CHECK(cache.state_key_count() == 2);
            CHECK(cache.state_eviction_count() == 0);
        }
    }

    SECTION("single code change batch => search hit") {
        auto batch = new_batch_with_code(kTestViewId0, kTestBlockNumber, kTestBlockHash, std::vector<silkworm::Bytes>{},
                                         /*unwind=*/false, /*num_code_changes=*/1);
        cache.on_new_block(batch);
        CHECK(cache.latest_code_size() == 1);

        test::MockTransaction txn;
        EXPECT_CALL(txn, tx_id()).Times(2).WillRepeatedly(Return(kTestViewId0));

        std::unique_ptr<StateView> view = cache.get_view(txn);
        CHECK(view != nullptr);
        if (view) {
            const ethash::hash256 code_hash{silkworm::keccak256(kTestCode1)};
            const silkworm::Bytes code_hash_key{code_hash.bytes, silkworm::kHashLength};
            auto result = asio::co_spawn(pool, view->get_code(code_hash_key), asio::use_future);
            const auto value = result.get();
            CHECK(value.has_value());
            if (value) {
                CHECK(*value == kTestCode1);
            }
            CHECK(cache.code_hit_count() == 1);
            CHECK(cache.code_miss_count() == 0);
            CHECK(cache.code_key_count() == 1);
            CHECK(cache.code_eviction_count() == 0);
        }
    }
}

TEST_CASE("CoherentStateCache::get_view returns two views", "[silkrpc][ethdb][kv][state_cache]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);
    CoherentStateCache cache;

    SECTION("two single-upsert change batches => two search hits in different views") {
        auto batch1 = new_batch_with_upsert(kTestViewId1, kTestBlockNumber, kTestBlockHash, std::vector<silkworm::Bytes>{},
                                            /*unwind=*/false);
        auto batch2 = new_batch_with_upsert(kTestViewId2, kTestBlockNumber, kTestBlockHash, std::vector<silkworm::Bytes>{},
                                            /*unwind=*/false);
        cache.on_new_block(batch1);
        cache.on_new_block(batch2);
        CHECK(cache.latest_data_size() == 1);

        test::MockTransaction txn1, txn2;
        EXPECT_CALL(txn1, tx_id()).Times(2).WillRepeatedly(Return(kTestViewId1));
        EXPECT_CALL(txn2, tx_id()).Times(2).WillRepeatedly(Return(kTestViewId2));

        check_upsert(cache, txn1, kTestAddress, kTestAccountData);
        CHECK(cache.state_hit_count() == 1);
        CHECK(cache.state_miss_count() == 0);
        CHECK(cache.state_key_count() == 1);
        CHECK(cache.state_eviction_count() == 1);

        check_upsert(cache, txn2, kTestAddress, kTestAccountData);
        CHECK(cache.state_hit_count() == 2);
        CHECK(cache.state_miss_count() == 0);
        CHECK(cache.state_key_count() == 1);
        CHECK(cache.state_eviction_count() == 1);
    }
}

}  // namespace silkrpc::ethdb::kv
