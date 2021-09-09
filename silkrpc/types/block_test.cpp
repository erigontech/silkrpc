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

#include "block.hpp"

#include <catch2/catch.hpp>

namespace silkrpc {

using Catch::Matchers::Message;
using evmc::literals::operator""_address, evmc::literals::operator""_bytes32;

TEST_CASE("block_number_or_hash") {
    SECTION("default ctor") {
        BlockNumberOrHash bnoh;
        CHECK(bnoh.is_undefined() == true);
        CHECK(bnoh.is_hash() == false);
        CHECK(bnoh.is_number() == false);
        CHECK(bnoh.is_tag() == false);
    }
    SECTION("ctor from hash string") {
        BlockNumberOrHash bnoh{"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c"};
        CHECK(bnoh.is_undefined() == false);
        CHECK(bnoh.is_hash() == true);
        CHECK(bnoh.is_number() == false);
        CHECK(bnoh.is_tag() == false);
        CHECK(bnoh.hash() == 0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32);
    }
    SECTION("ctor from decimal number string") {
        BlockNumberOrHash bnoh{"1966"};
        CHECK(bnoh.is_undefined() == false);
        CHECK(bnoh.is_hash() == false);
        CHECK(bnoh.is_number() == true);
        CHECK(bnoh.is_tag() == false);
        CHECK(bnoh.number() == 1966);
    }
    SECTION("ctor from hex number string") {
        BlockNumberOrHash bnoh{"0x374f3"};
        CHECK(bnoh.is_undefined() == false);
        CHECK(bnoh.is_hash() == false);
        CHECK(bnoh.is_number() == true);
        CHECK(bnoh.is_tag() == false);
        CHECK(bnoh.number() == 0x374f3);
    }
    SECTION("ctor from tag string") {
        BlockNumberOrHash bnoh{"latest"};
        CHECK(bnoh.is_undefined() == false);
        CHECK(bnoh.is_hash() == false);
        CHECK(bnoh.is_number() == false);
        CHECK(bnoh.is_tag() == true);
        CHECK(bnoh.tag() == "latest");
    }
    SECTION("ctor from number") {
        BlockNumberOrHash bnoh{123456};
        CHECK(bnoh.is_undefined() == false);
        CHECK(bnoh.is_hash() == false);
        CHECK(bnoh.is_number() == true);
        CHECK(bnoh.is_tag() == false);
        CHECK(bnoh.number() == 123456);
    }
    SECTION("copy ctor") {
        BlockNumberOrHash bnoh{"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c"};
        BlockNumberOrHash copy{bnoh};
        CHECK(bnoh.is_undefined() == copy.is_undefined());
        CHECK(bnoh.is_hash() == copy.is_hash());
        CHECK(bnoh.is_number() == copy.is_number());
        CHECK(bnoh.is_tag() == copy.is_tag());
        CHECK(bnoh.hash() == copy.hash());
    }
    SECTION("assign from hash string") {
        BlockNumberOrHash bnoh;
        bnoh = "0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c";
        CHECK(bnoh.is_undefined() == false);
        CHECK(bnoh.is_hash() == true);
        CHECK(bnoh.is_number() == false);
        CHECK(bnoh.is_tag() == false);
        CHECK(bnoh.hash() == 0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32);
    }
    SECTION("assign from decimal number string") {
        BlockNumberOrHash bnoh;
        bnoh = "1966";
        CHECK(bnoh.is_undefined() == false);
        CHECK(bnoh.is_hash() == false);
        CHECK(bnoh.is_number() == true);
        CHECK(bnoh.is_tag() == false);
        CHECK(bnoh.number() == 1966);
    }
    SECTION("assign from hex number string") {
        BlockNumberOrHash bnoh;
        bnoh = "0x374f3";
        CHECK(bnoh.is_undefined() == false);
        CHECK(bnoh.is_hash() == false);
        CHECK(bnoh.is_number() == true);
        CHECK(bnoh.is_tag() == false);
        CHECK(bnoh.number() == 0x374f3);
    }
    SECTION("assign from tag string") {
        BlockNumberOrHash bnoh;
        bnoh = "latest";
        CHECK(bnoh.is_undefined() == false);
        CHECK(bnoh.is_hash() == false);
        CHECK(bnoh.is_number() == false);
        CHECK(bnoh.is_tag() == true);
        CHECK(bnoh.tag() == "latest");
    }
    SECTION("assign from number") {
        BlockNumberOrHash bnoh;
        bnoh = 123456;
        CHECK(bnoh.is_undefined() == false);
        CHECK(bnoh.is_hash() == false);
        CHECK(bnoh.is_number() == true);
        CHECK(bnoh.is_tag() == false);
        CHECK(bnoh.number() == 123456);
    }
    SECTION("reassign from hash string") {
        BlockNumberOrHash bnoh{10};
        bnoh = "0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c";
        CHECK(bnoh.is_undefined() == false);
        CHECK(bnoh.is_hash() == true);
        CHECK(bnoh.is_number() == false);
        CHECK(bnoh.is_tag() == false);
        CHECK(bnoh.hash() == 0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32);
    }
    SECTION("reassign from decimal number string") {
        BlockNumberOrHash bnoh{10};
        bnoh = "1966";
        CHECK(bnoh.is_undefined() == false);
        CHECK(bnoh.is_hash() == false);
        CHECK(bnoh.is_number() == true);
        CHECK(bnoh.is_tag() == false);
        CHECK(bnoh.number() == 1966);
    }
    SECTION("reassign from hex number string") {
        BlockNumberOrHash bnoh{10};
        bnoh = "0x374f3";
        CHECK(bnoh.is_undefined() == false);
        CHECK(bnoh.is_hash() == false);
        CHECK(bnoh.is_number() == true);
        CHECK(bnoh.is_tag() == false);
        CHECK(bnoh.number() == 0x374f3);
    }
    SECTION("reassign from tag string") {
        BlockNumberOrHash bnoh{10};
        bnoh = "latest";
        CHECK(bnoh.is_undefined() == false);
        CHECK(bnoh.is_hash() == false);
        CHECK(bnoh.is_number() == false);
        CHECK(bnoh.is_tag() == true);
        CHECK(bnoh.tag() == "latest");
    }
    SECTION("reassign from number") {
        BlockNumberOrHash bnoh{10};
        bnoh = 123456;
        CHECK(bnoh.is_undefined() == false);
        CHECK(bnoh.is_hash() == false);
        CHECK(bnoh.is_number() == true);
        CHECK(bnoh.is_tag() == false);
        CHECK(bnoh.number() == 123456);
    }
    SECTION("emptying") {
        BlockNumberOrHash bnoh{10};
        bnoh = "";
        CHECK(bnoh.is_undefined() == true);
        CHECK(bnoh.is_hash() == false);
        CHECK(bnoh.is_number() == false);
        CHECK(bnoh.is_tag() == false);
    }
    SECTION("number overflow") {
        BlockNumberOrHash bnoh{"0x1ffffffffffffffff"};
        CHECK(bnoh.is_undefined() == true);
        CHECK(bnoh.is_hash() == false);
        CHECK(bnoh.is_number() == false);
        CHECK(bnoh.is_tag() == false);
    }
    SECTION("invalid string") {
        BlockNumberOrHash bnoh{"invalid"};
        CHECK(bnoh.is_undefined() == true);
        CHECK(bnoh.is_hash() == false);
        CHECK(bnoh.is_number() == false);
        CHECK(bnoh.is_tag() == false);
    }
}
} // namespace silkrpc

