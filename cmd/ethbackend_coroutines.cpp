/*
   Copyright 2020 The Silkrpc Authors

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

#include <silkrpc/config.hpp>

#include <exception>
#include <iomanip>
#include <iostream>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <asio/co_spawn.hpp>
#include <asio/signal_set.hpp>
#include <grpcpp/grpcpp.h>
#include <silkworm/common/util.hpp>

#include <cmd/ethbackend.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/context_pool.hpp>
#include <silkrpc/ethbackend/backend.hpp>

ABSL_FLAG(std::string, target, silkrpc::kDefaultTarget, "server location as string <address>:<port>");
ABSL_FLAG(silkrpc::LogLevel, logLevel, silkrpc::LogLevel::Critical, "logging level");

using silkrpc::LogLevel;

asio::awaitable<void> ethbackend_etherbase(silkrpc::ethbackend::BackEnd& backend) {
    try {
        std::cout << "ETHBACKEND Etherbase ->\n";
        const auto address = co_await backend.etherbase();
        std::cout << "ETHBACKEND Etherbase <- address: " << address << "\n";
    } catch (const std::exception& e) {
        std::cout << "ETHBACKEND Etherbase <- error: " << e.what() << "\n";
    }
}

int main(int argc, char* argv[]) {
    absl::SetProgramUsageMessage("Query Erigon/Silkworm ETHBACKEND remote interface");
    absl::ParseCommandLine(argc, argv);

    SILKRPC_LOG_VERBOSITY(absl::GetFlag(FLAGS_logLevel));

    try {
        auto target{absl::GetFlag(FLAGS_target)};
        if (target.empty() || target.find(":") == std::string::npos) {
            std::cerr << "Parameter target is invalid: [" << target << "]\n";
            std::cerr << "Use --target flag to specify the location of Erigon running instance\n";
            return -1;
        }

        // TODO(canepat): handle also secure channel for remote
        silkrpc::ChannelFactory create_channel = [&]() {
            return grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
        };
        // TODO(canepat): handle also local (shared-memory) database
        silkrpc::ContextPool context_pool{1, create_channel};
        auto& context = context_pool.get_context();
        auto& io_context = context.io_context;
        auto& grpc_queue = context.grpc_queue;

        asio::signal_set signals(*io_context, SIGINT, SIGTERM);
        signals.async_wait([&](const asio::system_error& error, int signal_number) {
            std::cout << "Signal caught, error: " << error.what() << " number: " << signal_number << std::endl << std::flush;
            context_pool.stop();
        });

        const auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());

        // Etherbase
        silkrpc::ethbackend::BackEnd eth_backend{*io_context, channel, grpc_queue.get()};
        asio::co_spawn(*io_context, ethbackend_etherbase(eth_backend), [&](std::exception_ptr exptr) {
            context_pool.stop();
        });

        context_pool.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n" << std::flush;
    } catch (...) {
        std::cerr << "Unexpected exception\n" << std::flush;
    }
    return 0;
}
