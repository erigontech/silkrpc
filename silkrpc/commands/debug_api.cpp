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

#include "debug_api.hpp"

#include <set>
#include <string>
#include <sstream>
#include <vector>

#include <silkworm/common/util.hpp>
#include <silkworm/node/silkworm/db/util.hpp>

#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/core/account_dumper.hpp>
#include <silkrpc/core/account_walker.hpp>
#include <silkrpc/core/blocks.hpp>
#include <silkrpc/core/rawdb/chain.hpp>
#include <silkrpc/core/state_reader.hpp>
#include <silkrpc/ethdb/tables.hpp>
#include <silkrpc/ethdb/transaction_database.hpp>
#include <silkrpc/json/types.hpp>
#include <silkrpc/types/block.hpp>
#include <silkrpc/types/dump_account.hpp>

namespace silkrpc::commands {

// https://github.com/ethereum/retesteth/wiki/RPC-Methods#debug_accountrange
asio::awaitable<void> DebugRpcApi::handle_debug_account_range(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 5) {
        auto error_msg = "invalid debug_accountRange params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_number_or_hash = params[0].get<BlockNumberOrHash>();
    const auto start_key_array = params[1].get<std::vector<std::uint8_t>>();
    auto max_result = params[2].get<int16_t>();
    const auto exclude_code = params[3].get<bool>();
    const auto exclude_storage = params[4].get<bool>();

    silkworm::Bytes start_key(start_key_array.data(), start_key_array.size());
    const auto start_address = silkworm::to_address(start_key);

    if (max_result > kAccountRangeMaxResults || max_result <= 0) {
        max_result = kAccountRangeMaxResults;
    }

    SILKRPC_INFO << "block_number_or_hash: " << block_number_or_hash
        << " start_address: 0x" << silkworm::to_hex(start_address)
        << " max_result: " << max_result
        << " exclude_code: " << exclude_code
        << " exclude_storage: " << exclude_storage
        << "\n";

    auto tx = co_await database_->begin();

    try {
        AccountDumper dumper{*tx};
        DumpAccounts dump_accounts = co_await dumper.dump_accounts(block_number_or_hash, start_address, max_result, exclude_code, exclude_storage);

        reply = make_json_content(request["id"], dump_accounts);
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

// https://github.com/ethereum/retesteth/wiki/RPC-Methods#debug_getmodifiedaccountsbynumber
asio::awaitable<void> DebugRpcApi::handle_debug_get_modified_accounts_by_number(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() == 0 || params.size() > 2) {
        auto error_msg = "invalid debug_getModifiedAccountsByNumber params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }

    auto start_block_id = params[0].get<std::string>();
    auto end_block_id = start_block_id;
    if (params.size() == 2) {
       end_block_id = params[1].get<std::string>();
    }
    SILKRPC_DEBUG << "start_block_id: " << start_block_id << " end_block_id: " << end_block_id << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};
        auto start_block_number = co_await silkrpc::core::get_block_number(start_block_id, tx_database);
        auto end_block_number = co_await silkrpc::core::get_block_number(end_block_id, tx_database);
        auto last_block_number = co_await silkrpc::core::get_block_number(silkrpc::core::kLatestBlockId, tx_database);

        SILKRPC_INFO << "start_block_number: " << start_block_number << " end_block_number: " << end_block_number << " last_block_number: " << last_block_number << "\n";

        if (start_block_number > last_block_number) {
            std::stringstream msg;
            msg << "start block " << start_block_number << " is later than the latest block";
            reply = make_json_error(request["id"], 100, msg.str());
        } else if (start_block_number > end_block_number) {
            std::stringstream msg;
            msg << "end block " << start_block_number << " must be less than or equal to end block " << end_block_number;
            reply = make_json_error(request["id"], 100, msg.str());
        } else {
            std::set<silkworm::Bytes> addresses;
            core::rawdb::Walker walker = [&](const silkworm::Bytes& key, const silkworm::Bytes& value) {
                auto block_number = std::stol(silkworm::to_hex(key), 0, 16);
                if (block_number <= end_block_number) {
                    auto address = value.substr(0, silkworm::kAddressLength);

                    SILKRPC_TRACE << "Walker: processing block " << block_number << " address 0x" << silkworm::to_hex(address) << "\n";
                    addresses.insert(address);
                }
                return block_number <= end_block_number;
            };

            const auto key = silkworm::db::block_key(start_block_number);
            SILKRPC_TRACE << "Ready to walk starting from key: " << silkworm::to_hex(key) << "\n";

            co_await tx_database.walk(db::table::kPlainAccountChangeSet, key, 0, walker);
            nlohmann::json result;
            for (auto item : addresses) {
                auto address = "0x" + silkworm::to_hex(item);
                result.push_back(address);
            }
            reply = make_json_content(request["id"], result);
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

// https://github.com/ethereum/retesteth/wiki/RPC-Methods#debug_getmodifiedaccountsbyhash
asio::awaitable<void> DebugRpcApi::handle_debug_get_modified_accounts_by_hash(const nlohmann::json& request, nlohmann::json& reply) {
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

// https://github.com/ethereum/retesteth/wiki/RPC-Methods#debug_storagerangeat
asio::awaitable<void> DebugRpcApi::handle_debug_storage_range_at(const nlohmann::json& request, nlohmann::json& reply) {
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

// https://github.com/ethereum/retesteth/wiki/RPC-Methods#debug_tracetransaction
asio::awaitable<void> DebugRpcApi::handle_debug_trace_transaction(const nlohmann::json& request, nlohmann::json& reply) {
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

// https://github.com/ethereum/retesteth/wiki/RPC-Methods#debug_tracecall
asio::awaitable<void> DebugRpcApi::handle_debug_trace_call(const nlohmann::json& request, nlohmann::json& reply) {
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

} // namespace silkrpc::commands
