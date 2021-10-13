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

#include "awaitables.hpp"

#include <catch2/catch.hpp>
#include <silkrpc/config.hpp>
#include <asio/use_awaitable.hpp>
#include <silkrpc/ethdb/kv/streaming_client.hpp>

#include <asio/co_spawn.hpp>
#include <asio/thread_pool.hpp>
#include <asio/use_future.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/context_pool.hpp>
#include <silkworm/common/util.hpp>

namespace silkrpc::ethdb::kv {

using Catch::Matchers::Message;

TEST_CASE("awaitable") {
    SECTION("async_start success ") {
       class StreamingClientTest : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { printf("StreamingClientTest: start_call\n"); start_completed(::grpc::Status::OK);}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {printf("StreamingClientTest: read_call\n"); pair.set_txid(4); read_completed(::grpc::Status::OK, pair);}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
          void completed(bool ok) override { printf("StreamingClientTest: completed \n");}

          private:
            ::remote::Pair pair;
      };

      class TestAwaitable {
       public:
         TestAwaitable(asio::io_context& context, StreamingClientTest client) : kv_awaitable_{context, client} {}
         virtual ~TestAwaitable() {}
         asio::awaitable<int64_t> open() {
            printf("StreamingClientTest: open\n");
            int64_t tx_id = co_await kv_awaitable_.async_start(asio::use_awaitable);
            co_return tx_id;
         }

         StreamingClientTest client_;
         KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable_;
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });

      int64_t txid;
      try {
        StreamingClientTest sct;
        TestAwaitable test{*(cp.get_context().io_context), sct };
        auto result{asio::co_spawn(cp.get_io_context(), test.open(), asio::use_future)};
        txid = result.get();
       } catch (...) {
           CHECK(false);
       }

       CHECK(txid == 4);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("async_start fails ") {
       class StreamingClientTest : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { printf("StreamingClientTest: start_call\n"); start_completed(::grpc::Status::CANCELLED); }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {printf("StreamingClientTest: read_call\n"); pair.set_txid(4); read_completed(::grpc::Status::OK, pair);}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
          void completed(bool ok) override { printf("StreamingClientTest: completed \n"); }

          private:
            ::remote::Pair pair;
      };

      class TestAwaitable {
       public:
         TestAwaitable(asio::io_context& context, StreamingClientTest client) : kv_awaitable_{context, client} {}
         virtual ~TestAwaitable() {}
         asio::awaitable<int64_t> open() {
            printf("StreamingClientTest: open\n");
            int64_t tx_id = co_await kv_awaitable_.async_start(asio::use_awaitable);
            co_return tx_id;
         }

         StreamingClientTest client_;
         KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable_;
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });

      int64_t txid;
      try {
        StreamingClientTest sct;
        TestAwaitable test{*(cp.get_context().io_context), sct };
        auto result{asio::co_spawn(cp.get_io_context(), test.open(), asio::use_future)};
        txid = result.get();
       } catch (...) {
           CHECK(true);
       }

       cp.stop();
       context_pool_thread.join();
    }
}

} // namespace silkrpc::ethdb::kv

