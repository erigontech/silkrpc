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

#include "net_api.hpp"

#include <silkrpc/json/types.hpp>
#include <string>

namespace silkrpc::commands {

// https://eth.wiki/json-rpc/API#net_listening
asio::awaitable<void> NetRpcApi::handle_net_listening(const nlohmann::json& request, nlohmann::json& reply) {
    reply = make_json_content(request["id"], true);
    // TODO(canepat): needs p2pSentry integration in TG Core (accumulate listening from multiple sentries)
    co_return;
}

// https://eth.wiki/json-rpc/API#net_peercount
asio::awaitable<void> NetRpcApi::handle_net_peer_count(const nlohmann::json& request, nlohmann::json& reply) {
    reply = make_json_content(request["id"], to_quantity(25));
    // TODO(canepat): needs p2pSentry integration in TG Core (accumulate peer counts from multiple sentries)
    co_return;
}

// https://eth.wiki/json-rpc/API#net_version
asio::awaitable<void> NetRpcApi::handle_net_version(const nlohmann::json& request, nlohmann::json& reply) {
    try {
        const auto net_version = co_await backend_->get_net_version();
        const auto net_version_str = std::to_string(net_version);
        reply = make_json_content(request["id"], net_version_str);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], -32000, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }
}

} // namespace silkrpc::commands
                    
