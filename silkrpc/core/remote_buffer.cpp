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

#include "remote_buffer.hpp"

#include <future>
#include <unordered_map>
#include <utility>

#include <asio/co_spawn.hpp>
#include <asio/use_future.hpp>
#include <silkworm/common/util.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/core/blocks.hpp>
#include <silkrpc/core/rawdb/chain.hpp>

namespace silkrpc::state {

static std::unordered_map<evmc::bytes32, silkworm::Bytes> code;

asio::awaitable<std::optional<silkworm::Account>> AsyncRemoteBuffer::read_account(const evmc::address& address) const noexcept {
    co_return co_await state_reader_.read_account(address, block_number_ + 1);
}

asio::awaitable<silkworm::ByteView> AsyncRemoteBuffer::read_code(const evmc::bytes32& code_hash) const noexcept {
    const auto optional_code{co_await state_reader_.read_code(code_hash)};
    if (optional_code) {
        code[code_hash] = std::move(*optional_code);
        co_return code[code_hash]; // NOLINT(runtime/arrays)
    }
    co_return silkworm::ByteView{};
}

asio::awaitable<evmc::bytes32> AsyncRemoteBuffer::read_storage(const evmc::address& address, uint64_t incarnation, const evmc::bytes32& location) const noexcept {
    co_return co_await state_reader_.read_storage(address, incarnation, location, block_number_ + 1);
}

asio::awaitable<uint64_t> AsyncRemoteBuffer::previous_incarnation(const evmc::address& address) const noexcept {
    co_return 0;
}

asio::awaitable<std::optional<silkworm::BlockHeader>> AsyncRemoteBuffer::read_header(uint64_t block_number, const evmc::bytes32& block_hash) const noexcept {
    co_return co_await core::rawdb::read_header(db_reader_, block_hash, block_number);
}

asio::awaitable<std::optional<silkworm::BlockBody>> AsyncRemoteBuffer::read_body(uint64_t block_number, const evmc::bytes32& block_hash) const noexcept {
    co_return co_await core::rawdb::read_body(db_reader_, block_hash, block_number);
}

asio::awaitable<std::optional<intx::uint256>> AsyncRemoteBuffer::total_difficulty(uint64_t block_number, const evmc::bytes32& block_hash) const noexcept {
    co_return co_await core::rawdb::read_total_difficulty(db_reader_, block_hash, block_number);
}

asio::awaitable<evmc::bytes32> AsyncRemoteBuffer::state_root_hash() const {
    co_return evmc::bytes32{};
}

asio::awaitable<uint64_t> AsyncRemoteBuffer::current_canonical_block() const {
    // This method should not be called by EVM::execute
    co_return 0;
}

asio::awaitable<std::optional<evmc::bytes32>> AsyncRemoteBuffer::canonical_hash(uint64_t block_number) const {
    // This method should not be called by EVM::execute
    co_return co_await core::rawdb::read_canonical_block_hash(db_reader_, block_number);
}

std::optional<silkworm::Account> RemoteBuffer::read_account(const evmc::address& address) const noexcept {
    SILKRPC_DEBUG << "RemoteBuffer::read_account address=" << address << " start\n";
    try {
        std::future<std::optional<silkworm::Account>> result{asio::co_spawn(io_context_, async_buffer_.read_account(address), asio::use_future)};
        const auto optional_account{result.get()};
        SILKRPC_DEBUG << "RemoteBuffer::read_account account.nonce=" << (optional_account ? optional_account->nonce : 0) << " end\n";
        return optional_account;
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "RemoteBuffer::read_account exception: " << e.what() << "\n";
        return std::nullopt;
    }
}

silkworm::ByteView RemoteBuffer::read_code(const evmc::bytes32& code_hash) const noexcept {
    SILKRPC_DEBUG << "RemoteBuffer::read_code code_hash=" << code_hash << " start\n";
    try {
        std::future<silkworm::ByteView> result{asio::co_spawn(io_context_, async_buffer_.read_code(code_hash), asio::use_future)};
        const auto code{result.get()};
        return code;
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "RemoteBuffer::read_code exception: " << e.what() << "\n";
        return silkworm::ByteView{};
    }
}

evmc::bytes32 RemoteBuffer::read_storage(const evmc::address& address, uint64_t incarnation, const evmc::bytes32& location) const noexcept {
    SILKRPC_DEBUG << "RemoteBuffer::read_storage address=" << address << " incarnation=" << incarnation << " location=" << location << " start\n";
    try {
        std::future<evmc::bytes32> result{asio::co_spawn(io_context_, async_buffer_.read_storage(address, incarnation, location), asio::use_future)};
        const auto storage_value{result.get()};
        SILKRPC_DEBUG << "RemoteBuffer::read_storage storage_value=" << storage_value << " end\n";
        return storage_value;
    } catch (const std::exception& e) {
       SILKRPC_ERROR << "RemoteBuffer::read_storage exception: " << e.what() << "\n";
       return evmc::bytes32{};
    }
}

uint64_t RemoteBuffer::previous_incarnation(const evmc::address& address) const noexcept {
    SILKRPC_DEBUG << "RemoteBuffer::previous_incarnation address=" << address << "\n";
    return 0;
}

std::optional<silkworm::BlockHeader> RemoteBuffer::read_header(uint64_t block_number, const evmc::bytes32& block_hash) const noexcept {
    SILKRPC_DEBUG << "RemoteBuffer::read_header block_number=" << block_number << " block_hash=" << block_hash << "\n";
    try {
        std::future<std::optional<silkworm::BlockHeader>> result{asio::co_spawn(io_context_, async_buffer_.read_header(block_number, block_hash), asio::use_future)};
        const auto optional_header{result.get()};
        SILKRPC_DEBUG << "RemoteBuffer::read_header block_number=" << block_number << " block_hash=" << block_hash << "\n";
        return optional_header;
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "RemoteBuffer::read_header exception: " << e.what() << "\n";
        return std::nullopt;
    }
}

bool RemoteBuffer::read_body(uint64_t block_number, const evmc::bytes32& block_hash, silkworm::BlockBody& filled_body) const noexcept {
    SILKRPC_DEBUG << "RemoteBuffer::read_body block_number=" << block_number << " block_hash=" << block_hash << "\n";
    try {
        std::future<std::optional<silkworm::BlockBody>> result{asio::co_spawn(io_context_, async_buffer_.read_body(block_number, block_hash), asio::use_future)};
        const auto optional_body{result.get()};
        SILKRPC_DEBUG << "RemoteBuffer::read_body block_number=" << block_number << " block_hash=" << block_hash << "\n";
        filled_body = *optional_body;
        return true;
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "RemoteBuffer::read_body exception: " << e.what() << "\n";
        return false;
    }
}

std::optional<intx::uint256> RemoteBuffer::total_difficulty(uint64_t block_number, const evmc::bytes32& block_hash) const noexcept {
    SILKRPC_DEBUG << "RemoteBuffer::total_difficulty block_number=" << block_number << " block_hash=" << block_hash << "\n";
    try {
        std::future<std::optional<intx::uint256>> result{asio::co_spawn(io_context_, async_buffer_.total_difficulty(block_number, block_hash), asio::use_future)};
        const auto optional_total_difficulty{result.get()};
        SILKRPC_DEBUG << "RemoteBuffer::total_difficulty block_number=" << block_number << " block_hash=" << block_hash << "\n";
        return optional_total_difficulty;
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "RemoteBuffer::total_difficulty exception: " << e.what() << "\n";
        return std::nullopt;
    }
}

evmc::bytes32 RemoteBuffer::state_root_hash() const {
    SILKRPC_DEBUG << "RemoteBuffer::state_root_hash\n";
    return evmc::bytes32{};
}

uint64_t RemoteBuffer::current_canonical_block() const {
    SILKRPC_DEBUG << "RemoteBuffer::current_canonical_block\n";
    return 0;
}

std::optional<evmc::bytes32> RemoteBuffer::canonical_hash(uint64_t block_number) const {
    SILKRPC_DEBUG << "RemoteBuffer::canonical_hash block_number=" << block_number << "\n";
    return std::nullopt;
}

} // namespace silkrpc::state
