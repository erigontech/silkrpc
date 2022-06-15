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
#include <silkrpc/core/rawdb/util.hpp>

namespace silkrpc::ethdb::kv {

CoherentStateView::CoherentStateView(Transaction& txn, CoherentStateCache* cache) : txn_(txn), cache_(cache) {
}

silkworm::Bytes CoherentStateView::get(const KeyValue& kv) {
    return cache_->get(kv, txn_);
}

silkworm::Bytes CoherentStateView::get_code(const KeyValue& kv) {
    return cache_->get_code(kv, txn_);
}

CoherentStateCache::CoherentStateCache(const CoherentCacheConfig& config) : config_(config) {
    if (config.max_views == 0) {
        throw std::invalid_argument{"unexpected zero max_views"};
    }
}

StateView* CoherentStateCache::get_view(Transaction& txn) {
    const auto view_id = txn.tx_id();
    CoherentStateRoot* root = get_root(view_id);
    SILKWORM_ASSERT(root->ready);
    return new CoherentStateView{txn, this};
}

void CoherentStateCache::on_new_block(const remote::StateChangeBatch& state_changes) {
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

    //root->ready = true;
}

void CoherentStateCache::process_upsert_change(CoherentStateRoot* root, StateViewId view_id, const remote::AccountChange& change) {
    const auto address = silkworm::rpc::address_from_H160(change.address());
    const auto data = silkworm::from_hex(change.data());
    if (data) {
        const silkworm::Bytes address_bytes{address.bytes, silkworm::kAddressLength};
        const silkworm::Bytes data_bytes{data.value()};
        add({address_bytes, data_bytes}, root, view_id);
    } else {
        SILKRPC_WARN << "Empty data in account upsert change for view: " << view_id << " address: " << address << "\n";
    }
}

void CoherentStateCache::process_code_change(CoherentStateRoot* root, StateViewId view_id, const remote::AccountChange& change) {
    const auto code = silkworm::from_hex(change.code());
    if (code) {
        const ethash::hash256 code_hash{silkworm::keccak256(code.value())};
        const silkworm::Bytes hash_bytes{code_hash.bytes, silkworm::kHashLength};
        const silkworm::Bytes code_bytes{code.value()};
        add_code({hash_bytes, code_bytes}, root, view_id);
    } else {
        const auto address = silkworm::rpc::address_from_H160(change.address());
        SILKRPC_WARN << "Empty code in contract code change for view: " << view_id << " address: " << address << "\n";
    }
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
        const auto data = silkworm::from_hex(storage_change.data());
        if (data) {
            const silkworm::Bytes data_bytes{data.value()};
            add({storage_key, data_bytes}, root, view_id);
        } else {
            SILKRPC_WARN << "Empty data in storage change for view: " << view_id << " address: " << address << "\n";
        }
    }
}

void CoherentStateCache::add(const KeyValue& kv, CoherentStateRoot* root, StateViewId view_id) {
    std::unique_lock write_lock{rw_mutex_};
}

void CoherentStateCache::add_code(const KeyValue& kv, CoherentStateRoot* root, StateViewId view_id) {
    std::unique_lock write_lock{rw_mutex_};
}

silkworm::Bytes CoherentStateCache::get(const KeyValue& kv, Transaction& txn) {
    const auto view_id = txn.tx_id();

    std::shared_lock read_lock{rw_mutex_};
    auto root_it = state_view_roots_.find(view_id);
    if (root_it == state_view_roots_.end()) {
        return silkworm::Bytes{};
    }
    auto& cache = root_it->second->cache;
    const auto kv_it = cache.find(kv);
    if (kv_it != cache.end() && view_id == latest_state_view_id_) {
        //state_evictions_.splice(state_evictions_.begin(), state_evictions_, );
    }

    return silkworm::Bytes{};
}

silkworm::Bytes CoherentStateCache::get_code(const KeyValue& kv, Transaction& /*txn*/) {
    return silkworm::Bytes{};
}

CoherentStateRoot* CoherentStateCache::get_root(StateViewId view_id) {
    return nullptr;
}

CoherentStateRoot* CoherentStateCache::advance_root(StateViewId view_id) {
    return nullptr;
}

} // namespace silkrpc::ethdb::kv
