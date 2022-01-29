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

#include <cxxabi.h>
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
#include <asio/thread_pool.hpp>
#include <boost/process/environment.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/context_pool.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/http/server.hpp>
#include <silkrpc/ethdb/kv/remote_database.hpp>
#include <silkrpc/protocol/version.hpp>

ABSL_FLAG(std::string, chaindata, silkrpc::kEmptyChainData, "chain data path as string");
ABSL_FLAG(std::string, eth1_local, silkrpc::kDefaultEth1Local, "Ethereum JSON RPC API local end-point as string <address>:<port>");
ABSL_FLAG(std::string, eth2_local, silkrpc::kDefaultEth2Local, "Engine JSON RPC API local end-point as string <address>:<port>");
ABSL_FLAG(std::string, target, silkrpc::kDefaultTarget, "Erigon Core gRPC service location as string <address>:<port>");
ABSL_FLAG(std::string, api_spec, silkrpc::kDefaultEth1ApiSpec, "JSON RPC API namespaces as comma-separated list of strings");
ABSL_FLAG(uint32_t, numContexts, std::thread::hardware_concurrency() / 2, "number of running I/O contexts as 32-bit integer");
ABSL_FLAG(uint32_t, numWorkers, 16, "number of worker threads as 32-bit integer");
ABSL_FLAG(uint32_t, timeout, silkrpc::kDefaultTimeout.count(), "gRPC call timeout as 32-bit integer");
ABSL_FLAG(silkrpc::LogLevel, logLevel, silkrpc::LogLevel::Critical, "logging level");

const char* currentExceptionTypeName() {
    int status;
    return abi::__cxa_demangle(abi::__cxa_current_exception_type()->name(), 0, 0, &status);
}

int main(int argc, char* argv[]) {
    const auto pid = boost::this_process::get_id();
    const auto tid = std::this_thread::get_id();

    using silkrpc::kAddressPortSeparator;
    using silkrpc::kDefaultEth2ApiSpec;

    absl::FlagsUsageConfig config;
    config.contains_helpshort_flags = [](absl::string_view) { return false; };
    config.contains_help_flags = [](absl::string_view filename) { return absl::EndsWith(filename, "main.cpp"); };
    config.contains_helppackage_flags = [](absl::string_view) { return false; };
    config.normalize_filename = [](absl::string_view f) { return std::string{f.substr(f.rfind("/") + 1)}; };
    config.version_string = []() { return "silkrpcdaemon 0.0.8-rc\n"; };
    absl::SetFlagsUsageConfig(config);
    absl::SetProgramUsageMessage("C++ implementation of Ethereum JSON RPC API service within Thorax architecture");
    absl::ParseCommandLine(argc, argv);

    SILKRPC_LOG_VERBOSITY(absl::GetFlag(FLAGS_logLevel));
    SILKRPC_LOG_THREAD(true);

    std::set_terminate([](){
        SILKRPC_CRIT << "silkrpc terminating with exception\n";
        try {
           auto exc = std::current_exception();
           if (exc)
               std::rethrow_exception(exc);
        } catch(const std::exception& e) {
           SILKRPC_CRIT << "Caught exception: " << e.what() << "\n";
        } catch(...) {
           SILKRPC_CRIT << "Type of caught exception is " << currentExceptionTypeName() << "\n";
        }

        std::abort();
    });

    try {
        auto chaindata{absl::GetFlag(FLAGS_chaindata)};
        if (!chaindata.empty() && !std::filesystem::exists(chaindata)) {
            SILKRPC_ERROR << "Parameter chaindata is invalid: [" << chaindata << "]\n";
            SILKRPC_ERROR << "Use --chaindata flag to specify the path of Erigon database\n";
            return -1;
        }

        auto eth1_local{absl::GetFlag(FLAGS_eth1_local)};
        if (!eth1_local.empty() && eth1_local.find(kAddressPortSeparator) == std::string::npos) {
            SILKRPC_ERROR << "Parameter eth1_local is invalid: [" << eth1_local << "]\n";
            SILKRPC_ERROR << "Use --eth1_local flag to specify the local binding for Ethereum JSON RPC service\n";
            return -1;
        }

        auto eth2_local{absl::GetFlag(FLAGS_eth2_local)};
        if (!eth2_local.empty() && eth2_local.find(kAddressPortSeparator) == std::string::npos) {
            SILKRPC_ERROR << "Parameter eth2_local is invalid: [" << eth2_local << "]\n";
            SILKRPC_ERROR << "Use --eth2_local flag to specify the local binding for Engine JSON RPC service\n";
            return -1;
        }

        auto target{absl::GetFlag(FLAGS_target)};
        if (!target.empty() && target.find(":") == std::string::npos) {
            SILKRPC_ERROR << "Parameter target is invalid: [" << target << "]\n";
            SILKRPC_ERROR << "Use --target flag to specify the location of Erigon running instance\n";
            return -1;
        }

        auto api_spec{absl::GetFlag(FLAGS_api_spec)};
        if (api_spec.empty()) {
            SILKRPC_ERROR << "Parameter api_spec is invalid: [" << api_spec << "]\n";
            SILKRPC_ERROR << "Use --api_spec flag to specify JSON RPC API namespaces as comma-separated list of strings\n";
            return -1;
        }

        if (chaindata.empty() && target.empty()) {
            SILKRPC_ERROR << "Parameters chaindata and target cannot be both empty, specify one of them\n";
            SILKRPC_ERROR << "Use --chaindata or --target flag to specify the path or the location of Erigon instance\n";
            return -1;
        }

        auto timeout{absl::GetFlag(FLAGS_timeout)};
        if (timeout < 0) {
            SILKRPC_ERROR << "Parameter timeout is invalid: [" << timeout << "]\n";
            SILKRPC_ERROR << "Use --timeout flag to specify the timeout in msecs for Erigon KV gRPC I/F\n";
            return -1;
        }

        auto numContexts{absl::GetFlag(FLAGS_numContexts)};
        if (numContexts < 0) {
            SILKRPC_ERROR << "Parameter numContexts is invalid: [" << numContexts << "]\n";
            SILKRPC_ERROR << "Use --numContexts flag to specify the number of threads running I/O contexts\n";
            return -1;
        }

        auto numWorkers{absl::GetFlag(FLAGS_numWorkers)};
        if (numWorkers < 0) {
            SILKRPC_ERROR << "Parameter numWorkers is invalid: [" << numWorkers << "]\n";
            SILKRPC_ERROR << "Use --numWorkers flag to specify the number of worker threads executing long-run operations\n";
            return -1;
        }

        if (chaindata.empty()) {
            SILKRPC_LOG << "Silkrpc launched with target " << target << " using " << numContexts << " contexts, " << numWorkers << " workers\n";
        } else {
            SILKRPC_LOG << "Silkrpc launched with chaindata " << chaindata << " using " << numContexts << " contexts, " << numWorkers << " workers\n";
        }

        // TODO(canepat): handle also secure channel for remote
        silkrpc::ChannelFactory create_channel = [&]() {
            return grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
        };

        // Check protocol version compatibility with Core Services
        const auto core_service_channel{create_channel()};
        const auto kv_protocol_check{silkrpc::wait_for_kv_protocol_check(core_service_channel)};
        if (!kv_protocol_check.compatible) {
            throw std::runtime_error{kv_protocol_check.result};
        }
        SILKRPC_LOG << kv_protocol_check.result << "\n";
        const auto ethbackend_protocol_check{silkrpc::wait_for_ethbackend_protocol_check(core_service_channel)};
        if (!ethbackend_protocol_check.compatible) {
            throw std::runtime_error{ethbackend_protocol_check.result};
        }
        SILKRPC_LOG << ethbackend_protocol_check.result << "\n";
        const auto mining_protocol_check{silkrpc::wait_for_mining_protocol_check(core_service_channel)};
        if (!mining_protocol_check.compatible) {
            throw std::runtime_error{mining_protocol_check.result};
        }
        SILKRPC_LOG << mining_protocol_check.result << "\n";
        const auto txpool_protocol_check{silkrpc::wait_for_txpool_protocol_check(core_service_channel)};
        if (!txpool_protocol_check.compatible) {
            throw std::runtime_error{txpool_protocol_check.result};
        }
        SILKRPC_LOG << txpool_protocol_check.result << "\n";

        // TODO(canepat): handle also local (shared-memory) database
        silkrpc::ContextPool context_pool{numContexts, create_channel};
        asio::thread_pool worker_pool{numWorkers};

        silkrpc::http::Server eth_rpc_service{eth1_local, api_spec, context_pool, worker_pool};
        silkrpc::http::Server engine_rpc_service{eth2_local, kDefaultEth2ApiSpec, context_pool, worker_pool};

        auto& io_context = context_pool.get_io_context();
        asio::signal_set signals{io_context, SIGINT, SIGTERM};
        SILKRPC_DEBUG << "Signals registered on io_context " << &io_context << "\n" << std::flush;
        signals.async_wait([&](const asio::system_error& error, int signal_number) {
            std::cout << "\n";
            SILKRPC_INFO << "Signal caught, error: " << error.what() << " number: " << signal_number << "\n" << std::flush;
            context_pool.stop();
            eth_rpc_service.stop();
            engine_rpc_service.stop();
        });

        SILKRPC_LOG << "Silkrpc starting Ethereum RPC API service at " << eth1_local << "\n";
        eth_rpc_service.start();

        SILKRPC_LOG << "Silkrpc running Engine RPC API service at " << eth2_local << "\n";
        engine_rpc_service.start();

        SILKRPC_LOG << "Silkrpc is now running [pid=" << pid << ", main thread=" << tid << "]\n";

        context_pool.run();
    } catch (const std::exception& e) {
        SILKRPC_CRIT << "Exception: " << e.what() << "\n" << std::flush;
    } catch (...) {
        SILKRPC_CRIT << "Unexpected exception\n" << std::flush;
        SILKRPC_CRIT << "Type of caught exception is " << currentExceptionTypeName() << "\n";
    }

    SILKRPC_LOG << "Silkrpc exiting [pid=" << pid << ", main thread=" << tid << "]\n" << std::flush;

    return 0;
}
