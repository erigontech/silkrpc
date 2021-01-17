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

#include "types.hpp"

#include <exception>
#include <iostream>

#include <silkworm/core/silkworm/common/util.hpp>
#include <silkworm/core/silkworm/types/receipt.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/core/blocks.hpp>
#include <silkrpc/core/rawdb/chain.hpp>

namespace silkrpc::json {

// https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_blockNumber
asio::awaitable<void> EthereumRpcApi::handle_eth_block_number(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = database_->begin();
    ethdb::TransactionDatabase tx_database{*tx};

    try {
        const auto block_height = co_await core::get_current_block_number(tx_database);
        reply = eth::make_json_content(request["id"], block_height);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << "\n";
        reply = eth::make_json_error(request["id"], e.what());

        tx->rollback(); // co_await when implemented
    }

    co_return;
}

// https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_getLogs
asio::awaitable<void> EthereumRpcApi::handle_eth_get_logs(const nlohmann::json& request, nlohmann::json& reply) {
    auto filter = request["params"].get<eth::Filter>();
    std::cout << "filter=" << filter << "\n" << std::flush;

    std::vector<silkworm::Log> logs;

    auto tx = database_->begin();
    ethdb::TransactionDatabase tx_database{*tx};

    try {
        uint64_t start{}, end{};
        if (filter.block_hash.has_value()) {
            auto block_hash = silkworm::to_bytes32(silkworm::from_hex(filter.block_hash.value()));
            auto block_number = co_await core::rawdb::read_header_number(tx_database, block_hash);
            start = end = block_number;
        } else {
            auto latest_block_number = co_await core::get_latest_block_number(tx_database);
            start = filter.from_block.value_or(latest_block_number);
            end = filter.from_block.value_or(latest_block_number);
        }

        Roaring block_numbers;
        block_numbers.addRange(start, end + 1);

        if (filter.topics.has_value()) {
            auto topics_bitmap = get_topics_bitmap(tx_database, filter.topics.value(), start, end); // [3] implement
            if (!topics_bitmap.isEmpty()) {
                if (block_numbers.isEmpty()) {
                    block_numbers = topics_bitmap;
                } else {
                    block_numbers &= topics_bitmap;
                }
            }
        }

        if (filter.addresses.has_value()) {
            auto addresses_bitmap = get_addresses_bitmap(tx_database, filter.addresses.value(), start, end); // [3] implement
            if (!addresses_bitmap.isEmpty()) {
                if (block_numbers.isEmpty()) {
                    block_numbers = addresses_bitmap;
                } else {
                    block_numbers &= addresses_bitmap;
                }
            }
        }

        if (block_numbers.cardinality() == 0) {
            reply = eth::make_json_content(request["id"], logs);
            co_return;
        }

        std::vector<uint32_t> ans{uint32_t(block_numbers.cardinality())};
        block_numbers.toUint32Array(ans.data());
        for (auto block_to_match : block_numbers) {
            auto block_hash = co_await core::rawdb::read_canonical_block_hash(tx_database, uint64_t(block_to_match));
            if (block_hash == evmc::bytes32{}) {
                reply = eth::make_json_content(request["id"], logs);
                co_return;
            }

            auto receipts = co_await get_receipts(tx_database, uint64_t(block_to_match), block_hash);
            std::vector<silkworm::Log> unfiltered_logs{receipts.size()};
            for (auto receipt: receipts) {
                unfiltered_logs.insert(unfiltered_logs.end(), receipt.logs.begin(), receipt.logs.end());
            }
            auto filtered_logs = filter_logs(unfiltered_logs, filter);
            logs.insert(logs.end(), filtered_logs.begin(), filtered_logs.end());
        }

        reply = eth::make_json_content(request["id"], logs);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << "\n";
        reply = eth::make_json_error(request["id"], e.what());

        tx->rollback(); // co_await when implemented
    }

    co_return;
}

Roaring EthereumRpcApi::get_topics_bitmap(ethdb::TransactionDatabase& tx_db, eth::FilterTopics& topics, uint64_t start, uint64_t end) {
    Roaring r;
    return r;
}

Roaring EthereumRpcApi::get_addresses_bitmap(ethdb::TransactionDatabase& tx_db, eth::FilterAddresses& addresses, uint64_t start, uint64_t end) {
    Roaring r;
    return r;
}

asio::awaitable<Receipts> EthereumRpcApi::get_receipts(ethdb::TransactionDatabase& tx_db, uint64_t number, evmc::bytes32 hash) {
    co_return Receipts{};
}

std::vector<silkworm::Log> EthereumRpcApi::filter_logs(std::vector<silkworm::Log>& unfiltered, const eth::Filter& filter) {
    std::vector<silkworm::Log> filtered_logs;

    return filtered_logs;
}

} // namespace silkrpc::json