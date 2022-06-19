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

#include <exception>

#include <magic_enum.hpp>
#include <silkworm/common/assert.hpp>
#include <silkworm/common/util.hpp>
#include <silkworm/rpc/conversion.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/core/rawdb/util.hpp>
#include <silkrpc/ethdb/tables.hpp>
#include <silkrpc/ethdb/transaction_database.hpp>

namespace silkrpc::ethdb::kv {

CoherentStateView::CoherentStateView(Transaction& txn, CoherentStateCache* cache) : txn_(txn), cache_(cache) {
}

asio::awaitable<std::optional<silkworm::Bytes>> CoherentStateView::get(const silkworm::Bytes& key) {
    co_return co_await cache_->get(key, txn_);
}

asio::awaitable<std::optional<silkworm::Bytes>> CoherentStateView::get_code(const silkworm::Bytes& key) {
    co_return co_await cache_->get_code(key, txn_);
}

CoherentStateCache::CoherentStateCache(const CoherentCacheConfig& config) : config_(config) {
    if (config.max_views == 0) {
        throw std::invalid_argument{"unexpected zero max_views"};
    }
}

std::unique_ptr<StateView> CoherentStateCache::get_view(Transaction& txn) {
    const auto view_id = txn.tx_id();
    CoherentStateRoot* root = get_root(view_id);
    return root->ready ? std::make_unique<CoherentStateView>(txn, this) : nullptr;
}

std::size_t CoherentStateCache::size() {
    std::shared_lock read_lock{rw_mutex_};
    if (latest_state_view_ == nullptr) {
        return 0;
    }
    return latest_state_view_->cache.size();
}

void CoherentStateCache::on_new_block(const remote::StateChangeBatch& state_changes) {
    std::unique_lock write_lock{rw_mutex_};

    const auto view_id = state_changes.databaseviewid();
    CoherentStateRoot* root = advance_root(view_id);
    for (const auto& state_change : state_changes.changebatch()) {
        for (const auto& change : state_change.changes()) {
            switch (change.action()) {
                case remote::Action::UPSERT: {
                    process_upsert_change(root, view_id, change);
                    break;
                }
                case remote::Action::UPSERT_CODE: {
                    process_upsert_change(root, view_id, change);
                    process_code_change(root, view_id, change);
                    break;
                }
                case remote::Action::DELETE: {
                    process_delete_change(root, view_id, change);
                    break;
                }
                case remote::Action::STORAGE: {
                    if (config_.with_storage && change.storagechanges_size() > 0) {
                        process_storage_change(root, view_id, change);
                    }
                    break;
                }
                case remote::Action::CODE: {
                    process_code_change(root, view_id, change);
                    break;
                }
                default: {
                    SILKRPC_ERROR << "Unexpected action: " << magic_enum::enum_name(change.action()) << " skipped\n";
                }
            }
        }
    }

    root->ready = true;
}

void CoherentStateCache::process_upsert_change(CoherentStateRoot* root, StateViewId view_id, const remote::AccountChange& change) {
    const auto address = silkworm::rpc::address_from_H160(change.address());
    const auto data_bytes = silkworm::bytes_of_string(change.data());
    SILKRPC_DEBUG << "CoherentStateCache::process_upsert_change data: " << data_bytes << " address: " << address << "\n";
    const silkworm::Bytes address_bytes{address.bytes, silkworm::kAddressLength};
    add({address_bytes, data_bytes}, root, view_id);
}

void CoherentStateCache::process_code_change(CoherentStateRoot* root, StateViewId view_id, const remote::AccountChange& change) {
    const auto code_bytes = silkworm::bytes_of_string(change.code());
    SILKRPC_DEBUG << "CoherentStateCache::process_code_change code: " << code_bytes << "\n";
    const ethash::hash256 code_hash{silkworm::keccak256(code_bytes)};
    const silkworm::Bytes hash_bytes{code_hash.bytes, silkworm::kHashLength};
    add_code({hash_bytes, code_bytes}, root, view_id);
}

void CoherentStateCache::process_delete_change(CoherentStateRoot* root, StateViewId view_id, const remote::AccountChange& change) {
    const auto address = silkworm::rpc::address_from_H160(change.address());
    const silkworm::Bytes address_bytes{address.bytes, silkworm::kAddressLength};
    add({address_bytes, {}}, root, view_id);
}

void CoherentStateCache::process_storage_change(CoherentStateRoot* root, StateViewId view_id, const remote::AccountChange& change) {
    const auto address = silkworm::rpc::address_from_H160(change.address());
    for (const auto& storage_change : change.storagechanges()) {
        const auto location_hash = silkworm::rpc::bytes32_from_H256(storage_change.location());
        const auto storage_key = composite_storage_key(address, change.incarnation(), location_hash.bytes);
        const auto data_bytes = silkworm::bytes_of_string(storage_change.data());
        SILKRPC_DEBUG << "CoherentStateCache::process_storage_change data: " << data_bytes << " address: " << address << "\n";
        add({storage_key, data_bytes}, root, view_id);
    }
}

KeyValue* CoherentStateCache::add(KeyValue&& kv, CoherentStateRoot* root, StateViewId view_id) {
    auto [it, inserted] = root->cache.insert(kv);
    KeyValue* replaced{nullptr};
    if (!inserted) {
        replaced = &*it;
        root->cache.erase(it);
        std::tie(it, inserted) = root->cache.insert(kv);
        SILKWORM_ASSERT(inserted);
    }
    if (latest_state_view_id_ != view_id) {
        return &*it;
    }
    if (replaced != nullptr) {
        state_evictions_.remove(*replaced);
    }
    state_evictions_.push_front(kv);
    if (state_evictions_.size() > config_.max_state_keys) {
        state_evictions_.pop_back(); // remove oldest
    }
    return &*it;
}

KeyValue* CoherentStateCache::add_code(KeyValue&& kv, CoherentStateRoot* root, StateViewId view_id) {
    auto [it, inserted] = root->code_cache.insert(kv);
    KeyValue* replaced{nullptr};
    if (!inserted) {
        replaced = &*it;
        root->code_cache.erase(it);
        std::tie(it, inserted) = root->code_cache.insert(kv);
        SILKWORM_ASSERT(inserted);
    }
    if (latest_state_view_id_ != view_id) {
        return &*it;
    }
    if (replaced != nullptr) {
        code_evictions_.remove(*replaced);
    }
    code_evictions_.push_front(kv);
    if (code_evictions_.size() > config_.max_code_keys) {
        code_evictions_.pop_back(); // remove oldest
    }
    return &*it;
}

asio::awaitable<std::optional<silkworm::Bytes>> CoherentStateCache::get(const silkworm::Bytes& key, Transaction& txn) {
    std::shared_lock read_lock{rw_mutex_};

    const auto view_id = txn.tx_id();
    const auto root_it = state_view_roots_.find(view_id);
    if (root_it == state_view_roots_.end()) {
        co_return std::nullopt;
    }
    KeyValue kv{key};
    auto& cache = root_it->second->cache;
    const auto kv_it = cache.find(kv);
    if (kv_it != cache.end() && view_id == latest_state_view_id_) {
        state_evictions_.remove(kv);
        state_evictions_.push_front(kv);
    }

    //TODO(canepat) add hit/miss stats

    TransactionDatabase tx_database{txn};
    const auto value = co_await tx_database.get_one(db::table::kPlainState, key);

    read_lock.unlock();
    std::unique_lock write_lock{rw_mutex_};

    add({key, value}, root_it->second.get(), view_id);

    co_return key;
}

asio::awaitable<std::optional<silkworm::Bytes>> CoherentStateCache::get_code(const silkworm::Bytes& key, Transaction& txn) {
    std::shared_lock read_lock{rw_mutex_};

    const auto view_id = txn.tx_id();
    const auto root_it = state_view_roots_.find(view_id);
    if (root_it == state_view_roots_.end()) {
        co_return std::nullopt;
    }
    KeyValue kv{key};
    auto& code_cache = root_it->second->code_cache;
    const auto kv_it = code_cache.find(kv);
    if (kv_it != code_cache.end() && view_id == latest_state_view_id_) {
        code_evictions_.remove(kv);
        code_evictions_.push_front(kv);
    }

    //TODO(canepat) add hit/miss stats

    TransactionDatabase tx_database{txn};
    const auto value = co_await tx_database.get_one(db::table::kCode, key);

    read_lock.unlock();
    std::unique_lock write_lock{rw_mutex_};

    add_code({key, value}, root_it->second.get(), view_id);

    co_return key;
}

CoherentStateRoot* CoherentStateCache::get_root(StateViewId view_id) {
    const auto root_it = state_view_roots_.find(view_id);
    if (root_it != state_view_roots_.end()) {
        return root_it->second.get();
    }
    const auto [new_root_it, _] = state_view_roots_.emplace(view_id, std::make_unique<CoherentStateRoot>());
    return new_root_it->second.get();
}

CoherentStateRoot* CoherentStateCache::advance_root(StateViewId view_id) {
    CoherentStateRoot* root = get_root(view_id);

    const auto previous_root_it = state_view_roots_.find(view_id - 1);
    if (previous_root_it != state_view_roots_.end() && previous_root_it->second->canonical) {
        root->cache = previous_root_it->second->cache;
        root->code_cache = previous_root_it->second->code_cache;
    } else {
        state_evictions_.clear();
        for (const auto& kv : root->cache) {
            state_evictions_.push_front(kv);
        }
        code_evictions_.clear();
        for (const auto& kv : root->code_cache) {
            code_evictions_.push_front(kv);
        }
    }
    root->canonical = true;

    evict_roots();

    latest_state_view_id_ = view_id;
    latest_state_view_ = root;

    //TODO(canepat) add keys/code_keys state_evictions/code_evictions stats

    return root;
}

void CoherentStateCache::evict_roots() {
    if (latest_state_view_id_ <= config_.max_views) {
        return;
    }
    if (state_view_roots_.size() < config_.max_views) {
        return;
    }
    // Erase older state views in order to not exceed max_views
    const auto max_view_id_to_delete = latest_state_view_id_ - config_.max_views;
    std::erase_if(state_view_roots_, [&](const auto& item) {
        auto const& [view_id, _] = item;
        return view_id <= max_view_id_to_delete;
    });
}

} // namespace silkrpc::ethdb::kv
