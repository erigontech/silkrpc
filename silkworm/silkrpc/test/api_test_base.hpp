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

#ifndef SILKRPC_TEST_API_TEST_BASE_HPP_
#define SILKRPC_TEST_API_TEST_BASE_HPP_

#include <memory>
#include <silkrpc/config.hpp>
#include <silkrpc/test/context_test_base.hpp>
#include <utility>

namespace silkrpc::test {

template <typename JsonApi>
class JsonApiTestBase : public ContextTestBase {
  public:
    template <auto method, typename... Args>
    auto run(Args&&... args) {
        JsonApi api{context_};
        return spawn_and_wait((api.*method)(std::forward<Args>(args)...));
    }
};

template <typename GrpcApi, typename Stub>
class GrpcApiTestBase : public ContextTestBase {
  public:
    template <auto method, typename... Args>
    auto run(Args&&... args) {
        GrpcApi api{io_context_.get_executor(), std::move(stub_), grpc_context_};
        return spawn_and_wait((api.*method)(std::forward<Args>(args)...));
    }

    std::unique_ptr<Stub> stub_{std::make_unique<Stub>()};
};

}  // namespace silkrpc::test

#endif  // SILKRPC_TEST_API_TEST_BASE_HPP_
