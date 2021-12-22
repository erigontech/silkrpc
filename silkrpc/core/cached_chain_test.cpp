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

#include <string>

#include <silkrpc/config.hpp> // NOLINT(build/include_order)

#include <asio/awaitable.hpp>
#include <asio/thread_pool.hpp>
#include <evmc/evmc.hpp>
#include <nlohmann/json.hpp>

#include <silkrpc/context_pool.hpp>

#include <silkworm/common/util.hpp>
#include <silkworm/common/base.hpp>
#include <silkworm/chain/config.hpp>
#include <silkworm/common/log.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/types/receipt.hpp>

#include <silkrpc/core/rawdb/accessors.hpp>
#include <silkrpc/core/rawdb/chain.hpp>
#include <silkrpc/types/block.hpp>
#include <silkrpc/types/chain_config.hpp>

#include "cached_chain.hpp"


#include <catch2/catch.hpp>

namespace silkrpc {

using Catch::Matchers::Message;
using Catch::Matchers::Message;
using evmc::literals::operator""_address, evmc::literals::operator""_bytes32;

static silkworm::Bytes kKey{*silkworm::from_hex("00")};
static silkworm::Bytes kValue{*silkworm::from_hex("00")};


class MockDatabaseReader final : public core::rawdb::DatabaseReader {
public:
    MockDatabaseReader() {}

    asio::awaitable<KeyValue> get(const std::string& table, const silkworm::ByteView& key) const override {
        co_return KeyValue{};
    }

    asio::awaitable<silkworm::Bytes> get_one(const std::string& table, const silkworm::ByteView& key)  const override {
        co_return silkworm::Bytes{};
    }

    asio::awaitable<std::optional<silkworm::Bytes>> get_both_range(const std::string& table, const silkworm::ByteView& key, const silkworm::ByteView& subkey) const override  {
       co_return std::nullopt;
    }

    asio::awaitable<void> walk(const std::string& table, const silkworm::ByteView& start_key, uint32_t fixed_bits, silkrpc::core::rawdb::Walker w) const override  {
       co_return;
    }

    asio::awaitable<void> for_prefix(const std::string& table, const silkworm::ByteView& prefix, silkrpc::core::rawdb::Walker w) const override {
       co_return;
    }
};

TEST_CASE("", "[silkrpc][commands][eth_util_api]") {
    SECTION("read blocks by hash") {
       MockDatabaseReader mymock;
    }
}

} // namespace silkrpc

