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

#include "chain.hpp"

#include <boost/endian/conversion.hpp>

#include <silkworm/core/silkworm/common/util.hpp>
#include <silkworm/core/silkworm/rlp/decode.hpp>
#include <silkworm/db/silkworm/db/tables.hpp>
#include <silkworm/db/silkworm/db/util.hpp>

namespace silkrpc::core::rawdb {

asio::awaitable<uint64_t> read_header_number(DatabaseReader& reader, evmc::bytes32 block_hash) {
    silkworm::ByteView value{co_await reader.get(silkworm::db::table::kHeaderNumbers.name, block_hash.bytes)};
    if (value.empty()) {
        co_return 0;
    }
    co_return boost::endian::load_big_u64(value.data());
}

asio::awaitable<evmc::bytes32> read_canonical_block_hash(DatabaseReader& reader, uint64_t block_number) {
    const auto header_hash_key = silkworm::db::header_hash_key(block_number);
    auto data = co_await reader.get(silkworm::db::table::kBlockHeaders.name, header_hash_key);
    if (data.empty()) {
        co_return evmc::bytes32{};
    }
    co_return silkworm::to_bytes32(data);
}

asio::awaitable<silkworm::BlockWithHash> read_block_by_number(DatabaseReader& reader, uint64_t block_number) {
    auto block_hash = co_await read_canonical_block_hash(reader, block_number);
    if (!block_hash) {
        co_return silkworm::BlockWithHash{};
    }
    co_return co_await read_block(reader, block_hash, block_number);
}

asio::awaitable<silkworm::BlockWithHash> read_block(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number) {
    auto header = co_await read_header(reader, block_hash, block_number);
    if (header == silkworm::BlockHeader{}) {
        co_return silkworm::BlockWithHash{};
    }
    auto body = co_await read_body(reader, block_hash, block_number);
    if (body == silkworm::BlockBody{}) {
        co_return silkworm::BlockWithHash{};
    }
    silkworm::BlockWithHash block{silkworm::Block{body.transactions, body.ommers, header}, block_hash};
    co_return block;
}

asio::awaitable<silkworm::BlockHeader> read_header(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number) {
    auto data = co_await read_header_rlp(reader, block_hash, block_number);
    if (data.empty()) {
        co_return silkworm::BlockHeader{};
    }
    silkworm::ByteView data_view{data};
    silkworm::BlockHeader header{};
    silkworm::rlp::decode(data_view, header);
    co_return header;
}

asio::awaitable<silkworm::BlockBody> read_body(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number) {
    auto data = co_await read_body_rlp(reader, block_hash, block_number);
    if (data.empty()) {
        co_return silkworm::BlockBody{};
    }
    silkworm::ByteView data_view{data};
    silkworm::BlockBody body{};
    silkworm::rlp::decode(data_view, body);
    co_return body;
}

asio::awaitable<silkworm::Bytes> read_header_rlp(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number) {
    const auto header_hash_key = silkworm::db::header_hash_key(block_number);
    auto data = co_await reader.get(silkworm::db::table::kBlockHeaders.name, header_hash_key);
    co_return data;
}

asio::awaitable<silkworm::Bytes> read_body_rlp(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number) {
    const auto block_key = silkworm::db::block_key(block_number, block_hash.bytes);
    auto data = co_await reader.get(silkworm::db::table::kBlockBodies.name, block_key);
    co_return data;
}

} // namespace silkrpc::core::rawdb