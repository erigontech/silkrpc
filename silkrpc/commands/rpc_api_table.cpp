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

#include "rpc_api_table.hpp"

#include <cstring>

#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/http/methods.hpp>

namespace silkrpc::commands {

RpcApiTable::RpcApiTable(const std::string& api_spec) {
    build_handlers(api_spec);
}

std::optional<RpcApiTable::HandleMethod> RpcApiTable::find_handler(const std::string& method) const {
    const auto handle_method_pair = handlers_.find(method);
    if (handle_method_pair == handlers_.end()) {
        return std::nullopt;
    }
    return handle_method_pair->second;
}

void RpcApiTable::build_handlers(const std::string& api_spec) {
    auto start = 0u;
    auto end = api_spec.find(kApiSpecSeparator);
    while (end != std::string::npos) {
        add_handlers(api_spec.substr(start, end - start));
        start = end + std::strlen(kApiSpecSeparator);
        end = api_spec.find(kApiSpecSeparator, start);
    }
    add_handlers(api_spec.substr(start, end));
}

void RpcApiTable::add_handlers(const std::string& api_namespace) {
    if (api_namespace == kDebugApiNamespace) {
        add_debug_handlers();
    } else if (api_namespace == kEthApiNamespace) {
        add_eth_handlers();
    } else if (api_namespace == kNetApiNamespace) {
        add_net_handlers();
    } else if (api_namespace == kParityApiNamespace) {
        add_parity_handlers();
    } else if (api_namespace == kErigonApiNamespace) {
        add_erigon_handlers();
    } else if (api_namespace == kTraceApiNamespace) {
        add_trace_handlers();
    } else if (api_namespace == kWeb3ApiNamespace) {
        add_web3_handlers();
    } else if (api_namespace == kEngineApiNamespace) {
        add_engine_handlers();
    } else if (api_namespace == kTxPoolApiNamespace) {
        add_txpool_handlers();
    } else {
        SILKRPC_WARN << "Server::add_handlers invalid namespace [" << api_namespace << "] ignored\n";
    }
}

void RpcApiTable::add_debug_handlers() {
    handlers_[http::method::k_debug_accountRange] = &commands::RpcApi::handle_debug_account_range;
    handlers_[http::method::k_debug_getModifiedAccountsByNumber] = &commands::RpcApi::handle_debug_get_modified_accounts_by_number;
    handlers_[http::method::k_debug_getModifiedAccountsByHash] = &commands::RpcApi::handle_debug_get_modified_accounts_by_hash;
    handlers_[http::method::k_debug_storageRangeAt] = &commands::RpcApi::handle_debug_storage_range_at;
    handlers_[http::method::k_debug_traceTransaction] = &commands::RpcApi::handle_debug_trace_transaction;
    handlers_[http::method::k_debug_traceCall] = &commands::RpcApi::handle_debug_trace_call;
    handlers_[http::method::k_debug_traceBlockByNumber] = &commands::RpcApi::handle_debug_trace_block_by_number;
    handlers_[http::method::k_debug_traceBlockByHash] = &commands::RpcApi::handle_debug_trace_block_by_hash;
}

void RpcApiTable::add_eth_handlers() {
    handlers_[http::method::k_eth_blockNumber] = &commands::RpcApi::handle_eth_block_number;
    handlers_[http::method::k_eth_chainId] = &commands::RpcApi::handle_eth_chain_id;
    handlers_[http::method::k_eth_protocolVersion] = &commands::RpcApi::handle_eth_protocol_version;
    handlers_[http::method::k_eth_syncing] = &commands::RpcApi::handle_eth_syncing;
    handlers_[http::method::k_eth_gasPrice] = &commands::RpcApi::handle_eth_gas_price;
    handlers_[http::method::k_eth_getBlockByHash] = &commands::RpcApi::handle_eth_get_block_by_hash;
    handlers_[http::method::k_eth_getBlockByNumber] = &commands::RpcApi::handle_eth_get_block_by_number;
    handlers_[http::method::k_eth_getBlockTransactionCountByHash] = &commands::RpcApi::handle_eth_get_block_transaction_count_by_hash;
    handlers_[http::method::k_eth_getBlockTransactionCountByNumber] = &commands::RpcApi::handle_eth_get_block_transaction_count_by_number;
    handlers_[http::method::k_eth_getUncleByBlockHashAndIndex] = &commands::RpcApi::handle_eth_get_uncle_by_block_hash_and_index;
    handlers_[http::method::k_eth_getUncleByBlockNumberAndIndex] = &commands::RpcApi::handle_eth_get_uncle_by_block_number_and_index;
    handlers_[http::method::k_eth_getUncleCountByBlockHash] = &commands::RpcApi::handle_eth_get_uncle_count_by_block_hash;
    handlers_[http::method::k_eth_getUncleCountByBlockNumber] = &commands::RpcApi::handle_eth_get_uncle_count_by_block_number;
    handlers_[http::method::k_eth_getTransactionByHash] = &commands::RpcApi::handle_eth_get_transaction_by_hash;
    handlers_[http::method::k_eth_getTransactionByBlockHashAndIndex] = &commands::RpcApi::handle_eth_get_transaction_by_block_hash_and_index;
    handlers_[http::method::k_eth_getTransactionByBlockNumberAndIndex] = &commands::RpcApi::handle_eth_get_transaction_by_block_number_and_index;
    handlers_[http::method::k_eth_getRawTransactionByHash] = &commands::RpcApi::handle_eth_get_raw_transaction_by_hash;
    handlers_[http::method::k_eth_getRawTransactionByBlockHashAndIndex] = &commands::RpcApi::handle_eth_get_raw_transaction_by_block_hash_and_index;
    handlers_[http::method::k_eth_getRawTransactionByBlockNumberAndIndex] = &commands::RpcApi::handle_eth_get_raw_transaction_by_block_number_and_index;
    handlers_[http::method::k_eth_getTransactionReceipt] = &commands::RpcApi::handle_eth_get_transaction_receipt;
    handlers_[http::method::k_eth_estimateGas] = &commands::RpcApi::handle_eth_estimate_gas;
    handlers_[http::method::k_eth_getBalance] = &commands::RpcApi::handle_eth_get_balance;
    handlers_[http::method::k_eth_getCode] = &commands::RpcApi::handle_eth_get_code;
    handlers_[http::method::k_eth_getTransactionCount] = &commands::RpcApi::handle_eth_get_transaction_count;
    handlers_[http::method::k_eth_getStorageAt] = &commands::RpcApi::handle_eth_get_storage_at;
    handlers_[http::method::k_eth_call] = &commands::RpcApi::handle_eth_call;
    handlers_[http::method::k_eth_callBundle] = &commands::RpcApi::handle_eth_call_bundle;
    handlers_[http::method::k_eth_createAccessList] = &commands::RpcApi::handle_eth_create_access_list;
    handlers_[http::method::k_eth_newFilter] = &commands::RpcApi::handle_eth_new_filter;
    handlers_[http::method::k_eth_newBlockFilter] = &commands::RpcApi::handle_eth_new_block_filter;
    handlers_[http::method::k_eth_newPendingTransactionFilter] = &commands::RpcApi::handle_eth_new_pending_transaction_filter;
    handlers_[http::method::k_eth_getFilterChanges] = &commands::RpcApi::handle_eth_get_filter_changes;
    handlers_[http::method::k_eth_uninstallFilter] = &commands::RpcApi::handle_eth_uninstall_filter;
    handlers_[http::method::k_eth_getLogs] = &commands::RpcApi::handle_eth_get_logs;
    handlers_[http::method::k_eth_sendRawTransaction] = &commands::RpcApi::handle_eth_send_raw_transaction;
    handlers_[http::method::k_eth_sendTransaction] = &commands::RpcApi::handle_eth_send_transaction;
    handlers_[http::method::k_eth_signTransaction] = &commands::RpcApi::handle_eth_sign_transaction;
    handlers_[http::method::k_eth_getProof] = &commands::RpcApi::handle_eth_get_proof;
    handlers_[http::method::k_eth_mining] = &commands::RpcApi::handle_eth_mining;
    handlers_[http::method::k_eth_coinbase] = &commands::RpcApi::handle_eth_coinbase;
    handlers_[http::method::k_eth_hashrate] = &commands::RpcApi::handle_eth_hashrate;
    handlers_[http::method::k_eth_submitHashrate] = &commands::RpcApi::handle_eth_submit_hashrate;
    handlers_[http::method::k_eth_getWork] = &commands::RpcApi::handle_eth_get_work;
    handlers_[http::method::k_eth_submitWork] = &commands::RpcApi::handle_eth_submit_work;
    handlers_[http::method::k_eth_subscribe] = &commands::RpcApi::handle_eth_subscribe;
    handlers_[http::method::k_eth_unsubscribe] = &commands::RpcApi::handle_eth_unsubscribe;
    handlers_[http::method::k_eth_getBlockReceipts] = &commands::RpcApi::handle_parity_get_block_receipts;
}

void RpcApiTable::add_net_handlers() {
    handlers_[http::method::k_net_listening] = &commands::RpcApi::handle_net_listening;
    handlers_[http::method::k_net_peerCount] = &commands::RpcApi::handle_net_peer_count;
    handlers_[http::method::k_net_version] = &commands::RpcApi::handle_net_version;
}

void RpcApiTable::add_parity_handlers() {
    handlers_[http::method::k_parity_getBlockReceipts] = &commands::RpcApi::handle_parity_get_block_receipts;
    handlers_[http::method::k_parity_listStorageKeys] = &commands::RpcApi::handle_parity_list_storage_keys;
}

void RpcApiTable::add_erigon_handlers() {
    handlers_[http::method::k_erigon_getBlockByTimestamp] = &commands::RpcApi::handle_erigon_get_block_by_timestamp;
    handlers_[http::method::k_erigon_getHeaderByHash] = &commands::RpcApi::handle_erigon_get_header_by_hash;
    handlers_[http::method::k_erigon_getHeaderByNumber] = &commands::RpcApi::handle_erigon_get_header_by_number;
    handlers_[http::method::k_erigon_getLogsByHash] = &commands::RpcApi::handle_erigon_get_logs_by_hash;
    handlers_[http::method::k_erigon_forks] = &commands::RpcApi::handle_erigon_forks;
    handlers_[http::method::k_erigon_issuance] = &commands::RpcApi::handle_erigon_issuance;
}

void RpcApiTable::add_trace_handlers() {
    handlers_[http::method::k_trace_call] = &commands::RpcApi::handle_trace_call;
    handlers_[http::method::k_trace_callMany] = &commands::RpcApi::handle_trace_call_many;
    handlers_[http::method::k_trace_rawTransaction] = &commands::RpcApi::handle_trace_raw_transaction;
    handlers_[http::method::k_trace_replayBlockTransactions] = &commands::RpcApi::handle_trace_replay_block_transactions;
    handlers_[http::method::k_trace_replayTransaction] = &commands::RpcApi::handle_trace_replay_transaction;
    handlers_[http::method::k_trace_block] = &commands::RpcApi::handle_trace_block;
    handlers_[http::method::k_trace_filter] = &commands::RpcApi::handle_trace_filter;
    handlers_[http::method::k_trace_get] = &commands::RpcApi::handle_trace_get;
    handlers_[http::method::k_trace_transaction] = &commands::RpcApi::handle_trace_transaction;
}

void RpcApiTable::add_web3_handlers() {
    handlers_[http::method::k_web3_clientVersion] = &commands::RpcApi::handle_web3_client_version;
    handlers_[http::method::k_web3_sha3] = &commands::RpcApi::handle_web3_sha3;
}

void RpcApiTable::add_engine_handlers() {
    handlers_[http::method::k_engine_getPayloadV1] = &commands::RpcApi::handle_engine_get_payload_v1;
    handlers_[http::method::k_engine_newPayloadV1] = &commands::RpcApi::handle_engine_new_payload_v1;
    handlers_[http::method::k_engine_forkchoiceUpdatedV1] = &commands::RpcApi::handle_engine_forkchoice_updated_v1;
    handlers_[http::method::k_engine_exchangeTransitionConfiguration] = &commands::RpcApi::handle_engine_exchange_transition_configuration_v1;
}

void RpcApiTable::add_txpool_handlers() {
    handlers_[http::method::k_txpool_status] = &commands::RpcApi::handle_txpool_status;
    handlers_[http::method::k_txpool_content] = &commands::RpcApi::handle_txpool_content;
}

} // namespace silkrpc::commands
