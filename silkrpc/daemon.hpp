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

#ifndef SILKRPC_DAEMON_HPP_
#define SILKRPC_DAEMON_HPP_

#include <memory>
#include <string>
#include <vector>

#include <boost/asio/thread_pool.hpp>

#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/concurrency/context_pool.hpp>
#include <silkrpc/ethdb/kv/state_changes_stream.hpp>
#include <silkrpc/http/server.hpp>
#include <silkrpc/protocol/version.hpp>

namespace silkrpc {

struct DaemonSettings {
    std::string chaindata;
    std::string http_port; // eth_end_point
    std::string engine_port; // engine_end_point
    std::string api_spec; // eth_api_spec
    std::string target; // backend_kv_address
    //std::string txpool_address;
    uint32_t num_contexts;
    uint32_t num_workers;
    LogLevel log_verbosity;
    WaitMode wait_mode;
};

struct DaemonInfo {
    std::string build;
    std::string libraries;
};

struct DaemonChecklist {
    std::vector<ProtocolVersionResult> protocol_checklist;

    void success_or_throw() const;
};

class Daemon {
  public:
    static int run(const DaemonSettings& settings, const DaemonInfo& info = {});

    explicit Daemon(const DaemonSettings& settings);

    Daemon(const Daemon&) = delete;
    Daemon& operator=(const Daemon&) = delete;

    DaemonChecklist run_checklist();

    void start();
    void stop();

    void join();

  protected:
    static bool validate_settings(const DaemonSettings& settings);
    static ChannelFactory make_channel_factory(const DaemonSettings& settings);

    //! The RPC daemon configuration settings.
    const DaemonSettings& settings_;

    //! The factory of gRPC client-side channels.
    ChannelFactory create_channel_;

    //! The execution contexts capturing the asynchronous scheduling model.
    ContextPool context_pool_;

    //! The pool of workers for long-running tasks.
    boost::asio::thread_pool worker_pool_;

    std::vector<std::unique_ptr<http::Server>> rpc_services_;

    //! The gRPC KV interface client stub.
    std::unique_ptr<remote::KV::StubInterface> kv_stub_;

    //! The stream handling StateChanges server-streaming RPC.
    std::unique_ptr<ethdb::kv::StateChangesStream> state_changes_stream_;
};

} // namespace silkrpc

#endif // SILKRPC_DAEMON_HPP_
