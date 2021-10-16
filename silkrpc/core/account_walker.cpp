/*
   Copyright 2020 The Silkrpc Authors

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


#include "account_walker.hpp"

#include <sstream>

#include <silkworm/core/silkworm/common/endian.hpp>
#include <silkworm/node/silkworm/db/bitmap.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/core/rawdb/chain.hpp>
#include <silkrpc/core/state_reader.hpp>
#include <silkrpc/ethdb/cursor.hpp>
#include <silkrpc/ethdb/tables.hpp>
#include <silkrpc/ethdb/transaction_database.hpp>
#include <silkrpc/json/types.hpp>

namespace silkrpc {

asio::awaitable<void> AccountWalker::walk_of_accounts(uint64_t block_number, const evmc::address& start_address, Collector& collector) {
    auto ps_cursor = co_await transaction_.cursor(db::table::kPlainState);
    SILKRPC_TRACE << "AccountWalker::walk_of_accounts on table " << db::table::kPlainState
        << " cursor_id: " << ps_cursor->cursor_id() << "\n";

    auto start_key = silkworm::full_view(start_address);
    auto ps_kv = co_await seek(*ps_cursor, start_key, silkworm::kAddressLength);
    if (ps_kv.key.empty()) {
        co_return;
    }

    auto ah_cursor = co_await transaction_.cursor(db::table::kAccountHistory);
    SILKRPC_TRACE << "AccountWalker::walk_of_accounts on table " << db::table::kAccountHistory
        << " cursor_id: " << ah_cursor->cursor_id() << "\n";
    silkrpc::ethdb::SplitCursor split_cursor{*ah_cursor, start_key, 0, silkworm::kAddressLength,
        silkworm::kAddressLength, silkworm::kAddressLength + 8};

    auto s_kv = co_await seek(split_cursor, block_number);

    auto go_on = true;
    while (go_on) {
        SILKRPC_TRACE << "ITERATE *****  main cursor key: 0x" << silkworm::to_hex(ps_kv.key)
            << " split cursor key: 0x" << silkworm::to_hex(s_kv.key1) << "\n";
        if (ps_kv.key.empty() && s_kv.key1.empty()) {
            break;
        }
        auto cmp = ps_kv.key.compare(s_kv.key1);
        if (cmp < 0) {
           go_on = collector(ps_kv.key, ps_kv.value);
        } else {
            const auto bitmap = silkworm::db::bitmap::read(s_kv.value);
            uint64_t* ans = new uint64_t[bitmap.cardinality()];
            bitmap.toUint64Array(ans);

            std::optional<silkworm::Account> result;
            if (bitmap.contains(block_number)) {
                const auto address = silkworm::to_address(s_kv.key1);
                SILKRPC_TRACE << "Ready to read historical account for address 0x" << silkworm::to_hex(address) << " and block_number " << block_number << "\n";

                ethdb::TransactionDatabase tx_database{transaction_};
                StateReader state_reader{tx_database};
                const auto historical_account = co_await state_reader.read_historical_account(address, block_number);

                if (historical_account) {
                    const auto data = historical_account.value();
                    auto [account, err]{silkworm::decode_account_from_storage(data)};
                    SILKRPC_TRACE << "Found in historical account: address 0x" << address
                        << ", len " << data.size()
                        << ", data 0x" << silkworm::to_hex(data)
                        << "\n";

                    silkworm::Bytes value(data);
                    go_on = collector(s_kv.key1, value);
                }
            } else if (cmp == 0) {
                go_on = collector(ps_kv.key, ps_kv.value);
            }
        }

        if (go_on) {
            if (cmp <= 0) {
                ps_kv = co_await next(*ps_cursor, silkworm::kAddressLength);
            }
            if (cmp >= 0) {
                s_kv = co_await next(split_cursor, block_number);
            }
        }
    }
}

asio::awaitable<KeyValue> AccountWalker::next(silkrpc::ethdb::Cursor& cursor, uint64_t len) {
    auto kv = co_await cursor.next();

    while (!kv.key.empty() && kv.key.size() > len) {
        kv = co_await cursor.next();
    }
    co_return kv;
}

asio::awaitable<KeyValue> AccountWalker::seek(silkrpc::ethdb::Cursor& cursor, const silkworm::ByteView& key, uint64_t len) {
    auto kv = co_await cursor.seek(key);
    if (kv.key.size() > len) {
        co_return co_await next(cursor, len);
    }
    co_return kv;
}

asio::awaitable<silkrpc::ethdb::SplittedKeyValue> AccountWalker::next(silkrpc::ethdb::SplitCursor& cursor, uint64_t number) {
    auto kv = co_await cursor.next();
    if (kv.key1.empty()) {
        co_return kv;
    }
    uint64_t block = silkworm::endian::load_big_u64(kv.key2.data());
    while (block < number) {
        kv = co_await cursor.next();
        if (kv.key1.empty()) {
            break;
        }
        block = silkworm::endian::load_big_u64(kv.key2.data());
    }
    co_return kv;
}

asio::awaitable<silkrpc::ethdb::SplittedKeyValue> AccountWalker::seek(silkrpc::ethdb::SplitCursor& cursor, uint64_t number) {
    auto kv = co_await cursor.seek();
    if (kv.key1.empty()) {
        co_return kv;
    }
    uint64_t block = silkworm::endian::load_big_u64(kv.key2.data());
    if (block < number) {
        co_return co_await next(cursor, number);
    }
    co_return kv;
}

} // namespace silkrpc
