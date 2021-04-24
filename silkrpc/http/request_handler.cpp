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
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "request_handler.hpp"

#include <iostream>
#include <sstream>

#include "methods.hpp"
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"

#include <silkrpc/common/clock_time.hpp>
#include <silkrpc/common/log.hpp>

namespace silkrpc::http {

std::map<std::string, RequestHandler::HandleMethod> RequestHandler::handlers_ = {
    {method::k_eth_blockNumber, &commands::RpcApi::handle_eth_block_number},
    {method::k_eth_call, &commands::RpcApi::handle_eth_call},
    {method::k_eth_chainId, &commands::RpcApi::handle_eth_chain_id},
    {method::k_eth_protocolVersion, &commands::RpcApi::handle_eth_protocol_version},
    {method::k_eth_syncing, &commands::RpcApi::handle_eth_syncing},
    {method::k_eth_getBlockByHash, &commands::RpcApi::handle_eth_get_block_by_hash},
    {method::k_eth_getBlockByNumber, &commands::RpcApi::handle_eth_get_block_by_number},
    {method::k_eth_getBlockTransactionCountByHash, &commands::RpcApi::handle_eth_get_block_transaction_count_by_hash},
    {method::k_eth_getBlockTransactionCountByNumber, &commands::RpcApi::handle_eth_get_block_transaction_count_by_number},
    {method::k_eth_getLogs, &commands::RpcApi::handle_eth_get_logs},
    {method::k_net_listening, &commands::RpcApi::handle_net_listening},
    {method::k_net_peerCount, &commands::RpcApi::handle_net_peer_count},
    {method::k_net_version, &commands::RpcApi::handle_net_version},
    {method::k_web3_clientVersion, &commands::RpcApi::handle_web3_client_version},
    {method::k_web3_sha3, &commands::RpcApi::handle_web3_sha3},
};

asio::awaitable<void> RequestHandler::handle_request(const Request& request, Reply& reply) {
    SILKRPC_DEBUG << "handle_request content: " << request.content << "\n";
    auto start = clock_time::now();

    try {
        if (request.content.empty()) {
            reply.content = "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":\"content missing\"}";
            reply.status = Reply::no_content;
            reply.headers.reserve(2);
            reply.headers.emplace_back(Header{"Content-Length", std::to_string(reply.content.size())});
            reply.headers.emplace_back(Header{"Content-Type", "application/json"});
            SILKRPC_INFO << "handle_request t=" << clock_time::since(start) << "ns\n";
            co_return;
        }

        auto request_json = nlohmann::json::parse(request.content);
        if (!request_json.contains("method")) {
            reply.content = "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":\"method missing\"}";
            reply.status = Reply::bad_request;
            reply.headers.reserve(2);
            reply.headers.emplace_back(Header{"Content-Length", std::to_string(reply.content.size())});
            reply.headers.emplace_back(Header{"Content-Type", "application/json"});
            SILKRPC_INFO << "handle_request t=" << clock_time::since(start) << "ns\n";
            co_return;
        }

        auto method = request_json["method"].get<std::string>();
        if (RequestHandler::handlers_.find(method) == RequestHandler::handlers_.end()) {
            reply.content = "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":\"method not implemented\"}";
            reply.status = Reply::not_implemented;
            reply.headers.reserve(2);
            reply.headers.emplace_back(Header{"Content-Length", std::to_string(reply.content.size())});
            reply.headers.emplace_back(Header{"Content-Type", "application/json"});
            SILKRPC_INFO << "handle_request t=" << clock_time::since(start) << "ns\n";
            co_return;
        }

        nlohmann::json reply_json;
        auto handle_method = RequestHandler::handlers_[method];
        co_await (&rpc_api_->*handle_method)(request_json, reply_json);

        reply.content = reply_json.dump();
        reply.status = Reply::ok;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";

        std::stringstream rc_stream;
        rc_stream << "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":\"" << e.what() << "\"}";
        reply.content = rc_stream.str();
        reply.status = Reply::internal_server_error;
    }

    reply.headers.reserve(2);
    reply.headers.emplace_back(Header{"Content-Length", std::to_string(reply.content.size())});
    reply.headers.emplace_back(Header{"Content-Type", "application/json"});

    SILKRPC_INFO << "handle_request t=" << clock_time::since(start) << "ns\n";
    co_return;
}

} // namespace silkrpc::http
