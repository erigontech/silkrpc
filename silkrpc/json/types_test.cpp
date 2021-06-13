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

#include <optional>
#include <string>
#include <vector>

#include <catch2/catch.hpp>
#include <evmc/evmc.hpp>
#include <intx/intx.hpp>
#include <nlohmann/json.hpp>

namespace silkrpc {

using evmc::literals::operator""_address, evmc::literals::operator""_bytes32;

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

TEST_CASE("deserialize empty log", "[silkrpc][from_json]") {
    auto j1 = R"({"address":"0000000000000000000000000000000000000000","topics":[],"data":[]})"_json;
    auto f1 = j1.get<Log>();
    CHECK(f1.address == evmc::address{});
    CHECK(f1.topics == std::vector<evmc::bytes32>{});
    CHECK(f1.data == silkworm::Bytes{});
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

} // namespace silkrpc

namespace silkrpc::json {

TEST_CASE("serialize empty filter", "[silkrpc::json][to_json]") {
    Filter f{{0}, {0}, {{"", ""}}, {{{"", ""}, {"", ""}}}, {""}};
    nlohmann::json j = f;
    CHECK(j == R"({"address":[],"blockHash":"","fromBlock":0,"toBlock":0,"topics":[[], []]})"_json);
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
    CHECK_THROWS(j1.get<Call>());
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
        "gasPrice": "0x010C388C00",
        "data": "0xdaa6d5560000000000000000000000000000000000000000000000000000000000000000"
    })"_json;
    auto c1 = j1.get<Call>();
    CHECK(c1.from == 0x52c24586c31cff0485a6208bb63859290fba5bce_address);
    CHECK(c1.to == 0x0715a7794a1dc8e42615f059dd6e406a6594651a_address);
    CHECK(c1.gas == intx::uint256{1000000});
    CHECK(c1.gas_price == intx::uint256{4499999744});
    CHECK(c1.data == silkworm::from_hex("0xdaa6d5560000000000000000000000000000000000000000000000000000000000000000"));
}

} // namespace silkrpc::json
