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

#include "backend.hpp"

#include <string>
#include <system_error>
#include <thread>
#include <utility>

#include <asio/io_context.hpp>
#include <asio/co_spawn.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch.hpp>
#include <evmc/evmc.hpp>
#include <gmock/gmock.h>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/completion_runner.hpp>
#include <silkrpc/interfaces/remote/ethbackend.grpc.pb.h>

namespace silkrpc {

using Catch::Matchers::Message;
using evmc::literals::operator""_address;

class EmptyBackEndService : public ::remote::ETHBACKEND::Service {
public:
    ::grpc::Status Etherbase(::grpc::ServerContext* context, const ::remote::EtherbaseRequest* request, ::remote::EtherbaseReply* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status ProtocolVersion(::grpc::ServerContext* context, const ::remote::ProtocolVersionRequest* request, ::remote::ProtocolVersionReply* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status NetVersion(::grpc::ServerContext* context, const ::remote::NetVersionRequest* request, ::remote::NetVersionReply* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status ClientVersion(::grpc::ServerContext *context, const ::remote::ClientVersionRequest *request, ::remote::ClientVersionReply *response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status NetPeerCount(::grpc::ServerContext* context, const ::remote::NetPeerCountRequest* request, ::remote::NetPeerCountReply* response) override {
        return ::grpc::Status::OK;
    }
};

class FailureBackEndService : public ::remote::ETHBACKEND::Service {
public:
    ::grpc::Status Etherbase(::grpc::ServerContext* context, const ::remote::EtherbaseRequest* request, ::remote::EtherbaseReply* response) override {
        return ::grpc::Status::CANCELLED;
    }
    ::grpc::Status ProtocolVersion(::grpc::ServerContext* context, const ::remote::ProtocolVersionRequest* request, ::remote::ProtocolVersionReply* response) override {
        return ::grpc::Status::CANCELLED;
    }
    ::grpc::Status NetVersion(::grpc::ServerContext* context, const ::remote::NetVersionRequest* request, ::remote::NetVersionReply* response) override {
        return ::grpc::Status::CANCELLED;
    }
    ::grpc::Status ClientVersion(::grpc::ServerContext *context, const ::remote::ClientVersionRequest *request, ::remote::ClientVersionReply *response) override {
        return ::grpc::Status::CANCELLED;
    }
    ::grpc::Status NetPeerCount(::grpc::ServerContext* context, const ::remote::NetPeerCountRequest* request, ::remote::NetPeerCountReply* response) override {
        return ::grpc::Status::CANCELLED;
    }
};

class TestBackEndService : public ::remote::ETHBACKEND::Service {
public:
    ::grpc::Status Etherbase(::grpc::ServerContext* context, const ::remote::EtherbaseRequest* request, ::remote::EtherbaseReply* response) override {
        auto h128_ptr = new ::types::H128();
        h128_ptr->set_hi(0x7F);
        auto h160_ptr = new ::types::H160();
        h160_ptr->set_lo(0xFF);
        h160_ptr->set_allocated_hi(h128_ptr);
        response->set_allocated_address(h160_ptr);
        return ::grpc::Status::OK;
    }
    ::grpc::Status ProtocolVersion(::grpc::ServerContext* context, const ::remote::ProtocolVersionRequest* request, ::remote::ProtocolVersionReply* response) override {
        response->set_id(15);
        return ::grpc::Status::OK;
    }
    ::grpc::Status NetVersion(::grpc::ServerContext* context, const ::remote::NetVersionRequest* request, ::remote::NetVersionReply* response) override {
        response->set_id(66);
        return ::grpc::Status::OK;
    }
    ::grpc::Status ClientVersion(::grpc::ServerContext *context, const ::remote::ClientVersionRequest *request, ::remote::ClientVersionReply *response) override {
        response->set_nodename("erigon");
        return ::grpc::Status::OK;
    }
    ::grpc::Status NetPeerCount(::grpc::ServerContext* context, const ::remote::NetPeerCountRequest* request, ::remote::NetPeerCountReply* response) override {
        response->set_count(20);
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
asio::awaitable<R> test_comethod(::remote::ETHBACKEND::Service* service, Args... args) {
    ClientServerTestBox test_box{service};
    auto method_proxy{test_box.make_method_proxy<method, T>()};
    co_return co_await method_proxy(args...);
}

auto test_etherbase = test_comethod<ethbackend::BackEnd, &ethbackend::BackEnd::etherbase, evmc::address>;
auto test_protocol_version = test_comethod<ethbackend::BackEnd, &ethbackend::BackEnd::protocol_version, uint64_t>;
auto test_net_version = test_comethod<ethbackend::BackEnd, &ethbackend::BackEnd::net_version, uint64_t>;
auto test_client_version = test_comethod<ethbackend::BackEnd, &ethbackend::BackEnd::client_version, std::string>;
auto test_net_peer_count = test_comethod<ethbackend::BackEnd, &ethbackend::BackEnd::net_peer_count, uint64_t>;

TEST_CASE("BackEnd::etherbase", "[silkrpc][ethbackend][backend]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("call etherbase and get address") {
        TestBackEndService service;
        asio::io_context io_context;
        auto etherbase{asio::co_spawn(io_context, test_etherbase(&service), asio::use_future)};
        io_context.run();
        CHECK(etherbase.get() == 0x000000000000007f0000000000000000000000ff_address);
    }

    SECTION("call etherbase and get empty address") {
        EmptyBackEndService service;
        asio::io_context io_context;
        auto etherbase{asio::co_spawn(io_context, test_etherbase(&service), asio::use_future)};
        io_context.run();
        CHECK(etherbase.get() == evmc::address{});
    }

    SECTION("call etherbase and get error") {
        FailureBackEndService service;
        asio::io_context io_context;
        auto etherbase{asio::co_spawn(io_context, test_etherbase(&service), asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(etherbase.get(), std::system_error);
    }
}

TEST_CASE("BackEnd::protocol_version", "[silkrpc][ethbackend][backend]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("call protocol_version and get version") {
        TestBackEndService service;
        asio::io_context io_context;
        auto version{asio::co_spawn(io_context, test_protocol_version(&service), asio::use_future)};
        io_context.run();
        CHECK(version.get() == 15);
    }

    SECTION("call protocol_version and get empty version") {
        EmptyBackEndService service;
        asio::io_context io_context;
        auto version{asio::co_spawn(io_context, test_protocol_version(&service), asio::use_future)};
        io_context.run();
        CHECK(version.get() == 0);
    }

    SECTION("call protocol_version and get error") {
        FailureBackEndService service;
        asio::io_context io_context;
        auto version{asio::co_spawn(io_context, test_protocol_version(&service), asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(version.get(), std::system_error);
    }
}

TEST_CASE("BackEnd::net_version", "[silkrpc][ethbackend][backend]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("call net_version and get version") {
        TestBackEndService service;
        asio::io_context io_context;
        auto version{asio::co_spawn(io_context, test_net_version(&service), asio::use_future)};
        io_context.run();
        CHECK(version.get() == 66);
    }

    SECTION("call net_version and get empty version") {
        EmptyBackEndService service;
        asio::io_context io_context;
        auto version{asio::co_spawn(io_context, test_net_version(&service), asio::use_future)};
        io_context.run();
        CHECK(version.get() == 0);
    }

    SECTION("call net_version and get error") {
        FailureBackEndService service;
        asio::io_context io_context;
        auto version{asio::co_spawn(io_context, test_net_version(&service), asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(version.get(), std::system_error);
    }
}

TEST_CASE("BackEnd::client_version", "[silkrpc][ethbackend][backend]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("call client_version and get version") {
        TestBackEndService service;
        asio::io_context io_context;
        auto version{asio::co_spawn(io_context, test_client_version(&service), asio::use_future)};
        io_context.run();
        CHECK(version.get() == "erigon");
    }

    SECTION("call client_version and get empty version") {
        EmptyBackEndService service;
        asio::io_context io_context;
        auto version{asio::co_spawn(io_context, test_client_version(&service), asio::use_future)};
        io_context.run();
        CHECK(version.get() == "");
    }

    SECTION("call client_version and get error") {
        FailureBackEndService service;
        asio::io_context io_context;
        auto version{asio::co_spawn(io_context, test_client_version(&service), asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(version.get(), std::system_error);
    }
}

TEST_CASE("BackEnd::net_peer_count", "[silkrpc][ethbackend][backend]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("call net_peer_count and get peer count") {
        TestBackEndService service;
        asio::io_context io_context;
        auto peer_count{asio::co_spawn(io_context, test_net_peer_count(&service), asio::use_future)};
        io_context.run();
        CHECK(peer_count.get() == 20);
    }

    SECTION("call net_peer_count and get empty peer count") {
        EmptyBackEndService service;
        asio::io_context io_context;
        auto peer_count{asio::co_spawn(io_context, test_net_peer_count(&service), asio::use_future)};
        io_context.run();
        CHECK(peer_count.get() == 0);
    }

    SECTION("call net_peer_count and get error") {
        FailureBackEndService service;
        asio::io_context io_context;
        auto peer_count{asio::co_spawn(io_context, test_net_peer_count(&service), asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(peer_count.get(), std::system_error);
    }
}

} // namespace silkrpc

