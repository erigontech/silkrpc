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

#include <string>

#include <silkworm/common/util.hpp>

#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/ethdb/kv/transaction_database.hpp>
#include <silkrpc/json/types.hpp>

namespace silkrpc::commands {

// https://eth.wiki/json-rpc/API#trace_call
asio::awaitable<void> TraceRpcApi::handle_trace_call(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::kv::TransactionDatabase tx_database{*tx};

        reply = make_json_error(request["id"], 500, "not yet implemented");
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

// https://eth.wiki/json-rpc/API#trace_callmany
asio::awaitable<void> TraceRpcApi::handle_trace_call_many(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::kv::TransactionDatabase tx_database{*tx};

        reply = make_json_error(request["id"], 500, "not yet implemented");
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

// https://eth.wiki/json-rpc/API#trace_rawtransaction
asio::awaitable<void> TraceRpcApi::handle_trace_raw_transaction(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::kv::TransactionDatabase tx_database{*tx};

        reply = make_json_error(request["id"], 500, "not yet implemented");
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

// https://eth.wiki/json-rpc/API#trace_replayblocktransactions
asio::awaitable<void> TraceRpcApi::handle_trace_replay_block_transactions(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::kv::TransactionDatabase tx_database{*tx};

        reply = make_json_error(request["id"], 500, "not yet implemented");
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

// https://eth.wiki/json-rpc/API#trace_replaytransaction
asio::awaitable<void> TraceRpcApi::handle_trace_replay_transaction(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::kv::TransactionDatabase tx_database{*tx};

        reply = make_json_error(request["id"], 500, "not yet implemented");
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

// https://eth.wiki/json-rpc/API#trace_block
asio::awaitable<void> TraceRpcApi::handle_trace_block(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::kv::TransactionDatabase tx_database{*tx};

        reply = make_json_error(request["id"], 500, "not yet implemented");
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

// https://eth.wiki/json-rpc/API#trace_filter
asio::awaitable<void> TraceRpcApi::handle_trace_filter(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::kv::TransactionDatabase tx_database{*tx};

        reply = make_json_error(request["id"], 500, "not yet implemented");
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

// https://eth.wiki/json-rpc/API#trace_get
asio::awaitable<void> TraceRpcApi::handle_trace_get(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::kv::TransactionDatabase tx_database{*tx};

        reply = make_json_error(request["id"], 500, "not yet implemented");
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

// https://eth.wiki/json-rpc/API#trace_transaction
asio::awaitable<void> TraceRpcApi::handle_trace_transaction(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = co_await database_->begin();

    try {
        ethdb::kv::TransactionDatabase tx_database{*tx};

        reply = make_json_error(request["id"], 500, "not yet implemented");
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
