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

#include <thread>

#include <asio/io_context.hpp>
#include <asio/co_spawn.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch.hpp>
#include <gmock/gmock.h>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/completion_runner.hpp>
#include <silkrpc/interfaces/remote/ethbackend.grpc.pb.h>

namespace silkrpc {

using Catch::Matchers::Message;

TEST_CASE("create BackEnd", "[silkrpc][ethbackend][backend]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    class TestService : public ::remote::ETHBACKEND::Service {
    public:
        TestService() {}
        ::grpc::Status Etherbase(::grpc::ServerContext* context, const ::remote::EtherbaseRequest* request, ::remote::EtherbaseReply* response) override {
            return ::grpc::Status::OK;
        }
    };

    SECTION("start call Etherbase, get status OK and get result") {
        TestService service;
        std::ostringstream server_address;
        server_address << "localhost:" << 12345; // TODO(canepat): grpc_pick_unused_port_or_die
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address.str(), grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        const auto server_ptr = builder.BuildAndStart();
        asio::io_context io_context;
        asio::io_context::work work{io_context};
        grpc::CompletionQueue queue;
        CompletionRunner completion_runner{queue, io_context};
        auto io_context_thread = std::thread([&]() { io_context.run(); });
        auto completion_runner_thread = std::thread([&]() { completion_runner.run(); });
        std::this_thread::yield();
        const auto channel = grpc::CreateChannel(server_address.str(), grpc::InsecureChannelCredentials());
        ethbackend::BackEnd backend{io_context, channel, &queue};
        //auto future_address{asio::co_spawn(io_context, backend.etherbase(), asio::use_future)};
        //const auto address = future_address.get();
        //std::cout << "address=" << address << "\n";
        server_ptr->Shutdown();
        completion_runner.stop();
        io_context.stop();
        CHECK_NOTHROW(completion_runner_thread.join());
        CHECK_NOTHROW(io_context_thread.join());
    }
}

} // namespace silkrpc

