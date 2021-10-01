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

#include "util.hpp"

#include <catch2/catch.hpp>
#include <silkworm/common/base.hpp>
#include <silkworm/common/util.hpp>

#include <silkrpc/common/log.hpp>

namespace silkrpc {

using Catch::Matchers::Message;

using evmc::literals::operator""_bytes32;

TEST_CASE("byte view from string", "[silkrpc][common][util]") {
    CHECK(silkworm::byte_view_of_string("").empty());
}

TEST_CASE("bytes from string", "[silkrpc][common][util]") {
    CHECK(silkworm::bytes_of_string("").empty());
}

TEST_CASE("calculate hash of byte array", "[silkrpc][common][util]") {
    const auto eth_hash{hash_of(silkworm::ByteView{})};
    CHECK(silkworm::to_bytes32(silkworm::ByteView{eth_hash.bytes, silkworm::kHashLength}) == silkworm::kEmptyHash);
}

TEST_CASE("calculate hash of transaction", "[silkrpc][common][util]") {
    const auto eth_hash{hash_of_transaction(silkworm::Transaction{})};
    CHECK(silkworm::to_bytes32(silkworm::ByteView{eth_hash.bytes, silkworm::kHashLength}) == 0x3763e4f6e4198413383534c763f3f5dac5c5e939f0a81724e3beb96d6e2ad0d5_bytes32);
}

TEST_CASE("print empty ByteView", "[silkrpc][common][util]") {
    silkworm::ByteView bv{};
    CHECK_NOTHROW(silkworm::null_stream() << bv);
}

TEST_CASE("print empty address", "[silkrpc][common][util]") {
    evmc::address addr{};
    CHECK_NOTHROW(silkworm::null_stream() << addr);
}

TEST_CASE("print empty bytes32", "[silkrpc][common][util]") {
    evmc::bytes32 b32{};
    CHECK_NOTHROW(silkworm::null_stream() << b32);
}

TEST_CASE("print empty const_buffer", "[silkrpc][common][util]") {
    asio::const_buffer cb{};
    CHECK_NOTHROW(silkworm::null_stream() << cb);
}

TEST_CASE("print empty vector of const_buffer", "[silkrpc][common][util]") {
    std::vector<asio::const_buffer> v{};
    CHECK_NOTHROW(silkworm::null_stream() << v);
}

} // namespace silkrpc

