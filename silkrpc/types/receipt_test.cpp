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

#include "receipt.hpp"

#include <iomanip>
#include <iostream>

#include <catch2/catch.hpp>
#include <silkworm/common/util.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/types/log.hpp>

namespace silkrpc {

using Catch::Matchers::Message;

using evmc::literals::operator""_address;

TEST_CASE("create empty receipt", "[silkrpc][types][receipt]") {
    Receipt r{};
    CHECK(r.success == false);
    CHECK(r.cumulative_gas_used == 0);
    CHECK(r.bloom == silkworm::Bloom{});
}

TEST_CASE("print empty receipt", "[silkrpc][types][receipt]") {
    Receipt r{};
    CHECK_NOTHROW(silkworm::null_stream() << r);
}

TEST_CASE("bloom from empty logs", "[silkrpc][types][receipt]") {
    Logs logs{};
    CHECK(bloom_from_logs(logs) == silkworm::Bloom{});
}

TEST_CASE("bloom from one empty log", "[silkrpc][types][receipt]") {
    Logs logs{
        Log{}
    };
    auto bloom = bloom_from_logs(logs);
    silkworm::Bloom expected_bloom{};
    expected_bloom[9] = uint8_t(128);
    expected_bloom[47] = uint8_t(2);
    expected_bloom[143] = uint8_t(1);
    CHECK(bloom_from_logs(logs) == expected_bloom);
}

TEST_CASE("bloom from more than one log", "[silkrpc][types][receipt]") {
    Logs logs{
        Log{
            0x0715a7794a1dc8e42615f059dd6e406a6594651a_address,
            {},
            *silkworm::from_hex("0x1234abcd")
        },
        Log{
            0x0715a7794a1dc8e42615f059dd6e406a6594651a_address,
            {},
            *silkworm::from_hex("0x1234abcd")
        }
    };
    auto bloom = bloom_from_logs(logs);
    silkworm::Bloom expected_bloom{};
    expected_bloom[73] = uint8_t(16);
    expected_bloom[87] = uint8_t(8);
    expected_bloom[190] = uint8_t(8);
    CHECK(bloom_from_logs(logs) == expected_bloom);
}

TEST_CASE("receipt with empty bloom", "[silkrpc][types][receipt]") {
    Logs logs{};
    Receipt r{
        true,
        210000,
        bloom_from_logs(logs),
        logs
    };
    CHECK(r.success == true);
    CHECK(r.cumulative_gas_used == 210000);
    CHECK(r.bloom == silkworm::Bloom{});
    CHECK(r.logs.size() == 0);
}

} // namespace silkrpc

