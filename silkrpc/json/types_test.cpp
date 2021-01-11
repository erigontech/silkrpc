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

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

namespace silkrpc::json::eth {

TEST_CASE("serialize empty filter", "[silkrpc::json::eth][to_json]") {
    Filter f{{0}, {0}, {{"", ""}}, {{"", ""}}, {""}};
    nlohmann::json j = f;
    CHECK(j == R"({"address":["",""],"blockHash":"","fromBlock":0,"toBlock":0,"topics":["",""]})"_json);
}

TEST_CASE("serialize filter with fromBlock and toBlock", "[silkrpc::json::eth][to_json]") {
    Filter f{{1000}, {2000}, {{"", ""}}, {{"", ""}}, {""}};
    nlohmann::json j = f;
    CHECK(j == R"({"address":["",""],"blockHash":"","fromBlock":1000,"toBlock":2000,"topics":["",""]})"_json);
}

TEST_CASE("deserialize null filter", "[silkrpc::json::eth][to_json]") {
    auto j1 = R"({})"_json;
    auto f1 = j1.get<Filter>();
    CHECK(f1.from_block == std::nullopt);
    CHECK(f1.to_block == std::nullopt);
}

TEST_CASE("deserialize empty filter", "[silkrpc::json::eth][to_json]") {
    auto j1 = R"({"address":["",""],"blockHash":"","fromBlock":0,"toBlock":0,"topics":["",""]})"_json;
    auto f1 = j1.get<Filter>();
    CHECK(f1.from_block == 0);
    CHECK(f1.to_block == 0);
}

} // namespace silkrpc