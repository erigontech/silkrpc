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

#include "filter.cpp"

#include <sstream>

#include <catch2/catch.hpp>
#include <evmc/evmc.h>

#include <silkrpc/common/util.hpp>

namespace silkrpc {

using namespace evmc::literals;

TEST_CASE("write 0-sized filter addresses to ostream", "[silkrpc][operator<<]") {
    using namespace evmc;
    FilterAddresses addresses{};
    std::ostringstream oss;
    oss << addresses;
    CHECK(oss.str() == "[]");
}

TEST_CASE("write 1-sized filter addresses to ostream", "[silkrpc][operator<<]") {
    using namespace evmc;
    FilterAddresses addresses{0x6090a6e47849629b7245dfa1ca21d94cd15878ef_address};
    std::ostringstream oss;
    oss << addresses;
    CHECK(oss.str() == "[0x6090a6e47849629b7245dfa1ca21d94cd15878ef]");
}

TEST_CASE("write 2-sized filter addresses to ostream", "[silkrpc][operator<<]") {
    using namespace evmc;
    FilterAddresses addresses{
        0x6090a6e47849629b7245dfa1ca21d94cd15878ef_address,
        0x702a999710cfd011b475505335d4f437d8132fae_address
    };
    std::ostringstream oss;
    oss << addresses;
    CHECK(oss.str() == "[0x6090a6e47849629b7245dfa1ca21d94cd15878ef 0x702a999710cfd011b475505335d4f437d8132fae]");
}

TEST_CASE("write 0-sized filter subtopics to ostream", "[silkrpc][operator<<]") {
    using namespace evmc;
    FilterSubTopics subtopics{};
    std::ostringstream oss;
    oss << subtopics;
    CHECK(oss.str() == "[]");
}

TEST_CASE("write 1-sized filter subtopics to ostream", "[silkrpc][operator<<]") {
    using namespace evmc;
    FilterSubTopics subtopics{0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32};
    std::ostringstream oss;
    oss << subtopics;
    CHECK(oss.str() == "[0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c]");
}

TEST_CASE("write 2-sized filter subtopics to ostream", "[silkrpc][operator<<]") {
    using namespace evmc;
    FilterSubTopics subtopics{
        0x0000000000000000000000000000000000000000000000000000000000000000_bytes32,
        0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32
    };
    std::ostringstream oss;
    oss << subtopics;
    CHECK(oss.str() == R"([0x0000000000000000000000000000000000000000000000000000000000000000 0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c])");
}

} // namespace silkrpc
