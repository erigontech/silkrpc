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

#include "txpool_api.hpp"

#include <algorithm>
#include <cstring>
#include <exception>
#include <iostream>
#include <map>
#include <string>
#include <utility>

#include <boost/endian/conversion.hpp>
#include <evmc/evmc.hpp>
#include <silkworm/chain/config.hpp>
#include <silkworm/common/util.hpp>
#include <silkworm/common/base.hpp>
#include <silkworm/execution/address.hpp>
#include <silkworm/db/util.hpp>
#include <silkworm/types/receipt.hpp>
#include <silkworm/types/transaction.hpp>

#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/core/cached_chain.hpp>
#include <silkrpc/core/blocks.hpp>
#include <silkrpc/core/evm_executor.hpp>
#include <silkrpc/core/evm_access_list_tracer.hpp>
#include <silkrpc/core/estimate_gas_oracle.hpp>
#include <silkrpc/core/gas_price_oracle.hpp>
#include <silkrpc/core/rawdb/chain.hpp>
#include <silkrpc/core/receipts.hpp>
#include <silkrpc/core/state_reader.hpp>
#include <silkrpc/ethdb/bitmap.hpp>
#include <silkrpc/ethdb/cbor.hpp>
#include <silkrpc/ethdb/tables.hpp>
#include <silkrpc/ethdb/transaction_database.hpp>
#include <silkrpc/json/types.hpp>
#include <silkrpc/types/block.hpp>
#include <silkrpc/types/call.hpp>
#include <silkrpc/types/filter.hpp>
#include <silkrpc/types/transaction.hpp>

namespace silkrpc::commands {

// https://eth.wiki/json-rpc/API#txpool_status
asio::awaitable<void> TxPoolRpcApi::handle_txpool_status(const nlohmann::json& request, nlohmann::json& reply) {
    try {
        struct TxPoolStatusInfo txpool_status;
        auto status_info = co_await tx_pool_->get_status();
        txpool_status.pending = status_info.pending;
        txpool_status.queued = status_info.queued;
        txpool_status.base_fee = status_info.base_fee;

        reply = make_json_content(request["id"], txpool_status);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_return;
}

// https://geth.ethereum.org/docs/rpc/ns-txpool
asio::awaitable<void> TxPoolRpcApi::handle_txpool_content(const nlohmann::json& request, nlohmann::json& reply) {
    try {
        silkworm::DecodingResult result = silkworm::DecodingResult::kOk;
        auto txpool_transactions = co_await tx_pool_->get_transactions();

        std::map <std::string, std::map<std::string, std::map<std::string, struct TransactionInfo>>> transactions_content;
        transactions_content["queued"];
        transactions_content["pending"];
        transactions_content["baseFee"];

        for (int i = 0; i < txpool_transactions.txs.size(); i++) {
           silkworm::ByteView from{txpool_transactions.txs[i].rlp};
           struct TransactionInfo txInfo{};
           silkworm::Transaction& silkworm_transaction = dynamic_cast<Transaction&>(txInfo.transaction);
           auto result = silkworm::rlp::decode(from, silkworm_transaction);
           if (result != silkworm::DecodingResult::kOk) {
              break;
           }
           silkworm::ByteView bv{txpool_transactions.txs[i].sender};
           std::string sender_str =  silkworm::to_hex(bv, true);
           if (txpool_transactions.txs[i].type == silkrpc::txpool::Type::QUEUED) {
              transactions_content["queued"][sender_str].insert(std::make_pair(std::to_string(txInfo.transaction.nonce), txInfo));
           } else if (txpool_transactions.txs[i].type == silkrpc::txpool::Type::PENDING) {
              transactions_content["pending"][sender_str].insert(std::make_pair(std::to_string(txInfo.transaction.nonce), txInfo));
           } else {
              transactions_content["baseFee"][sender_str].insert(std::make_pair(std::to_string(txInfo.transaction.nonce), txInfo));
           }
        }

        if (result == silkworm::DecodingResult::kOk) {
           reply = make_json_content(request["id"], transactions_content);
        } else {
           reply = make_json_error(request["id"], 100, "RLP decoding error");
        }
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_return;
}

} // namespace silkrpc::commands
