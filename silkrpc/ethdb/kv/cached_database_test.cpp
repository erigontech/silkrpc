/*
   Copyright 2020-2022 The Silkrpc Authors

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

#include "cached_database.hpp"

#include <asio/thread_pool.hpp>
#include <catch2/catch.hpp>
#include <gmock/gmock.h>

#include <silkrpc/ethdb/kv/state_cache.hpp>
#include <silkrpc/test/mock_transaction.hpp>
#include <silkrpc/types/block.hpp>

namespace silkrpc::ethdb::kv {

using Catch::Matchers::Message;
using testing::_;
using testing::InvokeWithoutArgs;

TEST_CASE("CachedDatabase", "[silkrpc][ethdb][kv][cached_reader]") {
    /*BlockNumberOrHash block_id{0};
    test::MockTransaction txn;
    kv::CoherentStateCache cache;
    CachedDatabase database{block_id, txn, cache};
    asio::thread_pool pool{1};*/

    // TODO(canepat) complete
}

}  // namespace silkrpc::ethdb::kv
