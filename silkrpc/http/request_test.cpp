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

#include "request.hpp"

#include <catch2/catch.hpp>

namespace silkrpc {

using Catch::Matchers::Message;

TEST_CASE("check reset method", "[silkrpc][request]") {
    silkrpc::http::Request req {
        "eth_call",
        "",
        1,
        3,
        {{"v", "1"}},
        4,
        "5678", 
    };
    req.reset();
    CHECK(req.method == "");    
    CHECK(req.uri == "");    
    CHECK(req.content == "");    
    CHECK(req.content_length == 0);    
    CHECK(req.headers.size() == 0);    
} 

} // namespace silkrpc

