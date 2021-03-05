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

#include "chain.cpp"

#include <catch2/catch.hpp>
#include <evmc/evmc.h>
#include <nlohmann/json.hpp>

#include <silkworm/common/util.hpp>
#include <silkworm/types/log_cbor.hpp>
#include <silkrpc/json/types.hpp>

namespace silkrpc::core {

TEST_CASE("decode empty receipt list", "[silkrpc::core][cbor_decode]") {
    std::vector<Receipt> receipts{};
    cbor_decode({}, receipts);
    CHECK(receipts.size() == 0);
}

TEST_CASE("decode receipt list", "[silkrpc::core][cbor_decode]") {
    std::vector<Receipt> receipts{};
    auto bytes = silkworm::from_hex("8283f6001a0032f05d83f6011a00beadd0").value();
    cbor_decode(bytes, receipts);
    CHECK(receipts.size() == 2);
    CHECK(receipts[0].success == false);
    CHECK(receipts[0].cumulative_gas_used == 0x32f05d);
    CHECK(receipts[1].success == true);
    CHECK(receipts[1].cumulative_gas_used == 0xbeadd0);
}

TEST_CASE("decode empty log list", "[silkrpc::core][cbor_decode]") {
    std::vector<Log> logs{};
    cbor_decode({}, logs);
    CHECK(logs.size() == 0);
}

TEST_CASE("encode log list", "[silkrpc::core][cbor_encode]") {
    using namespace evmc;
    std::vector<silkworm::Log> logs{
        silkworm::Log{
            0xea674fdde714fd979de3edf0f56aa9716b898ec8_address,
            {},
            silkworm::from_hex("0x010043").value(),
        }
    };
    auto bytes = silkworm::cbor_encode(logs);
    CHECK(silkworm::to_hex(bytes) == "818354ea674fdde714fd979de3edf0f56aa9716b898ec88043010043");
}

TEST_CASE("decode log", "[silkrpc::core][cbor_decode]") {
    using namespace evmc;
    std::vector<Log> logs{};
    auto bytes = silkworm::from_hex("818354ea674fdde714fd979de3edf0f56aa9716b898ec88043010043").value();
    cbor_decode(bytes, logs);
    CHECK(logs.size() == 1);
    CHECK(logs[0].address == 0xea674fdde714fd979de3edf0f56aa9716b898ec8_address);
    CHECK(logs[0].topics == std::vector<evmc::bytes32>{});
    CHECK(silkworm::to_hex(logs[0].data) == "010043");
}

TEST_CASE("decode log list", "[silkrpc::core][cbor_decode]") {
    using namespace evmc;
    std::vector<Log> logs{};
    auto bytes = silkworm::from_hex(
        "81835456c0369e002852c2570ca0cc3442e26df98e01a2835820ddf252ad1be2c89b69c2b068fc37"
        "8daa952ba7f163c4a11628f55a4df523b3ef5820000000000000000000000000a2e1ffe3aa9cbcde"
        "1955b04d22e2cc092c3738785820000000000000000000000000520d849db6e4bf7e0c58a45fc513"
        "a6d633baf77e5820000000000000000000000000000000000000000000084595161401484a000000").value();
    cbor_decode(bytes, logs);
    CHECK(logs.size() == 1);
    CHECK(logs[0].address == 0x56c0369e002852c2570ca0cc3442e26df98e01a2_address);
    CHECK(logs[0].topics.size() == 3);
    CHECK(logs[0].topics == std::vector<evmc::bytes32>{
        0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef_bytes32,
        0x000000000000000000000000a2e1ffe3aa9cbcde1955b04d22e2cc092c373878_bytes32,
        0x000000000000000000000000520d849db6e4bf7e0c58a45fc513a6d633baf77e_bytes32,
    });
    CHECK(silkworm::to_hex(logs[0].data) == "000000000000000000000000000000000000000000084595161401484a000000");
}

} // namespace silkrpc::core
