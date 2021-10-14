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

TEST_CASE("async_start") {
      class AwaitableWrap { 
       public:
         AwaitableWrap(asio::io_context& context, StreamingClient& client) : kv_awaitable_{context, client} {} // tipo interfaccia
         virtual ~AwaitableWrap() {}
         asio::awaitable<int64_t> async_start() {
            int64_t tx_id = co_await kv_awaitable_.async_start(asio::use_awaitable);
            co_return tx_id;
         }
         asio::awaitable<int64_t> async_open_cursor(const std::string& table_name) {
            uint32_t cursor_id = co_await kv_awaitable_.async_open_cursor(table_name, asio::use_awaitable);
            co_return cursor_id;
         }
         KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable_;
      };

    SECTION("success with sync call") {
       class MockStreamingClient : public StreamingClient { // asincrona std::async MockStreamingClient
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {  start_completed(::grpc::Status::OK);}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override { ::remote::Pair pair; pair.set_txid(4); read_completed(::grpc::Status::OK, pair);}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
          void completed(bool ok) override { }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      int64_t txid;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*(cp.get_context().io_context), sct }; 
        auto result{asio::co_spawn(cp.get_io_context(), test.async_start(), asio::use_future)};
        txid = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(txid == 4);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("success with async call") {
       class MockStreamingClient : public StreamingClient { 
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { 
            auto result = std::async([&]() {
               std::this_thread::yield();
               start_completed(::grpc::Status::OK);
            });
          }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override { ::remote::Pair pair; pair.set_txid(4); read_completed(::grpc::Status::OK, pair);}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
          void completed(bool ok) override { }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      int64_t txid;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*(cp.get_context().io_context), sct }; 
        auto result{asio::co_spawn(cp.get_io_context(), test.async_start(), asio::use_future)};
        txid = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(txid == 4);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("start_call fails ") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::CANCELLED); }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
          void completed(bool ok) override {}
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*(cp.get_context().io_context), sct }; 
        auto result{asio::co_spawn(cp.get_io_context(), test.async_start(), asio::use_future)};
        result.get();
       } catch (const std::system_error& e) { 
             CHECK(e.code().value() == 1);
       }
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("read_call fails ") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::OK);}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override { ::remote::Pair pair; pair.set_txid(4); read_completed(::grpc::Status::CANCELLED, pair);}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
          void completed(bool ok) override {}
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*(cp.get_context().io_context), sct }; 
        auto result{asio::co_spawn(cp.get_io_context(), test.async_start(), asio::use_future)};
        result.get();
       } catch (const std::system_error& e) { 
             CHECK(e.code().value() == 1);
       }
       cp.stop();
       context_pool_thread.join();
   }
}

} // namespace silkrpc::ethdb::kv

