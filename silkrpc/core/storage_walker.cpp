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


#include "storage_walker.hpp"

#include <sstream>

#include <boost/endian/conversion.hpp>

#include <silkworm/core/silkworm/common/endian.hpp>
#include <silkworm/node/silkworm/db/bitmap.hpp>
#include <silkworm/node/silkworm/db/util.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/core/rawdb/chain.hpp>
#include <silkrpc/core/state_reader.hpp>
#include <silkrpc/ethdb/cursor.hpp>
#include <silkrpc/ethdb/tables.hpp>
#include <silkrpc/ethdb/transaction_database.hpp>
#include <silkrpc/json/types.hpp>

namespace silkrpc {

silkworm::Bytes make_key(const evmc::address& address, const evmc::bytes32& location) {
    silkworm::Bytes res(silkworm::kAddressLength + silkworm::kHashLength, '\0');
    std::memcpy(&res[0], address.bytes, silkworm::kAddressLength);
    std::memcpy(&res[silkworm::kAddressLength], location.bytes, silkworm::kHashLength);
    return res;
}

silkworm::Bytes make_key(uint64_t block_number, const evmc::address& address) {
    silkworm::Bytes res(8 + silkworm::kAddressLength, '\0');
    boost::endian::store_big_u64(&res[0], block_number);
    std::memcpy(&res[8], address.bytes, silkworm::kAddressLength);
    return res;
}

silkworm::Bytes make_key(const evmc::address& address, uint64_t incarnation, const evmc::bytes32& location) {
    silkworm::Bytes res(silkworm::kAddressLength + silkworm::kHashLength + 8, '\0');
    std::memcpy(&res[0], address.bytes, silkworm::kAddressLength);
    boost::endian::store_big_u64(&res[silkworm::kAddressLength], incarnation);
    std::memcpy(&res[silkworm::kAddressLength + 8], location.bytes, silkworm::kHashLength);
    return res;
}

asio::awaitable<silkrpc::ethdb::SplittedKeyValue> next(silkrpc::ethdb::SplitCursor& cursor, uint64_t number) {
    auto kv = co_await cursor.next();
    if (kv.key2.empty()) {
        co_return kv;
    }
    uint64_t block = silkworm::endian::load_big_u64(kv.key3.data());
    SILKRPC_TRACE << "Curson on StorageHistory NEXT"
        << " addr 0x" <<  silkworm::to_hex(kv.key1)
        << " loc " << silkworm::to_hex(kv.key2)
        << " tsEnc " << silkworm::to_hex(kv.key3)
        << " v " << silkworm::to_hex(kv.value)
        << "\n";

    while (block < number) {
        kv = co_await cursor.next();
        if (kv.key2.empty()) {
            break;
        }
        block = silkworm::endian::load_big_u64(kv.key3.data());
        SILKRPC_TRACE << "Curson on StorageHistory NEXT"
            << " addr 0x" <<  silkworm::to_hex(kv.key1)
            << " loc " << silkworm::to_hex(kv.key2)
            << " tsEnc " << silkworm::to_hex(kv.key3)
            << " v " << silkworm::to_hex(kv.value)
            << "\n";
    }
    co_return kv;
}

asio::awaitable<void> StorageWalker::walk_of_storages(uint64_t block_number, const evmc::address& start_address,
        const evmc::bytes32& location_hash, uint64_t incarnation, Collector& collector) {
    auto ps_cursor = co_await transaction_.cursor(db::table::kPlainState);

    auto ps_key{make_key(start_address, incarnation, location_hash)};
    silkrpc::ethdb::SplitCursor ps_split_cursor{*ps_cursor,
        ps_key,
        8 * (silkworm::kAddressLength + 8),
        silkworm::kAddressLength,
        silkworm::kAddressLength + 8,
        silkworm::kAddressLength + 8 + silkworm::kHashLength};

    SILKRPC_TRACE << "Curson on PlainState"
        << " ps_key 0x" <<  silkworm::to_hex(ps_key)
        << " key len " <<  ps_key.length()
        << " match_bits " << 8 * (silkworm::kAddressLength + 8)
        << " length1 " << silkworm::kAddressLength
        << " length2 " << 8
        << "\n";

    auto sh_key{make_key(start_address, location_hash)};
    auto sh_cursor = co_await transaction_.cursor(db::table::kStorageHistory);
    silkrpc::ethdb::SplitCursor sh_split_cursor{*sh_cursor,
        sh_key,
        8 * silkworm::kAddressLength,
        silkworm::kAddressLength,
        silkworm::kAddressLength,
        silkworm::kAddressLength + silkworm::kHashLength};

    SILKRPC_TRACE << "Curson on StorageHistory"
        << " sh_key 0x" <<  silkworm::to_hex(sh_key)
        << " key len " <<  sh_key.length()
        << " match_bits " << 8 * silkworm::kAddressLength
        << " length1 " << silkworm::kAddressLength
        << " length2 " << 8
        << "\n";

    auto ps_skv = co_await ps_split_cursor.seek();
    SILKRPC_TRACE << "Curson on PlainState SEEK"
        << " addr 0x" <<  silkworm::to_hex(ps_skv.key1)
        << " loc " << silkworm::to_hex(ps_skv.key2)
        << " kk " << silkworm::to_hex(ps_skv.key3)
        << " v " << silkworm::to_hex(ps_skv.value)
        << "\n";

    auto sh_skv = co_await sh_split_cursor.seek();
    uint64_t block = silkworm::endian::load_big_u64(sh_skv.key3.data());
    SILKRPC_TRACE << "Curson on StorageHistory SEEK"
        << " addr 0x" <<  silkworm::to_hex(sh_skv.key1)
        << " loc " << silkworm::to_hex(sh_skv.key2)
        << " tsEnc " << silkworm::to_hex(sh_skv.key3)
        << " v " << silkworm::to_hex(sh_skv.value)
        << "\n";

    auto cs_cursor = co_await transaction_.cursor_dup_sort(db::table::kPlainStorageChangeSet);

    if (block < block_number) {
        sh_skv = co_await next(sh_split_cursor, block_number);
    }

    auto count = 0;
    auto go_on = true;
    while (go_on && count < 10) {
        count++;
        if (ps_skv.key1.empty() && sh_skv.key1.empty()) {
            SILKRPC_TRACE << "Both keys1 are empty: break loop\n";
            break;
        }
        auto cmp = ps_skv.key1.compare(sh_skv.key1);
        SILKRPC_TRACE << "ITERATE **  KeyCmp: addr 0x" << silkworm::to_hex(ps_skv.key1)
            << " hAddr 0x" << silkworm::to_hex(sh_skv.key1)
            << " cmp " << cmp
            << "\n";

        if (cmp == 0) {
            if (ps_skv.key2.empty() && sh_skv.key2.empty()) {
                SILKRPC_TRACE << "Both keys2 are empty: break loop\n";
                break;
            }
            auto cmp = ps_skv.key2.compare(sh_skv.key2);
            SILKRPC_TRACE << "ITERATE **  KeyCmp: loc 0x" << silkworm::to_hex(ps_skv.key2)
                << " hLoc 0x" << silkworm::to_hex(sh_skv.key2)
                << " cmp " << cmp
                << "\n";
        }
        if (cmp < 0) {
            auto address = silkworm::to_address(ps_skv.key1);
            go_on = collector(address, ps_skv.key2, ps_skv.value);
            SILKRPC_TRACE << "ITERATE **  COLLECTOR CALLED: address 0x" << silkworm::to_hex(address)
                << " loc 0x" << silkworm::to_hex(ps_skv.key2)
                << " data 0x" << silkworm::to_hex(ps_skv.value)
                << " go_on " << go_on
                << "\n";
        } else {
            SILKRPC_TRACE << "ITERATE ** built roaring64 from " << silkworm::to_hex(sh_skv.value) <<  "\n";
            const auto bitmap = silkworm::db::bitmap::read(sh_skv.value);

            std::optional<silkworm::Account> result;
            if (bitmap.contains(block_number)) {
                auto dup_key{silkworm::db::storage_change_key(block_number, start_address, incarnation)};

                SILKRPC_TRACE << "Curson on StorageHistory"
                    << " dup_key 0x" <<  silkworm::to_hex(dup_key)
                    << " key len " <<  dup_key.length()
                    << " hLoc " << silkworm::to_hex(sh_skv.key2)
                    << "\n";

                auto data = co_await cs_cursor->seek_both(dup_key, sh_skv.key2);
                SILKRPC_TRACE << "Curson on StorageHistory"
                    << " found data 0x" <<  silkworm::to_hex(data)
                    << "\n";

                data = data.substr(silkworm::kHashLength);
                if (data.length() > 0) { // Skip deleted entries
                    auto address = silkworm::to_address(ps_skv.key1);
                    go_on = collector(address, ps_skv.key2, data);
                    SILKRPC_TRACE << "ITERATE **  COLLECTOR CALLED: address 0x" << silkworm::to_hex(address)
                        << " loc 0x" << silkworm::to_hex(sh_skv.key2)
                        << " data 0x" << silkworm::to_hex(data)
                        << " go_on " << go_on
                        << "\n";
                }
            } else if (cmp == 0) {
                auto address = silkworm::to_address(ps_skv.key1);
                go_on = collector(address, ps_skv.key2, ps_skv.value);
                SILKRPC_TRACE << "ITERATE **  COLLECTOR CALLED: address 0x" << silkworm::to_hex(address)
                    << " loc 0x" << silkworm::to_hex(ps_skv.key2)
                    << " data 0x" << silkworm::to_hex(ps_skv.value)
                    << " go_on " << go_on
                    << "\n";
            }
        }
        if (go_on) {
            if (cmp <= 0) {
                ps_skv = co_await ps_split_cursor.next();
                SILKRPC_TRACE << "Curson on PlainState NEXT"
                    << " addr 0x" <<  silkworm::to_hex(ps_skv.key1)
                    << " loc " << silkworm::to_hex(ps_skv.key2)
                    << " kk " << silkworm::to_hex(ps_skv.key3)
                    << " v " << silkworm::to_hex(ps_skv.value)
                    << "\n";
            }
            if (cmp >= 0) {
                sh_skv = co_await next(sh_split_cursor, block_number);
            }
        }
    }

    co_return;
}

} // namespace silkrpc
