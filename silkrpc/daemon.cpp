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

#include "daemon.hpp"

#include <cxxabi.h>
#include <filesystem>
#include <stdexcept>

#include <asio/signal_set.hpp>
#include <boost/process/environment.hpp>
#include <grpcpp/grpcpp.h>

namespace silkrpc {

void DaemonChecklist::success_or_throw() const {
    for (const auto protocol_check : protocol_checklist) {
        if (!protocol_check.compatible) {
            throw std::runtime_error{protocol_check.result};
        }
    }
}

const char* current_exception_name() {
    int status;
    return abi::__cxa_demangle(abi::__cxa_current_exception_type()->name(), 0, 0, &status);
}

int Daemon::run(const DaemonSettings& settings, const DaemonInfo& info) {
    const bool are_settings_valid{validate_settings(settings)};
    if (!are_settings_valid) {
        return -1;
    }

    SILKRPC_LOG_VERBOSITY(settings.log_verbosity);
    SILKRPC_LOG_THREAD(true);

    SILKRPC_LOG << "Silkrpc build info: " << info.build << " " << info.libraries << "\n";

    std::set_terminate([]() {
        try {
            auto exc = std::current_exception();
            if (exc) {
                std::rethrow_exception(exc);
            }
        } catch (const std::exception& e) {
            SILKRPC_CRIT << "Silkrpc terminating due to exception: " << e.what() << "\n";
        } catch (...) {
            SILKRPC_CRIT << "Silkrpc terminating due to unexpected exception: " << current_exception_name() << "\n";
        }
        std::abort();
    });

    const auto pid = boost::this_process::get_id();
    const auto tid = std::this_thread::get_id();

    try {
        if (settings.chaindata.empty()) {
            SILKRPC_LOG << "Silkrpc launched with target " << settings.target << " using " << settings.num_contexts
                        << " contexts, " << settings.num_workers << " workers\n";
        } else {
            SILKRPC_LOG << "Silkrpc launched with chaindata " << settings.chaindata << " using " << settings.num_contexts
                        << " contexts, " << settings.num_workers << " workers\n";
        }

        // Create the one-and-only Silkrpc daemon
        silkrpc::Daemon rpc_daemon{settings};

        // Check protocol version compatibility with Core Services
        SILKRPC_LOG << "Checking protocol version compatibility with core services...\n";

        const auto checklist = rpc_daemon.run_checklist();

        for (const auto protocol_check : checklist.protocol_checklist) {
            SILKRPC_LOG << protocol_check.result << "\n";
        }

        checklist.success_or_throw();

        // Start execution context dedicated to handling termination signals
        asio::io_context signal_context;
        asio::signal_set signals{signal_context, SIGINT, SIGTERM};
        SILKRPC_DEBUG << "Signals registered on signal_context " << &signal_context << "\n" << std::flush;
        signals.async_wait([&](const asio::system_error& error, int signal_number) {
            if (signal_number == SIGINT) std::cout << "\n";
            SILKRPC_INFO << "Signal number: " << signal_number << " caught, error code: " << error.code() << "\n" << std::flush;
            rpc_daemon.stop();
        });

        SILKRPC_LOG << "Silkrpc starting ETH RPC API at " << settings.http_port << " ENGINE RPC API at " << settings.engine_port << "\n";

        rpc_daemon.start();

        SILKRPC_LOG << "Silkrpc is now running [pid=" << pid << ", main thread=" << tid << "]\n";

        signal_context.run();

        rpc_daemon.join();
    } catch (const std::exception& e) {
        SILKRPC_CRIT << "Exception: " << e.what() << "\n" << std::flush;
    } catch (...) {
        SILKRPC_CRIT << "Unexpected exception: " << current_exception_name() << "\n" << std::flush;
    }

    SILKRPC_LOG << "Silkrpc exiting [pid=" << pid << ", main thread=" << tid << "]\n" << std::flush;

    return 0;
}

bool Daemon::validate_settings(const DaemonSettings& settings) {
    const auto chaindata = settings.chaindata;
    if (!chaindata.empty() && !std::filesystem::exists(chaindata)) {
        SILKRPC_ERROR << "Parameter chaindata is invalid: [" << chaindata << "]\n";
        SILKRPC_ERROR << "Use --chaindata flag to specify the path of Erigon database\n";
        return false;
    }

    const auto http_port = settings.http_port;
    if (!http_port.empty() && http_port.find(silkrpc::kAddressPortSeparator) == std::string::npos) {
        SILKRPC_ERROR << "Parameter http_port is invalid: [" << http_port << "]\n";
        SILKRPC_ERROR << "Use --http_port flag to specify the local binding for Ethereum JSON RPC service\n";
        return false;
    }

    const auto engine_port = settings.engine_port;
    if (!engine_port.empty() && engine_port.find(silkrpc::kAddressPortSeparator) == std::string::npos) {
        SILKRPC_ERROR << "Parameter engine_port is invalid: [" << engine_port << "]\n";
        SILKRPC_ERROR << "Use --engine_port flag to specify the local binding for Engine JSON RPC service\n";
        return false;
    }

    const auto target = settings.target;
    if (!target.empty() && target.find(":") == std::string::npos) {
        SILKRPC_ERROR << "Parameter target is invalid: [" << target << "]\n";
        SILKRPC_ERROR << "Use --target flag to specify the location of Erigon running instance\n";
        return false;
    }

    if (chaindata.empty() && target.empty()) {
        SILKRPC_ERROR << "Parameters chaindata and target cannot be both empty, specify one of them\n";
        SILKRPC_ERROR << "Use --chaindata or --target flag to specify the path or the location of Erigon instance\n";
        return false;
    }

    const auto api_spec = settings.api_spec;
    if (api_spec.empty()) {
        SILKRPC_ERROR << "Parameter api_spec is invalid: [" << api_spec << "]\n";
        SILKRPC_ERROR << "Use --api_spec flag to specify JSON RPC API namespaces as comma-separated list of strings\n";
        return false;
    }

    const auto num_contexts = settings.num_contexts;
    if (num_contexts < 0) {
        SILKRPC_ERROR << "Parameter num_contexts is invalid: [" << num_contexts << "]\n";
        SILKRPC_ERROR << "Use --num_contexts flag to specify the number of threads running I/O contexts\n";
        return false;
    }

    const auto num_workers = settings.num_workers;
    if (num_workers < 0) {
        SILKRPC_ERROR << "Parameter num_workers is invalid: [" << num_workers << "]\n";
        SILKRPC_ERROR << "Use --num_workers flag to specify the number of worker threads executing long-run operations\n";
        return false;
    }

    return true;
}

ChannelFactory Daemon::make_channel_factory(const DaemonSettings& settings) {
    return [&settings]() {
        return grpc::CreateChannel(settings.target, grpc::InsecureChannelCredentials());
    };
}

Daemon::Daemon(const DaemonSettings& settings)
    : settings_(settings),
      create_channel_{make_channel_factory(settings_)},
      context_pool_{settings_.num_contexts, create_channel_, settings_.wait_mode},
      worker_pool_{settings_.num_workers} {
}

DaemonChecklist Daemon::run_checklist() {
    const auto core_service_channel{create_channel_()};

    const auto kv_protocol_check{silkrpc::wait_for_kv_protocol_check(core_service_channel)};
    const auto ethbackend_protocol_check{silkrpc::wait_for_ethbackend_protocol_check(core_service_channel)};
    const auto mining_protocol_check{silkrpc::wait_for_mining_protocol_check(core_service_channel)};
    const auto txpool_protocol_check{silkrpc::wait_for_txpool_protocol_check(core_service_channel)};

    DaemonChecklist checklist{{
        kv_protocol_check,
        ethbackend_protocol_check,
        mining_protocol_check,
        txpool_protocol_check
    }};
    return checklist;
}

void Daemon::start() {
    for (int i = 0; i < settings_.num_contexts; ++i) {
        auto& context = context_pool_.next_context();
        rpc_services_.emplace_back(
            std::make_unique<http::Server>(settings_.http_port, settings_.api_spec, context, worker_pool_));
        rpc_services_.emplace_back(
            std::make_unique<http::Server>(settings_.engine_port, kDefaultEth2ApiSpec, context, worker_pool_));
    }

    for (auto& service : rpc_services_) {
        service->start();
    }

    context_pool_.start();
}

void Daemon::stop() {
    context_pool_.stop();

    for (auto& service : rpc_services_) {
        service->stop();
    }
}

void Daemon::join() {
    context_pool_.join();
}

} // namespace silkrpc
