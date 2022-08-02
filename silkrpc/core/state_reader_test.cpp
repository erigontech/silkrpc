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

#include "state_reader.hpp"

#include <silkrpc/config.hpp>
#include <asio/awaitable.hpp>
#include <catch2/catch.hpp>
#include <evmc/evmc.hpp>

#include <silkrpc/common/util.hpp>
#include <silkrpc/core/blocks.hpp>
#include <silkrpc/ethdb/tables.hpp>
#include <silkrpc/test/context_test_base.hpp>
#include <silkrpc/test/mock_database_reader.hpp>

namespace silkrpc {

using evmc::literals::operator""_bytes32;
using testing::InvokeWithoutArgs;
using testing::_;

static const evmc::address kZeroAddress{};
static const silkworm::Bytes kEncodedAccount{*silkworm::from_hex(
    "0f01020203e8010520f1885eda54b7a053318cd41e2093220dab15d65381b1157a3633a83bfd5c9239")};

struct StateReaderTest : public test::ContextTestBase {
    test::MockDatabaseReader database_reader_;
    StateReader state_reader_{database_reader_};
};

TEST_CASE_METHOD(StateReaderTest, "StateReader::read_account") {
    SILKRPC_LOG_VERBOSITY(LogLevel::Trace);
    SILKRPC_DEBUG << "kEncodedAccount: " << silkworm::to_hex(kEncodedAccount) << "\n";

    SECTION("no account for history empty and current state empty") {
        // Set the call expectations:
        // 1. DatabaseReader::get call on kAccountHistory returns empty key-value
        EXPECT_CALL(database_reader_, get(db::table::kAccountHistory, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{}; }
        ));
        // 2. DatabaseReader::get_one call on kPlainState returns empty value
        EXPECT_CALL(database_reader_, get_one(db::table::kPlainState, full_view(kZeroAddress))).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return silkworm::Bytes{}; }
        ));

        // Execute the test: calling read_account should return no account
        std::optional<silkworm::Account> account;
        CHECK_NOTHROW(account = spawn_and_wait(state_reader_.read_account(kZeroAddress, core::kEarliestBlockNumber)));
        CHECK(!account.has_value());
    }

    SECTION("account found in current state") {
        // Set the call expectations:
        // 1. DatabaseReader::get call on kAccountHistory returns empty key-value
        EXPECT_CALL(database_reader_, get(db::table::kAccountHistory, _)).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<KeyValue> { co_return KeyValue{}; }
        ));
        // 2. DatabaseReader::get_one call on kPlainState returns account data
        EXPECT_CALL(database_reader_, get_one(db::table::kPlainState, full_view(kZeroAddress))).WillOnce(InvokeWithoutArgs(
            []() -> asio::awaitable<silkworm::Bytes> { co_return kEncodedAccount; }
        ));

        // Execute the test: calling read_account should return expected account
        std::optional<silkworm::Account> account;
        CHECK_NOTHROW(account = spawn_and_wait(state_reader_.read_account(kZeroAddress, core::kEarliestBlockNumber)));
        CHECK(account.has_value());
        CHECK(account->nonce == 2);
        CHECK(account->balance == 1000);
        CHECK(account->code_hash == 0xf1885eda54b7a053318cd41e2093220dab15d65381b1157a3633a83bfd5c9239_bytes32);
        CHECK(account->incarnation == 5);
    }
}

} // namespace silkrpc

