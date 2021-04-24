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

#ifndef SILKRPC_COMMANDS_ETH_API_HPP_
#define SILKRPC_COMMANDS_ETH_API_HPP_

#include <memory>
#include <vector>

#include <silkrpc/config.hpp> // NOLINT(build/include_order)

#include <asio/awaitable.hpp>
#include <evmc/evmc.hpp>
#include <nlohmann/json.hpp>

#include <silkworm/types/receipt.hpp>
#include <silkrpc/core/rawdb/accessors.hpp>
#include <silkrpc/croaring/roaring.hh> // NOLINT(build/include_order)
#include <silkrpc/json/types.hpp>
#include <silkrpc/ethdb/kv/database.hpp>
#include <silkrpc/ethdb/kv/transaction.hpp>
#include <silkrpc/ethdb/kv/transaction_database.hpp>
#include <silkrpc/types/log.hpp>
#include <silkrpc/types/receipt.hpp>

namespace silkrpc::http { class RequestHandler; }

namespace silkrpc::commands {

class EthereumRpcApi {
public:
    explicit EthereumRpcApi(std::unique_ptr<ethdb::kv::Database>& database) : database_(database) {}
    virtual ~EthereumRpcApi() {}

    EthereumRpcApi(const EthereumRpcApi&) = delete;
    EthereumRpcApi& operator=(const EthereumRpcApi&) = delete;

protected:
    asio::awaitable<void> handle_eth_block_number(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_eth_call(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_eth_chain_id(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_eth_protocol_version(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_eth_syncing(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_eth_get_block_by_hash(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_eth_get_block_by_number(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_eth_get_block_transaction_count_by_number(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_eth_get_logs(const nlohmann::json& request, nlohmann::json& reply);

    asio::awaitable<Roaring> get_topics_bitmap(core::rawdb::DatabaseReader& db_reader, FilterTopics& topics, uint64_t start, uint64_t end);
    asio::awaitable<Roaring> get_addresses_bitmap(core::rawdb::DatabaseReader& db_reader, FilterAddresses& addresses, uint64_t start, uint64_t end);
    asio::awaitable<Receipts> get_receipts(core::rawdb::DatabaseReader& db_reader, uint64_t number, evmc::bytes32 hash);
    std::vector<Log> filter_logs(std::vector<Log>& logs, const Filter& filter);

private:
    std::unique_ptr<ethdb::kv::Database>& database_;

    friend class silkrpc::http::RequestHandler;
};

} // namespace silkrpc::commands

#endif  // SILKRPC_COMMANDS_ETH_API_HPP_
