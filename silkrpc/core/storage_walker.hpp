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

#ifndef SILKRPC_CORE_STORAGE_WALKER_HPP_
#define SILKRPC_CORE_STORAGE_WALKER_HPP_

#include <optional>
#include <map>

#include <nlohmann/json.hpp>
#include <silkrpc/config.hpp>

#include <asio/awaitable.hpp>
#include <evmc/evmc.hpp>
#include <silkworm/common/util.hpp>
#include <silkworm/types/account.hpp>

#include <silkrpc/common/util.hpp>
#include <silkrpc/core/rawdb/accessors.hpp>
#include <silkrpc/ethdb/cursor.hpp>
#include <silkrpc/ethdb/database.hpp>
#include <silkrpc/types/block.hpp>

namespace silkrpc {

silkworm::Bytes make_key(const evmc::address& address, const evmc::bytes32& location);
silkworm::Bytes make_key(uint64_t block_number, const evmc::address& address);
silkworm::Bytes make_key(const evmc::address& address, uint64_t incarnation, const evmc::bytes32& location);

class StorageWalker {
public:
    using Collector = std::function<bool(const evmc::address&, const silkworm::Bytes&, const silkworm::Bytes&)>;

    explicit StorageWalker(silkrpc::ethdb::Transaction& transaction) : transaction_(transaction) {}

    StorageWalker(const StorageWalker&) = delete;
    StorageWalker& operator=(const StorageWalker&) = delete;

    asio::awaitable<void> walk_of_storages(uint64_t block_number,
        const evmc::address& start_address, const evmc::bytes32& start_location, uint64_t incarnation, Collector& collector);

private:
    silkrpc::ethdb::Transaction& transaction_;
};

} // namespace silkrpc

#endif  // SILKRPC_CORE_STORAGE_WALKER_HPP_
