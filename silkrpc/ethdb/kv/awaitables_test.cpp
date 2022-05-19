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
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_future.hpp>
#include <silkworm/common/util.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/context_pool.hpp>
#include <silkrpc/ethdb/kv/awaitables.hpp>

namespace silkrpc::ethdb::kv {

using Catch::Matchers::Message;

class AwaitableWrap {
public:
    AwaitableWrap(boost::asio::io_context& context, AsyncTxStreamingClient& client) : kv_awaitable_{context, client} {}

    boost::asio::awaitable<int64_t> async_start() {
        int64_t tx_id = co_await kv_awaitable_.async_start(boost::asio::use_awaitable);
        co_return tx_id;
    }

    boost::asio::awaitable<int64_t> open_cursor(const std::string& table_name) {
        uint32_t cursor_id = co_await kv_awaitable_.async_open_cursor(table_name, boost::asio::use_awaitable);
        co_return cursor_id;
    }

    boost::asio::awaitable<remote::Pair> async_seek(uint32_t cursor_id, const silkworm::ByteView& key) {
        remote::Pair seek_pair = co_await kv_awaitable_.async_seek(cursor_id, key, boost::asio::use_awaitable);
        co_return seek_pair;
    }

    boost::asio::awaitable<remote::Pair> async_seek_exact(uint32_t cursor_id, const silkworm::ByteView& key) {
        remote::Pair seek_pair = co_await kv_awaitable_.async_seek_exact(cursor_id, key, boost::asio::use_awaitable);
        co_return seek_pair;
    }

    boost::asio::awaitable<remote::Pair> async_seek_both(uint32_t cursor_id, const silkworm::ByteView& key, const silkworm::ByteView& value) {
        remote::Pair seek_pair = co_await kv_awaitable_.async_seek_both(cursor_id, key, value, boost::asio::use_awaitable);
        co_return seek_pair;
    }

    boost::asio::awaitable<remote::Pair> async_seek_both_exact(uint32_t cursor_id, const silkworm::ByteView& key, const silkworm::ByteView& value) {
        remote::Pair seek_pair = co_await kv_awaitable_.async_seek_both_exact(cursor_id, key, value, boost::asio::use_awaitable);
        co_return seek_pair;
    }

    boost::asio::awaitable<remote::Pair> async_next(uint32_t cursor_id) {
        remote::Pair seek_pair = co_await kv_awaitable_.async_next(cursor_id, boost::asio::use_awaitable);
        co_return seek_pair;
    }

    boost::asio::awaitable<uint32_t> async_close_cursor(uint32_t cursor_id) {
        uint32_t ret_cursor_id = co_await kv_awaitable_.async_close_cursor(cursor_id, boost::asio::use_awaitable);
        co_return ret_cursor_id;
    }

    boost::asio::awaitable<void> async_end() {
        co_return co_await kv_awaitable_.async_end(boost::asio::use_awaitable);
    }

private:
    KvAsioAwaitable<boost::asio::io_context::executor_type> kv_awaitable_;
};


TEST_CASE("async_start") {
    SECTION("success with sync call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {
             start_completed(::grpc::Status::OK);
          }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               ::remote::Pair pair;
               pair.set_txid(4);
               read_completed(::grpc::Status::OK, pair);
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      int64_t txid;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_start(), boost::asio::use_future)};
        txid = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(txid == 4);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("success with async call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {
            auto result = std::async([&]() {
               std::this_thread::yield();
               start_completed(::grpc::Status::OK);
            });
          }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
            auto result = std::async([&]() {
               std::this_thread::yield();
               ::remote::Pair pair;
               pair.set_txid(4);
               read_completed(::grpc::Status::OK, pair);
            });
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      int64_t txid;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_start(), boost::asio::use_future)};
        txid = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(txid == 4);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("start_call fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {
              start_completed(::grpc::Status::CANCELLED);
          }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_start(), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("read_start fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {
             start_completed(::grpc::Status::OK);
          }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
             ::remote::Pair pair;
             read_completed(::grpc::Status::CANCELLED, pair);
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_start(), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
   }
}

TEST_CASE("open_cursor") {
    SECTION("success with sync call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               ::remote::Pair pair;
               pair.set_cursorid(0x23);
               read_completed(::grpc::Status::OK, pair);
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::OK);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      int64_t cursorid;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.open_cursor("table1"), boost::asio::use_future)};
        cursorid = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(cursorid == 0x23);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("success with async call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
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
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      int64_t cursor_id;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.open_cursor("table"), boost::asio::use_future)};
        cursor_id = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(cursor_id == 0x47);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("read_start fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               ::remote::Pair pair;
               read_completed(::grpc::Status::CANCELLED, pair);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.open_cursor("table"), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("write_start fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::CANCELLED);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.open_cursor("table"), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
   }
}

TEST_CASE("async_seek") {
    SECTION("success with sync call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               ::remote::Pair seek_pair;
               seek_pair.set_k("KEY1");
               read_completed(::grpc::Status::OK, seek_pair);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      remote::Pair seek_pair;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        silkworm::ByteView key;
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_seek(1, key), boost::asio::use_future)};
        seek_pair = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(seek_pair.k() == "KEY1");
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("success with async call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               auto result = std::async([&]() {
                  write_completed(::grpc::Status::OK);
               });
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               auto result = std::async([&]() {
                  seek_pair.set_k("KEY1");
                  read_completed(::grpc::Status::OK, seek_pair);
               });
          }
       private:
           ::remote::Pair seek_pair;
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      remote::Pair seek_pair;
      try {
        MockStreamingClient sct;
        silkworm::ByteView key;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_seek(1, key), boost::asio::use_future)};
        seek_pair = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(seek_pair.k() == "KEY1");
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("read_start fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::CANCELLED); }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               ::remote::Pair pair;
               read_completed(::grpc::Status::CANCELLED, pair);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        silkworm::ByteView key;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_seek(1, key), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("write_start fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::OK);}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::CANCELLED);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        silkworm::ByteView key;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_seek(1, key), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
   }
}

TEST_CASE("async_seek_exact") {
    SECTION("success with sync call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               ::remote::Pair seek_pair;
               seek_pair.set_k("KEY1");
               read_completed(::grpc::Status::OK, seek_pair);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      remote::Pair seek_pair;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        silkworm::ByteView key;
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_seek_exact(1, key), boost::asio::use_future)};
        seek_pair = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(seek_pair.k() == "KEY1");
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("success with async call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               auto result = std::async([&]() {
                  write_completed(::grpc::Status::OK);
               });
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               auto result = std::async([&]() {
                  seek_pair.set_k("KEY1");
                  read_completed(::grpc::Status::OK, seek_pair);
               });
          }
        private:
          ::remote::Pair seek_pair;
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      remote::Pair seek_pair;
      try {
        MockStreamingClient sct;
        silkworm::ByteView key;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_seek_exact(1, key), boost::asio::use_future)};
        seek_pair = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(seek_pair.k() == "KEY1");
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("read_start fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::CANCELLED); }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               ::remote::Pair pair;
               read_completed(::grpc::Status::CANCELLED, pair);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        silkworm::ByteView key;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_seek_exact(1, key), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("write_start fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::OK);}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::CANCELLED);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        silkworm::ByteView key;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_seek_exact(1, key), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
   }
}

TEST_CASE("async_seek_both") {
    SECTION("success with sync call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               ::remote::Pair seek_pair;
               seek_pair.set_k("KEY1");
               seek_pair.set_v("VALUE112");
               read_completed(::grpc::Status::OK, seek_pair);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      remote::Pair seek_pair;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        silkworm::ByteView key;
        silkworm::ByteView value;
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_seek_both(1, key, value), boost::asio::use_future)};
        seek_pair = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(seek_pair.v() == "VALUE112");
       CHECK(seek_pair.k() == "KEY1");
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("success with async call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               auto result = std::async([&]() {
                  write_completed(::grpc::Status::OK);
               });
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               auto result = std::async([&]() {
                  seek_pair.set_k("KEY1");
                  seek_pair.set_v("VALUE123");
                  read_completed(::grpc::Status::OK, seek_pair);
               });
          }

          private:
              ::remote::Pair seek_pair;
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      remote::Pair seek_pair;
      try {
        MockStreamingClient sct;
        silkworm::ByteView key;
        silkworm::ByteView value;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_seek_both(1, key, value), boost::asio::use_future)};
        seek_pair = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(seek_pair.k() == "KEY1");
       CHECK(seek_pair.v() == "VALUE123");
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("read_start fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::CANCELLED); }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               ::remote::Pair pair;
               read_completed(::grpc::Status::CANCELLED, pair);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        silkworm::ByteView key;
        silkworm::ByteView value;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_seek_both(1, key, value), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("write_start fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::OK);}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::CANCELLED);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        silkworm::ByteView key;
        silkworm::ByteView value;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_seek_both(1, key, value), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
   }
}

TEST_CASE("async_seek_both_exact") {
    SECTION("success with sync call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               ::remote::Pair seek_pair;
               seek_pair.set_k("KEY1");
               seek_pair.set_v("VALUE112");
               read_completed(::grpc::Status::OK, seek_pair);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      remote::Pair seek_pair;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        silkworm::ByteView key;
        silkworm::ByteView value;
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_seek_both_exact(1, key, value), boost::asio::use_future)};
        seek_pair = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(seek_pair.v() == "VALUE112");
       CHECK(seek_pair.k() == "KEY1");
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("success with async call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               auto result = std::async([&]() {
                  write_completed(::grpc::Status::OK);
               });
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               auto result = std::async([&]() {
                  seek_pair.set_k("KEY1");
                  seek_pair.set_v("VALUE123");
                  read_completed(::grpc::Status::OK, seek_pair);
               });
          }

        private:
          ::remote::Pair seek_pair;
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      remote::Pair seek_pair;
      try {
        MockStreamingClient sct;
        silkworm::ByteView key;
        silkworm::ByteView value;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_seek_both_exact(1, key, value), boost::asio::use_future)};
        seek_pair = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(seek_pair.k() == "KEY1");
       CHECK(seek_pair.v() == "VALUE123");
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("read_start fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::CANCELLED); }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               ::remote::Pair pair;
               read_completed(::grpc::Status::CANCELLED, pair);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        silkworm::ByteView key;
        silkworm::ByteView value;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_seek_both_exact(1, key, value), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("write_start fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::OK);}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::CANCELLED);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        silkworm::ByteView key;
        silkworm::ByteView value;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_seek_both_exact(1, key, value), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
   }
}

TEST_CASE("async_seek_next") {
    SECTION("success with sync call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               ::remote::Pair next_pair;
               read_completed(::grpc::Status::OK, next_pair);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      remote::Pair seek_pair;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_next(1), boost::asio::use_future)};
        seek_pair = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(true);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("success with async call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               auto result = std::async([&]() {
                  write_completed(::grpc::Status::OK);
               });
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               auto result = std::async([&]() {
                  read_completed(::grpc::Status::OK, next_pair);
               });
          }

        private:
          ::remote::Pair next_pair;
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      remote::Pair seek_pair;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_next(1), boost::asio::use_future)};
        seek_pair = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(true);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("read_start fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::CANCELLED); }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               ::remote::Pair next_pair;
               read_completed(::grpc::Status::CANCELLED, next_pair);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_next(1), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("write_start fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::OK);}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::CANCELLED);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_next(1), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
   }
}

TEST_CASE("async_close_cursor") {
    SECTION("success with sync call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               write_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               ::remote::Pair close_pair;
               close_pair.set_cursorid(2);
               read_completed(::grpc::Status::OK, close_pair);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      uint32_t cursor_id;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_close_cursor(2), boost::asio::use_future)};
        cursor_id = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(cursor_id == 2);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("success with async call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
               auto result = std::async([&]() {
                  write_completed(::grpc::Status::OK);
               });
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               auto result = std::async([&]() {
                  close_pair.set_cursorid(2);
                  read_completed(::grpc::Status::OK, close_pair);
               });
          }
       private:
           ::remote::Pair close_pair;
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      uint32_t cursor_id;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_close_cursor(2), boost::asio::use_future)};
        cursor_id = result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(cursor_id == 2);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("read_start fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::CANCELLED); }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::OK);
          }
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {
               ::remote::Pair close_pair;
               read_completed(::grpc::Status::CANCELLED, close_pair);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_close_cursor(2), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("write_start fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::OK);}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {}
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {
             write_completed(::grpc::Status::CANCELLED);
          }
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_close_cursor(2), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
   }
}

TEST_CASE("async_end") {
    SECTION("success with sync call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {
               end_completed(::grpc::Status::OK);
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {}
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_end(), boost::asio::use_future)};
        result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(true);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("success with async call") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override {}
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {
               auto result = std::async([&]() {
                  end_completed(::grpc::Status::OK);
               });
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {}
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      uint32_t cursor_id;
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_end(), boost::asio::use_future)};
        result.get();
       } catch (...) {
           CHECK(false);
       }
       CHECK(true);
       cp.stop();
       context_pool_thread.join();
    }

    SECTION("end_call fails ") {
       class MockStreamingClient : public AsyncTxStreamingClient {
          void start_call(std::function<void(const grpc::Status&)> start_completed) override { start_completed(::grpc::Status::CANCELLED); }
          void end_call(std::function<void(const grpc::Status&)> end_completed) override {
               end_completed(::grpc::Status::CANCELLED);
          }
          void write_start(const ::remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
          void read_start(std::function<void(const grpc::Status&, const ::remote::Pair&)> read_completed) override {}
      };

      ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
      auto context_pool_thread = std::thread([&]() { cp.run(); });
      try {
        MockStreamingClient sct;
        AwaitableWrap test{*cp.next_context().io_context(), sct };
        auto result{boost::asio::co_spawn(cp.next_io_context(), test.async_end(), boost::asio::use_future)};
        result.get();
       } catch (const boost::system::system_error& e) {
             CHECK(std::error_code(e.code()).value() == grpc::StatusCode::CANCELLED);
       }
       cp.stop();
       context_pool_thread.join();
    }
}


} // namespace silkrpc::ethdb::kv

