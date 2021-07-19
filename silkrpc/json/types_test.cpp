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

#include "types.hpp"

#include <optional>
#include <string>
#include <vector>

#include <catch2/catch.hpp>
#include <evmc/evmc.hpp>
#include <intx/intx.hpp>
#include <nlohmann/json.hpp>

namespace silkrpc {

using Catch::Matchers::Message;
using evmc::literals::operator""_address, evmc::literals::operator""_bytes32;

TEST_CASE("convert zero uint256 to quantity", "[silkrpc][to_quantity]") {
    intx::uint256 zero_u256{0};
    const auto zero_quantity = to_quantity(zero_u256);
    CHECK(zero_quantity == "0x0");
}

TEST_CASE("convert positive uint256 to quantity", "[silkrpc][to_quantity]") {
    intx::uint256 positive_u256{100};
    const auto positive_quantity = to_quantity(positive_u256);
    CHECK(positive_quantity == "0x64");
}

TEST_CASE("serialize empty address", "[silkrpc][to_json]") {
    evmc::address address{};
    nlohmann::json j = address;
    CHECK(j == R"("0x0000000000000000000000000000000000000000")"_json);
}

TEST_CASE("serialize address", "[silkrpc][to_json]") {
    evmc::address address{0x0715a7794a1dc8e42615f059dd6e406a6594651a_address};
    nlohmann::json j = address;
    CHECK(j == R"("0x0715a7794a1dc8e42615f059dd6e406a6594651a")"_json);
}

TEST_CASE("deserialize empty address", "[silkrpc][from_json]") {
    auto j1 = R"("0000000000000000000000000000000000000000")"_json;
    auto address = j1.get<evmc::address>();
    CHECK(address == evmc::address{});
}

TEST_CASE("deserialize address", "[silkrpc][from_json]") {
    auto j1 = R"("0x0715a7794a1dc8e42615f059dd6e406a6594651a")"_json;
    auto address = j1.get<evmc::address>();
    CHECK(address == evmc::address{0x0715a7794a1dc8e42615f059dd6e406a6594651a_address});
}

TEST_CASE("serialize empty bytes32", "[silkrpc][to_json]") {
    evmc::bytes32 b32{};
    nlohmann::json j = b32;
    CHECK(j == R"("0x0000000000000000000000000000000000000000000000000000000000000000")"_json);
}

TEST_CASE("serialize non-empty bytes32", "[silkrpc][to_json]") {
    evmc::bytes32 b32{0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32};
    nlohmann::json j = b32;
    CHECK(j == R"("0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c")"_json);
}

TEST_CASE("serialize empty block header", "[silkrpc][to_json]") {
    silkworm::BlockHeader header{};
    nlohmann::json j = header;
    CHECK(j == R"({
        "parentHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "sha3Uncles":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "miner":"0x0000000000000000000000000000000000000000",
        "stateRoot":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "transactionsRoot":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "receiptsRoot":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        "difficulty":"0x",
        "nonce":"0x0",
        "number":"0x0",
        "gasLimit":"0x0",
        "gasUsed":"0x0",
        "timestamp":"0x0",
        "extraData":"0x",
        "mixHash":"0x0000000000000000000000000000000000000000000000000000000000000000"
    })"_json);
}

TEST_CASE("serialize block header", "[silkrpc][to_json]") {
    silkworm::BlockHeader header{
        0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32,
        0x474f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126d_bytes32,
        0x0715a7794a1dc8e42615f059dd6e406a6594651a_address,
        0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126d_bytes32,
        0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126e_bytes32,
        0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126f_bytes32,
        silkworm::Bloom{},
        intx::uint256{0},
        uint64_t(5),
        uint64_t(1000000),
        uint64_t(1000000),
        uint64_t(5405021),
        *silkworm::from_hex("0001FF0100"),
        0x0000000000000000000000000000000000000000000000000000000000000001_bytes32,
        {0, 0, 0, 0, 0, 0, 0, 255}
    };
    nlohmann::json j = header;
    CHECK(j == R"({
        "parentHash":"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c",
        "sha3Uncles":"0x474f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126d",
        "miner":"0x0715a7794a1dc8e42615f059dd6e406a6594651a",
        "stateRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126d",
        "transactionsRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126e",
        "receiptsRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126f",
        "logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        "difficulty":"0x",
        "number":"0x5",
        "gasLimit":"0xf4240",
        "gasUsed":"0xf4240",
        "timestamp":"0x52795d",
        "extraData":"0x0001ff0100",
        "mixHash":"0x0000000000000000000000000000000000000000000000000000000000000001",
        "nonce":"0xff"
    })"_json);
}

TEST_CASE("serialize empty block", "[silkrpc][to_json]") {
    silkrpc::Block block{};
    nlohmann::json j = block;
    CHECK(j == R"({
        "parentHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "sha3Uncles":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "miner":"0x0000000000000000000000000000000000000000",
        "stateRoot":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "transactionsRoot":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "receiptsRoot":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        "difficulty":"0x",
        "nonce":"0x0",
        "number":"0x0",
        "gasLimit":"0x0",
        "gasUsed":"0x0",
        "baseFeePerGas":"0x0",
        "timestamp":"0x0",
        "extraData":"0x",
        "mixHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "hash":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "size":"0x3",
        "totalDifficulty":"0x",
        "transactions":[],
        "uncles":[]
    })"_json);
}

TEST_CASE("serialize empty transaction", "[silkrpc][to_json]") {
    silkworm::Transaction txn{};
    nlohmann::json j = txn;
    CHECK(j == R"({
        "nonce":"0x0",
        "gas":"0x0",
        "to":null,
        "type":"0x0",
        "value":"0x0",
        "input":"0x",
        "hash":"0x3763e4f6e4198413383534c763f3f5dac5c5e939f0a81724e3beb96d6e2ad0d5",
        "r":"0x",
        "s":"0x",
        "v":"0x1b"
    })"_json);
}

TEST_CASE("serialize transaction from zero address", "[silkrpc][to_json]") {
    silkworm::Transaction txn{};
    txn.from = 0x0000000000000000000000000000000000000000_address;
    nlohmann::json j = txn;
    CHECK(j == R"({
        "nonce":"0x0",
        "gas":"0x0",
        "to":null,
        "from":"0x0000000000000000000000000000000000000000",
        "type":"0x0",
        "value":"0x0",
        "input":"0x",
        "hash":"0x3763e4f6e4198413383534c763f3f5dac5c5e939f0a81724e3beb96d6e2ad0d5",
        "r":"0x",
        "s":"0x",
        "v":"0x1b"
    })"_json);
}

TEST_CASE("serialize legacy transaction (type=0)", "[silkrpc][to_json]") {
    silkworm::Transaction txn1{
        std::nullopt,
        0,
        intx::uint256{0},
        intx::uint256{0},
        uint64_t{0},
        0x0715a7794a1dc8e42615f059dd6e406a6594651a_address,
        intx::uint256{0},
        *silkworm::from_hex("001122aabbcc"),
        false,
        intx::uint256{1},
        intx::uint256{18},
        intx::uint256{36},
        std::vector<silkworm::AccessListEntry>{},
        0x007fb8417eb9ad4d958b050fc3720d5b46a2c053_address
    };
    nlohmann::json j1 = txn1;
    CHECK(j1 == R"({
        "nonce":"0x0",
        "gas":"0x0",
        "to":"0x0715a7794a1dc8e42615f059dd6e406a6594651a",
        "from":"0x007fb8417eb9ad4d958b050fc3720d5b46a2c053",
        "type":"0x0",
        "value":"0x0",
        "input":"0x001122aabbcc",
        "hash":"0x861b1b1b1d2609b3dec5fcb8f0b411e5b88a2c2e896daa9ee8e80b9f4839e6d9",
        "r":"0x12",
        "s":"0x24",
        "v":"0x25"
    })"_json);

    silkrpc::Transaction txn2{
        std::nullopt,
        0,
        intx::uint256{0},
        intx::uint256{0},
        uint64_t{0},
        0x0715a7794a1dc8e42615f059dd6e406a6594651a_address,
        intx::uint256{0},
        *silkworm::from_hex("001122aabbcc"),
        false,
        intx::uint256{1},
        intx::uint256{18},
        intx::uint256{36},
        std::vector<silkworm::AccessListEntry>{},
        0x007fb8417eb9ad4d958b050fc3720d5b46a2c053_address,
        0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32,
        123123,
        intx::uint256{0},
        3
    };
    nlohmann::json j2 = txn2;
    CHECK(j2 == R"({
        "nonce":"0x0",
        "gasPrice":"0x0",
        "gas":"0x0",
        "to":"0x0715a7794a1dc8e42615f059dd6e406a6594651a",
        "from":"0x007fb8417eb9ad4d958b050fc3720d5b46a2c053",
        "type":"0x0",
        "value":"0x0",
        "input":"0x001122aabbcc",
        "hash":"0x861b1b1b1d2609b3dec5fcb8f0b411e5b88a2c2e896daa9ee8e80b9f4839e6d9",
        "r":"0x12",
        "s":"0x24",
        "v":"0x25",
        "blockHash":"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c",
        "blockNumber":"0x1e0f3",
        "transactionIndex":"0x3"
    })"_json);
}

TEST_CASE("serialize EIP-2930 transaction (type=1)", "[silkrpc][to_json]") {
    silkworm::Transaction txn1{
        silkworm::kEip2930TransactionType,
        0,
        intx::uint256{1},
        intx::uint256{2},
        uint64_t{0},
        0x0715a7794a1dc8e42615f059dd6e406a6594651a_address,
        intx::uint256{0},
        *silkworm::from_hex("001122aabbcc"),
        false,
        intx::uint256{1},
        intx::uint256{18},
        intx::uint256{36},
        std::vector<silkworm::AccessListEntry>{},
        0x007fb8417eb9ad4d958b050fc3720d5b46a2c053_address
    };
    nlohmann::json j1 = txn1;
    CHECK(j1 == R"({
        "nonce":"0x0",
        "gas":"0x0",
        "to":"0x0715a7794a1dc8e42615f059dd6e406a6594651a",
        "from":"0x007fb8417eb9ad4d958b050fc3720d5b46a2c053",
        "type":"0x1",
        "value":"0x0",
        "input":"0x001122aabbcc",
        "hash":"0xf9620bf84ce91b2bcdd151a5ae9891243f491b164367af25e23b2a450bf34834",
        "r":"0x12",
        "s":"0x24",
        "v":"0x25"
    })"_json);

    silkrpc::Transaction txn2{
        silkworm::kEip2930TransactionType,
        0,
        intx::uint256{1},
        intx::uint256{2},
        uint64_t{0},
        0x0715a7794a1dc8e42615f059dd6e406a6594651a_address,
        intx::uint256{0},
        *silkworm::from_hex("001122aabbcc"),
        false,
        intx::uint256{1},
        intx::uint256{18},
        intx::uint256{36},
        std::vector<silkworm::AccessListEntry>{},
        0x007fb8417eb9ad4d958b050fc3720d5b46a2c053_address,
        0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32,
        123123,
        intx::uint256{12},
        3
    };
    nlohmann::json j2 = txn2;
    CHECK(j2 == R"({
        "nonce":"0x0",
        "gasPrice":"0xd",
        "gas":"0x0",
        "to":"0x0715a7794a1dc8e42615f059dd6e406a6594651a",
        "from":"0x007fb8417eb9ad4d958b050fc3720d5b46a2c053",
        "type":"0x1",
        "value":"0x0",
        "input":"0x001122aabbcc",
        "hash":"0xf9620bf84ce91b2bcdd151a5ae9891243f491b164367af25e23b2a450bf34834",
        "r":"0x12",
        "s":"0x24",
        "v":"0x25",
        "blockHash":"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c",
        "blockNumber":"0x1e0f3",
        "transactionIndex":"0x3"
    })"_json);
}

TEST_CASE("serialize error", "[silkrpc][to_json]") {
    Error err{100, {"generic error"}};
    nlohmann::json j = err;
    CHECK(j == R"({
        "code":100,
        "message":"generic error"
    })"_json);
}

TEST_CASE("serialize empty log", "[silkrpc][to_json]") {
    Log l{{}, {}, {}};
    nlohmann::json j = l;
    CHECK(j == R"({
        "address":"0x0000000000000000000000000000000000000000",
        "topics":[],
        "data":"0x",
        "blockNumber":"0x0",
        "blockHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "transactionHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "transactionIndex":"0x0",
        "logIndex":"0x0",
        "removed":false
    })"_json);
}

TEST_CASE("shortest hex for 4206337", "[silkrpc][to_json]") {
    Log l{{}, {}, {}, 4206337};
    nlohmann::json j = l;
    CHECK(j == R"({
        "address":"0x0000000000000000000000000000000000000000",
        "topics":[],
        "data":"0x",
        "blockNumber":"0x402f01",
        "blockHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "transactionHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "transactionIndex":"0x0",
        "logIndex":"0x0",
        "removed":false
    })"_json);
}

TEST_CASE("deserialize wrong size log", "[silkrpc][from_json]") {
    const auto j1 = nlohmann::json::from_cbor(*silkworm::from_hex("80"));
    CHECK_THROWS_MATCHES(j1.get<Log>(), std::system_error, Message("Log CBOR: missing entries: Invalid argument"));
    const auto j2 = nlohmann::json::from_cbor(*silkworm::from_hex("81540000000000000000000000000000000000000000"));
    CHECK_THROWS_MATCHES(j2.get<Log>(), std::system_error, Message("Log CBOR: missing entries: Invalid argument"));
    const auto j3 = nlohmann::json::from_cbor(*silkworm::from_hex("8254000000000000000000000000000000000000000080"));
    CHECK_THROWS_MATCHES(j3.get<Log>(), std::system_error, Message("Log CBOR: missing entries: Invalid argument"));
    const auto j4 = nlohmann::json::from_cbor(*silkworm::from_hex("83808040"));
    CHECK_THROWS_MATCHES(j4.get<Log>(), std::system_error, Message("Log CBOR: binary expected in [0]: Invalid argument"));
    const auto j5 = nlohmann::json::from_cbor(*silkworm::from_hex("835400000000000000000000000000000000000000004040"));
    CHECK_THROWS_MATCHES(j5.get<Log>(), std::system_error, Message("Log CBOR: array expected in [1]: Invalid argument"));
    const auto j6 = nlohmann::json::from_cbor(*silkworm::from_hex("835400000000000000000000000000000000000000008080"));
    CHECK_THROWS_MATCHES(j6.get<Log>(), std::system_error, Message("Log CBOR: binary or null expected in [2]: Invalid argument"));
}

TEST_CASE("deserialize empty array log", "[silkrpc][from_json]") {
    const auto j1 = nlohmann::json::from_cbor(*silkworm::from_hex("835400000000000000000000000000000000000000008040"));
    const auto log1 = j1.get<Log>();
    CHECK(log1.address == evmc::address{});
    CHECK(log1.topics == std::vector<evmc::bytes32>{});
    CHECK(log1.data == silkworm::Bytes{});
    const auto j2 = nlohmann::json::from_cbor(*silkworm::from_hex("8354000000000000000000000000000000000000000080f6"));
    const auto log2 = j2.get<Log>();
    CHECK(log2.address == evmc::address{});
    CHECK(log2.topics == std::vector<evmc::bytes32>{});
    CHECK(log2.data == silkworm::Bytes{});
}

TEST_CASE("deserialize empty log", "[silkrpc][from_json]") {
    const auto j = R"({
        "address":"0000000000000000000000000000000000000000",
        "topics":[],
        "data":[]
    })"_json;
    const auto log = j.get<Log>();
    CHECK(log.address == evmc::address{});
    CHECK(log.topics == std::vector<evmc::bytes32>{});
    CHECK(log.data == silkworm::Bytes{});
}

TEST_CASE("deserialize array log", "[silkrpc][from_json]") {
    const auto bytes = silkworm::from_hex("8354ea674fdde714fd979de3edf0f56aa9716b898ec88043010043").value();
    const auto j = nlohmann::json::from_cbor(bytes);
    const auto log = j.get<Log>();
    CHECK(log.address == 0xea674fdde714fd979de3edf0f56aa9716b898ec8_address);
    CHECK(log.topics == std::vector<evmc::bytes32>{});
    CHECK(log.data == silkworm::Bytes{0x01, 0x00, 0x43});
}

TEST_CASE("deserialize topics", "[silkrpc][from_json]") {
    auto j1 = R"({
        "address":"0000000000000000000000000000000000000000",
        "topics":["0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c"],
        "data":[]
    })"_json;
    auto f1 = j1.get<Log>();
    CHECK(f1.address == evmc::address{});
    CHECK(f1.topics == std::vector<evmc::bytes32>{0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32});
    CHECK(f1.data == silkworm::Bytes{});
}

TEST_CASE("deserialize wrong size receipt", "[silkrpc][from_json]") {
    const auto j1 = nlohmann::json::from_cbor(*silkworm::from_hex("80"));
    CHECK_THROWS_MATCHES(j1.get<Receipt>(), std::system_error, Message("Receipt CBOR: missing entries: Invalid argument"));
    const auto j2 = nlohmann::json::from_cbor(*silkworm::from_hex("8100"));
    CHECK_THROWS_MATCHES(j2.get<Receipt>(), std::system_error, Message("Receipt CBOR: missing entries: Invalid argument"));
    const auto j3 = nlohmann::json::from_cbor(*silkworm::from_hex("8200f6"));
    CHECK_THROWS_MATCHES(j3.get<Receipt>(), std::system_error, Message("Receipt CBOR: missing entries: Invalid argument"));
    const auto j4 = nlohmann::json::from_cbor(*silkworm::from_hex("8300f600"));
    CHECK_THROWS_MATCHES(j4.get<Receipt>(), std::system_error, Message("Receipt CBOR: missing entries: Invalid argument"));
    const auto j5 = nlohmann::json::from_cbor(*silkworm::from_hex("84f4f60000"));
    CHECK_THROWS_MATCHES(j5.get<Receipt>(), std::system_error, Message("Receipt CBOR: number expected in [0]: Invalid argument"));
    const auto j6 = nlohmann::json::from_cbor(*silkworm::from_hex("8400f40000"));
    CHECK_THROWS_MATCHES(j6.get<Receipt>(), std::system_error, Message("Receipt CBOR: null expected in [1]: Invalid argument"));
    const auto j7 = nlohmann::json::from_cbor(*silkworm::from_hex("8400f6f500"));
    CHECK_THROWS_MATCHES(j7.get<Receipt>(), std::system_error, Message("Receipt CBOR: number expected in [2]: Invalid argument"));
    const auto j8 = nlohmann::json::from_cbor(*silkworm::from_hex("8400f600f5"));
    CHECK_THROWS_MATCHES(j8.get<Receipt>(), std::system_error, Message("Receipt CBOR: number expected in [3]: Invalid argument"));
}

TEST_CASE("deserialize wrong receipt", "[silkrpc][from_json]") {
    const auto j = R"({})"_json;
    CHECK_THROWS(j.get<Receipt>());
}

TEST_CASE("deserialize empty receipt", "[silkrpc][from_json]") {
    const auto j = R"({"success":false,"cumulative_gas_used":0})"_json;
    const auto r = j.get<Receipt>();
    CHECK(r.success == false);
    CHECK(r.cumulative_gas_used == 0);
}

TEST_CASE("deserialize wrong array receipt", "[silkrpc][from_json]") {
    CHECK_THROWS_AS(R"([])"_json.get<Receipt>(), std::system_error);
    CHECK_THROWS_AS(R"([""])"_json.get<Receipt>(), std::system_error);
    CHECK_THROWS_AS(R"([null])"_json.get<Receipt>(), std::system_error);
    CHECK_THROWS_AS(R"([0])"_json.get<Receipt>(), std::system_error);
    CHECK_THROWS_AS(R"([0,0])"_json.get<Receipt>(), std::system_error);
    CHECK_THROWS_AS(R"([0,""])"_json.get<Receipt>(), std::system_error);
    CHECK_THROWS_AS(R"([0,null])"_json.get<Receipt>(), std::system_error);
    CHECK_THROWS_AS(R"([0,null,""])"_json.get<Receipt>(), std::system_error);
    CHECK_THROWS_AS(R"([0,null,null])"_json.get<Receipt>(), std::system_error);
    CHECK_THROWS_AS(R"([0,null,0])"_json.get<Receipt>(), std::system_error);
    CHECK_THROWS_AS(R"(["",null,0,0])"_json.get<Receipt>(), std::system_error);
    CHECK_THROWS_AS(R"([0,"",0,0])"_json.get<Receipt>(), std::system_error);
    CHECK_THROWS_AS(R"([0,null,"",0])"_json.get<Receipt>(), std::system_error);
    CHECK_THROWS_AS(R"([0,null,0,""])"_json.get<Receipt>(), std::system_error);
    CHECK_THROWS_AS(R"([0,null,0,null])"_json.get<Receipt>(), std::system_error);
}

TEST_CASE("deserialize empty array receipt", "[silkrpc][from_json]") {
    const auto j1 = R"([0,null,0,0])"_json;
    const auto r1 = j1.get<Receipt>();
    CHECK(*r1.type == 0);
    CHECK(r1.success == false);
    CHECK(r1.cumulative_gas_used == 0);
    const auto j2 = nlohmann::json::from_cbor(*silkworm::from_hex("8400f60000"));
    const auto r2 = j2.get<Receipt>();
    CHECK(*r2.type == 0);
    CHECK(r2.success == false);
    CHECK(r2.cumulative_gas_used == 0);
}

TEST_CASE("deserialize array receipt", "[silkrpc][from_json]") {
    const auto j = R"([1,null,1,123456])"_json;
    const auto r = j.get<Receipt>();
    CHECK(*r.type == 1);
    CHECK(r.success == true);
    CHECK(r.cumulative_gas_used == 123456);
}

TEST_CASE("serialize empty receipt", "[silkrpc::json][to_json]") {
    Receipt r{};
    nlohmann::json j = r;
    CHECK(j == R"({
        "blockHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "blockNumber":"0x0",
        "contractAddress":null,
        "cumulativeGasUsed":"0x0",
        "from":"0x0000000000000000000000000000000000000000",
        "gasUsed":"0x0",
        "logs":[],
        "logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000","status":"0x0",
        "to":"0x0000000000000000000000000000000000000000",
        "transactionHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "transactionIndex":"0x0",
        "type":"0x0"
    })"_json);
}

TEST_CASE("serialize receipt", "[silkrpc::json][to_json]") {
    Receipt r{
        true,
        454647,
        silkworm::Bloom{},
        Logs{},
        0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32,
        0x0715a7794a1dc8e42615f059dd6e406a6594651a_address,
        10,
        0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126f_bytes32,
        5000000,
        3,
        0x22ea9f6b28db76a7162054c05ed812deb2f519cd_address,
        0x22ea9f6b28db76a7162054c05ed812deb2f519cd_address,
        1
    };
    nlohmann::json j = r;
    CHECK(j == R"({
        "blockHash":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126f",
        "blockNumber":"0x4c4b40",
        "contractAddress":"0x0715a7794a1dc8e42615f059dd6e406a6594651a",
        "cumulativeGasUsed":"0x6eff7",
        "from":"0x22ea9f6b28db76a7162054c05ed812deb2f519cd",
        "gasUsed":"0xa",
        "logs":[],
        "logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000","status":"0x0",
        "status":"0x1",
        "to":"0x22ea9f6b28db76a7162054c05ed812deb2f519cd",
        "transactionHash":"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c",
        "transactionIndex":"0x3",
        "type":"0x1"
    })"_json);
}

TEST_CASE("serialize empty filter", "[silkrpc::json][to_json]") {
    Filter f{{0}, {0}, {{"", ""}}, {{{"", ""}, {"", ""}}}, {""}};
    nlohmann::json j = f;
    CHECK(j == R"({"address":[],"blockHash":"","fromBlock":0,"toBlock":0,"topics":[[], []]})"_json);
}

TEST_CASE("serialize filter with one address", "[silkrpc::json][to_json]") {
    Filter f;
    f.addresses = {{0x007fb8417eb9ad4d958b050fc3720d5b46a2c053_address}};
    nlohmann::json j = f;
    CHECK(j == R"({"address":"0x007fb8417eb9ad4d958b050fc3720d5b46a2c053"})"_json);
}

TEST_CASE("serialize filter with fromBlock and toBlock", "[silkrpc::json][to_json]") {
    Filter f{{1000}, {2000}, {{"", ""}}, {{{"", ""}, {"", ""}}}, {""}};
    nlohmann::json j = f;
    CHECK(j == R"({"address":[],"blockHash":"","fromBlock":1000,"toBlock":2000,"topics":[[], []]})"_json);
}

TEST_CASE("deserialize null filter", "[silkrpc::json][from_json]") {
    auto j1 = R"({})"_json;
    auto f1 = j1.get<Filter>();
    CHECK(f1.from_block == std::nullopt);
    CHECK(f1.to_block == std::nullopt);
}

TEST_CASE("deserialize empty filter", "[silkrpc::json][from_json]") {
    auto j1 = R"({"address":["",""],"blockHash":"","fromBlock":0,"toBlock":0,"topics":[["",""], ["",""]]})"_json;
    auto f1 = j1.get<Filter>();
    CHECK(f1.from_block == 0);
    CHECK(f1.to_block == 0);
}

TEST_CASE("deserialize filter with topic", "[silkrpc::json][from_json]") {
    auto j = R"({
        "address": "0x6090a6e47849629b7245dfa1ca21d94cd15878ef",
        "fromBlock": "0x3d0000",
        "toBlock": "0x3d2600",
        "topics": [
            null,
            "0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c"
        ]
    })"_json;
    auto f = j.get<Filter>();
    CHECK(f.from_block == 3997696u);
    CHECK(f.to_block == 4007424u);
    CHECK(f.addresses == std::vector<evmc::address>{0x6090a6e47849629b7245dfa1ca21d94cd15878ef_address});
    CHECK(f.topics == std::vector<std::vector<evmc::bytes32>>{
        {0x0000000000000000000000000000000000000000000000000000000000000000_bytes32},
        {0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32}
    });
    CHECK(f.block_hash == std::nullopt);
}

TEST_CASE("deserialize null call", "[silkrpc::json][from_json]") {
    auto j1 = R"({})"_json;
    CHECK_NOTHROW(j1.get<Call>());
}

TEST_CASE("deserialize minimal call", "[silkrpc::json][from_json]") {
    auto j1 = R"({
        "to": "0x0715a7794a1dc8e42615f059dd6e406a6594651a"
    })"_json;
    auto c1 = j1.get<Call>();
    CHECK(c1.from == std::nullopt);
    CHECK(c1.to == evmc::address{0x0715a7794a1dc8e42615f059dd6e406a6594651a_address});
    CHECK(c1.gas == std::nullopt);
    CHECK(c1.gas_price == std::nullopt);
    CHECK(c1.value == std::nullopt);
    CHECK(c1.data == std::nullopt);
}

TEST_CASE("deserialize full call", "[silkrpc::json][from_json]") {
    auto j1 = R"({
        "from": "0x52c24586c31cff0485a6208bb63859290fba5bce",
        "to": "0x0715a7794a1dc8e42615f059dd6e406a6594651a",
        "gas": "0xF4240",
        "gasPrice": "0x10C388C00",
        "data": "0xdaa6d5560000000000000000000000000000000000000000000000000000000000000000"
    })"_json;
    auto c1 = j1.get<Call>();
    CHECK(c1.from == 0x52c24586c31cff0485a6208bb63859290fba5bce_address);
    CHECK(c1.to == 0x0715a7794a1dc8e42615f059dd6e406a6594651a_address);
    CHECK(c1.gas == intx::uint256{1000000});
    CHECK(c1.gas_price == intx::uint256{4499999744});
    CHECK(c1.data == silkworm::from_hex("0xdaa6d5560000000000000000000000000000000000000000000000000000000000000000"));

    auto j2 = R"({
        "from":"0x52c24586c31cff0485a6208bb63859290fba5bce",
        "to":"0x0715a7794a1dc8e42615f059dd6e406a6594651a",
        "gas":1000000,
        "gasPrice":"0x10C388C00",
        "data":"0xdaa6d5560000000000000000000000000000000000000000000000000000000000000000",
        "value":"0x124F80"
    })"_json;
    auto c2 = j2.get<Call>();
    CHECK(c2.from == 0x52c24586c31cff0485a6208bb63859290fba5bce_address);
    CHECK(c2.to == 0x0715a7794a1dc8e42615f059dd6e406a6594651a_address);
    CHECK(c2.gas == intx::uint256{1000000});
    CHECK(c2.gas_price == intx::uint256{4499999744});
    CHECK(c2.data == silkworm::from_hex("0xdaa6d5560000000000000000000000000000000000000000000000000000000000000000"));
    CHECK(c2.value == intx::uint256{1200000});
}

TEST_CASE("serialize zero forks", "[silkrpc::json][to_json]") {
    silkrpc::ChainConfig cc{};
    silkrpc::Forks f{{}};
    nlohmann::json j = f;
    CHECK(j == R"({
        "genesis":"0x0000000000000000000000000000000000000000000000000000000000000000",
        "forks":[]
    })"_json);
}

TEST_CASE("serialize forks", "[silkrpc::json][to_json]") {
    silkrpc::ChainConfig cc{
        0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32,
        R"({"istanbulBlock":100,"berlinBlock":200})"_json
    };
    silkrpc::Forks f{cc};
    nlohmann::json j = f;
    CHECK(j == R"({
        "genesis":"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c",
        "forks":[100,200]
    })"_json);
}

TEST_CASE("serialize empty issuance", "[silkrpc::json][to_json]") {
    silkrpc::Issuance issuance{};
    nlohmann::json j = issuance;
    CHECK(j.is_null());
}

TEST_CASE("serialize issuance", "[silkrpc::json][to_json]") {
    silkrpc::Issuance issuance{
        "0x0",
        "0x0",
        "0x0"
    };
    nlohmann::json j = issuance;
    CHECK(j == R"({
        "blockReward":"0x0",
        "uncleReward":"0x0",
        "issuance":"0x0"
    })"_json);
}

TEST_CASE("make empty json content", "[silkrpc::json][make_json_content]") {
    const auto j = silkrpc::make_json_content(0, {});
    CHECK(j == R"({
        "jsonrpc":"2.0",
        "id":0,
        "result":null
    })"_json);
}

TEST_CASE("make json content", "[silkrpc::json][make_json_content]") {
    nlohmann::json json_result = {{"currency", "ETH"}, {"value", 4.2}};
    const auto j = silkrpc::make_json_content(123, json_result);
    CHECK(j == R"({
        "jsonrpc":"2.0",
        "id":123,
        "result":{"currency":"ETH","value":4.2}
    })"_json);
}

TEST_CASE("make empty json error", "[silkrpc::json][make_json_error]") {
    const auto j = silkrpc::make_json_error(0, 0, "");
    CHECK(j == R"({
        "jsonrpc":"2.0",
        "id":0,
        "error":{"code":0,"message":""}
    })"_json);
}

TEST_CASE("make empty json revert error", "[silkrpc::json][make_json_error]") {
    const auto j = silkrpc::make_json_error(0, {0, "", silkworm::Bytes{}});
    CHECK(j == R"({
        "jsonrpc":"2.0",
        "id":0,
        "error":{"code":0,"message":"","data":"0x"}
    })"_json);
}

TEST_CASE("make json error", "[silkrpc::json][make_json_error]") {
    const auto j = silkrpc::make_json_error(123, -32000, "revert");
    CHECK(j == R"({
        "jsonrpc":"2.0",
        "id":123,
        "error":{"code":-32000,"message":"revert"}
    })"_json);
}

TEST_CASE("make json revert error", "[silkrpc::json][make_json_error]") {
    const auto j = silkrpc::make_json_error(123, {3, "execution reverted: Ownable: caller is not the owner", *silkworm::from_hex("0x00010203")});
    CHECK(j == R"({
        "jsonrpc":"2.0",
        "id":123,
        "error":{"code":3,"message":"execution reverted: Ownable: caller is not the owner","data":"0x00010203"}
    })"_json);
}

} // namespace silkrpc
