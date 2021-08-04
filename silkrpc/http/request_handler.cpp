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

#include "methods.hpp"
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"

#include <silkrpc/common/clock_time.hpp>
#include <silkrpc/common/log.hpp>

namespace silkrpc::http {

std::map<std::string, RequestHandler::HandleMethod> RequestHandler::handlers_ = {
    {method::k_web3_clientVersion, &commands::RpcApi::handle_web3_client_version},
    {method::k_web3_sha3, &commands::RpcApi::handle_web3_sha3},
    {method::k_net_listening, &commands::RpcApi::handle_net_listening},
    {method::k_net_peerCount, &commands::RpcApi::handle_net_peer_count},
    {method::k_net_version, &commands::RpcApi::handle_net_version},
    {method::k_eth_blockNumber, &commands::RpcApi::handle_eth_block_number},
    {method::k_eth_chainId, &commands::RpcApi::handle_eth_chain_id},
    {method::k_eth_protocolVersion, &commands::RpcApi::handle_eth_protocol_version},
    {method::k_eth_syncing, &commands::RpcApi::handle_eth_syncing},
    {method::k_eth_gasPrice, &commands::RpcApi::handle_eth_gas_price},
    {method::k_eth_getBlockByHash, &commands::RpcApi::handle_eth_get_block_by_hash},
    {method::k_eth_getBlockByNumber, &commands::RpcApi::handle_eth_get_block_by_number},
    {method::k_eth_getBlockTransactionCountByHash, &commands::RpcApi::handle_eth_get_block_transaction_count_by_hash},
    {method::k_eth_getBlockTransactionCountByNumber, &commands::RpcApi::handle_eth_get_block_transaction_count_by_number},
    {method::k_eth_getUncleByBlockHashAndIndex, &commands::RpcApi::handle_eth_get_uncle_by_block_hash_and_index},
    {method::k_eth_getUncleByBlockNumberAndIndex, &commands::RpcApi::handle_eth_get_uncle_by_block_number_and_index},
    {method::k_eth_getUncleCountByBlockHash, &commands::RpcApi::handle_eth_get_uncle_count_by_block_hash},
    {method::k_eth_getUncleCountByBlockNumber, &commands::RpcApi::handle_eth_get_uncle_count_by_block_number},
    {method::k_eth_getTransactionByHash, &commands::RpcApi::handle_eth_get_transaction_by_hash},
    {method::k_eth_getTransactionByBlockHashAndIndex, &commands::RpcApi::handle_eth_get_transaction_by_block_hash_and_index},
    {method::k_eth_getTransactionByBlockNumberAndIndex, &commands::RpcApi::handle_eth_get_transaction_by_block_number_and_index},
    {method::k_eth_getTransactionReceipt, &commands::RpcApi::handle_eth_get_transaction_receipt},
    {method::k_eth_estimateGas, &commands::RpcApi::handle_eth_estimate_gas},
    {method::k_eth_getBalance, &commands::RpcApi::handle_eth_get_balance},
    {method::k_eth_getCode, &commands::RpcApi::handle_eth_get_code},
    {method::k_eth_getTransactionCount, &commands::RpcApi::handle_eth_get_transaction_count},
    {method::k_eth_getStorageAt, &commands::RpcApi::handle_eth_get_storage_at},
    {method::k_eth_call, &commands::RpcApi::handle_eth_call},
    {method::k_eth_newFilter, &commands::RpcApi::handle_eth_new_filter},
    {method::k_eth_newBlockFilter, &commands::RpcApi::handle_eth_new_block_filter},
    {method::k_eth_newPendingTransactionFilter, &commands::RpcApi::handle_eth_new_pending_transaction_filter},
    {method::k_eth_getFilterChanges, &commands::RpcApi::handle_eth_get_filter_changes},
    {method::k_eth_uninstallFilter, &commands::RpcApi::handle_eth_uninstall_filter},
    {method::k_eth_getLogs, &commands::RpcApi::handle_eth_get_logs},
    {method::k_eth_sendRawTransaction, &commands::RpcApi::handle_eth_send_raw_transaction},
    {method::k_eth_sendTransaction, &commands::RpcApi::handle_eth_send_transaction},
    {method::k_eth_signTransaction, &commands::RpcApi::handle_eth_sign_transaction},
    {method::k_eth_getProof, &commands::RpcApi::handle_eth_get_proof},
    {method::k_eth_mining, &commands::RpcApi::handle_eth_mining},
    {method::k_eth_coinbase, &commands::RpcApi::handle_eth_coinbase},
    {method::k_eth_hashrate, &commands::RpcApi::handle_eth_hashrate},
    {method::k_eth_submitHashrate, &commands::RpcApi::handle_eth_submit_hashrate},
    {method::k_eth_getWork, &commands::RpcApi::handle_eth_get_work},
    {method::k_eth_submitWork, &commands::RpcApi::handle_eth_submit_work},
    {method::k_eth_subscribe, &commands::RpcApi::handle_eth_subscribe},
    {method::k_eth_unsubscribe, &commands::RpcApi::handle_eth_unsubscribe},
    {method::k_eth_getBlockReceipts, &commands::RpcApi::handle_parity_get_block_receipts},
    {method::k_debug_accountRange, &commands::RpcApi::handle_debug_account_range},
    {method::k_debug_getModifiedAccountsByNumber, &commands::RpcApi::handle_debug_get_modified_accounts_by_number},
    {method::k_debug_getModifiedAccountsByHash, &commands::RpcApi::handle_debug_get_modified_accounts_by_hash},
    {method::k_debug_storageRangeAt, &commands::RpcApi::handle_debug_storage_range_at},
    {method::k_debug_traceTransaction, &commands::RpcApi::handle_debug_trace_transaction},
    {method::k_debug_traceCall, &commands::RpcApi::handle_debug_trace_call},
    {method::k_trace_call, &commands::RpcApi::handle_trace_call},
    {method::k_trace_callMany, &commands::RpcApi::handle_trace_call_many},
    {method::k_trace_rawTransaction, &commands::RpcApi::handle_trace_raw_transaction},
    {method::k_trace_replayBlockTransactions, &commands::RpcApi::handle_trace_replay_block_transactions},
    {method::k_trace_replayTransaction, &commands::RpcApi::handle_trace_replay_transaction},
    {method::k_trace_block, &commands::RpcApi::handle_trace_block},
    {method::k_trace_filter, &commands::RpcApi::handle_trace_filter},
    {method::k_trace_get, &commands::RpcApi::handle_trace_get},
    {method::k_trace_transaction, &commands::RpcApi::handle_trace_transaction},
    {method::k_tg_getHeaderByHash, &commands::RpcApi::handle_tg_get_header_by_hash},
    {method::k_tg_getHeaderByNumber, &commands::RpcApi::handle_tg_get_header_by_number},
    {method::k_tg_getLogsByHash, &commands::RpcApi::handle_tg_get_logs_by_hash},
    {method::k_tg_forks, &commands::RpcApi::handle_tg_forks},
    {method::k_tg_issuance, &commands::RpcApi::handle_tg_issuance},
    {method::k_parity_getBlockReceipts, &commands::RpcApi::handle_parity_get_block_receipts},
};

asio::awaitable<void> RequestHandler::handle_request(const Request& request, Reply& reply) {
    SILKRPC_DEBUG << "handle_request content: " << request.content << "\n";
    auto start = clock_time::now();

    auto request_id{0};
    try {
        if (request.content.empty()) {
            reply.content = "";
            reply.status = Reply::no_content;
            reply.headers.reserve(2);
            reply.headers.emplace_back(Header{"Content-Length", std::to_string(reply.content.size())});
            reply.headers.emplace_back(Header{"Content-Type", "application/json"});
            SILKRPC_INFO << "handle_request t=" << clock_time::since(start) << "ns\n";
            co_return;
        }

        const auto request_json = nlohmann::json::parse(request.content);
        request_id = request_json["id"].get<uint32_t>();
        if (!request_json.contains("method")) {
            reply.content = make_json_error(request_id, -32600, "method missing").dump() + "\n";
            reply.status = Reply::bad_request;
            reply.headers.reserve(2);
            reply.headers.emplace_back(Header{"Content-Length", std::to_string(reply.content.size())});
            reply.headers.emplace_back(Header{"Content-Type", "application/json"});
            SILKRPC_INFO << "handle_request t=" << clock_time::since(start) << "ns\n";
            co_return;
        }

        auto method = request_json["method"].get<std::string>();
        if (RequestHandler::handlers_.find(method) == RequestHandler::handlers_.end()) {
            reply.content = make_json_error(request_id, -32601, "method not existent or not implemented").dump() + "\n";
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

        reply.content = reply_json.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace) + "\n";
        reply.status = Reply::ok;
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << "\n";
        reply.content = make_json_error(request_id, 100, e.what()).dump() + "\n";
        reply.status = Reply::internal_server_error;
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception\n";
        reply.content = make_json_error(request_id, 100, "unexpected exception").dump() + "\n";
        reply.status = Reply::internal_server_error;
    }

    reply.headers.reserve(2);
    reply.headers.emplace_back(Header{"Content-Length", std::to_string(reply.content.size())});
    reply.headers.emplace_back(Header{"Content-Type", "application/json"});

    SILKRPC_INFO << "handle_request t=" << clock_time::since(start) << "ns\n";
    co_return;
}

} // namespace silkrpc::http
