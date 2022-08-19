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

#include <boost/asio/awaitable.hpp>
#include <boost/asio/thread_pool.hpp>
#include <evmc/evmc.hpp>
#include <nlohmann/json.hpp>

#include <silkrpc/txpool/transaction_pool.hpp>
#include <silkworm/types/receipt.hpp>
#include <silkrpc/concurrency/context_pool.hpp>
#include <silkrpc/core/rawdb/accessors.hpp>
#include <silkrpc/croaring/roaring.hh>
#include <silkrpc/json/types.hpp>
#include <silkrpc/ethbackend/backend.hpp>
#include <silkrpc/ethdb/database.hpp>
#include <silkrpc/ethdb/transaction.hpp>
#include <silkrpc/types/log.hpp>
#include <silkrpc/types/receipt.hpp>

namespace silkrpc::http { class RequestHandler; }

namespace silkrpc::commands {

class EthereumRpcApi {
public:
    explicit EthereumRpcApi(Context& context, boost::asio::thread_pool& workers)
        : context_(context),
          block_cache_(context.block_cache()),
          state_cache_(context.state_cache()),
          database_(context.database()),
          backend_(context.backend()),
          miner_{context.miner()},
          tx_pool_{context.tx_pool()},
          workers_{workers} {}

    virtual ~EthereumRpcApi() {}

    EthereumRpcApi(const EthereumRpcApi&) = delete;
    EthereumRpcApi& operator=(const EthereumRpcApi&) = delete;

protected:
    boost::asio::awaitable<void> handle_eth_block_number(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_chain_id(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_protocol_version(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_syncing(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_gas_price(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_block_by_hash(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_block_by_number(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_block_transaction_count_by_hash(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_block_transaction_count_by_number(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_uncle_by_block_hash_and_index(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_uncle_by_block_number_and_index(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_uncle_count_by_block_hash(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_uncle_count_by_block_number(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_transaction_by_hash(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_transaction_by_block_hash_and_index(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_transaction_by_block_number_and_index(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_raw_transaction_by_hash(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_raw_transaction_by_block_hash_and_index(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_raw_transaction_by_block_number_and_index(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_transaction_receipt(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_estimate_gas(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_balance(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_code(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_transaction_count(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_storage_at(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_call(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_call_bundle(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_create_access_list(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_new_filter(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_new_block_filter(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_new_pending_transaction_filter(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_filter_changes(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_uninstall_filter(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_logs(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_send_raw_transaction(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_send_transaction(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_sign_transaction(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_proof(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_mining(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_coinbase(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_hashrate(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_submit_hashrate(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_get_work(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_submit_work(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_subscribe(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_eth_unsubscribe(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<roaring::Roaring> get_topics_bitmap(core::rawdb::DatabaseReader& db_reader, FilterTopics& topics, uint64_t start, uint64_t end);
    boost::asio::awaitable<roaring::Roaring> get_addresses_bitmap(core::rawdb::DatabaseReader& db_reader, FilterAddresses& addresses, uint64_t start, uint64_t end);

    std::vector<Log> filter_logs(std::vector<Log>& logs, const Filter& filter);

    Context& context_;
    std::shared_ptr<BlockCache>& block_cache_;
    std::shared_ptr<ethdb::kv::StateCache>& state_cache_;
    std::unique_ptr<ethdb::Database>& database_;
    std::unique_ptr<ethbackend::BackEnd>& backend_;
    std::unique_ptr<txpool::Miner>& miner_;
    std::unique_ptr<txpool::TransactionPool>& tx_pool_;
    boost::asio::thread_pool& workers_;

    friend class silkrpc::http::RequestHandler;
};

} // namespace silkrpc::commands

#endif  // SILKRPC_COMMANDS_ETH_API_HPP_
