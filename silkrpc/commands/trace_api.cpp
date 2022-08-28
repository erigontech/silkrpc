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

#include "trace_api.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include <silkworm/common/util.hpp>

#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/core/blocks.hpp>
#include <silkrpc/core/cached_chain.hpp>
#include <silkrpc/core/evm_trace.hpp>
#include <silkrpc/ethdb/kv/cached_database.hpp>
#include <silkrpc/ethdb/transaction_database.hpp>
#include <silkrpc/json/types.hpp>
#include <silkrpc/types/call.hpp>

namespace silkrpc::commands {

// https://eth.wiki/json-rpc/API#trace_call
boost::asio::awaitable<void> TraceRpcApi::handle_trace_call(const nlohmann::json& request, nlohmann::json& reply) {
    const auto params = request["params"];
    if (params.size() < 3) {
        auto error_msg = "invalid trace_call params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }

    const auto call = params[0].get<Call>();
    const auto config = params[1].get<trace::TraceConfig>();
    const auto block_number_or_hash = params[2].get<BlockNumberOrHash>();

    SILKRPC_INFO << "call: " << call << " block_number_or_hash: " << block_number_or_hash << " config: " << config << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::kv::CachedDatabase tx_database{block_number_or_hash, *tx, *context_.state_cache()};

        const auto block_with_hash = co_await core::read_block_by_number_or_hash(*context_.block_cache(), tx_database, block_number_or_hash);

        trace::TraceCallExecutor executor{*context_.io_context(), tx_database, workers_};
        const auto result = co_await executor.trace_call(block_with_hash.block, call, config);

        if (result.pre_check_error) {
            reply = make_json_error(request["id"], -32000, result.pre_check_error.value());
        } else {
            reply = make_json_content(request["id"], result.traces);
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

// https://eth.wiki/json-rpc/API#trace_callmany
boost::asio::awaitable<void> TraceRpcApi::handle_trace_call_many(const nlohmann::json& request, nlohmann::json& reply) {
    const auto params = request["params"];
    if (params.size() < 2) {
        auto error_msg = "invalid trace_callMany params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto trace_calls = params[0].get<std::vector<trace::TraceCall>>();
    const auto block_number_or_hash = params[1].get<BlockNumberOrHash>();

    SILKRPC_INFO << "#trace_calls: " << trace_calls.size() << " block_number_or_hash: " << block_number_or_hash << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::kv::CachedDatabase tx_database{block_number_or_hash, *tx, *context_.state_cache()};

        const auto block_with_hash = co_await core::read_block_by_number_or_hash(*context_.block_cache(), tx_database, block_number_or_hash);

        trace::TraceCallExecutor executor{*context_.io_context(), tx_database, workers_};
        const auto result = co_await executor.trace_calls(block_with_hash.block, trace_calls);

        if (result.pre_check_error) {
            reply = make_json_error(request["id"], -32000, result.pre_check_error.value());
        } else {
            reply = make_json_content(request["id"], result.traces);
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

// https://eth.wiki/json-rpc/API#trace_rawtransaction
boost::asio::awaitable<void> TraceRpcApi::handle_trace_raw_transaction(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        reply = make_json_error(request["id"], 500, "not yet implemented");
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

// https://eth.wiki/json-rpc/API#trace_replayblocktransactions
boost::asio::awaitable<void> TraceRpcApi::handle_trace_replay_block_transactions(const nlohmann::json& request, nlohmann::json& reply) {
    const auto params = request["params"];
    if (params.size() < 2) {
        auto error_msg = "invalid trace_replayBlockTransactions params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_number_or_hash = params[0].get<BlockNumberOrHash>();
    const auto config = params[1].get<trace::TraceConfig>();

    SILKRPC_INFO << " block_number_or_hash: " << block_number_or_hash << " config: " << config << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_with_hash = co_await core::read_block_by_number_or_hash(*context_.block_cache(), tx_database, block_number_or_hash);

        trace::TraceCallExecutor executor{*context_.io_context(), tx_database, workers_};
        const auto result = co_await executor.trace_block_transactions(block_with_hash.block, config);
        reply = make_json_content(request["id"], result);
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

// https://eth.wiki/json-rpc/API#trace_replaytransaction
boost::asio::awaitable<void> TraceRpcApi::handle_trace_replay_transaction(const nlohmann::json& request, nlohmann::json& reply) {
    const auto params = request["params"];
    if (params.size() < 2) {
        auto error_msg = "invalid trace_replayTransaction params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto transaction_hash = params[0].get<evmc::bytes32>();
    const auto config = params[1].get<trace::TraceConfig>();

    SILKRPC_INFO << "transaction_hash: " << transaction_hash << " config: " << config << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};
        const auto tx_with_block = co_await core::read_transaction_by_hash(*context_.block_cache(), tx_database, transaction_hash);
        if (!tx_with_block) {
            std::ostringstream oss;
            oss << "transaction 0x" << transaction_hash << " not found";
            reply = make_json_error(request["id"], -32000, oss.str());
        } else {
            trace::TraceCallExecutor executor{*context_.io_context(), tx_database, workers_};
            const auto result = co_await executor.trace_transaction(tx_with_block->block_with_hash.block, tx_with_block->transaction, config);

            if (result.pre_check_error) {
                reply = make_json_error(request["id"], -32000, result.pre_check_error.value());
            } else {
                reply = make_json_content(request["id"], result.traces);
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

// https://eth.wiki/json-rpc/API#trace_block
boost::asio::awaitable<void> TraceRpcApi::handle_trace_block(const nlohmann::json& request, nlohmann::json& reply) {
    const auto params = request["params"];
    if (params.size() < 1) {
        auto error_msg = "invalid trace_block params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_number_or_hash = params[0].get<BlockNumberOrHash>();

    SILKRPC_INFO << " block_number_or_hash: " << block_number_or_hash;

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_with_hash = co_await core::read_block_by_number_or_hash(*context_.block_cache(), tx_database, block_number_or_hash);

        trace::TraceCallExecutor executor{*context_.io_context(), tx_database, workers_};
        const auto result = co_await executor.trace_block(block_with_hash);
        reply = make_json_content(request["id"], result);
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

// https://eth.wiki/json-rpc/API#trace_filter
boost::asio::awaitable<void> TraceRpcApi::handle_trace_filter(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        reply = make_json_error(request["id"], 500, "not yet implemented");
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

// https://eth.wiki/json-rpc/API#trace_get
boost::asio::awaitable<void> TraceRpcApi::handle_trace_get(const nlohmann::json& request, nlohmann::json& reply) {
    const auto params = request["params"];
    if (params.size() < 2) {
        auto error_msg = "invalid trace_get params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto transaction_hash = params[0].get<evmc::bytes32>();
    const auto str_indices = params[1].get<std::vector<std::string>>();

    std::vector<std::uint16_t> indices;
    std::transform(str_indices.begin(), str_indices.end(), std::back_inserter(indices),
               [](const std::string& str) { return std::stoi(str, nullptr, 16); });
    SILKRPC_INFO << "transaction_hash: " << transaction_hash << ", #indices: " << indices.size() << "\n";

    // TODO(sixtysixter) for RPCDAEMON compatibility
    // Parity fails if it gets more than a single index. It returns nothing in this case. Must we?
    if (indices.size() > 1) {
        reply = make_json_content(request["id"]);
        co_return;
    }

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto tx_with_block = co_await core::read_transaction_by_hash(*context_.block_cache(), tx_database, transaction_hash);
        if (!tx_with_block) {
            reply = make_json_content(request["id"]);
        } else {
            trace::TraceCallExecutor executor{*context_.io_context(), tx_database, workers_};
            const auto result = co_await executor.trace_transaction(tx_with_block->block_with_hash, tx_with_block->transaction);

            // TODO(sixtysixter) for RPCDAEMON compatibility
            auto index = indices[0] + 1;
            if (result.size() > index) {
                reply = make_json_content(request["id"], result[index]);
            } else {
                reply = make_json_content(request["id"]);
            }
        }
    } catch (const std::exception& e) {
        reply = make_json_content(request["id"]);
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_await tx->close(); // RAII not (yet) available with coroutines
    co_return;
}

// https://eth.wiki/json-rpc/API#trace_transaction
boost::asio::awaitable<void> TraceRpcApi::handle_trace_transaction(const nlohmann::json& request, nlohmann::json& reply) {
    const auto params = request["params"];
    if (params.size() < 1) {
        auto error_msg = "invalid trace_transaction params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto transaction_hash = params[0].get<evmc::bytes32>();

    SILKRPC_INFO << "transaction_hash: " << transaction_hash << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};
        const auto tx_with_block = co_await core::read_transaction_by_hash(*context_.block_cache(), tx_database, transaction_hash);
        if (!tx_with_block) {
            reply = make_json_content(request["id"]);
        } else {
            trace::TraceCallExecutor executor{*context_.io_context(), tx_database, workers_};
            auto result = co_await executor.trace_transaction(tx_with_block->block_with_hash, tx_with_block->transaction);
            reply = make_json_content(request["id"], result);
        }
    } catch (const std::exception& e) {
        reply = make_json_content(request["id"]);
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_await tx->close(); // RAII not (yet) available with coroutines
    co_return;
}

} // namespace silkrpc::commands
