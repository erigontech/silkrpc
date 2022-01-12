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

#include "execution_payload.hpp"

#include <catch2/catch.hpp>
#include <silkworm/common/util.hpp>
#include <evmc/evmc.hpp>
#include <silkrpc/common/log.hpp>

namespace silkrpc {

using Catch::Matchers::Message;

TEST_CASE("create empty execution_payload", "[silkrpc][types][execution_payload]") {
    ExecutionPayload payload{};
    CHECK(payload.suggestedFeeRecipient == evmc::address{});
    CHECK(payload.transactions == std::vector<silkworm::Bytes>{});
    CHECK(payload.parent_hash == evmc::bytes32{});
    CHECK(payload.state_root == evmc::bytes32{});
    CHECK(payload.receipts_root == evmc::bytes32{});
    CHECK(payload.block_hash == evmc::bytes32{});
    CHECK(payload.random == evmc::bytes32{});
    CHECK(payload.number == 0);
    CHECK(payload.timestamp == 0);
    CHECK(payload.gas_limit == 0);
    CHECK(payload.gas_used == 0);
    CHECK(payload.base_fee == intx::uint256{});
    CHECK(payload.logs_bloom == silkworm::Bloom{});
}

TEST_CASE("print empty execution_payload", "[silkrpc][types][execution_payload]") {
    ExecutionPayload payload{};
    CHECK_NOTHROW(null_stream() << payload);
}

} // namespace silkrpc