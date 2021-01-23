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
#include <nlohmann/json.hpp>

#include <silkrpc/common/clock.hpp>
#include <silkrpc/json/types.hpp>

namespace silkrpc::core {

TEST_CASE("decode empty receipt list", "[silkrpc::core][cbor_decode]") {
    std::vector<Receipt> receipts{};
    cbor_decode({}, receipts);
    CHECK(receipts.size() == 0);
}

TEST_CASE("decode receipt list", "[silkrpc::core][cbor_decode]") {
    std::vector<Receipt> receipts{};
    auto bytes = silkworm::from_hex("8283f6001a0032f05d83f6011a00beadd0");
    cbor_decode(bytes, receipts);
    CHECK(receipts.size() == 2);
    CHECK(receipts[0].success == false);
    CHECK(receipts[0].cumulative_gas_used == 0x32f05d);
    CHECK(receipts[1].success == true);
    CHECK(receipts[1].cumulative_gas_used == 0xbeadd0);
}

} // namespace silkrpc::core
