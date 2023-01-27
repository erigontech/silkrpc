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

#include "account_dumper.hpp"

#include <sstream>
#include <utility>

#include <silkworm/core/silkworm/common/endian.hpp>
#include <silkworm/core/silkworm/trie/hash_builder.hpp>
#include <silkworm/core/silkworm/trie/nibbles.hpp>
#include <silkworm/node/silkworm/common/rlp_err.hpp>
#include <silkworm/node/silkworm/db/bitmap.hpp>
#include <silkworm/node/silkworm/db/util.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/core/cached_chain.hpp>
#include <silkrpc/core/account_walker.hpp>
#include <silkrpc/core/rawdb/chain.hpp>
#include <silkrpc/core/state_reader.hpp>
#include <silkrpc/core/storage_walker.hpp>
#include <silkrpc/ethdb/cursor.hpp>
#include <silkrpc/ethdb/tables.hpp>
#include <silkrpc/ethdb/transaction_database.hpp>
#include <silkrpc/json/types.hpp>

namespace silkrpc {

boost::asio::awaitable<DumpAccounts> AccountDumper::dump_accounts(BlockCache& cache, const BlockNumberOrHash& bnoh, const evmc::address& start_address, int16_t max_result,
                                                           bool exclude_code, bool exclude_storage) {
    DumpAccounts dump_accounts;
    ethdb::TransactionDatabase tx_database{transaction_};

    const auto block_with_hash = co_await core::read_block_by_number_or_hash(cache, tx_database, bnoh);
    const auto block_number = block_with_hash.block.header.number;

    dump_accounts.root = block_with_hash.block.header.state_root;

    std::vector<silkrpc::KeyValue> collected_data;

    AccountWalker::Collector collector = [&](silkworm::ByteView k, silkworm::ByteView v) {
        if (max_result > 0 && collected_data.size() >= max_result) {
            dump_accounts.next = silkworm::to_evmc_address(k);
            return false;
        }

        if (k.size() > silkworm::kAddressLength) {
            return true;
        }

        silkrpc::KeyValue kv;
        kv.key = k;
        kv.value = v;
        collected_data.push_back(kv);
        return true;
    };

    AccountWalker walker{transaction_};
    co_await walker.walk_of_accounts(block_number + 1, start_address, collector);

    co_await load_accounts(tx_database, collected_data, dump_accounts, exclude_code);
    if (!exclude_storage) {
        co_await load_storage(block_number, dump_accounts);
    }

    co_return dump_accounts;
}

boost::asio::awaitable<void> AccountDumper::load_accounts(ethdb::TransactionDatabase& tx_database,
    const std::vector<silkrpc::KeyValue>& collected_data, DumpAccounts& dump_accounts, bool exclude_code) {

    StateReader state_reader{tx_database};
    for (auto kv : collected_data) {
        const auto address = silkworm::to_evmc_address(kv.key);

        auto [account, err]{silkworm::Account::from_encoded_storage(kv.value)};
        silkworm::rlp::success_or_throw(err);

        DumpAccount dump_account;
        dump_account.balance = account.balance;
        dump_account.nonce = account.nonce;
        dump_account.code_hash = account.code_hash;
        dump_account.incarnation = account.incarnation;

        if (account.incarnation > 0 && account.code_hash == silkworm::kEmptyHash) {
            const auto storage_key{silkworm::db::storage_prefix(full_view(address), account.incarnation)};
            auto code_hash{co_await tx_database.get_one(db::table::kPlainContractCode, storage_key)};
            if (code_hash.length() == silkworm::kHashLength) {
                std::memcpy(dump_account.code_hash.bytes, code_hash.data(), silkworm::kHashLength);
            }
        }
        if (!exclude_code) {
            auto code = co_await state_reader.read_code(account.code_hash);
            dump_account.code.swap(code);
        }
        dump_accounts.accounts.insert(std::pair<evmc::address, DumpAccount>(address, dump_account));
    }

    co_return;
}

boost::asio::awaitable<void> AccountDumper::load_storage(uint64_t block_number, DumpAccounts& dump_accounts) {
    SILKRPC_TRACE << "block_number " << block_number << " START\n";
    StorageWalker storage_walker{transaction_};
    evmc::bytes32 start_location{};
    for (AccountsMap::iterator itr = dump_accounts.accounts.begin(); itr != dump_accounts.accounts.end(); itr++) {
        auto& address = itr->first;
        auto& account = itr->second;

        std::map<silkworm::Bytes, silkworm::Bytes> collected_entries;
        StorageWalker::AccountCollector collector = [&](const evmc::address& address, silkworm::ByteView loc, silkworm::ByteView data) {
            if (!account.storage.has_value()) {
                account.storage = Storage{};
            }
            auto& storage = *account.storage;
            storage[silkworm::to_bytes32(loc)] = data;
            auto hash = hash_of(loc);
            auto key = full_view(hash);
            collected_entries[silkworm::Bytes{key}] = data;

            return true;
        };

        co_await storage_walker.walk_of_storages(block_number, address, start_location, account.incarnation, collector);

        silkworm::trie::HashBuilder hb;
        for (const auto& [key, value] : collected_entries) {
            silkworm::Bytes encoded{};
            silkworm::rlp::encode(encoded, value);
            silkworm::Bytes unpacked = silkworm::trie::unpack_nibbles(key);

            hb.add_leaf(unpacked, encoded);
        }

        account.root = hb.root_hash();
    }
    SILKRPC_TRACE << "block_number " << block_number << " END\n";
    co_return;
}

} // namespace silkrpc
