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

#include "eth_api.hpp"

#include <silkrpc/croaring/roaring.hh>
#include <silkrpc/json/types.hpp>
#include <silkrpc/stagedsync/stages.hpp>

namespace silkrpc::json {

// https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_blockNumber
asio::awaitable<void> EthereumRpcApi::handle_eth_block_number(const nlohmann::json& request, nlohmann::json& reply) {
    const auto block_height = co_await stages::get_sync_stage_progress(*database_, stages::kFinish);

    // TODO: define namespace for JSON RPC structs (eth::jsonrpc), then use arbitrary type conv
    reply = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"" + std::to_string(block_height) + "\"}";
    co_return;
}

// https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_getlogs
asio::awaitable<void> EthereumRpcApi::handle_eth_get_logs(const nlohmann::json& request, nlohmann::json& reply) {
    auto filter = request["params"].get<eth::Filter>();

    Roaring r;

    // TODO: define namespace for JSON RPC structs (eth::jsonrpc), then use arbitrary type conv
    reply = "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":\"method not YET implemented\"}";
    co_return;
}

} // namespace silkrpc::json