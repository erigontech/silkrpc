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


#include "account_walker.hpp"

#include <sstream>

#include <silkworm/core/silkworm/common/endian.hpp>
#include <silkworm/db/util.hpp>
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
    SILKRPC_DEBUG << "Ready to walk accounts: block_number " << std::dec << block_number << " start_address 0x" << silkworm::to_hex(start_address) << "\n";

    auto ps_cursor = co_await transaction_.cursor(db::table::kPlainState);

    auto start_key = silkworm::full_view(start_address);
    auto ps_kv = co_await seek(*ps_cursor, start_key, silkworm::kAddressLength);
    if (ps_kv.key.empty()) {
        co_return;
    }

    auto ah_cursor = co_await transaction_.cursor(db::table::kAccountHistory);
    silkrpc::ethdb::SplitCursor split_cursor{*ah_cursor, start_key, 0, silkworm::kAddressLength,
        silkworm::kAddressLength, silkworm::kAddressLength + 8};

    auto s_kv = co_await seek(split_cursor, block_number);

    auto acs_cursor = co_await transaction_.cursor_dup_sort(db::table::kPlainAccountChangeSet);

    // SILKRPC_TRACE << "AccountWalker : ready to start" << " addr " <<  silkworm::to_hex(s_kv.key1) << "\n";

    auto count{1};
    auto go_on = true;
    while (go_on) {
      SILKRPC_TRACE << "ITERATE ************************* " << count << "\n";
        auto start = std::chrono::system_clock::now();

        SILKRPC_TRACE << "ITERATE *****  main cursor key: 0x" << silkworm::to_hex(ps_kv.key) << " split cursor key: 0x" << silkworm::to_hex(s_kv.key1) << "\n";
        if (ps_kv.key.empty() && s_kv.key1.empty()) {
            break;
        }
        auto cmp = ps_kv.key.compare(s_kv.key1);
        if (cmp < 0) {
            go_on = collector(ps_kv.key, ps_kv.value);

            // SILKRPC_TRACE << "ITERATE *****  WALKER CALLED : key 0x" << silkworm::to_hex(ps_kv.key) << " value: 0x" << silkworm::to_hex(ps_kv.value) << "\n";
        } else {
            SILKRPC_TRACE << "ITERATE *****  building roaring64 from " << silkworm::to_hex(s_kv.value) << "\n";

            const auto bitmap = silkworm::db::bitmap::read(s_kv.value);
            uint64_t* ans = new uint64_t[bitmap.cardinality()];
            bitmap.toUint64Array(ans);

            std::optional<silkworm::Account> result;
            const auto found = silkworm::db::bitmap::seek(bitmap, block_number);
            SILKRPC_TRACE << "ITERATE ** SeekInBitmap64 looking for block number " << std::dec << block_number << " found block number " << std::dec << found.value_or(0) << "\n";
            if (found) {
                const auto block_key{silkworm::db::block_key(found.value())};
                SILKRPC_TRACE << "seek_both: block_key 0x" << silkworm::to_hex(block_key) << " key 0x" << silkworm::to_hex(s_kv.key1) << "\n";

                auto data = co_await acs_cursor->seek_both(block_key, s_kv.key1);
                SILKRPC_TRACE << "seek_both: data 0x" << silkworm::to_hex(data) << "\n";

                if (data.size() > silkworm::kAddressLength) {
                    data = data.substr(silkworm::kAddressLength);
                    go_on = collector(s_kv.key1, data);
                    SILKRPC_TRACE << "ITERATE **  COLLECTOR CALLED: key 0x" << silkworm::to_hex(s_kv.key1) << " data 0x" << silkworm::to_hex(data) << " go_on " << go_on << "\n";
                } else {
                    SILKRPC_TRACE << "Empty data for account at address 0x" << silkworm::to_hex(s_kv.key1) << " SKIPPED\n";
                }
            } else if (cmp == 0) {
                go_on = collector(ps_kv.key, ps_kv.value);
                SILKRPC_TRACE << "ITERATE **  COLLECTOR CALLED: key 0x" << silkworm::to_hex(ps_kv.key) << " value 0x" << silkworm::to_hex(ps_kv.value) << " go_on " << go_on << "\n";
            }
        }

        if (go_on) {
            if (cmp <= 0) {
                ps_kv = co_await next(*ps_cursor, silkworm::kAddressLength);
            }
            if (cmp >= 0) {
                auto block = silkworm::endian::load_big_u64(s_kv.key2.data());
                s_kv = co_await next(split_cursor, block_number, block, s_kv.key1);
                SILKRPC_TRACE << "WalkAsOfStorage: key1 new value 0x" << silkworm::to_hex(s_kv.key1) << "\n";
            }
        }

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;
        SILKRPC_TRACE << "ITERATE ************************* " << count++ << " in: " << elapsed_seconds.count() << "\n";
    }
}

asio::awaitable<KeyValue> AccountWalker::next(silkrpc::ethdb::Cursor& cursor, uint64_t len) {
    auto kv = co_await cursor.next();
    // SILKRPC_TRACE << "Curson on PlainState NEXT: key 0x" << silkworm::to_hex(kv.key) << " value 0x" << silkworm::to_hex(kv.value) << "\n";

    while (!kv.key.empty() && kv.key.size() > len) {
        kv = co_await cursor.next();
        SILKRPC_TRACE << "Curson on PlainState NEXT: key 0x" << silkworm::to_hex(kv.key) << " value 0x" << silkworm::to_hex(kv.value) << "\n";
    }
    co_return kv;
}

asio::awaitable<KeyValue> AccountWalker::seek(silkrpc::ethdb::Cursor& cursor, silkworm::ByteView key, uint64_t len) {
    auto kv = co_await cursor.seek(key);
    // SILKRPC_TRACE << "Curson on PlainState SEEK: key 0x" << silkworm::to_hex(kv.key) << " value 0x" << silkworm::to_hex(kv.value) << "\n";

    if (kv.key.size() > len) {
        co_return co_await next(cursor, len);
    }
    co_return kv;
}

asio::awaitable<silkrpc::ethdb::SplittedKeyValue> AccountWalker::next(silkrpc::ethdb::SplitCursor& cursor, uint64_t number, uint64_t block, silkworm::Bytes addr) {
    silkrpc::ethdb::SplittedKeyValue skv;
    auto tmp_addr = addr;
    while (!addr.empty() && (tmp_addr == addr || block < number)) {
        skv = co_await cursor.next();
        // SILKRPC_TRACE << "Curson on AccountHistory NEXT: key1 0x" << silkworm::to_hex(skv.key1) << " key2 0x " << silkworm::to_hex(skv.key2) << " v " << silkworm::to_hex(skv.value) << "\n";

        if (skv.key1.empty()) {
            break;
        }
        block = silkworm::endian::load_big_u64(skv.key2.data());
        addr = skv.key1;
    }
    co_return skv;
}

asio::awaitable<silkrpc::ethdb::SplittedKeyValue> AccountWalker::seek(silkrpc::ethdb::SplitCursor& cursor, uint64_t number) {
    auto kv = co_await cursor.seek();
    // SILKRPC_TRACE << "Curson on AccountHistory SEEK: addr 0x" << silkworm::to_hex(kv.key1) << " block " << silkworm::to_hex(kv.key2) << " v " << silkworm::to_hex(kv.value) << "\n";

    if (kv.key1.empty()) {
        co_return kv;
    }

    uint64_t block = silkworm::endian::load_big_u64(kv.key2.data());
    // SILKRPC_TRACE << "Curson on AccountHistory NEXT:" << " addr 0x" <<  silkworm::to_hex(kv.key1) << " block " << std::dec << block << " v " << silkworm::to_hex(kv.value) << "\n";

    while (block < number) {
        kv = co_await cursor.next();
        if (kv.key2.empty()) {
            break;
        }
        block = silkworm::endian::load_big_u64(kv.key2.data());
        // SILKRPC_TRACE << "Curson on AccountHistory NEXT: addr 0x" <<  silkworm::to_hex(kv.key1) <<" block " << std::dec << block << " v " << silkworm::to_hex(kv.value) << "\n";
    }

    co_return kv;
}

} // namespace silkrpc
