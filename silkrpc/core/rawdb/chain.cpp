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

#include <iterator>
#include <iostream>
#include <iomanip>

#include <boost/endian/conversion.hpp>
#include <nlohmann/json.hpp>

#include <silkworm/common/util.hpp>
#include <silkworm/db/silkworm/db/tables.hpp>
#include <silkworm/db/silkworm/db/util.hpp>
#include <silkworm/execution/address.hpp>
#include <silkworm/rlp/decode.hpp>
#include <silkworm/rlp/encode.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/json/types.hpp>
#include <silkrpc/types/log.hpp>
#include <silkrpc/types/receipt.hpp>

// TODO: move to db/types/log_cbor.*,receipt_cbor.*
namespace silkrpc::core {

void cbor_decode(const silkworm::Bytes& bytes, std::vector<Log>& logs) {
    if (bytes.size() == 0) {
        return;
    }
    auto json = nlohmann::json::from_cbor(bytes);
    if (json.is_array()) {
        logs = json.get<std::vector<Log>>();
    }
}

void cbor_decode(const silkworm::Bytes& bytes, std::vector<Receipt>& receipts) {
    if (bytes.size() == 0) {
        return;
    }
    auto json = nlohmann::json::from_cbor(bytes);
    if (json.is_array()) {
        receipts = json.get<std::vector<Receipt>>();
    }
}

} // namespace silkrpc::core

namespace silkrpc::core::rawdb {

asio::awaitable<uint64_t> read_header_number(DatabaseReader& reader, evmc::bytes32 block_hash) {
    silkworm::ByteView value{co_await reader.get(silkworm::db::table::kHeaderNumbers.name, block_hash.bytes)};
    if (value.empty()) {
        throw std::runtime_error{"empty block number value in read_header_number"};
    }
    co_return boost::endian::load_big_u64(value.data());
}

asio::awaitable<evmc::bytes32> read_canonical_block_hash(DatabaseReader& reader, uint64_t block_number) {
    const auto block_key = silkworm::db::block_key(block_number);
    auto data = co_await reader.get(silkworm::db::table::kCanonicalHashes.name, block_key);
    if (data.empty()) {
        throw std::runtime_error{"empty block hash value in read_header_number"};
    }
    co_return silkworm::to_bytes32(data);
}

asio::awaitable<intx::uint256> read_total_difficulty(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number) {
    co_return 42;
}

asio::awaitable<silkworm::BlockWithHash> read_block_by_hash(DatabaseReader& reader, evmc::bytes32 block_hash) {
    const auto block_number = co_await read_header_number(reader, block_hash);
    co_return co_await read_block(reader, block_hash, block_number);
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
    SILKRPC_TRACE << "data: " << silkworm::to_hex(data) << "\n";
    silkworm::ByteView data_view{data};
    silkworm::BlockHeader header{};
    const auto error = silkworm::rlp::decode(data_view, header);
    if (error != silkworm::rlp::DecodingResult::kOk) {
        SILKRPC_ERROR << "invalid RLP decoding for block header #" << block_number << "\n";
        co_return silkworm::BlockHeader{};
    }
    co_return header;
}

asio::awaitable<silkworm::BlockBody> read_body(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number) {
    const auto data = co_await read_body_rlp(reader, block_hash, block_number);
    if (data.empty()) {
        co_return silkworm::BlockBody{};
    }
    SILKRPC_TRACE << "RLP data for block body #" << block_number << ": " << silkworm::to_hex(data) << "\n";

    try {
        silkworm::ByteView data_view{data};
        auto stored_body{silkworm::db::detail::decode_stored_block_body(data_view)};
        SILKRPC_TRACE << "base_txn_id: " << stored_body.base_txn_id << " txn_count: " << stored_body.txn_count << "\n";
        auto transactions = co_await read_transactions(reader, stored_body.base_txn_id, stored_body.txn_count);

        silkworm::BlockBody body{transactions, stored_body.ommers};
        co_return body;
    } catch (silkworm::rlp::DecodingResult error) {
        SILKRPC_ERROR << "RLP decoding error for block body #" << block_number << " [" << static_cast<int>(error) << "]\n";
        co_return silkworm::BlockBody{};
    }
}

asio::awaitable<silkworm::Bytes> read_header_rlp(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number) {
    const auto block_key = silkworm::db::block_key(block_number, block_hash.bytes);
    const auto data = co_await reader.get(silkworm::db::table::kHeaders.name, block_key);
    co_return data;
}

asio::awaitable<silkworm::Bytes> read_body_rlp(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number) {
    const auto block_key = silkworm::db::block_key(block_number, block_hash.bytes);
    const auto data = co_await reader.get(silkworm::db::table::kBlockBodies.name, block_key);
    co_return data;
}

asio::awaitable<Addresses> read_senders(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number) {
    const auto block_key = silkworm::db::block_key(block_number, block_hash.bytes);
    const auto data = co_await reader.get(silkworm::db::table::kSenders.name, block_key);
    Addresses senders{data.size() / silkworm::kAddressLength};
    for (size_t i{0}; i < senders.size(); i++) {
        senders[i] = silkworm::to_address(silkworm::ByteView{&data[i * silkworm::kAddressLength], silkworm::kAddressLength});
    }
    co_return senders;
}

asio::awaitable<Receipts> read_raw_receipts(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number) {
    const auto block_key = silkworm::db::block_key(block_number);
    const auto data = co_await reader.get(silkworm::db::table::kBlockReceipts.name, block_key);
    if (data.empty()) {
        co_return Receipts{}; // TODO: use std::null_opt with asio::awaitable<std::optional<Receipts>>?
    }
    Receipts receipts{};
    cbor_decode(data, receipts);
    SILKRPC_TRACE << "#receipts: " << receipts.size() << "\n";

    auto log_key = silkworm::db::log_key(block_number, 0);
    SILKRPC_TRACE << "log_key: " << silkworm::to_hex(log_key) << "\n";
    Walker walker = [&](const silkworm::Bytes& k, const silkworm::Bytes& v) {
        auto tx_id = boost::endian::load_big_u32(&k[sizeof(uint64_t)]);
        cbor_decode(v, receipts[tx_id].logs);
        SILKRPC_TRACE << "#receipts[" << tx_id << "].logs: " << receipts[tx_id].logs.size() << "\n";
        return true;
    };
    co_await reader.walk(silkworm::db::table::kLogs.name, log_key, 8 * CHAR_BIT, walker);

    co_return receipts;
}

asio::awaitable<Receipts> read_receipts(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number) {
    auto receipts = co_await read_raw_receipts(reader, block_hash, block_number);
    auto body = co_await read_body(reader, block_hash, block_number);
    auto senders = co_await read_senders(reader, block_hash, block_number);

    // Add derived fields to the receipts
    auto transactions = body.transactions;
    if (body.transactions.size() != receipts.size()) {
        throw std::runtime_error{"invalid receipt count in read_receipts"};
    }
    if (senders.size() != receipts.size()) {
        throw std::runtime_error{"invalid sender count in read_receipts"};
    }
    size_t log_index{0};
    for (size_t i{0}; i < receipts.size(); i++) {
        // The tx hash can be calculated by the tx content itself
        auto tx_hash{hash_of_transaction(transactions[i])};
        receipts[i].tx_hash = silkworm::to_bytes32(silkworm::full_view(tx_hash.bytes));
        receipts[i].tx_index = uint32_t(i);

        receipts[i].block_hash = block_hash;
        receipts[i].block_number = block_number;

        // When tx receiver is not set, create a contract with address depending on tx sender and its nonce
        if (!transactions[i].to.has_value()) {
            receipts[i].contract_address = silkworm::create_address(senders[i], transactions[i].nonce);
        }

        // The gas used can be calculated by the previous receipt
        if (i == 0) {
            receipts[i].gas_used = receipts[i].cumulative_gas_used;
        } else {
            receipts[i].gas_used = receipts[i].cumulative_gas_used - receipts[i-1].cumulative_gas_used;;
        }

        // The derived fields of receipt are taken from block and transaction
        for (size_t j{0}; j < receipts[i].logs.size(); j++) {
            receipts[i].logs[j].block_number = block_number;
            receipts[i].logs[j].block_hash = block_hash;
            receipts[i].logs[j].tx_hash = receipts[i].tx_hash;
            receipts[i].logs[j].tx_index = uint32_t(i);
            receipts[i].logs[j].index = log_index++;
            receipts[i].logs[j].removed = false;
        }
    }

    co_return receipts;
}

asio::awaitable<Transactions> read_transactions(DatabaseReader& reader, uint64_t base_txn_id, uint64_t txn_count) {
    Transactions txns{};
    if (txn_count == 0) {
        co_return txns;
    }

    txns.reserve(txn_count);

    silkworm::Bytes txn_id_key(8, '\0');
    boost::endian::store_big_u64(&txn_id_key[0], base_txn_id); // tx_id_key.data()?
    SILKRPC_TRACE << "txn_id_key: " << silkworm::to_hex(txn_id_key) << "\n";
    size_t i{0};
    Walker walker = [&](const silkworm::Bytes&, const silkworm::Bytes& v) {
        SILKRPC_TRACE << "v: " << silkworm::to_hex(v) << "\n";
        silkworm::ByteView value{v};
        silkworm::Transaction tx{};
        const auto error = silkworm::rlp::decode(value, tx);
        if (error != silkworm::rlp::DecodingResult::kOk) {
            SILKRPC_ERROR << "invalid RLP decoding for transaction index " << i << "\n";
            return false;
        }
        SILKRPC_TRACE << "index: " << i << " tx_hash: " << silkworm::to_hex(hash_of_transaction_rlp(v).bytes) << "\n";
        txns.emplace(txns.end(), std::move(tx));
        i++;
        return i < txn_count;
    };
    co_await reader.walk(silkworm::db::table::kEthTx.name, txn_id_key, 0, walker);

    SILKRPC_TRACE << "#txns: " << txns.size() << "\n";

    co_return txns;
}

} // namespace silkrpc::core::rawdb