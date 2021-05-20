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

#include "state_reader.hpp"

#include <silkworm/common/util.hpp>
#include <silkworm/db/access_layer.hpp>
#include <silkworm/db/bitmap.hpp>
#include <silkworm/db/tables.hpp>
#include <silkworm/db/util.hpp>
#include <silkworm/types/account.hpp>

#include <silkrpc/common/log.hpp>

namespace silkrpc {

asio::awaitable<std::optional<silkworm::Account>> StateReader::read_account(const evmc::address& address, uint64_t block_number) const {
    std::optional<silkworm::Bytes> encoded{co_await read_historical_account(address, block_number)};
    if (!encoded) {
        encoded = co_await db_reader_.get_one(silkworm::db::table::kPlainState.name, silkworm::full_view(address));
    }
    if (!encoded || encoded->empty()) {
        co_return std::nullopt;
    }

    auto [account, err]{silkworm::decode_account_from_storage(*encoded)};
    silkworm::rlp::err_handler(err); // TODO(canepat) suggest rename as throw_if_error or better throw_if(err != kOk)

    if (account.incarnation > 0 && account.code_hash == silkworm::kEmptyHash) {
        // Restore code hash
        const auto storage_key{silkworm::db::storage_prefix(address, account.incarnation)};
        auto code_hash{co_await db_reader_.get_one(silkworm::db::table::kPlainContractCode.name, storage_key)};
        if (code_hash.length() == silkworm::kHashLength) {
            std::memcpy(account.code_hash.bytes, code_hash.data(), silkworm::kHashLength);
        }
    }

    co_return account;
}

asio::awaitable<std::optional<silkworm::Bytes>> StateReader::read_storage(const evmc::address& address, uint64_t incarnation,
    const evmc::bytes32& location_hash, uint64_t block_number) const {
    co_return silkworm::Bytes{};
}

asio::awaitable<std::optional<silkworm::Bytes>> StateReader::read_code(const evmc::address& address, uint64_t incarnation,
    const evmc::bytes32& code_hash, uint64_t block_number) const {
    if (code_hash == silkworm::kEmptyHash) {
        co_return std::nullopt;
    }
    auto code{co_await db_reader_.get_one(silkworm::db::table::kCode.name, silkworm::full_view(code_hash))};
    co_return code;
}

asio::awaitable<std::optional<silkworm::ByteView>> StateReader::read_historical_account(const evmc::address& address, uint64_t block_number) const {
    const auto account_history_key{silkworm::db::account_history_key(address, block_number)};
    SILKRPC_DEBUG << "StateReader::read_historical_account account_history_key: " << account_history_key << "\n";
    const auto kv_pair{co_await db_reader_.get(silkworm::db::table::kAccountHistory.name, account_history_key)};

    if (!silkworm::has_prefix(kv_pair.key, silkworm::full_view(address))) {
        co_return std::nullopt;
    }

    const auto bitmap{silkworm::db::bitmap::read(kv_pair.value)};
    SILKRPC_DEBUG << "StateReader::read_historical_account bitmap: " << bitmap.toString() << "\n";

    const auto change_block{silkworm::db::bitmap::seek(bitmap, block_number)};
    if (!change_block) {
        co_return std::nullopt;
    }

    const auto block_key{silkworm::db::block_key(*change_block)};
    SILKRPC_DEBUG << "StateReader::read_historical_account block_key: " << block_key << "\n";
    const auto address_subkey{silkworm::full_view(address)};
    SILKRPC_DEBUG << "StateReader::read_historical_account address_subkey: " << address_subkey << "\n";
    const auto value{co_await db_reader_.get_both_range(silkworm::db::table::kPlainAccountChangeSet.name, block_key, address_subkey)};
    SILKRPC_DEBUG << "StateReader::read_historical_account value: " << (value ? *value : silkworm::ByteView{}) << "\n";

    co_return value;
}

asio::awaitable<std::optional<silkworm::ByteView>> StateReader::read_historical_storage(const evmc::address& address, uint64_t incarnation,
    const evmc::bytes32& location_hash, uint64_t block_number) const {
    co_return silkworm::ByteView{};
}

} // namespace silkrpc
