/*
   Copyright 2022 The Silkrpc Authors

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

#include "state_changes_stream.hpp"

#include <future>
#include <system_error>

#include <asio/co_spawn.hpp>
#include <asio/use_future.hpp>
#include <asio/io_context.hpp>
#include <catch2/catch.hpp>

namespace silkrpc::ethdb::kv {

using Catch::Matchers::Message;

TEST_CASE("StateChangesStream::set_registration_interval", "[silkrpc][ethdb][kv][state_changes_stream]") {
}

} // namespace silkrpc::ethdb::kv
