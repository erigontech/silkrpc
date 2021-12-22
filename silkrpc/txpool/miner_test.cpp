/*
   Copyright 2020-2021 The Silkrpc Authors

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

#include "miner.hpp"

#include <functional>
#include <memory>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>

#include <asio/io_context.hpp>
#include <asio/co_spawn.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch.hpp>
#include <evmc/evmc.hpp>
#include <gmock/gmock.h>
#include <grpcpp/grpcpp.h>
#include <silkworm/common/base.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/completion_runner.hpp>
#include <silkrpc/interfaces/txpool/mining.grpc.pb.h>
#include <silkrpc/interfaces/txpool/mining_mock_fix24351.grpc.pb.h>

namespace grpc {

inline bool operator==(const Status& lhs, const Status& rhs) {
    return lhs.error_code() == rhs.error_code() &&
        lhs.error_message() == rhs.error_message() &&
        lhs.error_details() == rhs.error_details();
}

} // namespace grpc

namespace txpool {

inline bool operator==(const GetWorkReply& lhs, const GetWorkReply& rhs) {
    if (lhs.headerhash() != rhs.headerhash()) return false;
    if (lhs.seedhash() != rhs.seedhash()) return false;
    if (lhs.target() != rhs.target()) return false;
    if (lhs.blocknumber() != rhs.blocknumber()) return false;
    return true;
}

} // namespace txpool

namespace silkrpc::txpool {

using Catch::Matchers::Message;
using testing::MockFunction;
using testing::Return;
using testing::_;

using evmc::literals::operator""_bytes32;

TEST_CASE("create GetWorkClient", "[silkrpc][txpool][miner]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    class MockClientAsyncGetWorkReader final : public grpc::ClientAsyncResponseReaderInterface<::txpool::GetWorkReply> {
    public:
        MockClientAsyncGetWorkReader(::txpool::GetWorkReply msg, ::grpc::Status status) : msg_(std::move(msg)), status_(std::move(status)) {}
        ~MockClientAsyncGetWorkReader() final = default;
        void StartCall() override {};
        void ReadInitialMetadata(void* tag) override {};
        void Finish(::txpool::GetWorkReply* msg, ::grpc::Status* status, void* tag) override {
            *msg = msg_;
            *status = status_;
        };
    private:
        ::txpool::GetWorkReply msg_;
        grpc::Status status_;
    };

    SECTION("start async GetWork call, get status OK and non-empty work") {
        std::unique_ptr<::txpool::Mining::StubInterface> stub{std::make_unique<::txpool::FixIssue24351_MockMiningStub>()};
        grpc::CompletionQueue queue;
        txpool::GetWorkClient client{stub, &queue};

        ::txpool::GetWorkReply reply;
        reply.set_headerhash("0x209f062567c161c5f71b3f57a7de277b0e95c3455050b152d785ad7524ef8ee7");
        reply.set_seedhash("0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347");
        reply.set_target("0xe7536c5b61ed0e0ab7f3ce7f085806d40f716689c0c086676757de401b595658");
        reply.set_blocknumber("0x00000000");
        MockClientAsyncGetWorkReader mock_reader{reply, ::grpc::Status::OK};
        EXPECT_CALL(*dynamic_cast<::txpool::FixIssue24351_MockMiningStub*>(stub.get()), PrepareAsyncGetWorkRaw(_, _, _)).WillOnce(Return(&mock_reader));

        MockFunction<void(::grpc::Status, ::txpool::GetWorkReply)> mock_callback;
        EXPECT_CALL(mock_callback, Call(grpc::Status::OK, reply));

        client.async_call(::txpool::GetWorkRequest{}, mock_callback.AsStdFunction());
        client.completed(true);
    }

    SECTION("start async GetWork call, get status OK and empty work") {
        std::unique_ptr<::txpool::Mining::StubInterface> stub{std::make_unique<::txpool::FixIssue24351_MockMiningStub>()};
        grpc::CompletionQueue queue;
        txpool::GetWorkClient client{stub, &queue};

        MockClientAsyncGetWorkReader mock_reader{::txpool::GetWorkReply{}, ::grpc::Status::OK};
        EXPECT_CALL(*dynamic_cast<::txpool::FixIssue24351_MockMiningStub*>(stub.get()), PrepareAsyncGetWorkRaw(_, _, _)).WillOnce(Return(&mock_reader));

        MockFunction<void(::grpc::Status, ::txpool::GetWorkReply)> mock_callback;
        EXPECT_CALL(mock_callback, Call(grpc::Status::OK, ::txpool::GetWorkReply{}));

        client.async_call(::txpool::GetWorkRequest{}, mock_callback.AsStdFunction());
        client.completed(true);
    }

    SECTION("start async GetWork call and get status KO") {
        std::unique_ptr<::txpool::Mining::StubInterface> stub{std::make_unique<::txpool::FixIssue24351_MockMiningStub>()};
        grpc::CompletionQueue queue;
        txpool::GetWorkClient client{stub, &queue};

        MockClientAsyncGetWorkReader mock_reader{::txpool::GetWorkReply{}, ::grpc::Status{::grpc::StatusCode::INTERNAL, "internal error"}};
        EXPECT_CALL(*dynamic_cast<::txpool::FixIssue24351_MockMiningStub*>(stub.get()), PrepareAsyncGetWorkRaw(_, _, _)).WillOnce(Return(&mock_reader));

        MockFunction<void(::grpc::Status, ::txpool::GetWorkReply)> mock_callback;
        EXPECT_CALL(mock_callback, Call(::grpc::Status{::grpc::StatusCode::INTERNAL, "internal error"}, ::txpool::GetWorkReply{}));

        client.async_call(::txpool::GetWorkRequest{}, mock_callback.AsStdFunction());
        client.completed(false);
    }
}

class EmptyMiningService : public ::txpool::Mining::Service {
public:
    ::grpc::Status GetWork(::grpc::ServerContext* context, const ::txpool::GetWorkRequest* request, ::txpool::GetWorkReply* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status SubmitWork(::grpc::ServerContext* context, const ::txpool::SubmitWorkRequest* request, ::txpool::SubmitWorkReply* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status HashRate(::grpc::ServerContext* context, const ::txpool::HashRateRequest* request, ::txpool::HashRateReply* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status SubmitHashRate(::grpc::ServerContext* context, const ::txpool::SubmitHashRateRequest* request, ::txpool::SubmitHashRateReply* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status Mining(::grpc::ServerContext* context, const ::txpool::MiningRequest* request, ::txpool::MiningReply* response) override {
        return ::grpc::Status::OK;
    }
};

class ClientServerTestBox {
public:
    explicit ClientServerTestBox(grpc::Service* service) : completion_runner_{queue_, io_context_} {
        server_address_ << "localhost:" << 12345; // TODO(canepat): grpc_pick_unused_port_or_die
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address_.str(), grpc::InsecureServerCredentials());
        builder.RegisterService(service);
        server_ = builder.BuildAndStart();
        io_context_thread_ = std::thread([&]() { io_context_.run(); });
        completion_runner_thread_ = std::thread([&]() { completion_runner_.run(); });
    }

    template<auto method, typename T>
    auto make_method_proxy() {
        const auto channel = grpc::CreateChannel(server_address_.str(), grpc::InsecureChannelCredentials());
        std::unique_ptr<T> target{std::make_unique<T>(io_context_, channel, &queue_)};
        return [target = std::move(target)](auto&&... args) {
            return (target.get()->*method)(std::forward<decltype(args)>(args)...);
        };
    }

    ~ClientServerTestBox() {
        server_->Shutdown();
        io_context_.stop();
        completion_runner_.stop();
        if (io_context_thread_.joinable()) {
            io_context_thread_.join();
        }
        if (completion_runner_thread_.joinable()) {
            completion_runner_thread_.join();
        }
    }

private:
    asio::io_context io_context_;
    asio::io_context::work work_{io_context_};
    grpc::CompletionQueue queue_;
    CompletionRunner completion_runner_;
    std::ostringstream server_address_;
    std::unique_ptr<grpc::Server> server_;
    std::thread io_context_thread_;
    std::thread completion_runner_thread_;
};

template<typename T, auto method, typename R, typename ...Args>
asio::awaitable<R> test_comethod(::txpool::Mining::Service* service, Args... args) {
    ClientServerTestBox test_box{service};
    auto method_proxy{test_box.make_method_proxy<method, T>()};
    co_return co_await method_proxy(args...);
}

auto test_get_work = test_comethod<txpool::Miner, &txpool::Miner::get_work, txpool::WorkResult>;
auto test_submit_work = test_comethod<txpool::Miner, &txpool::Miner::submit_work, bool, silkworm::Bytes, evmc::bytes32, evmc::bytes32>;
auto test_get_hashrate = test_comethod<txpool::Miner, &txpool::Miner::get_hash_rate, uint64_t>;
auto test_submit_hashrate = test_comethod<txpool::Miner, &txpool::Miner::submit_hash_rate, bool, intx::uint256, evmc::bytes32>;
auto test_get_mining = test_comethod<txpool::Miner, &txpool::Miner::get_mining, txpool::MiningResult>;

TEST_CASE("Miner::get_work", "[silkrpc][txpool][miner]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("call get_work and get result") {
        class TestSuccessMiningService : public ::txpool::Mining::Service {
        public:
            ::grpc::Status GetWork(::grpc::ServerContext* context, const ::txpool::GetWorkRequest* request, ::txpool::GetWorkReply* response) override {
                response->set_headerhash("0x209f062567c161c5f71b3f57a7de277b0e95c3455050b152d785ad7524ef8ee7");
                response->set_seedhash("0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347");
                response->set_target("0xe7536c5b61ed0e0ab7f3ce7f085806d40f716689c0c086676757de401b595658");
                response->set_blocknumber("0x00000000");
                return ::grpc::Status::OK;
            }
        };
        TestSuccessMiningService service;
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_get_work(&service), asio::use_future)};
        io_context.run();
        const auto work_result = result.get();
        CHECK(work_result.header_hash == 0x209f062567c161c5f71b3f57a7de277b0e95c3455050b152d785ad7524ef8ee7_bytes32);
        CHECK(work_result.seed_hash == 0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347_bytes32);
        CHECK(work_result.target == 0xe7536c5b61ed0e0ab7f3ce7f085806d40f716689c0c086676757de401b595658_bytes32);
        CHECK(work_result.block_number == *silkworm::from_hex("0x00000000"));
    }

    SECTION("call get_work and get default empty result") {
        EmptyMiningService service;
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_get_work(&service), asio::use_future)};
        io_context.run();
        const auto work_result = result.get();
        CHECK(!work_result.header_hash);
        CHECK(!work_result.seed_hash);
        CHECK(!work_result.target);
        CHECK(work_result.block_number == *silkworm::from_hex("0x"));
    }

    SECTION("call get_work and get failure") {
        class TestFailureMiningService : public ::txpool::Mining::Service {
        public:
            ::grpc::Status GetWork(::grpc::ServerContext* context, const ::txpool::GetWorkRequest* request, ::txpool::GetWorkReply* response) override {
                return ::grpc::Status::CANCELLED;
            }
        };
        TestFailureMiningService service;
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_get_work(&service), asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(result.get(), std::system_error);
    }
}

TEST_CASE("Miner::get_hashrate", "[silkrpc][txpool][miner]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("call get_hashrate and get result") {
        class TestSuccessMiningService : public ::txpool::Mining::Service {
        public:
            ::grpc::Status HashRate(::grpc::ServerContext* context, const ::txpool::HashRateRequest* request, ::txpool::HashRateReply* response) override {
                response->set_hashrate(1234567);
                return ::grpc::Status::OK;
            }
        };
        TestSuccessMiningService service;
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_get_hashrate(&service), asio::use_future)};
        io_context.run();
        const auto hash_rate = result.get();
        CHECK(hash_rate == 1234567);
    }

    SECTION("call get_hashrate and get default empty result") {
        EmptyMiningService service;
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_get_hashrate(&service), asio::use_future)};
        io_context.run();
        const auto hash_rate = result.get();
        CHECK(hash_rate == 0);
    }

    SECTION("call get_hashrate and get failure") {
        class TestFailureMiningService : public ::txpool::Mining::Service {
        public:
            ::grpc::Status HashRate(::grpc::ServerContext* context, const ::txpool::HashRateRequest* request, ::txpool::HashRateReply* response) override {
                return ::grpc::Status::CANCELLED;
            }
        };
        TestFailureMiningService service;
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_get_hashrate(&service), asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(result.get(), std::system_error);
    }
}

TEST_CASE("Miner::get_mining", "[silkrpc][txpool][miner]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    class TestSuccessMiningService : public ::txpool::Mining::Service {
    public:
        explicit TestSuccessMiningService(bool enabled, bool running) : enabled_(enabled), running_(running) {}

        ::grpc::Status Mining(::grpc::ServerContext* context, const ::txpool::MiningRequest* request, ::txpool::MiningReply* response) override {
            response->set_enabled(enabled_);
            response->set_running(running_);
            return ::grpc::Status::OK;
        }

    private:
        bool enabled_;
        bool running_;
    };

    SECTION("call get_mining and get result for enabled and running") {
        TestSuccessMiningService service{true, true};
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_get_mining(&service), asio::use_future)};
        io_context.run();
        const auto mining_result = result.get();
        CHECK(mining_result.enabled);
        CHECK(mining_result.running);
    }

    SECTION("call get_mining and get result for enabled and running") {
        TestSuccessMiningService service{/*enabled=*/true, /*running=*/true};
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_get_mining(&service), asio::use_future)};
        io_context.run();
        const auto mining_result = result.get();
        CHECK(mining_result.enabled);
        CHECK(mining_result.running);
    }

    SECTION("call get_mining and get result for enabled and not running") {
        TestSuccessMiningService service{/*enabled=*/true, /*running=*/false};
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_get_mining(&service), asio::use_future)};
        io_context.run();
        const auto mining_result = result.get();
        CHECK(mining_result.enabled);
        CHECK(!mining_result.running);
    }

    SECTION("call get_mining and get result for not enabled and not running") {
        TestSuccessMiningService service{/*enabled=*/false, /*running=*/false};
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_get_mining(&service), asio::use_future)};
        io_context.run();
        const auto mining_result = result.get();
        CHECK(!mining_result.enabled);
        CHECK(!mining_result.running);
    }

    SECTION("call get_mining and get default empty result") {
        EmptyMiningService service;
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_get_mining(&service), asio::use_future)};
        io_context.run();
        const auto mining_result = result.get();
        CHECK(!mining_result.enabled);
        CHECK(!mining_result.running);
    }

    SECTION("call get_mining and get failure") {
        class TestFailureMiningService : public ::txpool::Mining::Service {
        public:
            ::grpc::Status Mining(::grpc::ServerContext* context, const ::txpool::MiningRequest* request, ::txpool::MiningReply* response) override {
                return ::grpc::Status::CANCELLED;
            }
        };
        TestFailureMiningService service;
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_get_mining(&service), asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(result.get(), std::system_error);
    }
}

TEST_CASE("Miner::submit_work", "[silkrpc][txpool][miner]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    class TestSuccessMiningService : public ::txpool::Mining::Service {
    public:
        explicit TestSuccessMiningService(bool ok) : ok_(ok) {}

        ::grpc::Status SubmitWork(::grpc::ServerContext* context, const ::txpool::SubmitWorkRequest* request, ::txpool::SubmitWorkReply* response) override {
            response->set_ok(ok_);
            return ::grpc::Status::OK;
        }

    private:
        bool ok_;
    };

    SECTION("call submit_work and get OK result") {
        TestSuccessMiningService service{/*ok=*/true};
        silkworm::Bytes block_nonce{}; // don't care
        evmc::bytes32 pow_hash{silkworm::kEmptyHash}; // don't care
        evmc::bytes32 digest{silkworm::kEmptyHash}; // don't care
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_submit_work(&service, block_nonce, pow_hash, digest), asio::use_future)};
        io_context.run();
        const auto ok = result.get();
        CHECK(ok);
    }

    SECTION("call submit_work and get KO result") {
        TestSuccessMiningService service{/*ok=*/false};
        silkworm::Bytes block_nonce{}; // don't care
        evmc::bytes32 pow_hash{silkworm::kEmptyHash}; // don't care
        evmc::bytes32 digest{silkworm::kEmptyHash}; // don't care
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_submit_work(&service, block_nonce, pow_hash, digest), asio::use_future)};
        io_context.run();
        const auto ok = result.get();
        CHECK(!ok);
    }

    SECTION("call submit_work and get default empty result") {
        EmptyMiningService service;
        silkworm::Bytes block_nonce{}; // don't care
        evmc::bytes32 pow_hash{silkworm::kEmptyHash}; // don't care
        evmc::bytes32 digest{silkworm::kEmptyHash}; // don't care
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_submit_work(&service, block_nonce, pow_hash, digest), asio::use_future)};
        io_context.run();
        const auto ok = result.get();
        CHECK(!ok);
    }

    SECTION("call submit_work and get failure") {
        class TestFailureMiningService : public ::txpool::Mining::Service {
        public:
            ::grpc::Status HashRate(::grpc::ServerContext* context, const ::txpool::HashRateRequest* request, ::txpool::HashRateReply* response) override {
                return ::grpc::Status::CANCELLED;
            }
        };
        TestFailureMiningService service;
        silkworm::Bytes block_nonce{}; // don't care
        evmc::bytes32 pow_hash{silkworm::kEmptyHash}; // don't care
        evmc::bytes32 digest{silkworm::kEmptyHash}; // don't care
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_submit_work(&service, block_nonce, pow_hash, digest), asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(result.get(), std::system_error);
    }
}

TEST_CASE("Miner::submit_hash_rate", "[silkrpc][txpool][miner]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    class TestSuccessMiningService : public ::txpool::Mining::Service {
    public:
        explicit TestSuccessMiningService(bool ok) : ok_(ok) {}

        ::grpc::Status SubmitHashRate(::grpc::ServerContext* context, const ::txpool::SubmitHashRateRequest* request, ::txpool::SubmitHashRateReply* response) override {
            response->set_ok(ok_);
            return ::grpc::Status::OK;
        }

    private:
        bool ok_;
    };

    SECTION("call submit_hash_rate and get OK result") {
        TestSuccessMiningService service{/*ok=*/true};
        intx::uint256 rate{}; // don't care
        evmc::bytes32 id{silkworm::kEmptyHash}; // don't care
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_submit_hashrate(&service, rate, id), asio::use_future)};
        io_context.run();
        const auto ok = result.get();
        CHECK(ok);
    }

    SECTION("call submit_hash_rate and get KO result") {
        TestSuccessMiningService service{/*ok=*/false};
        intx::uint256 rate{}; // don't care
        evmc::bytes32 id{silkworm::kEmptyHash}; // don't care
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_submit_hashrate(&service, rate, id), asio::use_future)};
        io_context.run();
        const auto ok = result.get();
        CHECK(!ok);
    }

    SECTION("call submit_hash_rate and get default empty result") {
        EmptyMiningService service;
        intx::uint256 rate{}; // don't care
        evmc::bytes32 id{silkworm::kEmptyHash}; // don't care
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_submit_hashrate(&service, rate, id), asio::use_future)};
        io_context.run();
        const auto ok = result.get();
        CHECK(!ok);
    }

    SECTION("call submit_hash_rate and get failure") {
        class TestFailureMiningService : public ::txpool::Mining::Service {
        public:
            ::grpc::Status SubmitHashRate(::grpc::ServerContext* context, const ::txpool::SubmitHashRateRequest* request, ::txpool::SubmitHashRateReply* response) override {
                return ::grpc::Status::CANCELLED;
            }
        };
        TestFailureMiningService service;
        intx::uint256 rate{}; // don't care
        evmc::bytes32 id{silkworm::kEmptyHash}; // don't care
        asio::io_context io_context;
        auto result{asio::co_spawn(io_context, test_submit_hashrate(&service, rate, id), asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(result.get(), std::system_error);
    }
}

} // namespace silkrpc::txpool
