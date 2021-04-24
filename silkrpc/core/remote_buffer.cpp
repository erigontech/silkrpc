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

namespace silkrpc {

std::optional<silkworm::Account> RemoteBuffer::read_account(const evmc::address& address) const noexcept {
    return std::nullopt;
}

silkworm::Bytes RemoteBuffer::read_code(const evmc::bytes32& code_hash) const noexcept {
    return silkworm::Bytes{};
}

evmc::bytes32 RemoteBuffer::read_storage(const evmc::address& address, uint64_t incarnation, const evmc::bytes32& location) const noexcept {
    return evmc::bytes32{};
}

uint64_t RemoteBuffer::previous_incarnation(const evmc::address& address) const noexcept {
    return 0;
}

std::optional<silkworm::BlockHeader> RemoteBuffer::read_header(uint64_t block_number, const evmc::bytes32& block_hash) const noexcept {
    return std::nullopt;
}

std::optional<silkworm::BlockBody> RemoteBuffer::read_body(uint64_t block_number, const evmc::bytes32& block_hash) const noexcept {
    return std::nullopt;
}

std::optional<intx::uint256> RemoteBuffer::total_difficulty(uint64_t block_number, const evmc::bytes32& block_hash) const noexcept {
    return std::nullopt;
}

evmc::bytes32 RemoteBuffer::state_root_hash() const {
    return evmc::bytes32{};
}

uint64_t RemoteBuffer::current_canonical_block() const {
    return 0;
}

std::optional<evmc::bytes32> RemoteBuffer::canonical_hash(uint64_t block_number) const {
    return std::nullopt;
}

silkworm::Bytes RemoteBuffer::db_get(const std::string& table, const silkworm::ByteView& key) const {
    return silkworm::Bytes{};
}

} // namespace silkrpc
