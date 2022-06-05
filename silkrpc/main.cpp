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
#include <string>
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
#include <asio/version.hpp>
#include <boost/process/environment.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/buildinfo.h>
#include <silkrpc/context_pool.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/http/server.hpp>
#include <silkrpc/ethdb/kv/remote_database.hpp>
#include <silkrpc/protocol/version.hpp>

ABSL_FLAG(std::string, chaindata, silkrpc::kEmptyChainData, "chain data path as string");
ABSL_FLAG(std::string, http_port, silkrpc::kDefaultHttpPort, "Ethereum JSON RPC API local end-point as string <address>:<port>");
ABSL_FLAG(std::string, engine_port, silkrpc::kDefaultEnginePort, "Engine JSON RPC API local end-point as string <address>:<port>");
ABSL_FLAG(std::string, target, silkrpc::kDefaultTarget, "Erigon Core gRPC service location as string <address>:<port>");
ABSL_FLAG(std::string, api_spec, silkrpc::kDefaultEth1ApiSpec, "JSON RPC API namespaces as comma-separated list of strings");
ABSL_FLAG(uint32_t, num_contexts, std::thread::hardware_concurrency() / 3, "number of running I/O contexts as 32-bit integer");
ABSL_FLAG(uint32_t, num_workers, 16, "number of worker threads as 32-bit integer");
ABSL_FLAG(uint32_t, timeout, silkrpc::kDefaultTimeout.count(), "gRPC call timeout as 32-bit integer");
ABSL_FLAG(silkrpc::LogLevel, log_verbosity, silkrpc::LogLevel::Critical, "logging verbosity level");
ABSL_FLAG(silkrpc::WaitMode, wait_mode, silkrpc::WaitMode::blocking, "scheduler wait mode");

//! Assemble the application name using the Cable build information
std::string get_name_from_build_info() {
    const auto build_info{silkrpc_get_buildinfo()};

    std::string application_name{"silkrpc/"};
    application_name.append(build_info->git_branch);
    application_name.append(build_info->project_version);
    application_name.append("/");
    application_name.append(build_info->system_name);
    application_name.append("-");
    application_name.append(build_info->system_processor);
    application_name.append("_");
    application_name.append(build_info->build_type);
    application_name.append("/");
    application_name.append(build_info->compiler_id);
    application_name.append("-");
    application_name.append(build_info->compiler_version);
    return application_name;
}

//! Assemble the relevant library version information
std::string get_library_versions() {
    std::string library_versions{"gRPC: "};
    library_versions.append(grpc::Version());
    library_versions.append(" Asio: ");
    library_versions.append(std::to_string(ASIO_VERSION));
    return library_versions;
}

const char* currentExceptionTypeName() {
    int status;
    return abi::__cxa_demangle(abi::__cxa_current_exception_type()->name(), 0, 0, &status);
}

int main(int argc, char* argv[]) {
    const auto pid = boost::this_process::get_id();
    const auto tid = std::this_thread::get_id();

    using silkrpc::kAddressPortSeparator;
    using silkrpc::kDefaultEth2ApiSpec;
    using silkrpc::ChannelFactory;
    using silkrpc::ContextPool;
    using silkrpc::http::Server;

    absl::FlagsUsageConfig config;
    config.contains_helpshort_flags = [](absl::string_view) { return false; };
    config.contains_help_flags = [](absl::string_view filename) { return absl::EndsWith(filename, "main.cpp"); };
    config.contains_helppackage_flags = [](absl::string_view) { return false; };
    config.normalize_filename = [](absl::string_view f) { return std::string{f.substr(f.rfind("/") + 1)}; };
    config.version_string = []() { return "silkrpcdaemon 0.0.8-rc\n"; };
    absl::SetFlagsUsageConfig(config);
    absl::SetProgramUsageMessage("C++ implementation of Ethereum JSON RPC API service within Thorax architecture");
    absl::ParseCommandLine(argc, argv);

    SILKRPC_LOG_VERBOSITY(absl::GetFlag(FLAGS_log_verbosity));
    SILKRPC_LOG_THREAD(true);

    std::set_terminate([]() {
        try {
            auto exc = std::current_exception();
            if (exc) {
                std::rethrow_exception(exc);
            }
        } catch (const std::exception& e) {
            SILKRPC_CRIT << "Silkrpc terminating due to exception: " << e.what() << "\n";
        } catch (...) {
            SILKRPC_CRIT << "Silkrpc terminating due to unexpected exception: " << currentExceptionTypeName() << "\n";
        }

        std::abort();
    });

    SILKRPC_LOG << "Silkrpc build info: " << get_name_from_build_info() << " " << get_library_versions() << "\n";

    try {
        auto chaindata{absl::GetFlag(FLAGS_chaindata)};
        if (!chaindata.empty() && !std::filesystem::exists(chaindata)) {
            SILKRPC_ERROR << "Parameter chaindata is invalid: [" << chaindata << "]\n";
            SILKRPC_ERROR << "Use --chaindata flag to specify the path of Erigon database\n";
            return -1;
        }

        auto http_port{absl::GetFlag(FLAGS_http_port)};
        if (!http_port.empty() && http_port.find(kAddressPortSeparator) == std::string::npos) {
            SILKRPC_ERROR << "Parameter http_port is invalid: [" << http_port << "]\n";
            SILKRPC_ERROR << "Use --http_port flag to specify the local binding for Ethereum JSON RPC service\n";
            return -1;
        }

        auto engine_port{absl::GetFlag(FLAGS_engine_port)};
        if (!engine_port.empty() && engine_port.find(kAddressPortSeparator) == std::string::npos) {
            SILKRPC_ERROR << "Parameter engine_port is invalid: [" << engine_port << "]\n";
            SILKRPC_ERROR << "Use --engine_port flag to specify the local binding for Engine JSON RPC service\n";
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

        auto num_contexts{absl::GetFlag(FLAGS_num_contexts)};
        if (num_contexts < 0) {
            SILKRPC_ERROR << "Parameter num_contexts is invalid: [" << num_contexts << "]\n";
            SILKRPC_ERROR << "Use --num_contexts flag to specify the number of threads running I/O contexts\n";
            return -1;
        }

        auto num_workers{absl::GetFlag(FLAGS_num_workers)};
        if (num_workers < 0) {
            SILKRPC_ERROR << "Parameter num_workers is invalid: [" << num_workers << "]\n";
            SILKRPC_ERROR << "Use --num_workers flag to specify the number of worker threads executing long-run operations\n";
            return -1;
        }

        const auto wait_mode{absl::GetFlag(FLAGS_wait_mode)};

        if (chaindata.empty()) {
            SILKRPC_LOG << "Silkrpc launched with target " << target << " using " << num_contexts << " contexts, " << num_workers << " workers\n";
        } else {
            SILKRPC_LOG << "Silkrpc launched with chaindata " << chaindata << " using " << num_contexts << " contexts, " << num_workers << " workers\n";
        }

        // TODO(canepat): handle also secure channel for remote
        ChannelFactory create_channel = [&]() {
            return grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
        };

        // Check protocol version compatibility with Core Services
        SILKRPC_LOG << "Checking protocol version compatibility with core services...\n";

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
        ContextPool context_pool{num_contexts, create_channel, wait_mode};
        asio::thread_pool worker_pool{num_workers};

        std::vector<std::unique_ptr<Server>> active_services;
        for (int i = 0; i < num_contexts; ++i) {
            auto& context = context_pool.next_context();
            active_services.emplace_back(std::make_unique<Server>(http_port, api_spec, context, worker_pool));
            active_services.emplace_back(std::make_unique<Server>(engine_port, kDefaultEth2ApiSpec, context, worker_pool));
        }

        asio::io_context signal_context;
        asio::signal_set signals{signal_context, SIGINT, SIGTERM};
        SILKRPC_DEBUG << "Signals registered on signal_context " << &signal_context << "\n" << std::flush;
        signals.async_wait([&](const asio::system_error& error, int signal_number) {
            if (signal_number == SIGINT) std::cout << "\n";
            SILKRPC_INFO << "Signal number: " << signal_number << " caught, error code: " << error.code() << "\n" << std::flush;
            context_pool.stop();
            for (auto& service : active_services) {
                service->stop();
            }
        });

        SILKRPC_LOG << "Silkrpc starting ETH RPC API at " << http_port << " ENGINE RPC API at " << engine_port << "\n";

        for (auto& service : active_services) {
            service->start();
        }

        SILKRPC_LOG << "Silkrpc is now running [pid=" << pid << ", main thread=" << tid << "]\n";

        context_pool.start();
        signal_context.run();

        context_pool.join();
    } catch (const std::exception& e) {
        SILKRPC_CRIT << "Exception: " << e.what() << "\n" << std::flush;
    } catch (...) {
        SILKRPC_CRIT << "Unexpected exception: " << currentExceptionTypeName() << "\n" << std::flush;
    }

    SILKRPC_LOG << "Silkrpc exiting [pid=" << pid << ", main thread=" << tid << "]\n" << std::flush;

    return 0;
}
