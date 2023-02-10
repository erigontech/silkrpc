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

#pragma once

#include <grpcpp/grpcpp.h>

#include <boost/system/system_error.hpp>
#include <catch2/catch.hpp>

namespace silkrpc::test {

inline auto exception_has_grpc_status_code(grpc::StatusCode status_code) {
    return Catch::Predicate<const boost::system::system_error&>(
        [status_code](auto& e) { return std::error_code(e.code()).value() == status_code; });
}

inline auto exception_has_cancelled_grpc_status_code() {
    return test::exception_has_grpc_status_code(grpc::StatusCode::CANCELLED);
}

inline auto exception_has_unknown_grpc_status_code() {
    return test::exception_has_grpc_status_code(grpc::StatusCode::UNKNOWN);
}

}  // namespace silkrpc::test
