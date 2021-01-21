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

#include <cbor-cpp/src/decoder.h>
#include <cbor-cpp/src/input.h>

#include <silkworm/core/silkworm/common/util.hpp>
#include <silkworm/core/silkworm/rlp/decode.hpp>
#include <silkworm/db/silkworm/db/tables.hpp>
#include <silkworm/db/silkworm/db/util.hpp>

namespace silkworm {

class receipt_listener : public cbor::listener {
public:
    receipt_listener(std::vector<Receipt>& receipts) : receipts_(receipts) {}

    void on_integer(int value) override {}

    void on_bytes(unsigned char *data, int size) override {}

    void on_string(std::string &str) override {}

    void on_array(int size) override {
        receipts_.resize(size);
    }

    void on_map(int size) override {}

    void on_tag(unsigned int tag) override {}

    void on_special(unsigned int code) override {}

    void on_bool(bool) override {}

    void on_null() override {}

    void on_undefined() override {}

    void on_error(const char *error) override {}

    void on_extra_integer(unsigned long long value, int sign) override {}

    void on_extra_tag(unsigned long long tag) override {}

    void on_extra_special(unsigned long long tag) override {}
private:
    std::vector<Receipt>& receipts_;
};

void cbor_decode(const Bytes& b, std::vector<silkworm::Receipt>& receipts) {
    cbor::input input{const_cast<unsigned char*>(b.data()), static_cast<int>(b.size())};
    receipt_listener listener{receipts};
    cbor::decoder decoder{input, listener};
    decoder.run();

    // TODO: implement decode logic in receipt_listener

    /*if (v.empty()) {
        encoder.write_null();
    } else {
        encoder.write_array(v.size());
    }

    for (const Receipt& r : v) {
        encoder.write_array(3);

        encoder.write_null();  // no PostState
        encoder.write_int(r.success ? 1u : 0u);
        encoder.write_int(static_cast<unsigned long long>(r.cumulative_gas_used));
    }*/
}

class log_listener : public cbor::listener {
public:
    log_listener(std::vector<Log>& logs) : logs_(logs) {}

    void on_integer(int value) override {}

    void on_bytes(unsigned char *data, int size) override {}

    void on_string(std::string &str) override {}

    void on_array(int size) override {
        logs_.resize(size);
    }

    void on_map(int size) override {}

    void on_tag(unsigned int tag) override {}

    void on_special(unsigned int code) override {}

    void on_bool(bool) override {}

    void on_null() override {}

    void on_undefined() override {}

    void on_error(const char *error) override {}

    void on_extra_integer(unsigned long long value, int sign) override {}

    void on_extra_tag(unsigned long long tag) override {}

    void on_extra_special(unsigned long long tag) override {}
private:
    std::vector<Log>& logs_;
};

void cbor_decode(const Bytes& b, std::vector<silkworm::Log>& logs) {
    cbor::input input{const_cast<unsigned char*>(b.data()), static_cast<int>(b.size())};
    log_listener listener{logs};
    cbor::decoder decoder{input, listener};
    decoder.run();

    // TODO: implement decode logic in receipt_listener

    /*if (v.empty()) {
        encoder.write_null();
    } else {
        encoder.write_array(v.size());
    }

    for (const Receipt& r : v) {
        encoder.write_array(3);

        encoder.write_null();  // no PostState
        encoder.write_int(r.success ? 1u : 0u);
        encoder.write_int(static_cast<unsigned long long>(r.cumulative_gas_used));
    }*/
}

}  // namespace silkworm


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
    const auto data = co_await read_body_rlp(reader, block_hash, block_number);
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
    const auto data = co_await reader.get(silkworm::db::table::kBlockHeaders.name, header_hash_key);
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
    Addresses senders{data.size() / silkworm::kHashLength};
    for (size_t i{0}; i < senders.size(); i++) {
        senders[i] = silkworm::to_address(&data[i * silkworm::kHashLength]);
    }
    co_return senders;
}

asio::awaitable<Receipts> read_raw_receipts(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number) {
    const auto block_key = silkworm::db::block_key(block_number);
    const auto data = co_await reader.get(silkworm::db::table::kBlockReceipts.name, block_key);
    if (data.empty()) {
        co_return Receipts{};
    }
    Receipts receipts{};
    silkworm::cbor_decode(data, receipts);

    auto log_key = silkworm::db::log_key(block_number, 0);
    Walker walker = [&](const silkworm::Bytes& k, const silkworm::Bytes& v) {
        auto tx_id = boost::endian::load_big_u32(&k[sizeof(uint64_t)]);
        silkworm::cbor_decode(v, receipts[tx_id].logs);
        return true;
    };
    reader.walk(silkworm::db::table::kLogs.name, log_key, 8 * CHAR_BIT, walker);

    co_return receipts;
}

asio::awaitable<Receipts> read_receipts(DatabaseReader& reader, evmc::bytes32 block_hash, uint64_t block_number) {
    auto receipts = co_await read_raw_receipts(reader, block_hash, block_number);
    auto body = co_await read_body(reader, block_hash, block_number);
    auto senders = co_await read_senders(reader, block_hash, block_number);

    auto transactions = body.transactions;

    size_t log_index{0};
    if (body.transactions.size() != receipts.size()) {
        throw std::system_error{std::make_error_code(std::errc::value_too_large), "invalid receipt count in read_receipts"};
    }
    for (size_t i{0}; i < receipts.size(); i++) {
        //receipts[i].
    }

    co_return receipts;
}

} // namespace silkrpc::core::rawdb