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

#include <string>
#include <utility>

namespace silkrpc::commands {

// https://eth.wiki/json-rpc/API#txpool_status
asio::awaitable<void> TxPoolRpcApi::handle_txpool_status(const nlohmann::json& request, nlohmann::json& reply) {
    try {
        const auto status = co_await tx_pool_->get_status();
        TxPoolStatusInfo txpool_status{status.pending, status.queued, status.base_fee};
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
        const auto txpool_transactions = co_await tx_pool_->get_transactions();

        TransactionContent transactions_content;
        transactions_content["queued"];
        transactions_content["pending"];
        transactions_content["baseFee"];
 
        bool error = false;
        for (int i = 0; i < txpool_transactions.txs.size(); i++) {
            silkworm::ByteView from{txpool_transactions.txs[i].rlp};
            Transaction txn{};
            const auto result = silkworm::rlp::decode(from, dynamic_cast<silkworm::Transaction&>(txn));
            if (result != silkworm::DecodingResult::kOk) {
                error = true;
                break;
            }
            txn.queued_in_pool = true;
            std::string sender = silkworm::to_hex(txpool_transactions.txs[i].sender, true);
            if (txpool_transactions.txs[i].type == silkrpc::txpool::Type::QUEUED) {
                transactions_content["queued"][sender].insert(std::make_pair(std::to_string(txn.nonce), txn));
            } else if (txpool_transactions.txs[i].type == silkrpc::txpool::Type::PENDING) {
                transactions_content["pending"][sender].insert(std::make_pair(std::to_string(txn.nonce), txn));
            } else {
                transactions_content["baseFee"][sender].insert(std::make_pair(std::to_string(txn.nonce), txn));
            }
        }

        if (!error) {
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
