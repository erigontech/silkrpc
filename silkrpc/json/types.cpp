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

#include "types.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

#include <boost/endian/conversion.hpp>
#include <intx/intx.hpp>
#include <silkworm/common/util.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkworm/common/endian.hpp>

namespace silkrpc {

// ----- ADDs

size_t to_hex(char *hex_bytes, silkworm::ByteView bytes) {
    static const char* kHexDigits{"0123456789abcdef"};
    char* dest{&hex_bytes[0]};
    *dest++ = '0';
    *dest++ = 'x';
    for (const auto& b : bytes) {
        *dest++ = kHexDigits[b >> 4];    // Hi
        *dest++ = kHexDigits[b & 0x0f];  // Lo
    }
    return dest-hex_bytes;
}

template<std::size_t N>
std::size_t to_my_hex(char *hex_bytes, const uint8_t bytes[N]) {
    static const char* kHexDigits{"0123456789abcdef"};
    char* dest{&hex_bytes[0]};
    *dest++ = '0';
    *dest++ = 'x';
    for (int i = 0; i<N; i++) {
        const auto v = bytes[i];
        *dest++ = kHexDigits[v >> 4];    // Hi
        *dest++ = kHexDigits[v & 0x0f];  // Lo
    }
    return dest-hex_bytes;
}
constexpr auto address_to_hex2 = to_my_hex<20>;
constexpr auto bytes32_to_hex2 = to_my_hex<32>;
constexpr auto nonce_to_hex2 = to_my_hex<8>;
constexpr auto bloom_to_hex2 = to_my_hex<256>;

std::size_t to_hex_no_leading_zeros(char *hex_bytes, silkworm::ByteView bytes) {
    static const char* kHexDigits{"0123456789abcdef"};

    char* dest{&hex_bytes[0]};
    int len = bytes.length();
    *dest++ = '0';
    *dest++ = 'x';

    bool found_nonzero{false};
    for (size_t i{0}; i < len; ++i) {
        uint8_t x{bytes[i]};
        char lo{kHexDigits[x & 0x0f]};
        char hi{kHexDigits[x >> 4]};
        if (!found_nonzero && hi != '0') {
            found_nonzero = true;
        }
        if (found_nonzero) {
            *dest++ = hi;
        }
        if (!found_nonzero && lo != '0') {
            found_nonzero = true;
        }
        if (found_nonzero || i == len - 1) {
            *dest++ = lo;
        }
    }

    return dest-hex_bytes;
}

std::size_t to_quantity(char *quantity_hex_bytes, silkworm::ByteView bytes) {
    return to_hex_no_leading_zeros(quantity_hex_bytes, bytes);
}


std::size_t to_quantity(char *quantity_hex_bytes, uint64_t number) {
    silkworm::Bytes number_bytes(8, '\0');
    silkworm::endian::store_big_u64(number_bytes.data(), number);
    return to_hex_no_leading_zeros(quantity_hex_bytes, number_bytes);
}

std::size_t to_quantity(char *quantity_hex_bytes, intx::uint256 number) {
    if (number == 0) {
       quantity_hex_bytes[0] = '0';
       quantity_hex_bytes[1] = 'x';
       quantity_hex_bytes[2] = '0';
       return 3;
    }
    return to_quantity(quantity_hex_bytes, silkworm::endian::to_big_compact(number));
}


char number[] = "number";
char hash[] = "hash";
char parentHash[] = "parentHash";
char nonce[] = "nonce";
char sha3Uncles[] = "sha3Uncles";
char logsBloom[] = "logsBloom";
char transactionsRoot[] = "transactionsRoot";
char transactions[] = "transactions";
char uncles[] = "uncles";
char stateRoot[] = "stateRoot";
char receiptsRoot[] = "receiptsRoot";
char miner[] = "miner";
char difficulty[] = "difficulty";
char totalDifficulty[] = "totalDifficulty";
char extraData[] = "extraData";
char mixHash[] = "mixHash";
char size_str[] = "size";
char gasLimit[] = "gasLimit";
char gas[] = "gas";
char input[] = "input";
char gasUsed[] = "gasUsed";
char baseFeePerGas[] = "baseFeePerGas";
char timestamp[] = "timestamp";
char transactionIndex[] = "transactionIndex";
char blockhash[] = "blockHash";
char blockNumber[] = "blockNumber";
char gasPrice[] = "gasPrice";
char chainId[] = "chainId";
char v[] = "v";
char s[] = "s";
char r[] = "r";
char value[] = "value";
char to[] = "to";
char from[] = "from";
char type[] = "type";

inline json_buffer& to_json2(json_buffer& out, const silkworm::Transaction& transaction) {

    if (!transaction.from) {
        (const_cast<silkworm::Transaction&>(transaction)).recover_sender();
    }
    if (transaction.from) {
        out.add_attribute_name(from,sizeof(from)-1);
        out.add_attribute_value(address_to_hex2(out.get_addr(), transaction.from.value().bytes));
    }

    out.add_attribute_name(gas, sizeof(gas)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), transaction.gas_limit));

    auto ethash_hash{hash_of_transaction(transaction)};
    out.add_attribute_name(hash,sizeof(hash)-1);
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), ethash_hash.bytes));
 

    out.add_attribute_name(input,sizeof(input)-1);
    out.add_attribute_value(to_hex(out.get_addr(), transaction.data));

    out.add_attribute_name(nonce,sizeof(nonce)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), transaction.nonce));

    if (transaction.to) {
        out.add_attribute_name(to,sizeof(to)-1);
        out.add_attribute_value(address_to_hex2(out.get_addr(), transaction.to.value().bytes));
    } 
    out.add_attribute_name(type,sizeof(type)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), (uint8_t)transaction.type));

    if (transaction.type != silkworm::Transaction::Type::kLegacy) {
       out.add_attribute_name(chainId,sizeof(chainId)-1);
       out.add_attribute_value(to_quantity(out.get_addr(), *transaction.chain_id));

       out.add_attribute_name(v,sizeof(v)-1);
       out.add_attribute_value(to_quantity(out.get_addr(), transaction.odd_y_parity));
       //json["accessList"] = transaction.access_list; // EIP2930
    } else {
       out.add_attribute_name(v,sizeof(v)-1);
       out.add_attribute_value(to_quantity(out.get_addr(), silkworm::endian::to_big_compact(transaction.v())));
    }

    out.add_attribute_name(value,sizeof(value)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), transaction.value));

    out.add_attribute_name(r,sizeof(r)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), silkworm::endian::to_big_compact(transaction.r)));

    out.add_attribute_name(s,sizeof(s)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), silkworm::endian::to_big_compact(transaction.s)));

    return out;
}

inline json_buffer& to_json2(json_buffer& out, const silkworm::BlockHeader& header) {
    out.add_attribute_name(number, sizeof(number)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), header.number));

    out.add_attribute_name(parentHash, sizeof(parentHash)-1);
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), header.parent_hash.bytes));

    out.add_attribute_name(nonce, sizeof(nonce)-1);
    out.add_attribute_value(nonce_to_hex2(out.get_addr(), header.nonce.data()));

    out.add_attribute_name(sha3Uncles, sizeof(sha3Uncles)-1);
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), header.ommers_hash.bytes));

    out.add_attribute_name(logsBloom, sizeof(logsBloom)-1);
    out.add_attribute_value(bloom_to_hex2(out.get_addr(), header.logs_bloom.data()));

    out.add_attribute_name(transactionsRoot, sizeof(transactionsRoot)-1);
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), header.transactions_root.bytes));

    out.add_attribute_name(stateRoot, sizeof(stateRoot)-1);
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), header.state_root.bytes));

    out.add_attribute_name(receiptsRoot, sizeof(receiptsRoot)-1);
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), header.receipts_root.bytes));

    out.add_attribute_name(miner, sizeof(miner)-1);
    out.add_attribute_value(address_to_hex2(out.get_addr(), header.beneficiary.bytes));

    out.add_attribute_name(extraData,sizeof(extraData)-1);
    out.add_attribute_value(to_hex(out.get_addr(), header.extra_data));

    out.add_attribute_name(difficulty, sizeof(difficulty)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), silkworm::endian::to_big_compact(header.difficulty)));

    out.add_attribute_name(mixHash, sizeof(mixHash)-1);
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), header.mix_hash.bytes));

    out.add_attribute_name(gasLimit,sizeof(gasLimit)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), header.gas_limit));

    out.add_attribute_name(gasUsed,sizeof(gasUsed)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), header.gas_used));

    out.add_attribute_name(timestamp,sizeof(timestamp)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), header.timestamp));

    if (header.base_fee_per_gas.has_value()) {
       out.add_attribute_name(baseFeePerGas,sizeof(baseFeePerGas)-1);
       out.add_attribute_value(to_quantity(out.get_addr(), header.base_fee_per_gas.value_or(0)));
    }

    return out;
}

inline size_t copy_bn (char *dest, char *src, int len) {
   memcpy(dest, src, len);
   return len;
}

inline json_buffer& to_json2(json_buffer& out, const silkrpc::Block& b) {
    char buffer[100];
    const auto block_number_size = to_quantity(buffer, b.block.header.number);

    out.add_attribute_name(number,sizeof(number)-1);
    out.add_attribute_value(copy_bn(out.get_addr(), buffer, block_number_size));

    out.add_attribute_name(hash,sizeof(hash)-1);
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), b.hash.bytes));

    out.add_attribute_name(parentHash,sizeof(parentHash)-1);
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), b.block.header.parent_hash.bytes));

    out.add_attribute_name(nonce,sizeof(nonce)-1);
    out.add_attribute_value(nonce_to_hex2(out.get_addr(), b.block.header.nonce.data()));

    out.add_attribute_name(sha3Uncles,sizeof(sha3Uncles)-1);
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), b.block.header.ommers_hash.bytes));

    out.add_attribute_name(logsBloom,sizeof(logsBloom)-1);
    out.add_attribute_value(bloom_to_hex2(out.get_addr(), b.block.header.logs_bloom.data()));

    out.add_attribute_name(transactionsRoot,sizeof(transactionsRoot)-1);
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), b.block.header.transactions_root.bytes));

    out.add_attribute_name(stateRoot,sizeof(stateRoot)-1);
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), b.block.header.state_root.bytes));

    out.add_attribute_name(receiptsRoot,sizeof(receiptsRoot)-1);
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), b.block.header.receipts_root.bytes));

    out.add_attribute_name(miner,sizeof(miner)-1);
    out.add_attribute_value(address_to_hex2(out.get_addr(), b.block.header.beneficiary.bytes));

    out.add_attribute_name(difficulty,sizeof(difficulty)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), silkworm::endian::to_big_compact(b.block.header.difficulty)));

    out.add_attribute_name(totalDifficulty,sizeof(totalDifficulty)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), silkworm::endian::to_big_compact(b.total_difficulty)));

    out.add_attribute_name(extraData,sizeof(extraData)-1);
    out.add_attribute_value(to_hex(out.get_addr(), b.block.header.extra_data));

    out.add_attribute_name(mixHash,sizeof(mixHash)-1);
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), b.block.header.mix_hash.bytes));

    out.add_attribute_name(size_str, sizeof(size_str)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), b.get_block_size()));

    out.add_attribute_name(gasLimit, sizeof(gasLimit)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), b.block.header.gas_limit));

    out.add_attribute_name(gasUsed, sizeof(gasUsed)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), b.block.header.gas_used));

    if (b.block.header.base_fee_per_gas.has_value()) {
       out.add_attribute_name(baseFeePerGas, sizeof(baseFeePerGas)-1);
       out.add_attribute_value(to_quantity(out.get_addr(), b.block.header.base_fee_per_gas.value_or(0)));
    }
    out.add_attribute_name(timestamp,sizeof(timestamp)-1);
    out.add_attribute_value(to_quantity(out.get_addr(), b.block.header.timestamp));

    if (b.full_tx) {
        int n_elem =  b.block.transactions.size();
        out.start_vector(transactions, sizeof(transactions)-1);
        for (auto i{0}; i < n_elem; i++) {
            out.start_vector_element();
            to_json2(out, b.block.transactions[i]);

            out.add_attribute_name(transactionIndex,sizeof(transactionIndex)-1);
            out.add_attribute_value(to_quantity(out.get_addr(), i));

            out.add_attribute_name(blockhash,sizeof(blockhash)-1);
            out.add_attribute_value(bytes32_to_hex2(out.get_addr(), b.hash.bytes));

            out.add_attribute_name(blockNumber,sizeof(blockNumber)-1);
            out.add_attribute_value(copy_bn(out.get_addr(), buffer, block_number_size));

            out.add_attribute_name(gasPrice,sizeof(gasPrice)-1);
            out.add_attribute_value(to_quantity(out.get_addr(), b.block.transactions[i].effective_gas_price(b.block.header.base_fee_per_gas.value_or(0))));
            out.end_vector_element();
        }
        out.end_vector();
    }

    std::vector<evmc::bytes32> ommer_hashes;
    ommer_hashes.reserve(b.block.ommers.size());
    out.add_attribute_name_list(uncles,sizeof(uncles)-1);
    for (auto i{0}; i < b.block.ommers.size(); i++) {
        char buffer[1000];
        auto len = bytes32_to_hex2(buffer, b.block.ommers[i].hash().bytes);
        out.add_attribute_value_list(buffer, len);
    }
    out.add_end_attribute_list();
    return out;
}


// ----- END ADDs

std::string to_hex_no_leading_zeros(silkworm::ByteView bytes) {
    static const char* kHexDigits{"0123456789abcdef"};

    std::string out{};
    out.reserve(2 * bytes.length());

    bool found_nonzero{false};
    for (size_t i{0}; i < bytes.length(); ++i) {
        uint8_t x{bytes[i]};
        char lo{kHexDigits[x & 0x0f]};
        char hi{kHexDigits[x >> 4]};
        if (!found_nonzero && hi != '0') {
            found_nonzero = true;
        }
        if (found_nonzero) {
            out.push_back(hi);
        }
        if (!found_nonzero && lo != '0') {
            found_nonzero = true;
        }
        if (found_nonzero || i == bytes.length() - 1) {
            out.push_back(lo);
        }
    }

    return out;
}

std::string to_hex_no_leading_zeros(uint64_t number) {
    silkworm::Bytes number_bytes(8, '\0');
    boost::endian::store_big_u64(&number_bytes[0], number);
    return to_hex_no_leading_zeros(number_bytes);
}

std::string to_quantity(silkworm::ByteView bytes) {
    return "0x" + to_hex_no_leading_zeros(bytes);
}

std::string to_quantity(uint64_t number) {
    return "0x" + to_hex_no_leading_zeros(number);
}

std::string to_quantity(intx::uint256 number) {
    if (number == 0) {
       return "0x0";
    }
    return silkrpc::to_quantity(silkworm::endian::to_big_compact(number));
}

} // namespace silkrpc

namespace evmc {

void to_json(nlohmann::json& json, const address& addr) {
    json = "0x" + silkworm::to_hex(addr);
}

void from_json(const nlohmann::json& json, address& addr) {
    const auto address_bytes = silkworm::from_hex(json.get<std::string>());
    addr = silkworm::to_evmc_address(address_bytes.value_or(silkworm::Bytes{}));
}

void to_json(nlohmann::json& json, const bytes32& b32) {
    json = "0x" + silkworm::to_hex(b32);
}

void from_json(const nlohmann::json& json, bytes32& b32) {
    const auto b32_bytes = silkworm::from_hex(json.get<std::string>());
    b32 = silkworm::to_bytes32(b32_bytes.value_or(silkworm::Bytes{}));
}

} // namespace evmc

namespace intx {

void from_json(const nlohmann::json& json, uint256& ui256) {
    ui256 = intx::from_string<intx::uint256>(json.get<std::string>());
}

} // namespace intx

namespace silkworm {

void to_json(nlohmann::json& json, const BlockHeader& header) {
    const auto block_number = silkrpc::to_quantity(header.number);
    json["number"] = block_number;
    json["parentHash"] = header.parent_hash;
    json["nonce"] = "0x" + silkworm::to_hex({header.nonce.data(), header.nonce.size()});
    json["sha3Uncles"] = header.ommers_hash;
    json["logsBloom"] = "0x" + silkworm::to_hex(silkrpc::full_view(header.logs_bloom));
    json["transactionsRoot"] = header.transactions_root;
    json["stateRoot"] = header.state_root;
    json["receiptsRoot"] = header.receipts_root;
    json["miner"] = header.beneficiary;
    json["difficulty"] = silkrpc::to_quantity(silkworm::endian::to_big_compact(header.difficulty));
    json["extraData"] = "0x" + silkworm::to_hex(header.extra_data);
    json["mixHash"]= header.mix_hash;
    json["gasLimit"] = silkrpc::to_quantity(header.gas_limit);
    json["gasUsed"] = silkrpc::to_quantity(header.gas_used);
    json["timestamp"] = silkrpc::to_quantity(header.timestamp);
    if (header.base_fee_per_gas.has_value()) {
       json["baseFeePerGas"] = silkrpc::to_quantity(header.base_fee_per_gas.value_or(0));
    }
}

void to_json(nlohmann::json& json, const AccessListEntry& access_list) {
    json["account"] = access_list.account;
    json["storage_keys"] = access_list.storage_keys;
}

void to_json(nlohmann::json& json, const Transaction& transaction) {
    if (!transaction.from) {
        (const_cast<Transaction&>(transaction)).recover_sender();
    }
    if (transaction.from) {
        json["from"] = transaction.from.value();
    }
    json["gas"] = silkrpc::to_quantity(transaction.gas_limit);
    auto ethash_hash{hash_of_transaction(transaction)};
    json["hash"] = silkworm::to_bytes32({ethash_hash.bytes, silkworm::kHashLength});
    json["input"] = "0x" + silkworm::to_hex(transaction.data);
    json["nonce"] = silkrpc::to_quantity(transaction.nonce);
    if (transaction.to) {
        json["to"] =  transaction.to.value();
    } else {
        json["to"] =  nullptr;
    }
    json["type"] = silkrpc::to_quantity((uint64_t)transaction.type);

    if (transaction.type == silkworm::Transaction::Type::kEip1559) {
       json["maxPriorityFeePerGas"] = silkrpc::to_quantity(transaction.max_priority_fee_per_gas);
       json["maxFeePerGas"] = silkrpc::to_quantity(transaction.max_fee_per_gas);
    }
    if (transaction.type != silkworm::Transaction::Type::kLegacy) {
       json["chainId"] = silkrpc::to_quantity(*transaction.chain_id);
       json["v"] = silkrpc::to_quantity((uint64_t)transaction.odd_y_parity);
       //json["accessList"] = transaction.access_list; // EIP2930
    } else {
       json["v"] = silkrpc::to_quantity(silkworm::endian::to_big_compact(transaction.v()));
    }
    json["value"] = silkrpc::to_quantity(transaction.value);
    json["r"] = silkrpc::to_quantity(silkworm::endian::to_big_compact(transaction.r));
    json["s"] = silkrpc::to_quantity(silkworm::endian::to_big_compact(transaction.s));
}



} // namespace silkworm

namespace silkrpc {

void to_json(nlohmann::json& json, const Block& b) {
    const auto block_number = silkrpc::to_quantity(b.block.header.number);
    json["number"] = block_number;
    json["hash"] = b.hash;
    json["parentHash"] = b.block.header.parent_hash;
    json["nonce"] = "0x" + silkworm::to_hex({b.block.header.nonce.data(), b.block.header.nonce.size()});
    json["sha3Uncles"] = b.block.header.ommers_hash;
    json["logsBloom"] = "0x" + silkworm::to_hex(full_view(b.block.header.logs_bloom));
    json["transactionsRoot"] = b.block.header.transactions_root;
    json["stateRoot"] = b.block.header.state_root;
    json["receiptsRoot"] = b.block.header.receipts_root;
    json["miner"] = b.block.header.beneficiary;
    json["difficulty"] = silkrpc::to_quantity(silkworm::endian::to_big_compact(b.block.header.difficulty));
    json["totalDifficulty"] = silkrpc::to_quantity(silkworm::endian::to_big_compact(b.total_difficulty));
    json["extraData"] = "0x" + silkworm::to_hex(b.block.header.extra_data);
    json["mixHash"]= b.block.header.mix_hash;
    json["size"] = silkrpc::to_quantity(b.get_block_size());
    json["gasLimit"] = silkrpc::to_quantity(b.block.header.gas_limit);
    json["gasUsed"] = silkrpc::to_quantity(b.block.header.gas_used);
    if (b.block.header.base_fee_per_gas.has_value()) {
       json["baseFeePerGas"] = silkrpc::to_quantity(b.block.header.base_fee_per_gas.value_or(0));
    }
    json["timestamp"] = silkrpc::to_quantity(b.block.header.timestamp);
    if (b.full_tx) {
        json["transactions"] = b.block.transactions;
        for (auto i{0}; i < json["transactions"].size(); i++) {
            auto& json_txn = json["transactions"][i];
            json_txn["transactionIndex"] = silkrpc::to_quantity(i);
            json_txn["blockHash"] = b.hash;
            json_txn["blockNumber"] = block_number;
            json_txn["gasPrice"] = silkrpc::to_quantity(b.block.transactions[i].effective_gas_price(b.block.header.base_fee_per_gas.value_or(0)));
        }
    } else {
        std::vector<evmc::bytes32> transaction_hashes;
        transaction_hashes.reserve(b.block.transactions.size());
        for (auto i{0}; i < b.block.transactions.size(); i++) {
            auto ethash_hash{hash_of_transaction(b.block.transactions[i])};
            auto bytes32_hash = silkworm::to_bytes32({ethash_hash.bytes, silkworm::kHashLength});
            transaction_hashes.emplace(transaction_hashes.end(), std::move(bytes32_hash));
            SILKRPC_DEBUG << "transaction_hashes[" << i << "]: " << silkworm::to_hex({transaction_hashes[i].bytes, silkworm::kHashLength}) << "\n";
        }
        json["transactions"] = transaction_hashes;
    }
    std::vector<evmc::bytes32> ommer_hashes;
    ommer_hashes.reserve(b.block.ommers.size());
    for (auto i{0}; i < b.block.ommers.size(); i++) {
        ommer_hashes.emplace(ommer_hashes.end(), std::move(b.block.ommers[i].hash()));
        SILKRPC_DEBUG << "ommer_hashes[" << i << "]: " << silkworm::to_hex({ommer_hashes[i].bytes, silkworm::kHashLength}) << "\n";
    }
    json["uncles"] = ommer_hashes;
}

void to_json(nlohmann::json& json, const Transaction& transaction) {
    to_json(json, silkworm::Transaction(transaction));

    json["gasPrice"] = silkrpc::to_quantity(transaction.effective_gas_price());
    json["blockHash"] = transaction.block_hash;
    json["blockNumber"] = silkrpc::to_quantity(transaction.block_number);
    json["transactionIndex"] = silkrpc::to_quantity(transaction.transaction_index);
}

void from_json(const nlohmann::json& json, Call& call) {
    if (json.count("from") != 0) {
        call.from = json.at("from").get<evmc::address>();
    }
    if (json.count("to") != 0) {
        const auto to = json.at("to");
        if (!to.is_null()) {
            call.to = json.at("to").get<evmc::address>();
        }
    }
    if (json.count("gas") != 0) {
        auto json_gas = json.at("gas");
        if (json_gas.is_string()) {
            call.gas = std::stol(json_gas.get<std::string>(), 0, 16);
        } else {
            call.gas = json_gas.get<uint64_t>();
        }
    }
    if (json.count("gasPrice") != 0) {
        call.gas_price = json.at("gasPrice").get<intx::uint256>();
    }
    if (json.count("value") != 0) {
        call.value = json.at("value").get<intx::uint256>();
    }
    if (json.count("data") != 0) {
        const auto json_data = json.at("data").get<std::string>();
        call.data = silkworm::from_hex(json_data);
    }
}

void to_json(nlohmann::json& json, const Log& log) {
    json["address"] = log.address;
    json["topics"] = log.topics;
    json["data"] = "0x" + silkworm::to_hex(log.data);
    json["blockNumber"] = silkrpc::to_quantity(log.block_number);
    json["blockHash"] = log.block_hash;
    json["transactionHash"] = log.tx_hash;
    json["transactionIndex"] = silkrpc::to_quantity(log.tx_index);
    json["logIndex"] = silkrpc::to_quantity(log.index);
    json["removed"] = log.removed;
}

void from_json(const nlohmann::json& json, Log& log) {
    if (json.is_array()) {
        if (json.size() < 3) {
            throw std::system_error{std::make_error_code(std::errc::invalid_argument), "Log CBOR: missing entries"};
        }
        if (!json[0].is_binary()) {
            throw std::system_error{std::make_error_code(std::errc::invalid_argument), "Log CBOR: binary expected in [0]"};
        }
        auto address_bytes = json[0].get_binary();
        log.address = silkworm::to_evmc_address(silkworm::Bytes{address_bytes.begin(), address_bytes.end()});
        if (!json[1].is_array()) {
            throw std::system_error{std::make_error_code(std::errc::invalid_argument), "Log CBOR: array expected in [1]"};
        }
        std::vector<evmc::bytes32> topics{};
        topics.reserve(json[1].size());
        for (auto topic : json[1]) {
            auto topic_bytes = topic.get_binary();
            topics.push_back(silkworm::to_bytes32(silkworm::Bytes{topic_bytes.begin(), topic_bytes.end()}));
        }
        log.topics = topics;
        if (json[2].is_binary()) {
            auto data_bytes = json[2].get_binary();
            log.data = silkworm::Bytes{data_bytes.begin(), data_bytes.end()};
        } else if (json[2].is_null()) {
            log.data = silkworm::Bytes{};
        } else {
            throw std::system_error{std::make_error_code(std::errc::invalid_argument), "Log CBOR: binary or null expected in [2]"};
        }
    } else {
        log.address = json.at("address").get<evmc::address>();
        log.topics = json.at("topics").get<std::vector<evmc::bytes32>>();
        log.data = json.at("data").get<silkworm::Bytes>();
    }
}

void to_json(nlohmann::json& json, const Receipt& receipt) {
    json["blockHash"] = receipt.block_hash;
    json["blockNumber"] = silkrpc::to_quantity(receipt.block_number);
    json["transactionHash"] = receipt.tx_hash;
    json["transactionIndex"] = silkrpc::to_quantity(receipt.tx_index);
    json["from"] = receipt.from.value_or(evmc::address{});
    json["to"] = receipt.to.value_or(evmc::address{});
    json["type"] = silkrpc::to_quantity(receipt.type ? receipt.type.value() : 0);
    json["gasUsed"] = silkrpc::to_quantity(receipt.gas_used);
    json["cumulativeGasUsed"] = silkrpc::to_quantity(receipt.cumulative_gas_used);
    json["effectiveGasPrice"] = silkrpc::to_quantity(receipt.effective_gas_price);
    if (receipt.contract_address) {
        json["contractAddress"] = receipt.contract_address;
    } else {
        json["contractAddress"] = nlohmann::json{};
    }
    json["logs"] = receipt.logs;
    json["logsBloom"] = "0x" + silkworm::to_hex(full_view(receipt.bloom));
    json["status"] = silkrpc::to_quantity(receipt.success ? 1 : 0);
}

void from_json(const nlohmann::json& json, Receipt& receipt) {
    SILKRPC_TRACE << "from_json<Receipt> json: " << json.dump() << "\n";
    if (json.is_array()) {
        if (json.size() < 4) {
            throw std::system_error{std::make_error_code(std::errc::invalid_argument), "Receipt CBOR: missing entries"};
        }
        if (!json[0].is_number()) {
            throw std::system_error{std::make_error_code(std::errc::invalid_argument), "Receipt CBOR: number expected in [0]"};
        }
        receipt.type = json[0];

        if (!json[1].is_null()) {
            throw std::system_error{std::make_error_code(std::errc::invalid_argument), "Receipt CBOR: null expected in [1]"};
        }

        if (!json[2].is_number()) {
            throw std::system_error{std::make_error_code(std::errc::invalid_argument), "Receipt CBOR: number expected in [2]"};
        }
        receipt.success = json[2] == 1u;

        if (!json[3].is_number()) {
            throw std::system_error{std::make_error_code(std::errc::invalid_argument), "Receipt CBOR: number expected in [3]"};
        }
        receipt.cumulative_gas_used = json[3];
    } else {
        receipt.success = json.at("success").get<bool>();
        receipt.cumulative_gas_used = json.at("cumulative_gas_used").get<uint64_t>();
    }
}

void to_json(nlohmann::json& json, const Filter& filter) {
    if (filter.from_block != std::nullopt) {
        json["fromBlock"] = filter.from_block.value();
    }
    if (filter.to_block != std::nullopt) {
        json["toBlock"] = filter.to_block.value();
    }
    if (filter.addresses != std::nullopt) {
        if (filter.addresses.value().size() == 1) {
            json["address"] = filter.addresses.value()[0];
        } else {
            json["address"] = filter.addresses.value();
        }
    }
    if (filter.topics != std::nullopt) {
        json["topics"] = filter.topics.value();
    }
    if (filter.block_hash != std::nullopt) {
        json["blockHash"] = filter.block_hash.value();
    }
}

void from_json(const nlohmann::json& json, Filter& filter) {
    if (json.count("fromBlock") != 0) {
        auto json_from_block = json.at("fromBlock");
        if (json_from_block.is_string()) {
            filter.from_block = std::stol(json_from_block.get<std::string>(), 0, 16);
        } else {
            filter.from_block = json_from_block.get<uint64_t>();
        }
    }
    if (json.count("toBlock") != 0) {
        auto json_to_block = json.at("toBlock");
        if (json_to_block.is_string()) {
            filter.to_block = std::stol(json_to_block.get<std::string>(), 0, 16);
        } else {
            filter.to_block = json_to_block.get<uint64_t>();
        }
    }
    if (json.count("address") != 0) {
        if (json.at("address").is_string()) {
            filter.addresses = {json.at("address").get<evmc::address>()};
        } else {
            filter.addresses = json.at("address").get<FilterAddresses>();
        }
    }
    if (json.count("topics") != 0) {
        auto topics = json.at("topics");
        for (auto& topic_item : topics) {
            if (topic_item.is_null()) {
                topic_item = FilterSubTopics{evmc::bytes32{}};
            }
            if (topic_item.is_string()) {
                topic_item = FilterSubTopics{evmc::bytes32{topic_item}};
            }
        }
        filter.topics = topics.get<FilterTopics>();
    }
    if (json.count("blockHash") != 0) {
        filter.block_hash = json.at("blockHash").get<std::string>();
    }
}

void to_json(nlohmann::json& json, const Forks& forks) {
    json["genesis"] = forks.genesis_hash;
    json["forks"] = forks.block_numbers;
}

void to_json(nlohmann::json& json, const Issuance& issuance) {
    if (issuance.block_reward) {
        json["blockReward"] = issuance.block_reward.value();
    }
    if (issuance.ommer_reward) {
        json["uncleReward"] = issuance.ommer_reward.value();
    }
    if (issuance.issuance) {
        json["issuance"] = issuance.issuance.value();
    }
}

void to_json(nlohmann::json& json, const Error& error) {
    json = {{"code", error.code}, {"message", error.message}};
}

void to_json(nlohmann::json& json, const RevertError& error) {
    json = {{"code", error.code}, {"message", error.message}, {"data", "0x" + silkworm::to_hex(error.data)}};
}

void to_json(nlohmann::json& json, const std::set<evmc::address>& addresses) {
    json = nlohmann::json::array();
    for (const auto& address : addresses) {
        json.push_back("0x" + silkworm::to_hex(address));
    }
}

nlohmann::json make_json_content(uint32_t id, const nlohmann::json& result) {
    return {{"jsonrpc", "2.0"}, {"id", id}, {"result", result}};
}


nlohmann::json make_json_error(uint32_t id, int32_t code, const std::string& message) {
    const Error error{code, message};
    return {{"jsonrpc", "2.0"}, {"id", id}, {"error", error}};
}

nlohmann::json make_json_error(uint32_t id, const RevertError& error) {
    return {{"jsonrpc", "2.0"}, {"id", id}, {"error", error}};
}


char json_rpc[] = "jsonrpc";
char id_str[] = "id";
char code_str[] = "code";
char error[] = "error";
char vers[] = "2.0";
char result[] = "result";

inline size_t copy_int(char *buffer, int32_t value) {
   return sprintf (buffer, "%d", value);
}

inline size_t copy_str(char *buffer, const std::string& message) {
   memcpy(buffer, message.c_str(), message.length());
   return message.length();
}

inline size_t copy_str(char *buffer, char *message, int len) {
   memcpy(buffer, message, len);
   return len;
}

void make_json_error(json_buffer& out, uint32_t id, int32_t code, const std::string& message) {
    
    out.add_attribute_name(json_rpc,sizeof(json_rpc)-1);
    out.add_attribute_value(copy_str(out.get_addr(), vers, sizeof(vers)-1));

    out.add_attribute_name(id_str,sizeof(id_str)-1);
    out.add_attribute_value(copy_int(out.get_addr(), id));

    out.add_attribute_name(code_str,sizeof(code_str)-1);
    out.add_attribute_value(copy_int(out.get_addr(), code));

    out.add_attribute_name(error,sizeof(error)-1);
    out.add_attribute_value(copy_str(out.get_addr(), error));
    out.end();
}

void make_json_content(json_buffer& out, uint32_t id, const silkrpc::Block& block) {
    out.add_attribute_name(json_rpc,sizeof(json_rpc)-1);
    out.add_attribute_value(copy_str(out.get_addr(), vers, sizeof(vers)-1));

    out.add_attribute_name2(id_str,sizeof(id_str)-1);
    out.add_attribute_value2(copy_int(out.get_addr(), id));

    out.start_object(result, sizeof(result)-1);
    to_json2(out, block);
    out.end_object();
    out.end();
}

} // namespace silkrpc
