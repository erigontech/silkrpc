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

#include "parity_api.hpp"

#include <string>

#include <silkworm/common/util.hpp>

#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/core/blocks.hpp>
#include <silkrpc/core/receipts.hpp>
#include <silkrpc/core/rawdb/chain.hpp>
#include <silkrpc/ethdb/transaction_database.hpp>
#include <silkrpc/json/types.hpp>

namespace silkrpc::commands {

// https://eth.wiki/json-rpc/API#parity_getblockreceipts
asio::awaitable<void> ParityRpcApi::handle_parity_get_block_receipts(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid parity_getBlockReceipts params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_id = params[0].get<std::string>();
    SILKRPC_DEBUG << "block_id: " << block_id << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_number{co_await core::get_block_number(block_id, tx_database)};
        const auto block_hash{co_await core::rawdb::read_canonical_block_hash(tx_database, block_number)};
        const auto block_with_hash{co_await core::rawdb::read_block(tx_database, block_hash, block_number)};
        const auto receipts{co_await core::get_receipts(tx_database, block_hash, block_number)};

        for (auto receipt : receipts) {
            const auto tx = block_with_hash.block.transactions[receipt.tx_index];
            receipt.from = tx.from;
            receipt.to = tx.to;
            receipt.type = tx.type;
        }

        reply = make_json_content(request["id"], receipts);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << "\n";
        reply = make_json_error(request["id"], 100, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_await tx->close(); // RAII not (yet) available with coroutines
    co_return;
}

} // namespace silkrpc::commands
