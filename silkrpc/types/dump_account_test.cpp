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

#include "dump_account.hpp"

#include <catch2/catch.hpp>
#include <evmc/evmc.hpp>
#include <nlohmann/json.hpp>
#include <silkworm/common/util.hpp>

#include <silkrpc/common/log.hpp>

namespace silkrpc {

using Catch::Matchers::Message;

static const auto zero_address = evmc::address{};
static const auto empty_hash = evmc::bytes32{};
static const auto zero_balance = intx::uint256{0};

using evmc::literals::operator""_address;
using evmc::literals::operator""_bytes32;

TEST_CASE("Empty DumpAccounts") {
    DumpAccounts da;

    SECTION("check fields") {
        CHECK(da.root == empty_hash);
        CHECK(da.accounts.size() == 0);
        CHECK(da.next == zero_address);
    }

    SECTION("print") {
        CHECK_NOTHROW(silkrpc::null_stream() << da);
    }

    SECTION("json") {
        nlohmann::json json = da;

        CHECK(json == R"({
            "accounts": {},
            "next": "AAAAAAAAAAAAAAAAAAAAAAAAAAA=",
            "root": "0x0000000000000000000000000000000000000000000000000000000000000000"
        })"_json);
    }
}

TEST_CASE("Filled DumpAccounts") {
    DumpAccounts da{0xb10e2d527612073b26eecdfd717e6a320cf44b4afac2b0732d9fcbe2b7fa0cf6_bytes32, 0x79a4d418f7887dd4d5123a41b6c8c186686ae8cb_address};

    SECTION("check fields") {
        CHECK(da.root == 0xb10e2d527612073b26eecdfd717e6a320cf44b4afac2b0732d9fcbe2b7fa0cf6_bytes32);
        CHECK(da.accounts.size() == 0);
        CHECK(da.next == 0x79a4d418f7887dd4d5123a41b6c8c186686ae8cb_address);
    }

    SECTION("print") {
        CHECK_NOTHROW(silkrpc::null_stream() << da);
    }

    SECTION("json") {
        nlohmann::json json = da;

        CHECK(json == R"({
            "accounts": {},
            "next": "eaTUGPeIfdTVEjpBtsjBhmhq6Ms=",
            "root": "0xb10e2d527612073b26eecdfd717e6a320cf44b4afac2b0732d9fcbe2b7fa0cf6"
        })"_json);
    }
}

TEST_CASE("Empty DumpAccount") {
    DumpAccount da;

    SECTION("check fields") {
        CHECK(da.balance == zero_balance);
        CHECK(da.nonce == 0);
        CHECK(da.incarnation == 0);
        CHECK(da.root == empty_hash);
        CHECK(da.code_hash == empty_hash);
        CHECK(da.code == std::nullopt);
        CHECK(da.storage == std::nullopt);
    }
}

TEST_CASE("Filled DumpAccount") {
    DumpAccount da{10, 20, 30, 0xb10e2d527612073b26eecdfd717e6a320cf44b4afac2b0732d9fcbe2b7fa0cf6_bytes32, 0xc10e2d527612073b26eecdfd717e6a320cf44b4afac2b0732d9fcbe2b7fa0cf6_bytes32};

    SECTION("check fields") {
        CHECK(da.balance == 10);
        CHECK(da.nonce == 20);
        CHECK(da.incarnation == 30);
        CHECK(da.root == 0xb10e2d527612073b26eecdfd717e6a320cf44b4afac2b0732d9fcbe2b7fa0cf6_bytes32);
        CHECK(da.code_hash == 0xc10e2d527612073b26eecdfd717e6a320cf44b4afac2b0732d9fcbe2b7fa0cf6_bytes32);
        CHECK(da.code == std::nullopt);
        CHECK(da.storage == std::nullopt);
    }
}

} // namespace silkrpc
