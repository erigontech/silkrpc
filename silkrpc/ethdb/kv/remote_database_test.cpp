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

#include "remote_database.hpp"

#include <future>
#include <system_error>

#include <asio/co_spawn.hpp>
#include <asio/use_future.hpp>
#include <asio/io_context.hpp>
#include <catch2/catch.hpp>

#include <silkrpc/ethdb/kv/tx_streaming_client.hpp>

namespace silkrpc::ethdb::kv {

using Catch::Matchers::Message;

TEST_CASE("RemoteDatabase::begin", "[silkrpc][ethdb][kv][remote_database]") {
    SECTION("success") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {
                auto result = std::async([&]() {
                    start_completed(::grpc::Status::OK);
                });
            }
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                auto result = std::async([&]() {
                    remote::Pair pair;
                    pair.set_txid(4);
                    read_completed(::grpc::Status::OK, pair);
                });
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        CoherentStateCache state_cache;
        RemoteDatabase<MockStreamingClient> remote_db(io_context, channel, &queue, &state_cache);
        try {
            auto future_remote_tx{asio::co_spawn(io_context, remote_db.begin(), asio::use_future)};
            io_context.run();
            auto remote_tx = future_remote_tx.get();
            CHECK(remote_tx->tx_id() == 4);
            CHECK(true);
        } catch (...) {
            CHECK(false);
        }
    }

    SECTION("start_call failure") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {
                auto result = std::async([&]() {
                    start_completed(::grpc::Status::CANCELLED);
                });
            }
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                auto result = std::async([&]() {
                    remote::Pair pair;
                    pair.set_txid(4);
                    read_completed(::grpc::Status::OK, pair);
                });
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        CoherentStateCache state_cache;
        RemoteDatabase<MockStreamingClient> remote_db(io_context, channel, &queue, &state_cache);
        try {
            auto future_remote_tx{asio::co_spawn(io_context, remote_db.begin(), asio::use_future)};
            io_context.run();
            auto remote_tx = future_remote_tx.get();
            CHECK(remote_tx->tx_id() == 4);
            CHECK(false);
        } catch (const std::system_error& e) {
            CHECK(e.code().value() == grpc::StatusCode::CANCELLED);
        }
    }

    SECTION("read_start failure") {
        class MockStreamingClient : public AsyncTxStreamingClient {
        public:
            MockStreamingClient(std::unique_ptr<remote::KV::StubInterface>& /*stub*/, grpc::CompletionQueue* /*queue*/) {}
            void start_call(std::function<void(const grpc::Status&)> start_completed) override {
                auto result = std::async([&]() {
                    start_completed(::grpc::Status::OK);
                });
            }
            void end_call(std::function<void(const grpc::Status&)> end_completed) override {}
            void read_start(std::function<void(const grpc::Status&, const remote::Pair&)> read_completed) override {
                auto result = std::async([&]() {
                    read_completed(::grpc::Status::CANCELLED, {});
                });
            }
            void write_start(const remote::Cursor& cursor, std::function<void(const grpc::Status&)> write_completed) override {}
        };
        asio::io_context io_context;
        auto channel = grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
        grpc::CompletionQueue queue;
        CoherentStateCache state_cache;
        RemoteDatabase<MockStreamingClient> remote_db(io_context, channel, &queue, &state_cache);
        try {
            auto future_remote_tx{asio::co_spawn(io_context, remote_db.begin(), asio::use_future)};
            io_context.run();
            auto remote_tx = future_remote_tx.get();
            CHECK(remote_tx->tx_id() == 4);
            CHECK(false);
        } catch (const std::system_error& e) {
            CHECK(e.code().value() == grpc::StatusCode::CANCELLED);
        }
    }
}

} // namespace silkrpc::ethdb::kv
