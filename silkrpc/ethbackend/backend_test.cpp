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
#include <thread>

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
};

template<typename R, asio::awaitable<R>(ethbackend::BackEnd::*Method)()>
asio::awaitable<R> test_backend(::remote::ETHBACKEND::Service* service) {
    std::ostringstream server_address;
    server_address << "localhost:" << 12345; // TODO(canepat): grpc_pick_unused_port_or_die
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address.str(), grpc::InsecureServerCredentials());
    builder.RegisterService(service);
    const auto server_ptr = builder.BuildAndStart();
    asio::io_context io_context;
    asio::io_context::work work{io_context};
    grpc::CompletionQueue queue;
    CompletionRunner completion_runner{queue, io_context};
    auto io_context_thread = std::thread([&]() { io_context.run(); });
    auto completion_runner_thread = std::thread([&]() { completion_runner.run(); });
    const auto channel = grpc::CreateChannel(server_address.str(), grpc::InsecureChannelCredentials());
    ethbackend::BackEnd backend{io_context, channel, &queue};
    const auto result = co_await (backend.*Method)();
    server_ptr->Shutdown();
    io_context.stop();
    completion_runner.stop();
    if (io_context_thread.joinable()) {
        io_context_thread.join();
    }
    if (completion_runner_thread.joinable()) {
        completion_runner_thread.join();
    }
    co_return result;
}

auto backend_etherbase = test_backend<evmc::address, &ethbackend::BackEnd::etherbase>;
auto backend_protocol_version = test_backend<uint64_t , &ethbackend::BackEnd::protocol_version>;
auto backend_net_version = test_backend<uint64_t, &ethbackend::BackEnd::net_version>;
auto backend_client_version = test_backend<std::string, &ethbackend::BackEnd::client_version>;

TEST_CASE("create BackEnd", "[silkrpc][ethbackend][backend]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("call etherbase and check address") {
        TestBackEndService service;
        asio::io_context io_context;
        auto etherbase{asio::co_spawn(io_context, backend_etherbase(&service), asio::use_future)};
        io_context.run();
        CHECK(etherbase.get() == 0x000000000000007f0000000000000000000000ff_address);
    }

    SECTION("call etherbase and check empty address") {
        EmptyBackEndService service;
        asio::io_context io_context;
        auto etherbase{asio::co_spawn(io_context, backend_etherbase(&service), asio::use_future)};
        io_context.run();
        CHECK(etherbase.get() == evmc::address{});
    }

    SECTION("call protocol_version and check version") {
        TestBackEndService service;
        asio::io_context io_context;
        auto version{asio::co_spawn(io_context, backend_protocol_version(&service), asio::use_future)};
        io_context.run();
        CHECK(version.get() == 15);
    }

    SECTION("call protocol_version and check empty version") {
        EmptyBackEndService service;
        asio::io_context io_context;
        auto version{asio::co_spawn(io_context, backend_protocol_version(&service), asio::use_future)};
        io_context.run();
        CHECK(version.get() == 0);
    }

    SECTION("call net_version and check version") {
        TestBackEndService service;
        asio::io_context io_context;
        auto version{asio::co_spawn(io_context, backend_net_version(&service), asio::use_future)};
        io_context.run();
        CHECK(version.get() == 66);
    }

    SECTION("call net_version and check empty version") {
        EmptyBackEndService service;
        asio::io_context io_context;
        auto version{asio::co_spawn(io_context, backend_net_version(&service), asio::use_future)};
        io_context.run();
        CHECK(version.get() == 0);
    }

    SECTION("call client_version and check version") {
        TestBackEndService service;
        asio::io_context io_context;
        auto version{asio::co_spawn(io_context, backend_client_version(&service), asio::use_future)};
        io_context.run();
        CHECK(version.get() == "erigon");
    }

    SECTION("call client_version and check empty version") {
        EmptyBackEndService service;
        asio::io_context io_context;
        auto version{asio::co_spawn(io_context, backend_client_version(&service), asio::use_future)};
        io_context.run();
        CHECK(version.get() == "");
    }
}

} // namespace silkrpc

