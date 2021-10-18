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

#include "remote_transaction.hpp"
#include "remote_cursor.hpp"

#include <catch2/catch.hpp>
#include <silkrpc/config.hpp>
#include <silkrpc/ethdb/kv/streaming_client.hpp>

#include <asio/co_spawn.hpp>
#include <asio/thread_pool.hpp>
#include <asio/use_future.hpp>
#include <asio/io_context.hpp>
#include <asio/use_awaitable.hpp>

#include <silkrpc/context_pool.hpp>


namespace silkrpc::ethdb::kv {

using Catch::Matchers::Message;

TEST_CASE("open") {
    SECTION("success") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {
             auto result = std::async([&]() {
               std::this_thread::yield();
               start_completed(::grpc::Status::OK);
             });
          }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               auto result = std::async([&]() {
                 ::remote::Pair pair;
                 pair.set_txid(4);
                 read_completed(::grpc::Status::OK, pair);
               });
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
          void completed(bool ok) override {}
      };
      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      auto queue = std::make_unique<grpc::CompletionQueue>();
      auto client = std::make_unique<MockStreamingClient>();
      RemoteTransaction rt(*cp.get_context().io_context, std::move(client));
      try {
        auto result{asio::co_spawn(cp.get_io_context(), rt.open(), asio::use_future)};
        result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(true);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("fail start_call") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {
               start_completed(::grpc::Status::CANCELLED);
          }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               auto result = std::async([&]() {
                 ::remote::Pair pair;
                 pair.set_txid(4);
                 read_completed(::grpc::Status::OK, pair);
               });
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
          void completed(bool ok) override {}
      };
      auto createChannel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      auto queue = std::make_unique<grpc::CompletionQueue>();
      auto client = std::make_unique<MockStreamingClient>();
      RemoteTransaction rt(*cp.get_context().io_context, std::move(client));
      try {
        auto result{asio::co_spawn(cp.get_io_context(), rt.open(), asio::use_future)};
        result.get();
       } catch (const std::system_error& e) {
            CHECK(e.code().value() == 1);
       } 
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("fail read_start") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {
               start_completed(::grpc::Status::OK);
          }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               auto result = std::async([&]() {
                 ::remote::Pair pair;
                 pair.set_txid(4);
                 read_completed(::grpc::Status::CANCELLED, pair);
               });
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
          void completed(bool ok) override {}
      };
      auto createChannel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      auto queue = std::make_unique<grpc::CompletionQueue>();
      auto client = std::make_unique<MockStreamingClient>();
      RemoteTransaction rt(*cp.get_context().io_context, std::move(client));
      try {
        auto result{asio::co_spawn(cp.get_io_context(), rt.open(), asio::use_future)};
        result.get();
       } catch (const std::system_error& e) {
            CHECK(e.code().value() == 1);
       } 
       cp.stop();
       context_pool_thread.join();
    }
}

TEST_CASE("close") {
    SECTION("success no cursor in table") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {
                 end_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
          void completed(bool ok) override {}
      };
      auto createChannel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      auto queue = std::make_unique<grpc::CompletionQueue>();
      auto client = std::make_unique<MockStreamingClient>();
      RemoteTransaction rt(*cp.get_context().io_context, std::move(client));
      try {
        auto result{asio::co_spawn(cp.get_io_context(), rt.close(), asio::use_future)};
        result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(true);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("success with cursor in table") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {
                 end_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
             ::remote::Pair pair;
             read_completed(::grpc::Status::OK, pair);
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::OK);
          }
          void completed(bool ok) override {}
      };
      auto createChannel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      auto queue = std::make_unique<grpc::CompletionQueue>();
      auto client = std::make_unique<MockStreamingClient>();
      RemoteTransaction rt(*cp.get_context().io_context, std::move(client));
      try {
        auto result{asio::co_spawn(cp.get_io_context(), rt.cursor("table1"), asio::use_future)};
        auto cursor = result.get();
        auto result1{asio::co_spawn(cp.get_io_context(), rt.close(), asio::use_future)};
        result1.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(true);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("fail start_call") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {
                 end_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
          void completed(bool ok) override {}
      };
      auto createChannel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      auto queue = std::make_unique<grpc::CompletionQueue>();
      auto client = std::make_unique<MockStreamingClient>();
      RemoteTransaction rt(*cp.get_context().io_context, std::move(client));
      try {
        auto result{asio::co_spawn(cp.get_io_context(), rt.close(), asio::use_future)};
        result.get();
       } catch (const std::system_error& e) {
            CHECK(e.code().value() == 1);
       } 
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("fail read_start") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {
               start_completed(::grpc::Status::OK);
          }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               auto result = std::async([&]() {
                 ::remote::Pair pair;
                 pair.set_txid(4);
                 read_completed(::grpc::Status::CANCELLED, pair);
               });
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
          void completed(bool ok) override {}
      };
      auto createChannel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      auto queue = std::make_unique<grpc::CompletionQueue>();
      auto client = std::make_unique<MockStreamingClient>();
      RemoteTransaction rt(*cp.get_context().io_context, std::move(client));
      try {
        auto result{asio::co_spawn(cp.get_io_context(), rt.open(), asio::use_future)};
        result.get();
       } catch (const std::system_error& e) {
            CHECK(e.code().value() == 1);
       } 
       cp.stop();
       context_pool_thread.join();
    }
  }

  TEST_CASE("cursor") {
    SECTION("success") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               ::remote::Pair pair;
               pair.set_cursorid(0x23);
               read_completed(::grpc::Status::OK, pair);
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::OK);
          }
          void completed(bool ok) override {}
      };
      MockStreamingClient sc;
      auto createChannel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      auto queue = std::make_unique<grpc::CompletionQueue>();
      auto client = std::make_unique<MockStreamingClient>();
      RemoteTransaction rt(*cp.get_context().io_context, std::move(client));
      try {
        auto result{asio::co_spawn(cp.get_io_context(), rt.cursor("table1"), asio::use_future)};
        auto cursor = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(true);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("success 2 cursor") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               ::remote::Pair pair;
               pair.set_cursorid(0x23);
               read_completed(::grpc::Status::OK, pair);
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::OK);
          }
          void completed(bool ok) override {}
       };
       auto createChannel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
       ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
       auto context_pool_thread = std::thread([&]() { cp.run(); });
       auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
       auto queue = std::make_unique<grpc::CompletionQueue>();
       auto client = std::make_unique<MockStreamingClient>();
       RemoteTransaction rt(*cp.get_context().io_context, std::move(client));
       try {
          auto result{asio::co_spawn(cp.get_io_context(), rt.cursor("table1"), asio::use_future)};
          auto cursor = result.get();
          auto result1{asio::co_spawn(cp.get_io_context(), rt.cursor("table1"), asio::use_future)};
          auto cursor1 = result1.get();
       } catch (...) {
             CHECK(false);
       }
       CHECK(true);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("fail write_start") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               ::remote::Pair pair;
               pair.set_cursorid(0x23);
               read_completed(::grpc::Status::OK, pair);
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::CANCELLED);
          }
          void completed(bool ok) override {}
      };
      auto createChannel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      auto queue = std::make_unique<grpc::CompletionQueue>();
      auto client = std::make_unique<MockStreamingClient>();
      RemoteTransaction rt(*cp.get_context().io_context, std::move(client));
      try {
        auto result{asio::co_spawn(cp.get_io_context(), rt.cursor("table1"), asio::use_future)};
        auto cursor = result.get();
       } catch (const std::system_error& e) {
            CHECK(e.code().value() == 1);
       } 
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("fail read_start") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               ::remote::Pair pair;
               pair.set_cursorid(0x23);
               read_completed(::grpc::Status::CANCELLED, pair);
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::OK);
          }
          void completed(bool ok) override {}
      };
      auto createChannel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      auto queue = std::make_unique<grpc::CompletionQueue>();
      auto client = std::make_unique<MockStreamingClient>();
      RemoteTransaction rt(*cp.get_context().io_context, std::move(client));
      try {
        auto result{asio::co_spawn(cp.get_io_context(), rt.cursor("table1"), asio::use_future)};
        auto cursor = result.get();
       } catch (const std::system_error& e) {
            CHECK(e.code().value() == 1);
       } 
       cp.stop();
       context_pool_thread.join();
    }
  }

  TEST_CASE("cursor_dup_sort") {
    SECTION("success") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               ::remote::Pair pair;
               pair.set_cursorid(0x23);
               read_completed(::grpc::Status::OK, pair);
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::OK);
          }
          void completed(bool ok) override {}
      };
      auto createChannel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      auto queue = std::make_unique<grpc::CompletionQueue>();
      auto client = std::make_unique<MockStreamingClient>();
      RemoteTransaction rt(*cp.get_context().io_context, std::move(client));
      try {
        auto result{asio::co_spawn(cp.get_io_context(), rt.cursor_dup_sort("table1"), asio::use_future)};
        auto cursor = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(true);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("success 2 cursor") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               ::remote::Pair pair;
               pair.set_cursorid(0x23);
               read_completed(::grpc::Status::OK, pair);
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::OK);
          }
          void completed(bool ok) override {}
       };
       auto createChannel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
       ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
       auto context_pool_thread = std::thread([&]() { cp.run(); });
       auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
       auto queue = std::make_unique<grpc::CompletionQueue>();
       auto client = std::make_unique<MockStreamingClient>();
       RemoteTransaction rt(*cp.get_context().io_context, std::move(client));
       try {
          auto result{asio::co_spawn(cp.get_io_context(), rt.cursor_dup_sort("table1"), asio::use_future)};
          auto cursor = result.get();
          auto result1{asio::co_spawn(cp.get_io_context(), rt.cursor_dup_sort("table1"), asio::use_future)};
          auto cursor1 = result1.get();
       } catch (...) {
             CHECK(false);
       }
       CHECK(true);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("fail write_start") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               ::remote::Pair pair;
               pair.set_cursorid(0x23);
               read_completed(::grpc::Status::OK, pair);
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::CANCELLED);
          }
          void completed(bool ok) override {}
      };
      auto createChannel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      auto queue = std::make_unique<grpc::CompletionQueue>();
      auto client = std::make_unique<MockStreamingClient>();
      RemoteTransaction rt(*cp.get_context().io_context, std::move(client));
      try {
        auto result{asio::co_spawn(cp.get_io_context(), rt.cursor_dup_sort("table1"), asio::use_future)};
        auto cursor = result.get();
       } catch (const std::system_error& e) {
            CHECK(e.code().value() == 1);
       } 
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("fail read_start") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               ::remote::Pair pair;
               pair.set_cursorid(0x23);
               read_completed(::grpc::Status::CANCELLED, pair);
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::OK);
          }
          void completed(bool ok) override {}
      };
      auto createChannel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
      auto queue = std::make_unique<grpc::CompletionQueue>();
      auto client = std::make_unique<MockStreamingClient>();
      RemoteTransaction rt(*cp.get_context().io_context, std::move(client));
      try {
        auto result{asio::co_spawn(cp.get_io_context(), rt.cursor_dup_sort("table1"), asio::use_future)};
        auto cursor = result.get();
       } catch (const std::system_error& e) {
            CHECK(e.code().value() == 1);
       } 
       cp.stop();
       context_pool_thread.join();
    }
 }
} // namespace silkrpc::ethdb::kv

