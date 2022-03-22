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

#include <algorithm>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>

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

// https://eth.wiki/json-rpc/API#eth_blocknumber
asio::awaitable<void> EthereumRpcApi::handle_eth_block_number(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};
        const auto block_height = co_await core::get_current_block_number(tx_database);
        reply = make_json_content(request["id"], to_quantity(block_height));
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

// https://eth.wiki/json-rpc/API#eth_chainid
asio::awaitable<void> EthereumRpcApi::handle_eth_chain_id(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};
        const auto chain_id = co_await core::rawdb::read_chain_id(tx_database);
        reply = make_json_content(request["id"], to_quantity(chain_id));
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

// https://eth.wiki/json-rpc/API#eth_protocolversion
asio::awaitable<void> EthereumRpcApi::handle_eth_protocol_version(const nlohmann::json& request, nlohmann::json& reply) {
    try {
        const auto protocol_version = co_await backend_->protocol_version();
        reply = make_json_content(request["id"], to_quantity(protocol_version));
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], -32000, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_return;
}

// https://eth.wiki/json-rpc/API#eth_syncing
asio::awaitable<void> EthereumRpcApi::handle_eth_syncing(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};
        const auto current_block_height = co_await core::get_current_block_number(tx_database);
        const auto highest_block_height = co_await core::get_highest_block_number(tx_database);
        if (current_block_height >= highest_block_height) {
            reply = make_json_content(request["id"], false);
        } else {
            reply = make_json_content(request["id"], {
                {"currentBlock", to_quantity(current_block_height)},
                {"highestBlock", to_quantity(highest_block_height)},
            });
        }
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

// https://eth.wiki/json-rpc/API#eth_gasprice
asio::awaitable<void> EthereumRpcApi::handle_eth_gas_price(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};
        auto block_number = co_await core::get_block_number(silkrpc::core::kLatestBlockId, tx_database);
        SILKRPC_INFO << "block_number " << block_number << "\n";

        BlockProvider block_provider = [this, &tx_database](uint64_t block_number) {
            return core::read_block_by_number(*context_.block_cache, tx_database, block_number);
        };

        GasPriceOracle gas_price_oracle{ block_provider};
        const auto gas_price = co_await gas_price_oracle.suggested_price(block_number);
        reply = make_json_content(request["id"], to_quantity(gas_price));
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

// https://eth.wiki/json-rpc/API#eth_getblockbyhash
asio::awaitable<void> EthereumRpcApi::handle_eth_get_block_by_hash(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 2) {
        auto error_msg = "invalid eth_getBlockByHash params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    auto block_hash = params[0].get<evmc::bytes32>();
    auto full_tx = params[1].get<bool>();
    SILKRPC_DEBUG << "block_hash: " << block_hash << " full_tx: " << std::boolalpha << full_tx << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_with_hash = co_await core::read_block_by_hash(*context_.block_cache, tx_database, block_hash);
        const auto block_number = block_with_hash.block.header.number;
        const auto total_difficulty = co_await core::rawdb::read_total_difficulty(tx_database, block_hash, block_number);
        const Block extended_block{block_with_hash, total_difficulty, full_tx};

        reply = make_json_content(request["id"], extended_block);
    } catch (const std::invalid_argument& iv) {
        SILKRPC_DEBUG << "invalid_argument: " << iv.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_content(request["id"], {});
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

// https://eth.wiki/json-rpc/API#eth_getblockbynumber
asio::awaitable<void> EthereumRpcApi::handle_eth_get_block_by_number(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 2) {
        auto error_msg = "invalid getBlockByNumber params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_id = params[0].get<std::string>();
    auto full_tx = params[1].get<bool>();
    SILKRPC_DEBUG << "block_id: " << block_id << " full_tx: " << std::boolalpha << full_tx << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_number = co_await core::get_block_number(block_id, tx_database);
        const auto block_with_hash = co_await core::read_block_by_number(*context_.block_cache, tx_database, block_number);
        const auto total_difficulty = co_await core::rawdb::read_total_difficulty(tx_database, block_with_hash.hash, block_number);
        const Block extended_block{block_with_hash, total_difficulty, full_tx};

        reply = make_json_content(request["id"], extended_block);
    } catch (const std::invalid_argument& iv) {
        SILKRPC_DEBUG << "invalid_argument: " << iv.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_content(request["id"], {});
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

// https://eth.wiki/json-rpc/API#eth_getblocktransactioncountbyhash
asio::awaitable<void> EthereumRpcApi::handle_eth_get_block_transaction_count_by_hash(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid eth_getBlockTransactionCountByHash params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    auto block_hash = params[0].get<evmc::bytes32>();
    SILKRPC_DEBUG << "block_hash: " << block_hash << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_with_hash = co_await core::read_block_by_hash(*context_.block_cache, tx_database, block_hash);
        const auto tx_count = block_with_hash.block.transactions.size();

        reply = make_json_content(request["id"], to_quantity(tx_count));
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

// https://eth.wiki/json-rpc/API#eth_getblocktransactioncountbynumber
asio::awaitable<void> EthereumRpcApi::handle_eth_get_block_transaction_count_by_number(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid eth_getBlockTransactionCountByNumber params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_id = params[0].get<std::string>();
    SILKRPC_DEBUG << "block_id: " << block_id << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_number = co_await core::get_block_number(block_id, tx_database);
        const auto block_with_hash = co_await core::read_block_by_number(*context_.block_cache, tx_database, block_number);

        reply = make_json_content(request["id"], to_quantity(block_with_hash.block.transactions.size()));
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

// https://eth.wiki/json-rpc/API#eth_getunclebyblockhashandindex
asio::awaitable<void> EthereumRpcApi::handle_eth_get_uncle_by_block_hash_and_index(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 2) {
        auto error_msg = "invalid eth_getUncleByBlockHashAndIndex params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    auto block_hash = params[0].get<evmc::bytes32>();
    auto index_string = params[1].get<std::string>();
    SILKRPC_DEBUG << "block_hash: " << block_hash << " index: " << index_string << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_with_hash = co_await core::read_block_by_hash(*context_.block_cache, tx_database, block_hash);

        const auto ommers = block_with_hash.block.ommers;

        auto index = std::stoul(index_string, 0, 16);
        if (index >= ommers.size()) {
            SILKRPC_WARN << "Requested uncle not found " << index_string << "\n";
            reply = make_json_content(request["id"], nullptr);
        } else {
            const auto block_number = block_with_hash.block.header.number;
            const auto total_difficulty = co_await core::rawdb::read_total_difficulty(tx_database, block_hash, block_number);
            auto uncle = ommers[index];

            silkworm::BlockWithHash uncle_block_with_hash{{{}, uncle}, uncle.hash()};
            const Block uncle_block_with_hash_and_td{uncle_block_with_hash, total_difficulty};

            reply = make_json_content(request["id"], uncle_block_with_hash_and_td);
        }
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

// https://eth.wiki/json-rpc/API#eth_getunclebyblocknumberandindex
asio::awaitable<void> EthereumRpcApi::handle_eth_get_uncle_by_block_number_and_index(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 2) {
        auto error_msg = "invalid eth_getUncleByBlockNumberAndIndex params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_id = params[0].get<std::string>();
    const auto index = params[1].get<std::string>();
    SILKRPC_DEBUG << "block_id: " << block_id << " index: " << index << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_number = co_await core::get_block_number(block_id, tx_database);
        const auto block_with_hash = co_await core::read_block_by_number(*context_.block_cache, tx_database, block_number);
        const auto ommers = block_with_hash.block.ommers;

        auto idx = std::stoul(index, 0, 16);
        if (idx >= ommers.size()) {
            SILKRPC_WARN << "Requested uncle not found " << index << "\n";
            reply = make_json_content(request["id"], nullptr);
        } else {
            const auto total_difficulty = co_await core::rawdb::read_total_difficulty(tx_database, block_with_hash.hash, block_number);
            auto uncle = ommers[idx];

            silkworm::BlockWithHash uncle_block_with_hash{{{}, uncle}, uncle.hash()};
            const Block uncle_block_with_hash_and_td{uncle_block_with_hash, total_difficulty};

            reply = make_json_content(request["id"], uncle_block_with_hash_and_td);
        }
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

// https://eth.wiki/json-rpc/API#eth_getunclecountbyblockhash
asio::awaitable<void> EthereumRpcApi::handle_eth_get_uncle_count_by_block_hash(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid eth_getUncleCountByBlockHash params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    auto block_hash = params[0].get<evmc::bytes32>();
    SILKRPC_DEBUG << "block_hash: " << block_hash << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_with_hash = co_await core::read_block_by_hash(*context_.block_cache, tx_database, block_hash);
        const auto ommers = block_with_hash.block.ommers;

        reply = make_json_content(request["id"], to_quantity(ommers.size()));
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

// https://eth.wiki/json-rpc/API#eth_getunclecountbyblocknumber
asio::awaitable<void> EthereumRpcApi::handle_eth_get_uncle_count_by_block_number(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid eth_getUncleCountByBlockNumber params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_id = params[0].get<std::string>();
    SILKRPC_DEBUG << "block_id: " << block_id << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_number = co_await core::get_block_number(block_id, tx_database);
        const auto block_with_hash = co_await core::read_block_by_number(*context_.block_cache, tx_database, block_number);
        const auto ommers = block_with_hash.block.ommers;

        reply = make_json_content(request["id"], to_quantity(ommers.size()));
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

// https://eth.wiki/json-rpc/API#eth_gettransactionbyhash
asio::awaitable<void> EthereumRpcApi::handle_eth_get_transaction_by_hash(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid eth_getTransactionByHash params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    auto transaction_hash = params[0].get<evmc::bytes32>();
    SILKRPC_DEBUG << "transaction_hash: " << transaction_hash << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};
        const auto optional_transaction = co_await core::rawdb::read_transaction_by_hash(tx_database, transaction_hash);
        if (!optional_transaction) {
            // TODO(sixtysixter)
            // Maybe no finalized transaction, try to retrieve it from the pool
            SILKRPC_DEBUG << "Retrieving not finalized transactions from pool not implemented yet\n";
            reply = make_json_content(request["id"], nullptr);
        } else {
            reply = make_json_content(request["id"], *optional_transaction);
        }
    } catch (const std::invalid_argument& iv) {
        SILKRPC_DEBUG << "invalid_argument: " << iv.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_content(request["id"], {});
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

// https://eth.wiki/json-rpc/API#eth_getrawtransactionbyhash
asio::awaitable<void> EthereumRpcApi::handle_eth_get_raw_transaction_by_hash(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid eth_getRawTransactionByHash params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    auto transaction_hash = params[0].get<evmc::bytes32>();
    SILKRPC_DEBUG << "transaction_hash: " << transaction_hash << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};
        const auto optional_transaction = co_await core::rawdb::read_transaction_by_hash(tx_database, transaction_hash);
        if (!optional_transaction) {
            // TODO(sixtysixter)
            // Maybe no finalized transaction, try to retrieve it from the pool
            SILKRPC_DEBUG << "Retrieving not finalized transactions from pool not implemented yet\n";
            reply = make_json_content(request["id"], nullptr);
        } else {
            silkworm::Bytes rlp{};
            silkworm::rlp::encode(rlp, *optional_transaction);
            reply = make_json_content(request["id"], rlp);
        }
    } catch (const std::invalid_argument& iv) {
        SILKRPC_DEBUG << "invalid_argument: " << iv.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_content(request["id"], {});
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

// https://eth.wiki/json-rpc/API#eth_gettransactionbyblockhashandindex
asio::awaitable<void> EthereumRpcApi::handle_eth_get_transaction_by_block_hash_and_index(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 2) {
        auto error_msg = "invalid eth_getTransactionByBlockHashAndIndex params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_hash = params[0].get<evmc::bytes32>();
    const auto index = params[1].get<std::string>();
    SILKRPC_DEBUG << "block_hash: " << block_hash << " index: " << index << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_with_hash = co_await core::read_block_by_hash(*context_.block_cache, tx_database, block_hash);
        const auto transactions = block_with_hash.block.transactions;

        const auto idx = std::stoul(index, 0, 16);
        if (idx >= transactions.size()) {
            SILKRPC_WARN << "Transaction not found for index: " << index << "\n";
            reply = make_json_content(request["id"], nullptr);
        } else {
            const auto block_header = block_with_hash.block.header;
            silkrpc::Transaction txn{transactions[idx], block_with_hash.hash, block_header.number, block_header.base_fee_per_gas, idx};
            reply = make_json_content(request["id"], txn);
        }
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

// https://eth.wiki/json-rpc/API#eth_getrawtransactionbyblockhashandindex
asio::awaitable<void> EthereumRpcApi::handle_eth_get_raw_transaction_by_block_hash_and_index(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 2) {
        auto error_msg = "invalid eth_getRawTransactionByBlockHashAndIndex params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_hash = params[0].get<evmc::bytes32>();
    const auto index = params[1].get<std::string>();
    SILKRPC_DEBUG << "block_hash: " << block_hash << " index: " << index << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_with_hash = co_await core::read_block_by_hash(*context_.block_cache, tx_database, block_hash);
        const auto transactions = block_with_hash.block.transactions;

        const auto idx = std::stoul(index, 0, 16);
        if (idx >= transactions.size()) {
            SILKRPC_WARN << "Transaction not found for index: " << index << "\n";
            reply = make_json_content(request["id"], nullptr);
        } else {
            silkworm::Bytes rlp{};
            silkworm::rlp::encode(rlp, transactions[idx]);
            reply = make_json_content(request["id"], rlp);
        }
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

// https://eth.wiki/json-rpc/API#eth_gettransactionbyblocknumberandindex
asio::awaitable<void> EthereumRpcApi::handle_eth_get_transaction_by_block_number_and_index(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 2) {
        auto error_msg = "invalid eth_getTransactionByBlockNumberAndIndex params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_id = params[0].get<std::string>();
    const auto index = params[1].get<std::string>();
    SILKRPC_DEBUG << "block_id: " << block_id << " index: " << index << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_number = co_await core::get_block_number(block_id, tx_database);
        const auto block_with_hash = co_await core::read_block_by_number(*context_.block_cache, tx_database, block_number);
        const auto transactions = block_with_hash.block.transactions;

        const auto idx = std::stoul(index, 0, 16);
        if (idx >= transactions.size()) {
            SILKRPC_WARN << "Transaction not found for index: " << index << "\n";
            reply = make_json_content(request["id"], nullptr);
        } else {
            const auto block_header = block_with_hash.block.header;
            silkrpc::Transaction txn{transactions[idx], block_with_hash.hash, block_header.number, block_header.base_fee_per_gas, idx};
            reply = make_json_content(request["id"], txn);
        }
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

// https://eth.wiki/json-rpc/API#eth_getrawtransactionbyblocknumberandindex
asio::awaitable<void> EthereumRpcApi::handle_eth_get_raw_transaction_by_block_number_and_index(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 2) {
        auto error_msg = "invalid eth_getRawTransactionByBlockNumberAndIndex params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_id = params[0].get<std::string>();
    const auto index = params[1].get<std::string>();
    SILKRPC_DEBUG << "block_id: " << block_id << " index: " << index << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_number = co_await core::get_block_number(block_id, tx_database);
        const auto block_with_hash = co_await core::read_block_by_number(*context_.block_cache, tx_database, block_number);
        const auto transactions = block_with_hash.block.transactions;

        const auto idx = std::stoul(index, 0, 16);
        if (idx >= transactions.size()) {
            SILKRPC_WARN << "Transaction not found for index: " << index << "\n";
            reply = make_json_content(request["id"], nullptr);
        } else {
            silkworm::Bytes rlp{};
            silkworm::rlp::encode(rlp, transactions[idx]);
            reply = make_json_content(request["id"], rlp);
        }
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

// https://eth.wiki/json-rpc/API#eth_gettransactionreceipt
asio::awaitable<void> EthereumRpcApi::handle_eth_get_transaction_receipt(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid eth_getTransactionReceipt params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    auto transaction_hash = params[0].get<evmc::bytes32>();
    SILKRPC_DEBUG << "transaction_hash: " << transaction_hash << "\n";
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};
        reply = make_json_content(request["id"], nullptr);
        const auto block_with_hash = co_await core::read_block_by_transaction_hash(*context_.block_cache, tx_database, transaction_hash);
        auto receipts = co_await core::get_receipts(tx_database, block_with_hash);
        auto transactions = block_with_hash.block.transactions;
        if (receipts.size() != transactions.size()) {
            throw std::invalid_argument{"Unexpected size for receipts in handle_eth_get_transaction_receipt"};
        }

        size_t tx_index = -1;
        for (size_t idx{0}; idx < transactions.size(); idx++) {
            auto ethash_hash{hash_of_transaction(transactions[idx])};

            SILKRPC_TRACE << "tx " << idx << ") hash: " << silkworm::to_bytes32({ethash_hash.bytes, silkworm::kHashLength}) << "\n";
            if (std::memcmp(transaction_hash.bytes, ethash_hash.bytes, silkworm::kHashLength) == 0) {
                tx_index = idx;
                break;
            }
        }

        if (tx_index == -1) {
            throw std::invalid_argument{"Unexpected transaction index in handle_eth_get_transaction_receipt"};
        }
        reply = make_json_content(request["id"], receipts[tx_index]);
    } catch (const std::invalid_argument& iv) {
        SILKRPC_DEBUG << "invalid_argument: " << iv.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_content(request["id"], {});
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

// https://eth.wiki/json-rpc/API#eth_estimategas
asio::awaitable<void> EthereumRpcApi::handle_eth_estimate_gas(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid eth_estimategas params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto call = params[0].get<Call>();
    SILKRPC_DEBUG << "call: " << call << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto chain_id = co_await core::rawdb::read_chain_id(tx_database);
        const auto chain_config_ptr = silkworm::lookup_chain_config(chain_id);
        auto latest_block_number = co_await core::get_block_number(silkrpc::core::kLatestBlockId, tx_database);
        SILKRPC_DEBUG << "chain_id: " << chain_id << ", latest_block_number: " << latest_block_number << "\n";

        const auto latest_block_with_hash = co_await core::read_block_by_number(*context_.block_cache, tx_database, latest_block_number);
        const auto latest_block = latest_block_with_hash.block;

        EVMExecutor evm_executor{context_, tx_database, *chain_config_ptr, workers_, latest_block.header.number};

        ego::Executor executor = [&latest_block, &evm_executor](const silkworm::Transaction &transaction) {
            return evm_executor.call(latest_block, transaction);
        };

        ego::BlockHeaderProvider block_header_provider = [&tx_database](uint64_t block_number) {
            return core::rawdb::read_header_by_number(tx_database, block_number);
        };

        StateReader state_reader{tx_database};
        ego::AccountReader account_reader = [&state_reader](const evmc::address& address, uint64_t block_number) {
            return state_reader.read_account(address, block_number + 1);
        };

        ego::EstimateGasOracle estimate_gas_oracle{block_header_provider, account_reader, executor};

        auto estimated_gas = co_await estimate_gas_oracle.estimate_gas(call, latest_block_number);

        reply = make_json_content(request["id"], to_quantity(estimated_gas));
    } catch (const ego::EstimateGasException& e) {
        SILKRPC_ERROR << "EstimateGasException: code: " << e.error_code() << " message: " << e.message() << " processing request: " << request.dump() << "\n";
        if (e.data().empty()) {
            reply = make_json_error(request["id"], e.error_code(), e.message());
        } else {
            reply = make_json_error(request["id"], {3, e.message(), e.data()});
        }
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

// https://eth.wiki/json-rpc/API#eth_getbalance
asio::awaitable<void> EthereumRpcApi::handle_eth_get_balance(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 2) {
        auto error_msg = "invalid eth_getBalance params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto address = params[0].get<evmc::address>();
    const auto block_id = params[1].get<std::string>();
    SILKRPC_DEBUG << "address: " << silkworm::to_hex(address) << " block_id: " << block_id << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};
        StateReader state_reader{tx_database};

        const auto block_number = co_await core::get_block_number(block_id, tx_database);
        std::optional<silkworm::Account> account{co_await state_reader.read_account(address, block_number + 1)};

        reply = make_json_content(request["id"], "0x" + (account ? intx::to_string(account->balance) : "0"));
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

// https://eth.wiki/json-rpc/API#eth_getcode
asio::awaitable<void> EthereumRpcApi::handle_eth_get_code(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 2) {
        auto error_msg = "invalid eth_getCode params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto address = params[0].get<evmc::address>();
    const auto block_id = params[1].get<std::string>();
    SILKRPC_DEBUG << "address: " << silkworm::to_hex(address) << " block_id: " << block_id << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};
        StateReader state_reader{tx_database};

        const auto block_number = co_await core::get_block_number(block_id, tx_database);
        std::optional<silkworm::Account> account{co_await state_reader.read_account(address, block_number + 1)};

        if (account) {
            auto code{co_await state_reader.read_code(account->code_hash)};
            reply = make_json_content(request["id"], code ? ("0x" + silkworm::to_hex(*code)) : "0x");
        } else {
            reply = make_json_content(request["id"], "0x");
        }
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

// https://eth.wiki/json-rpc/API#eth_gettransactioncount
asio::awaitable<void> EthereumRpcApi::handle_eth_get_transaction_count(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 2) {
        auto error_msg = "invalid eth_getTransactionCount params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto address = params[0].get<evmc::address>();
    const auto block_id = params[1].get<std::string>();
    SILKRPC_DEBUG << "address: " << silkworm::to_hex(address) << " block_id: " << block_id << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};
        StateReader state_reader{tx_database};
        const auto block_number = co_await core::get_block_number(block_id, tx_database);
        std::optional<silkworm::Account> account{co_await state_reader.read_account(address, block_number + 1)};

        if (account) {
            reply = make_json_content(request["id"], to_quantity(account->nonce));
        } else {
            reply = make_json_content(request["id"], "0x");
        }
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

// https://eth.wiki/json-rpc/API#eth_getstorageat
asio::awaitable<void> EthereumRpcApi::handle_eth_get_storage_at(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 3) {
        auto error_msg = "invalid eth_getStorageAt params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto address = params[0].get<evmc::address>();
    auto location = params[1].get<evmc::bytes32>();
    const auto block_id = params[2].get<std::string>();
    SILKRPC_DEBUG << "address: " << silkworm::to_hex(address) << " block_id: " << block_id << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};
        StateReader state_reader{tx_database};
        const auto block_number = co_await core::get_block_number(block_id, tx_database);
        std::optional<silkworm::Account> account{co_await state_reader.read_account(address, block_number + 1)};

        if (account) {
           auto storage{co_await state_reader.read_storage(address, account->incarnation, location, block_number + 1)};
           reply = make_json_content(request["id"], "0x" + silkworm::to_hex(storage));
        } else {
           reply = make_json_content(request["id"], "0x");
        }
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

// https://eth.wiki/json-rpc/API#eth_call
asio::awaitable<void> EthereumRpcApi::handle_eth_call(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 2) {
        auto error_msg = "invalid eth_call params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto call = params[0].get<Call>();
    const auto block_id = params[1].get<std::string>();
    SILKRPC_DEBUG << "call: " << call << " block_id: " << block_id << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto chain_id = co_await core::rawdb::read_chain_id(tx_database);
        const auto chain_config_ptr = silkworm::lookup_chain_config(chain_id);
        const auto block_number = co_await core::get_block_number(block_id, tx_database);

        EVMExecutor executor{context_, tx_database, *chain_config_ptr, workers_, block_number};
        const auto block_with_hash = co_await core::read_block_by_number(*context_.block_cache, tx_database, block_number);
        silkworm::Transaction txn{call.to_transaction()};
        const auto execution_result = co_await executor.call(block_with_hash.block, txn);

        if (execution_result.pre_check_error) {
            reply = make_json_error(request["id"], -32000, execution_result.pre_check_error.value());
        } else if (execution_result.error_code == evmc_status_code::EVMC_SUCCESS) {
            reply = make_json_content(request["id"], "0x" + silkworm::to_hex(execution_result.data));
        } else {
            const auto error_message = EVMExecutor<>::get_error_message(execution_result.error_code, execution_result.data);
            if (execution_result.data.empty()) {
                reply = make_json_error(request["id"], -32000, error_message);
            } else {
                reply = make_json_error(request["id"], {3, error_message, execution_result.data});
            }
        }
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

// https://eth.wiki/json-rpc/API#eth_newfilter
asio::awaitable<void> EthereumRpcApi::handle_eth_new_filter(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        reply = make_json_content(request["id"], to_quantity(0));
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

// https://eth.wiki/json-rpc/API#eth_newblockfilter
asio::awaitable<void> EthereumRpcApi::handle_eth_new_block_filter(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        reply = make_json_content(request["id"], to_quantity(0));
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

// https://eth.wiki/json-rpc/API#eth_newpendingtransactionfilter
asio::awaitable<void> EthereumRpcApi::handle_eth_new_pending_transaction_filter(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        reply = make_json_content(request["id"], to_quantity(0));
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

// https://eth.wiki/json-rpc/API#eth_getfilterchanges
asio::awaitable<void> EthereumRpcApi::handle_eth_get_filter_changes(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        reply = make_json_content(request["id"], to_quantity(0));
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

// https://eth.wiki/json-rpc/API#eth_uninstallfilter
asio::awaitable<void> EthereumRpcApi::handle_eth_uninstall_filter(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        reply = make_json_content(request["id"], to_quantity(0));
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

// https://eth.wiki/json-rpc/API#eth_getlogs
asio::awaitable<void> EthereumRpcApi::handle_eth_get_logs(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid eth_getLogs params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    auto filter = params[0].get<Filter>();
    SILKRPC_DEBUG << "filter: " << filter << "\n";

    std::vector<Log> logs;

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        uint64_t start{}, end{};
        if (filter.block_hash.has_value()) {
            auto block_hash_bytes = silkworm::from_hex(filter.block_hash.value());
            if (!block_hash_bytes.has_value()) {
                auto error_msg = "invalid eth_getLogs filter block_hash: " + filter.block_hash.value();
                SILKRPC_ERROR << error_msg << "\n";
                reply = make_json_error(request["id"], 100, error_msg);
                co_await tx->close(); // RAII not (yet) available with coroutines
                co_return;
            }
            auto block_hash = silkworm::to_bytes32(block_hash_bytes.value());
            auto block_number = co_await core::rawdb::read_header_number(tx_database, block_hash);
            start = end = block_number;
        } else {
            auto latest_block_number = co_await core::get_latest_block_number(tx_database);
            start = filter.from_block.value_or(0);
            end = filter.to_block.value_or(latest_block_number);
        }
        SILKRPC_INFO << "start block: " << start << " end block: " << end << "\n";

        roaring::Roaring block_numbers;
        block_numbers.addRange(start, end + 1); // [min, max)

        SILKRPC_DEBUG << "block_numbers.cardinality(): " << block_numbers.cardinality() << "\n";

        if (filter.topics.has_value()) {
            auto topics_bitmap = co_await get_topics_bitmap(tx_database, filter.topics.value(), start, end);
            SILKRPC_TRACE << "topics_bitmap: " << topics_bitmap.toString() << "\n";
            if (topics_bitmap.isEmpty()) {
                block_numbers = topics_bitmap;
            } else {
                block_numbers &= topics_bitmap;
            }
        }
        SILKRPC_DEBUG << "block_numbers.cardinality(): " << block_numbers.cardinality() << "\n";
        SILKRPC_TRACE << "block_numbers: " << block_numbers.toString() << "\n";

        if (filter.addresses.has_value()) {
            auto addresses_bitmap = co_await get_addresses_bitmap(tx_database, filter.addresses.value(), start, end);
            if (addresses_bitmap.isEmpty()) {
                block_numbers = addresses_bitmap;
            } else {
                block_numbers &= addresses_bitmap;
            }
        }
        SILKRPC_DEBUG << "block_numbers.cardinality(): " << block_numbers.cardinality() << "\n";
        SILKRPC_TRACE << "block_numbers: " << block_numbers.toString() << "\n";

        if (block_numbers.cardinality() == 0) {
            reply = make_json_content(request["id"], logs);
            co_await tx->close(); // RAII not (yet) available with coroutines
            co_return;
        }

        for (auto block_to_match : block_numbers) {
            uint64_t log_index{0};

            Logs filtered_block_logs{};
            const auto block_key = silkworm::db::block_key(block_to_match);
            SILKRPC_TRACE << "block_to_match: " << block_to_match << " block_key: " << silkworm::to_hex(block_key) << "\n";
            co_await tx_database.for_prefix(silkrpc::db::table::kLogs, block_key, [&](const silkworm::Bytes& k, const silkworm::Bytes& v) {
                Logs chunck_logs{};
                const bool decoding_ok{cbor_decode(v, chunck_logs)};
                if (!decoding_ok) {
                    return false;
                }
                for (auto& log : chunck_logs) {
                    log.index = log_index++;
                }
                SILKRPC_DEBUG << "chunck_logs.size(): " << chunck_logs.size() << "\n";
                auto filtered_chunck_logs = filter_logs(chunck_logs, filter);
                SILKRPC_DEBUG << "filtered_chunck_logs.size(): " << filtered_chunck_logs.size() << "\n";
                if (filtered_chunck_logs.size() > 0) {
                    const auto tx_id = boost::endian::load_big_u32(&k[sizeof(uint64_t)]);
                    SILKRPC_DEBUG << "tx_id: " << tx_id << "\n";
                    for (auto& log : filtered_chunck_logs) {
                        log.tx_index = tx_id;
                    }
                    filtered_block_logs.insert(filtered_block_logs.end(), filtered_chunck_logs.begin(), filtered_chunck_logs.end());
                }
                return true;
            });
            SILKRPC_DEBUG << "filtered_block_logs.size(): " << filtered_block_logs.size() << "\n";

            if (filtered_block_logs.size() > 0) {
                const auto block_with_hash = co_await core::read_block_by_number(*context_.block_cache, tx_database, block_to_match);
                SILKRPC_DEBUG << "block_hash: " << silkworm::to_hex(block_with_hash.hash) << "\n";
                for (auto& log : filtered_block_logs) {
                    const auto tx_hash{hash_of_transaction(block_with_hash.block.transactions[log.tx_index])};
                    log.block_number = block_to_match;
                    log.block_hash = block_with_hash.hash;
                    log.tx_hash = silkworm::to_bytes32({tx_hash.bytes, silkworm::kHashLength});
                }
                logs.insert(logs.end(), filtered_block_logs.begin(), filtered_block_logs.end());
            }
        }
        SILKRPC_INFO << "logs.size(): " << logs.size() << "\n";

        reply = make_json_content(request["id"], logs);
    } catch (const std::invalid_argument& iv) {
        SILKRPC_DEBUG << "invalid_argument: " << iv.what() << " processing request: " << request.dump() << "\n";
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

// https://eth.wiki/json-rpc/API#eth_sendrawtransaction
asio::awaitable<void> EthereumRpcApi::handle_eth_send_raw_transaction(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid eth_sendRawTransaction params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto encoded_tx_string = params[0].get<std::string>();
    const auto encoded_tx_bytes = silkworm::from_hex(encoded_tx_string);
    if (!encoded_tx_bytes.has_value()) {
        auto error_msg = "invalid eth_sendRawTransaction encoded tx: " + encoded_tx_string;
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], -32602, error_msg);
        co_return;
    }

    silkworm::ByteView encoded_tx_view{*encoded_tx_bytes};
    Transaction txn;
    auto err{silkworm::rlp::decode<silkworm::Transaction>(encoded_tx_view, txn)};
    if (err != silkworm::DecodingResult::kOk) {
        auto error_msg = decoding_result_to_string(err);
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], -32000, error_msg);
        co_return;
    }

    const float kTxFeeCap = 1; // 1 ether

    if (!check_tx_fee_less_cap(kTxFeeCap, txn.max_fee_per_gas, txn.gas_limit)) {
        auto error_msg = "tx fee exceeds the configured cap";
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], -32000, error_msg);
        co_return;
    }

    if (!is_replay_protected(txn)) {
        auto error_msg = "only replay-protected (EIP-155) transactions allowed over RPC";
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], -32000, error_msg);
        co_return;
    }

    silkworm::ByteView encoded_tx{*encoded_tx_bytes};
    const auto result = co_await tx_pool_->add_transaction(encoded_tx);
    if (!result.success) {
        SILKRPC_ERROR << "cannot add transaction: " << result.error_descr << "\n";
        reply = make_json_error(request["id"], -32000, result.error_descr);
        co_return;
    }

    txn.recover_sender();
    if (!txn.from.has_value()) {
        auto error_msg = "cannot recover sender";
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], -32000, error_msg);
        co_return;
    }

    auto ethash_hash{hash_of_transaction(txn)};
    auto hash = silkworm::to_bytes32({ethash_hash.bytes, silkworm::kHashLength});
    if (!txn.to.has_value()) {
        auto contract_address = silkworm::create_address(*txn.from, txn.nonce);
        SILKRPC_DEBUG << "submitted contract creation hash: " << hash << " from: " << *txn.from <<  " nonce: " << txn.nonce << " contract: " << contract_address <<
                         " value: " << txn.value << "\n";
    } else {
        SILKRPC_DEBUG << "submitted transaction hash: " << hash << " from: " << *txn.from <<  " nonce: " << txn.nonce << " recipient: " << *txn.to << " value: " << txn.value << "\n";
    }

    reply = make_json_content(request["id"], hash);

    co_return;
}

// https://eth.wiki/json-rpc/API#eth_sendtransaction
asio::awaitable<void> EthereumRpcApi::handle_eth_send_transaction(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        reply = make_json_content(request["id"], to_quantity(0));
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

// https://eth.wiki/json-rpc/API#eth_signtransaction
asio::awaitable<void> EthereumRpcApi::handle_eth_sign_transaction(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        reply = make_json_content(request["id"], to_quantity(0));
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

// https://eth.wiki/json-rpc/API#eth_getproof
asio::awaitable<void> EthereumRpcApi::handle_eth_get_proof(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        reply = make_json_content(request["id"], to_quantity(0));
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

// https://eth.wiki/json-rpc/API#eth_mining
asio::awaitable<void> EthereumRpcApi::handle_eth_mining(const nlohmann::json& request, nlohmann::json& reply) {
    try {
        const auto mining_result = co_await miner_->get_mining();
        reply = make_json_content(request["id"], mining_result.enabled && mining_result.running);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], -32000, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_return;
}

// https://eth.wiki/json-rpc/API#eth_coinbase
asio::awaitable<void> EthereumRpcApi::handle_eth_coinbase(const nlohmann::json& request, nlohmann::json& reply) {
    try {
        const auto coinbase_address = co_await backend_->etherbase();
        reply = make_json_content(request["id"], coinbase_address);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], -32000, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_return;
}

// https://eth.wiki/json-rpc/API#eth_hashrate
asio::awaitable<void> EthereumRpcApi::handle_eth_hashrate(const nlohmann::json& request, nlohmann::json& reply) {
    try {
        const auto hash_rate = co_await miner_->get_hash_rate();
        reply = make_json_content(request["id"], to_quantity(hash_rate));
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], -32000, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_return;
}

// https://eth.wiki/json-rpc/API#eth_submithashrate
asio::awaitable<void> EthereumRpcApi::handle_eth_submit_hashrate(const nlohmann::json& request, nlohmann::json& reply) {
    const auto params = request["params"];
    if (params.size() != 2) {
        const auto error_msg = "invalid eth_submitHashrate params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }

    try {
        const auto hash_rate = params[0].get<intx::uint256>();
        const auto id = params[1].get<evmc::bytes32>();
        const auto success = co_await miner_->submit_hash_rate(hash_rate, id);
        reply = make_json_content(request["id"], success);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], -32000, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_return;
}

// https://eth.wiki/json-rpc/API#eth_getwork
asio::awaitable<void> EthereumRpcApi::handle_eth_get_work(const nlohmann::json& request, nlohmann::json& reply) {
    try {
        const auto work = co_await miner_->get_work();
        const std::vector<std::string> current_work{
            silkworm::to_hex(work.header_hash),
            silkworm::to_hex(work.seed_hash),
            silkworm::to_hex(work.target),
            silkworm::to_hex(work.block_number)
        };
        reply = make_json_content(request["id"], current_work);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], -32000, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_return;
}

// https://eth.wiki/json-rpc/API#eth_submitwork
asio::awaitable<void> EthereumRpcApi::handle_eth_submit_work(const nlohmann::json& request, nlohmann::json& reply) {
    const auto params = request["params"];
    if (params.size() != 3) {
        const auto error_msg = "invalid eth_submitWork params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }

    try {
        const auto block_nonce = silkworm::from_hex(params[0].get<std::string>());
        if (!block_nonce.has_value()) {
            const auto error_msg = "invalid eth_submitWork params: " + params.dump();
            SILKRPC_ERROR << error_msg << "\n";
            reply = make_json_error(request["id"], 100, error_msg);
            co_return;
        }
        const auto pow_hash = params[1].get<evmc::bytes32>();
        const auto digest = params[2].get<evmc::bytes32>();
        const auto success = co_await miner_->submit_work(block_nonce.value(), pow_hash, digest);
        reply = make_json_content(request["id"], success);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], -32000, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_return;
}

// https://eth.wiki/json-rpc/API#eth_subscribe
asio::awaitable<void> EthereumRpcApi::handle_eth_subscribe(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        reply = make_json_content(request["id"], to_quantity(0));
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

// https://eth.wiki/json-rpc/API#eth_unsubscribe
asio::awaitable<void> EthereumRpcApi::handle_eth_unsubscribe(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        reply = make_json_content(request["id"], to_quantity(0));
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

asio::awaitable<roaring::Roaring> EthereumRpcApi::get_topics_bitmap(core::rawdb::DatabaseReader& db_reader, FilterTopics& topics, uint64_t start, uint64_t end) {
    SILKRPC_DEBUG << "#topics: " << topics.size() << " start: " << start << " end: " << end << "\n";
    roaring::Roaring result_bitmap;
    for (auto subtopics : topics) {
        SILKRPC_DEBUG << "#subtopics: " << subtopics.size() << "\n";
        roaring::Roaring subtopic_bitmap;
        for (auto topic : subtopics) {
            silkworm::Bytes topic_key{std::begin(topic.bytes), std::end(topic.bytes)};
            SILKRPC_TRACE << "topic: " << topic << " topic_key: " << silkworm::to_hex(topic) <<"\n";
            auto bitmap = co_await ethdb::bitmap::get(db_reader, silkrpc::db::table::kLogTopicIndex, topic_key, start, end);
            SILKRPC_TRACE << "bitmap: " << bitmap.toString() << "\n";
            subtopic_bitmap |= bitmap;
            SILKRPC_TRACE << "subtopic_bitmap: " << subtopic_bitmap.toString() << "\n";
        }
        if (!subtopic_bitmap.isEmpty()) {
            if (result_bitmap.isEmpty()) {
                result_bitmap = subtopic_bitmap;
            } else {
                result_bitmap &= subtopic_bitmap;
            }
        }
        SILKRPC_DEBUG << "result_bitmap: " << result_bitmap.toString() << "\n";
    }
    co_return result_bitmap;
}

asio::awaitable<roaring::Roaring> EthereumRpcApi::get_addresses_bitmap(core::rawdb::DatabaseReader& db_reader, FilterAddresses& addresses, uint64_t start, uint64_t end) {
    SILKRPC_TRACE << "#addresses: " << addresses.size() << " start: " << start << " end: " << end << "\n";
    roaring::Roaring result_bitmap;
    for (auto address : addresses) {
        silkworm::Bytes address_key{std::begin(address.bytes), std::end(address.bytes)};
        auto bitmap = co_await ethdb::bitmap::get(db_reader, silkrpc::db::table::kLogAddressIndex, address_key, start, end);
        SILKRPC_TRACE << "bitmap: " << bitmap.toString() << "\n";
        result_bitmap |= bitmap;
    }
    SILKRPC_TRACE << "result_bitmap: " << result_bitmap.toString() << "\n";
    co_return result_bitmap;
}

std::vector<Log> EthereumRpcApi::filter_logs(std::vector<Log>& logs, const Filter& filter) {
    std::vector<Log> filtered_logs;

    auto addresses = filter.addresses;
    auto topics = filter.topics;
    SILKRPC_DEBUG << "filter.addresses: " << filter.addresses << "\n";
    for (auto log : logs) {
        SILKRPC_DEBUG << "log: " << log << "\n";
        if (addresses.has_value() && std::find(addresses.value().begin(), addresses.value().end(), log.address) == addresses.value().end()) {
            SILKRPC_DEBUG << "skipped log for address: 0x" << silkworm::to_hex(log.address) << "\n";
            continue;
        }
        auto matches = true;
        if (topics.has_value()) {
            if (topics.value().size() > log.topics.size()) {
                SILKRPC_DEBUG << "#topics: " << topics.value().size() << " #log.topics: " << log.topics.size() << "\n";
                continue;
            }
            for (size_t i{0}; i < topics.value().size(); i++) {
                SILKRPC_DEBUG << "log.topics[i]: " << log.topics[i] << "\n";
                auto subtopics = topics.value()[i];
                auto matches_subtopics = subtopics.empty(); // empty rule set == wildcard
                SILKRPC_TRACE << "matches_subtopics: " << std::boolalpha << matches_subtopics << "\n";
                for (auto topic : subtopics) {
                    SILKRPC_DEBUG << "topic: " << topic << "\n";
                    if (log.topics[i] == topic) {
                        matches_subtopics = true;
                        SILKRPC_TRACE << "matches_subtopics: " << matches_subtopics << "\n";
                        break;
                    }
                }
                if (!matches_subtopics) {
                    SILKRPC_TRACE << "No subtopic matches\n";
                    matches = false;
                    break;
                }
            }
        }
        SILKRPC_DEBUG << "matches: " << matches << "\n";
        if (matches) {
            filtered_logs.push_back(log);
        }
    }
    return filtered_logs;
}

} // namespace silkrpc::commands
