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
#include <thread>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/flags/usage_config.h>
#include <absl/strings/match.h>
#include <absl/strings/string_view.h>
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/signal_set.hpp>
#include <boost/process/environment.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/context_pool.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/http/server.hpp>
#include <silkrpc/ethdb/kv/remote_database.hpp>

ABSL_FLAG(std::string, chaindata, silkrpc::common::kEmptyChainData, "chain data path as string");
ABSL_FLAG(std::string, local, silkrpc::common::kDefaultLocal, "HTTP JSON local binding as string <address>:<port>");
ABSL_FLAG(std::string, target, silkrpc::common::kDefaultTarget, "TG Core gRPC service location as string <address>:<port>");
ABSL_FLAG(uint32_t, numContexts, std::thread::hardware_concurrency() / 2, "number of running I/O contexts as 32-bit integer");
ABSL_FLAG(uint32_t, timeout, silkrpc::common::kDefaultTimeout.count(), "gRPC call timeout as 32-bit integer");
ABSL_FLAG(silkrpc::LogLevel, logLevel, silkrpc::LogLevel::Critical, "logging level");

int main(int argc, char* argv[]) {
    const auto pid = boost::this_process::get_id();
    const auto tid = std::this_thread::get_id();

    using namespace silkrpc;
    using silkrpc::common::kAddressPortSeparator;

    absl::FlagsUsageConfig config;
    config.contains_helpshort_flags = [](absl::string_view) { return false; };
    config.contains_help_flags = [](absl::string_view filename) { return absl::EndsWith(filename, "main.cpp"); };
    config.contains_helppackage_flags = [](absl::string_view) { return false; };
    config.normalize_filename = [](absl::string_view f) { return std::string{f.substr(f.rfind("/") + 1)}; };
    config.version_string = []() { return "silkrpcdaemon 0.0.4\n"; };
    absl::SetFlagsUsageConfig(config);
    absl::SetProgramUsageMessage("C++ implementation of ETH JSON Remote Procedure Call (RPC) daemon");
    absl::ParseCommandLine(argc, argv);

    SILKRPC_LOG_VERBOSITY(absl::GetFlag(FLAGS_logLevel));
    SILKRPC_LOG_THREAD(true);

    try {
        auto chaindata{absl::GetFlag(FLAGS_chaindata)};
        if (!chaindata.empty() && !std::filesystem::exists(chaindata)) {
            SILKRPC_ERROR << "Parameter chaindata is invalid: [" << chaindata << "]\n";
            SILKRPC_ERROR << "Use --chaindata flag to specify the path of Turbo-Geth database\n";
            return -1;
        }

        auto local{absl::GetFlag(FLAGS_local)};
        if (!local.empty() && local.find(kAddressPortSeparator) == std::string::npos) {
            SILKRPC_ERROR << "Parameter local is invalid: [" << local << "]\n";
            SILKRPC_ERROR << "Use --local flag to specify the local binding for HTTP JSON server\n";
            return -1;
        }

        auto target{absl::GetFlag(FLAGS_target)};
        if (!target.empty() && target.find(":") == std::string::npos) {
            SILKRPC_ERROR << "Parameter target is invalid: [" << target << "]\n";
            SILKRPC_ERROR << "Use --target flag to specify the location of Turbo-Geth running instance\n";
            return -1;
        }

        if (chaindata.empty() && target.empty()) {
            SILKRPC_ERROR << "Parameters chaindata and target cannot be both empty, specify one of them\n";
            SILKRPC_ERROR << "Use --chaindata or --target flag to specify the path or the location of Turbo-Geth instance\n";
            return -1;
        }

        auto timeout{absl::GetFlag(FLAGS_timeout)};
        if (timeout < 0) {
            SILKRPC_ERROR << "Parameter timeout is invalid: [" << timeout << "]\n";
            SILKRPC_ERROR << "Use --timeout flag to specify the timeout in msecs for Turbo-Geth KV gRPC I/F\n";
            return -1;
        }

        auto numContexts{absl::GetFlag(FLAGS_numContexts)};
        if (numContexts < 0) {
            SILKRPC_ERROR << "Parameter numContexts is invalid: [" << numContexts << "]\n";
            SILKRPC_ERROR << "Use --numContexts flag to specify the number of threads running I/O contexts\n";
            return -1;
        }

        if (chaindata.empty()) {
            SILKRPC_LOG << "Silkrpc launched with target " + target + " using " << numContexts << " contexts\n";
        } else {
            SILKRPC_LOG << "Silkrpc launched with chaindata " + chaindata + " using " << numContexts << " contexts\n";
        }

        // TODO(canepat): handle also secure channel for remote
        silkrpc::ChannelFactory create_channel = [&]() {
            return ::grpc::CreateChannel(target, ::grpc::InsecureChannelCredentials());
        };
        // TODO(canepat): handle also local (shared-memory) database
        silkrpc::ContextPool context_pool{numContexts, create_channel};

        const auto http_host = local.substr(0, local.find(kAddressPortSeparator));
        const auto http_port = local.substr(local.find(kAddressPortSeparator) + 1, std::string::npos);
        silkrpc::http::Server http_server{http_host, http_port, context_pool};

        auto& io_context = context_pool.get_io_context();
        asio::signal_set signals{io_context, SIGINT, SIGTERM};
        SILKRPC_DEBUG << "Signals registered on io_context " << &io_context << "\n" << std::flush;
        signals.async_wait([&](const asio::system_error& error, int signal_number) {
            std::cout << "\n";
            SILKRPC_INFO << "Signal caught, error: " << error.what() << " number: " << signal_number << "\n" << std::flush;
            context_pool.stop();
            http_server.stop();
        });

        http_server.start();

        SILKRPC_LOG << "Silkrpc running at " + local + " [pid=" << pid << ", main thread: " << tid << "]\n";

        context_pool.run();
    } catch (const std::exception& e) {
        SILKRPC_CRIT << "Exception: " << e.what() << "\n" << std::flush;
    } catch (...) {
        SILKRPC_CRIT << "Unexpected exception\n" << std::flush;
    }

    SILKRPC_LOG << "Silkrpc exiting [pid=" << pid << ", main thread: " << tid << "]\n" << std::flush;

    return 0;
}
