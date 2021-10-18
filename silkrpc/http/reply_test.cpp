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

#include "reply.hpp"

#include <catch2/catch.hpp>

namespace silkrpc::http {

using Catch::Matchers::Message;

TEST_CASE("check reply reset method", "[silkrpc][http][reply]") {
    Reply reply{
        Reply::StatusType::ok,
        std::vector<Header>{{"Accept", "*/*"}},
        "{\"json\": \"2.0\"}",
    };
    CHECK(reply.status == Reply::StatusType::ok);
    CHECK(reply.headers == std::vector<Header>{{"Accept", "*/*"}});
    CHECK(reply.content == "{\"json\": \"2.0\"}");
    reply.reset();
    CHECK(reply.headers == std::vector<Header>{});
    CHECK(reply.content == "");
}

} // namespace silkrpc::http
