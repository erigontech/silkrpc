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

class AwaitableWrap {
public:
   AwaitableWrap(asio::io_context& context, StreamingClient& client) : kv_awaitable_{context, client} {} // tipo interfaccia
   virtual ~AwaitableWrap() {}
   asio::awaitable<int64_t> async_start() {
      int64_t tx_id = co_await kv_awaitable_.async_start(asio::use_awaitable);
      co_return tx_id;
   }
   asio::awaitable<int64_t> open_cursor(const std::string& table_name) {
      uint32_t cursor_id = co_await kv_awaitable_.async_open_cursor(table_name, asio::use_awaitable);
      co_return cursor_id;
   }
   asio::awaitable<remote::Pair> async_seek(uint32_t cursor_id, const silkworm::ByteView& key) {
      remote::Pair seek_pair = co_await kv_awaitable_.async_seek(cursor_id, key, asio::use_awaitable);
      co_return seek_pair;
   }

private:
   KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable_;
};


TEST_CASE("async_start") {
    SECTION("success with sync call") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {  start_completed(::grpc::Status::OK);}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               ::remote::Pair pair;
               pair.set_txid(4);
               read_completed(::grpc::Status::OK, pair);
          }
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
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
            auto result = std::async([&]() {
               std::this_thread::yield();
               ::remote::Pair pair;
               pair.set_txid(4);
               read_completed(::grpc::Status::OK, pair);
            });
          }
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
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
             ::remote::Pair pair;
             read_completed(::grpc::Status::CANCELLED, pair);
          }
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

TEST_CASE("open_cursor") {
    SECTION("success with sync call") {
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
          void completed(bool ok) override { }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      int64_t cursorid;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*(cp.get_context().io_context), sct };
        auto result{asio::co_spawn(cp.get_io_context(), test.open_cursor("table1"), asio::use_future)};
        cursorid = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(cursorid == 0x23);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("success with async call") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               auto result = std::async([&]() {
                  ::remote::Pair pair;
                  pair.set_cursorid(0x47);
                  read_completed(::grpc::Status::OK, pair);
               });
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               auto result = std::async([&]() {
                  write_completed(::grpc::Status::OK);
               });
          }
          void completed(bool ok) override { }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      int64_t cursor_id;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*(cp.get_context().io_context), sct };
        auto result{asio::co_spawn(cp.get_io_context(), test.open_cursor("table"), asio::use_future)};
        cursor_id = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(cursor_id == 0x47);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("read_start fails ") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::CANCELLED); }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               ::remote::Pair pair;
               read_completed(::grpc::Status::CANCELLED, pair);
          }
          void completed(bool ok) override { }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*(cp.get_context().io_context), sct };
        auto result{asio::co_spawn(cp.get_io_context(), test.open_cursor("table"), asio::use_future)};
        result.get();
       } catch (const std::system_error& e) {
             CHECK(e.code().value() == 1);
       }
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("write_start fails ") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::OK);}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::CANCELLED);
          }
          void completed(bool ok) override { }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*(cp.get_context().io_context), sct };
        auto result{asio::co_spawn(cp.get_io_context(), test.open_cursor("table"), asio::use_future)};
        result.get();
       } catch (const std::system_error& e) {
             CHECK(e.code().value() == 1);
       }
       cp.stop();
       context_pool_thread.join();
   }
}

TEST_CASE("async_seek") {
    SECTION("success with sync call") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               ::remote::Pair seek_pair;
               seek_pair.set_k("KEY1");
               seek_pair.set_v("VALUE1");
               read_completed(::grpc::Status::OK, seek_pair);
          }
          void completed(bool ok) override { }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      remote::Pair seek_pair;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*(cp.get_context().io_context), sct };
        silkworm::ByteView key;
        auto result{asio::co_spawn(cp.get_io_context(), test.async_seek(1, key), asio::use_future)};
        seek_pair = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(seek_pair.v() == "VALUE1");
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("success with async call") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               auto result = std::async([&]() {
                  write_completed(::grpc::Status::OK);
               });
          }
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               auto result = std::async([&]() {
                  ::remote::Pair seek_pair;
                  seek_pair.set_k("KEY1");
                  seek_pair.set_v("VALUE123");
                  read_completed(::grpc::Status::OK, seek_pair);
               });
          }
          void completed(bool ok) override { }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      remote::Pair seek_pair;
      try {
        MockStreamingClient sct;
        silkworm::ByteView key;
        AwaitableWrap test{*(cp.get_context().io_context), sct };
        auto result{asio::co_spawn(cp.get_io_context(), test.async_seek(1, key), asio::use_future)};
        seek_pair = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(seek_pair.v() == "VALUE123");
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("read_start fails ") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::CANCELLED); }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {
               ::remote::Pair pair;
               read_completed(::grpc::Status::CANCELLED, pair);
          }
          void completed(bool ok) override { }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        silkworm::ByteView key;
        AwaitableWrap test{*(cp.get_context().io_context), sct };
        auto result{asio::co_spawn(cp.get_io_context(), test.async_seek(1, key), asio::use_future)};
        result.get();
       } catch (const std::system_error& e) {
             CHECK(e.code().value() == 1);
       }
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("write_start fails ") {
       class MockStreamingClient : public StreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::OK);}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, ::remote::Pair)> read_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::CANCELLED);
          }
          void completed(bool ok) override { }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        silkworm::ByteView key;
        AwaitableWrap test{*(cp.get_context().io_context), sct };
        auto result{asio::co_spawn(cp.get_io_context(), test.async_seek(1, key), asio::use_future)};
        result.get();
       } catch (const std::system_error& e) {
             CHECK(e.code().value() == 1);
       }
       cp.stop();
       context_pool_thread.join();
   }
}


} // namespace silkrpc::ethdb::kv

