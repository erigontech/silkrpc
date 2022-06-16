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

#ifndef SILKRPC_ETHDB_KV_STATE_CACHE_HPP_
#define SILKRPC_ETHDB_KV_STATE_CACHE_HPP_

#include <chrono>
#include <cstddef>
#include <list>
#include <map>
#include <memory>
#include <shared_mutex>

#include <absl/container/btree_set.h>
#include <silkworm/common/base.hpp>

#include <silkrpc/common/util.hpp>
#include <silkrpc/ethdb/transaction.hpp>
#include <silkrpc/interfaces/remote/kv.pb.h>

namespace silkrpc::ethdb::kv {

class StateView {
public:
    virtual silkworm::Bytes get(const KeyValue& kv) = 0;

    virtual silkworm::Bytes get_code(const KeyValue& kv) = 0;
};

class StateCache {
public:
    virtual std::unique_ptr<StateView> get_view(Transaction& txn) = 0;

    virtual void on_new_block(const remote::StateChangeBatch& state_changes) = 0;

    virtual std::size_t size() = 0;
};

struct CoherentStateRoot {
    absl::btree_set<KeyValue> cache;
    absl::btree_set<KeyValue> code_cache;
    bool ready;
    bool canonical;
};

using namespace std::chrono_literals; // NOLINT(build/namespaces)
using StateViewId = uint64_t;

constexpr auto kDefaultMaxViews{5ul};
constexpr auto kDefaultNewblockTimeout{50ms};
constexpr auto kDefaultLabel{"default"};
constexpr auto kDefaultMaxStateKeys{1'000'000u};
constexpr auto kDefaultMaxCodeKeys{10'000u};

struct CoherentCacheConfig {
    uint64_t max_views{kDefaultMaxViews};
    std::chrono::milliseconds new_block_timeout{kDefaultNewblockTimeout};
    const char* label{kDefaultLabel};
    bool with_storage{true};
    uint32_t max_state_keys{kDefaultMaxStateKeys};
    uint32_t max_code_keys{kDefaultMaxCodeKeys};
};

class CoherentStateCache;

class CoherentStateView : public StateView {
public:
    explicit CoherentStateView(Transaction& txn, CoherentStateCache* cache);

    CoherentStateView(const CoherentStateView&) = delete;
    CoherentStateView& operator=(const CoherentStateView&) = delete;

    silkworm::Bytes get(const KeyValue& kv) override;

    silkworm::Bytes get_code(const KeyValue& kv) override;

private:
    Transaction& txn_;
    CoherentStateCache* cache_;
};

class CoherentStateCache : public StateCache {
public:
    explicit CoherentStateCache(const CoherentCacheConfig& config = {});

    CoherentStateCache(const CoherentStateCache&) = delete;
    CoherentStateCache& operator=(const CoherentStateCache&) = delete;

    std::unique_ptr<StateView> get_view(Transaction& txn) override;

    void on_new_block(const remote::StateChangeBatch& batch) override;

    std::size_t size() override;

private:
    friend class CoherentStateView;

    void process_upsert_change(CoherentStateRoot* root, StateViewId view_id, const remote::AccountChange& change);
    void process_code_change(CoherentStateRoot* root, StateViewId view_id, const remote::AccountChange& change);
    void process_delete_change(CoherentStateRoot* root, StateViewId view_id, const remote::AccountChange& change);
    void process_storage_change(CoherentStateRoot* root, StateViewId view_id, const remote::AccountChange& change);
    void add(KeyValue&& kv, CoherentStateRoot* root, StateViewId view_id);
    void add_code(KeyValue&& kv, CoherentStateRoot* root, StateViewId view_id);
    silkworm::Bytes get(const KeyValue& kv, Transaction& txn);
    silkworm::Bytes get_code(const KeyValue& kv, Transaction& txn);
    CoherentStateRoot* get_root(StateViewId view_id);
    CoherentStateRoot* advance_root(StateViewId view_id);
    void evict_roots();

    const CoherentCacheConfig& config_;

    std::map<StateViewId, std::unique_ptr<CoherentStateRoot>> state_view_roots_;
    StateViewId latest_state_view_id_{0};
    CoherentStateRoot* latest_state_view_{nullptr};
    std::list<KeyValue> state_evictions_;
    std::list<KeyValue> code_evictions_;
    std::shared_mutex rw_mutex_;

    uint64_t state_hit_count_{0};
    uint64_t state_miss_count_{0};
    uint64_t state_key_count_{0};
    uint64_t state_eviction_count_{0};
    uint64_t code_hit_count_{0};
    uint64_t code_miss_count_{0};
    uint64_t code_key_count_{0};
    uint64_t code_eviction_count_{0};
    uint64_t timeout_{0};
};

} // namespace silkrpc::ethdb::kv

#endif  // SILKRPC_ETHDB_KV_STATE_CACHE_HPP_
