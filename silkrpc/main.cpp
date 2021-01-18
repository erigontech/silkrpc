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
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/process/environment.hpp>
#include <boost/stacktrace.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/constants.hpp>
#include <silkrpc/http/server.hpp>
#include <silkrpc/ethdb/kv/remote_database.hpp>

ABSL_FLAG(std::string, chaindata, silkrpc::common::kEmptyChainData, "chain data path as string");
ABSL_FLAG(std::string, target, silkrpc::common::kEmptyTarget, "server location as string <address>:<port>");
ABSL_FLAG(uint32_t, timeout, silkrpc::common::kDefaultTimeout.count(), "gRPC call timeout as 32-bit integer");

int main(int argc, char* argv[]) {
    const auto pid = boost::this_process::get_id();
    const auto tid = std::this_thread::get_id();

    try {
        absl::SetProgramUsageMessage("Seek Turbo-Geth/Silkworm Key-Value (KV) remote interface to database");
        absl::ParseCommandLine(argc, argv);

        auto chaindata{absl::GetFlag(FLAGS_chaindata)};
        if (!chaindata.empty() && !std::filesystem::exists(chaindata)) {
            std::cerr << "Parameter chaindata is invalid: [" << chaindata << "]\n";
            std::cerr << "Use --chaindata flag to specify the path of Turbo-Geth database\n";
            return -1;
        }

        auto target{absl::GetFlag(FLAGS_target)};
        if (!target.empty() && target.find(":") == std::string::npos) {
            std::cerr << "Parameter target is invalid: [" << target << "]\n";
            std::cerr << "Use --target flag to specify the location of Turbo-Geth running instance\n";
            return -1;
        }

        if (chaindata.empty() && target.empty()) {
            std::cerr << "Parameters chaindata and target cannot be both empty, specify one of them\n";
            std::cerr << "Use --chaindata or --target flag to specify the path or the location of Turbo-Geth instance\n";
            return -1;
        }

        auto timeout{absl::GetFlag(FLAGS_timeout)};
        if (timeout < 0) {
            std::cerr << "Parameter timeout is invalid: [" << timeout << "]\n";
            std::cerr << "Use --timeout flag to specify the timeout in msecs for Turbo-Geth KV gRPC I/F\n";
            return -1;
        }

        asio::io_context context{1};
        asio::signal_set signals{context, SIGINT, SIGTERM};

        // TODO: handle also secure channel for remote
        auto grpc_channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
        // TODO: handle also local (shared-memory) database
        std::unique_ptr<silkrpc::ethdb::kv::Database> database =
            std::make_unique<silkrpc::ethdb::kv::RemoteDatabase>(context, grpc_channel);

        silkrpc::http::Server http_server{context, "localhost", "51515", database};

        signals.async_wait([&](const asio::system_error& error, int signal_number) {
            std::cout << "\nSignal caught, error: " << error.what() << " number: " << signal_number << "\n" << std::flush;
            context.stop();
            http_server.stop();
        });

        asio::co_spawn(context, http_server.start(), [&](std::exception_ptr eptr) {
            if (eptr) std::rethrow_exception(eptr);
        });

        std::cout << "Silkrpc running [pid=" << pid << ", main thread: " << tid << "]\n" << std::flush;

        context.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        typedef boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace> traced;
        const boost::stacktrace::stacktrace* st = boost::get_error_info<traced>(e);
        if (st) {
            std::cerr << *st << '\n' << std::flush;
        }
    }

    std::cout << "Silkrpc exiting [pid=" << pid << ", main thread: " << tid << "]\n" << std::flush;

    return 0;
}
