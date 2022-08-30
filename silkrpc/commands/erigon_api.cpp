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

#include "erigon_api.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include <intx/intx.hpp>

#include <silkrpc/common/binary_search.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/consensus/ethash.hpp>
#include <silkrpc/core/blocks.hpp>
#include <silkrpc/core/cached_chain.hpp>
#include <silkrpc/core/receipts.hpp>
#include <silkrpc/core/rawdb/chain.hpp>
#include <silkrpc/ethdb/kv/cached_database.hpp>
#include <silkrpc/ethdb/transaction_database.hpp>
#include <silkrpc/json/types.hpp>
#include <silkworm/common/binary_search.hpp>
#include <silkworm/common/util.hpp>

namespace silkrpc::commands {

ErigonRpcApi::ErigonRpcApi(Context& context)
    : database_(context.database()),
      context_(context),
      block_cache_(context.block_cache()),
      state_cache_(context.state_cache()) {}

// https://eth.wiki/json-rpc/API#erigon_getBlockByTimestamp
boost::asio::awaitable<void> ErigonRpcApi::handle_erigon_get_block_by_timestamp(const nlohmann::json& request, nlohmann::json& reply) {
    // Decode request parameters
    const auto params = request["params"];
    if (params.size() != 2) {
        auto error_msg = "invalid erigon_getBlockByTimestamp params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_timestamp = params[0].get<std::string>();
    const auto full_tx = params[1].get<bool>();
    SILKRPC_DEBUG << "block_timestamp: " << block_timestamp << " full_tx: " << full_tx << "\n";

    const std::string::size_type begin = block_timestamp.find_first_not_of(" \"");
    const std::string::size_type end = block_timestamp.find_last_not_of(" \"");
    const uint64_t timestamp = std::stol(block_timestamp.substr(begin, end - begin + 1), 0, 0);

    // Open a new remote database transaction (no need to close if code throws before the end)
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        // Lookup the first and last block headers
        const auto first_header = co_await core::rawdb::read_header_by_number(tx_database, core::kEarliestBlockNumber);
        const auto current_header = co_await core::rawdb::read_current_header(tx_database);
        const uint64_t current_block_number = current_header.number;

        // Find the lowest block header w/ timestamp greater or equal to provided timestamp
        uint64_t block_number;
        if (current_header.timestamp <= timestamp) {
            block_number = current_block_number;
        } else if (first_header.timestamp >= timestamp) {
            block_number = core::kEarliestBlockNumber;
        } else {
            // Good-ol' binary search to find the lowest block header matching timestamp
            const auto matching_block_number = co_await binary_search(current_block_number, [&](uint64_t i) -> boost::asio::awaitable<bool> {
                const auto header = co_await core::rawdb::read_header_by_number(tx_database, i);
                co_return header.timestamp >= timestamp;
            });
            // TODO(canepat) we should try to avoid this block header lookup (just done in search)
            const auto matching_header = co_await core::rawdb::read_header_by_number(tx_database, matching_block_number);
            if (matching_header.timestamp > timestamp) {
                block_number = matching_block_number - 1;
            } else {
                block_number = matching_block_number;
            }
        }

        // Lookup and return the matching block
        const auto block_with_hash = co_await core::read_block_by_number(*block_cache_, tx_database, block_number);
        const auto total_difficulty = co_await core::rawdb::read_total_difficulty(tx_database, block_with_hash.hash, block_number);
        const Block extended_block{block_with_hash, total_difficulty, full_tx};

        reply = make_json_content(request["id"], extended_block);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    // Close remote database transaction, RAII not available with coroutines
    co_await tx->close();
    co_return;
}

// https://eth.wiki/json-rpc/API#erigon_getHeaderByHash
boost::asio::awaitable<void> ErigonRpcApi::handle_erigon_get_header_by_hash(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid erigon_getHeaderByHash params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_hash = params[0].get<evmc::bytes32>();
    SILKRPC_DEBUG << "block_hash: " << block_hash << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto header{co_await core::rawdb::read_header_by_hash(tx_database, block_hash)};

        reply = make_json_content(request["id"], header);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_await tx->close(); // RAII not (yet) available with coroutines
    co_return;
}

// https://eth.wiki/json-rpc/API#erigon_getHeaderByNumber
boost::asio::awaitable<void> ErigonRpcApi::handle_erigon_get_header_by_number(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid erigon_getHeaderByNumber params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_id = params[0].get<std::string>();
    SILKRPC_DEBUG << "block_id: " << block_id << "\n";

    if (block_id == core::kPendingBlockId) {
        // TODO(canepat): add pending block only known to the miner
        auto error_msg = "pending block not implemented in erigon_getHeaderByNumber";
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_number{co_await core::get_block_number(block_id, tx_database)};
        const auto header{co_await core::rawdb::read_header_by_number(tx_database, block_number)};

        reply = make_json_content(request["id"], header);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_await tx->close(); // RAII not (yet) available with coroutines
    co_return;
}

// https://eth.wiki/json-rpc/API#erigon_getlogsbyhash
boost::asio::awaitable<void> ErigonRpcApi::handle_erigon_get_logs_by_hash(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid erigon_getHeaderByHash params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_hash = params[0].get<evmc::bytes32>();
    SILKRPC_DEBUG << "block_hash: " << block_hash << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_with_hash = co_await core::read_block_by_hash(*block_cache_, tx_database, block_hash);
        const auto receipts{co_await core::get_receipts(tx_database, block_with_hash)};

        SILKRPC_DEBUG << "receipts.size(): " << receipts.size() << "\n";
        std::vector<Log> logs{};
        logs.reserve(receipts.size());
        for (const auto receipt : receipts) {
            SILKRPC_DEBUG << "receipt.logs.size(): " << receipt.logs.size() << "\n";
            logs.insert(logs.end(), receipt.logs.begin(), receipt.logs.end());
        }
        SILKRPC_DEBUG << "logs.size(): " << logs.size() << "\n";

        reply = make_json_content(request["id"], logs);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_await tx->close(); // RAII not (yet) available with coroutines
    co_return;
}

// https://eth.wiki/json-rpc/API#erigon_forks
boost::asio::awaitable<void> ErigonRpcApi::handle_erigon_forks(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto chain_config{co_await silkrpc::core::rawdb::read_chain_config(tx_database)};
        SILKRPC_DEBUG << "chain config: " << chain_config << "\n";

        Forks forks{chain_config};

        reply = make_json_content(request["id"], forks);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_await tx->close(); // RAII not (yet) available with coroutines
    co_return;
}

// https://eth.wiki/json-rpc/API#erigon_issuance
boost::asio::awaitable<void> ErigonRpcApi::handle_erigon_issuance(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid erigon_issuance params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_id = params[0].get<std::string>();
    SILKRPC_DEBUG << "block_id: " << block_id << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto chain_config{co_await silkrpc::core::rawdb::read_chain_config(tx_database)};
        SILKRPC_DEBUG << "chain config: " << chain_config << "\n";

        Issuance issuance{}; // default is empty: no PoW => no issuance
        if (chain_config.config.count("ethash") != 0) {
            const auto block_number{co_await core::get_block_number(block_id, tx_database)};
            const auto block_with_hash{co_await core::rawdb::read_block_by_number(tx_database, block_number)};
            const auto block_reward{ethash::compute_reward(chain_config, block_with_hash.block)};
            intx::uint256 total_ommer_reward;
            for (const auto ommer_reward :  block_reward.ommer_rewards) {
                total_ommer_reward += ommer_reward;
            }
            intx::uint256 block_issuance = block_reward.miner_reward + total_ommer_reward;
            issuance.block_reward = "0x" + intx::to_string(block_reward.miner_reward);
            issuance.ommer_reward = "0x" + intx::to_string(total_ommer_reward);
            issuance.issuance = "0x" + intx::to_string(block_issuance);
        }
        reply = make_json_content(request["id"], issuance);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_await tx->close(); // RAII not (yet) available with coroutines
    co_return;
}

} // namespace silkrpc::commands
