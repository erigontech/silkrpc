/*
   Copyright 2020-2022 The Silkrpc Authors

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

#ifndef SILKRPC_ETHDB_KV_CACHED_DATABASE_HPP_
#define SILKRPC_ETHDB_KV_CACHED_DATABASE_HPP_

#include <optional>
#include <string>

#include <silkrpc/core/rawdb/accessors.hpp>
#include <silkrpc/ethdb/kv/state_cache.hpp>
#include <silkrpc/ethdb/transaction.hpp>
#include <silkrpc/ethdb/transaction_database.hpp>
#include <silkrpc/types/block.hpp>
#include <silkworm/common/util.hpp>

namespace silkrpc::ethdb::kv {

class CachedDatabase : public core::rawdb::DatabaseReader {
public:
    explicit CachedDatabase(const BlockNumberOrHash& block_id, Transaction& txn, kv::StateCache& state_cache);

    CachedDatabase(const CachedDatabase&) = delete;
    CachedDatabase& operator=(const CachedDatabase&) = delete;

    asio::awaitable<KeyValue> get(const std::string& table, const silkworm::ByteView& key) const override;

    asio::awaitable<silkworm::Bytes> get_one(const std::string& table, const silkworm::ByteView& key) const override;

    asio::awaitable<std::optional<silkworm::Bytes>> get_both_range(const std::string& table, const silkworm::ByteView& key,
                                                                   const silkworm::ByteView& subkey) const override;

    asio::awaitable<void> walk(const std::string& table, const silkworm::ByteView& start_key, uint32_t fixed_bits,
                               core::rawdb::Walker w) const override;

    asio::awaitable<void> for_prefix(const std::string& table, const silkworm::ByteView& prefix,
                                     core::rawdb::Walker w) const override;

private:
    const BlockNumberOrHash& block_id_;
    Transaction& txn_;
    kv::StateCache& state_cache_;
    TransactionDatabase txn_database_;
};

}  // namespace silkrpc::ethdb::kv

#endif  // SILKRPC_ETHDB_KV_CACHED_DATABASE_HPP_
