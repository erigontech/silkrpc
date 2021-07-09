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

#include <catch2/catch.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/types/log.hpp>

namespace silkrpc {

using Catch::Matchers::Message;

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

TEST_CASE("bloom from no logs", "[silkrpc][types][receipt]") {
    Logs logs{};
    CHECK(bloom_from_logs(logs) == silkworm::Bloom{});
}

} // namespace silkrpc

