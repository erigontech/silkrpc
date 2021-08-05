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

#include "async_operation.hpp"

#include <asio/error_code.hpp>
#include <catch2/catch.hpp>

namespace silkrpc {

using Catch::Matchers::Message;

TEST_CASE("call hook on async operation completion", "[silkrpc][grpc][async_operation]") {
    class AO : public async_operation<void, asio::error_code> {
    public:
        AO() : async_operation<void, asio::error_code>(&AO::do_complete) {}
        static void do_complete(void* owner, async_operation<void, asio::error_code>* base, asio::error_code error = {}) {
            auto self = static_cast<AO*>(base);
            self->called = true;
        }
        bool called{false};
    };
    AO op{};
    op.complete(reinterpret_cast<void*>(1), {});
    CHECK(op.called == true);
}

} // namespace silkrpc

