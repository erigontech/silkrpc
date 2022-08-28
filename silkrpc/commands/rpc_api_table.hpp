/*
   Copyright 2020-2021 The Silkrpc Authors

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

#ifndef SILKRPC_COMMANDS_RPC_API_TABLE_HPP_
#define SILKRPC_COMMANDS_RPC_API_TABLE_HPP_

#include <map>
#include <memory>
#include <string>

#include <silkrpc/config.hpp>

#include <boost/asio/awaitable.hpp>
#include <nlohmann/json.hpp>

#include <silkrpc/commands/rpc_api.hpp>

namespace silkrpc::commands {

class RpcApiTable {
public:
    typedef boost::asio::awaitable<void> (RpcApi::*HandleMethod)(const nlohmann::json&, nlohmann::json&);

    explicit RpcApiTable(const std::string& api_spec);

    RpcApiTable(const RpcApiTable&) = delete;
    RpcApiTable& operator=(const RpcApiTable&) = delete;

    std::optional<HandleMethod> find_handler(const std::string& method) const;

private:
    void build_handlers(const std::string& api_spec);
    void add_handlers(const std::string& api_namespace);
    void add_debug_handlers();
    void add_eth_handlers();
    void add_net_handlers();
    void add_parity_handlers();
    void add_erigon_handlers();
    void add_trace_handlers();
    void add_web3_handlers();
    void add_engine_handlers();
    void add_txpool_handlers();

    std::map<std::string, HandleMethod> handlers_;
};

} // namespace silkrpc::commands

#endif  // SILKRPC_COMMANDS_RPC_API_TABLE_HPP_
