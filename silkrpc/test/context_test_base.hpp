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

#ifndef SILKRPC_TEST_CONTEXT_TEST_BASE_HPP_
#define SILKRPC_TEST_CONTEXT_TEST_BASE_HPP_

#include <memory>
#include <utility>

#include <asio/co_spawn.hpp>
#include <asio/io_context.hpp>
#include <asio/use_future.hpp>

#include <silkrpc/config.hpp>
#include <silkrpc/concurrency/context_pool.hpp>

namespace silkrpc::test {

class ContextTestBase {
  public:
    ContextTestBase();

    template <typename AwaitableOrFunction>
    auto spawn_and_wait(AwaitableOrFunction&& awaitable) {
        return asio::co_spawn(io_context_, std::forward<AwaitableOrFunction>(awaitable), asio::use_future)
            .get();
    }

    ~ContextTestBase();

  private:
    bool init_dummy;

  public:
    Context context_;
    asio::io_context& io_context_;
    agrpc::GrpcContext& grpc_context_;
    std::thread context_thread_;
};

}  // namespace silkrpc::test

#endif  // SILKRPC_TEST_CONTEXT_TEST_BASE_HPP_
