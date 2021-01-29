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
#include <exception>
#include <iostream>

#include <silkworm/common/util.hpp>
#include <silkworm/types/receipt.hpp>
#include <silkworm/db/tables.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/core/blocks.hpp>
#include <silkrpc/core/rawdb/chain.hpp>
#include <silkrpc/ethdb/bitmap/database.hpp>
#include <silkrpc/json/types.hpp>
#include <silkrpc/types/filter.hpp>

namespace silkrpc::commands {

// https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_blockNumber
asio::awaitable<void> EthereumRpcApi::handle_eth_block_number(const nlohmann::json& request, nlohmann::json& reply) {
    auto tx = database_->begin();
    ethdb::kv::TransactionDatabase tx_database{*tx};

    try {
        const auto block_height = co_await core::get_current_block_number(tx_database);
        reply = json::make_json_content(request["id"], block_height);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << "\n";
        reply = json::make_json_error(request["id"], e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception\n";
        reply = json::make_json_error(request["id"], "unexpected exception");
    }

    co_await tx->close(); // RAII not (yet) available with coroutines
    co_return;
}

// https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_getLogs
asio::awaitable<void> EthereumRpcApi::handle_eth_get_logs(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error = "invalid eth_getLogs params: " + params.dump();
        //SILKRPC_ERROR << error << "\n";
        reply = json::make_json_error(request["id"], error);
        co_return;
    }
    auto filter = params[0].get<Filter>();
    //SILKRPC_INFO << "filter: " << filter << "\n" << std::flush;

    std::vector<Log> logs;

    auto tx = database_->begin();
    ethdb::kv::TransactionDatabase tx_database{*tx};

    try {
        uint64_t start{}, end{};
        if (filter.block_hash.has_value()) {
            auto block_hash = silkworm::to_bytes32(silkworm::from_hex(filter.block_hash.value()));
            auto block_number = co_await core::rawdb::read_header_number(tx_database, block_hash);
            start = end = block_number;
        } else {
            auto latest_block_number = co_await core::get_latest_block_number(tx_database);
            start = filter.from_block.value_or(latest_block_number);
            end = filter.to_block.value_or(latest_block_number);
        }

        Roaring block_numbers;
        block_numbers.addRange(start, end + 1);

        if (filter.topics.has_value()) {
            auto topics_bitmap = co_await get_topics_bitmap(tx_database, filter.topics.value(), start, end);
            //SILKRPC_INFO << "topics_bitmap: " << topics_bitmap.toString() << "\n" << std::flush;
            if (!topics_bitmap.isEmpty()) {
                if (block_numbers.isEmpty()) {
                    block_numbers = topics_bitmap;
                } else {
                    block_numbers &= topics_bitmap;
                }
            }
        }
        //SILKRPC_INFO << "block_numbers: " << block_numbers.toString() << "\n" << std::flush;

        if (filter.addresses.has_value()) {
            auto addresses_bitmap = co_await get_addresses_bitmap(tx_database, filter.addresses.value(), start, end); // [2] implement
            if (!addresses_bitmap.isEmpty()) {
                if (block_numbers.isEmpty()) {
                    block_numbers = addresses_bitmap;
                } else {
                    block_numbers &= addresses_bitmap;
                }
            }
        }
        //SILKRPC_INFO << "block_numbers: " << block_numbers.toString() << "\n" << std::flush;
        //SILKRPC_INFO << "block_numbers.cardinality(): " << block_numbers.cardinality() << "\n" << std::flush;

        if (block_numbers.cardinality() == 0) {
            reply = json::make_json_content(request["id"], logs);
            co_return;
        }

        std::vector<uint32_t> block_number_vector{};
        block_number_vector.reserve(uint32_t(block_numbers.cardinality()));
        //SILKRPC_INFO << "block_number_vector vector size: " << block_number_vector.size() << "\n" << std::flush;
        block_numbers.toUint32Array(block_number_vector.data());
        for (auto block_to_match : block_numbers) {
            //SILKRPC_INFO << "block_to_match: " << block_to_match << "\n" << std::flush;
            auto block_hash = co_await core::rawdb::read_canonical_block_hash(tx_database, uint64_t(block_to_match));
            //SILKRPC_INFO << "block_hash: " << silkworm::to_hex(block_hash) << "\n" << std::flush;
            if (block_hash == evmc::bytes32{}) {
                reply = json::make_json_content(request["id"], logs);
                co_return;
            }

            auto receipts = co_await get_receipts(tx_database, uint64_t(block_to_match), block_hash);
            //SILKRPC_INFO << "receipts.size(): " << receipts.size() << "\n" << std::flush;
            std::vector<Log> unfiltered_logs{};
            unfiltered_logs.reserve(receipts.size());
            for (auto receipt: receipts) {
                //SILKRPC_INFO << "receipt.logs.size(): " << receipt.logs.size() << "\n" << std::flush;
                unfiltered_logs.insert(unfiltered_logs.end(), receipt.logs.begin(), receipt.logs.end());
            }
            //SILKRPC_INFO << "unfiltered_logs.size(): " << unfiltered_logs.size() << "\n" << std::flush;
            auto filtered_logs = filter_logs(unfiltered_logs, filter);
            //SILKRPC_INFO << "filtered_logs.size(): " << filtered_logs.size() << "\n" << std::flush;
            logs.insert(logs.end(), filtered_logs.begin(), filtered_logs.end());
        }
        SILKRPC_INFO << "logs.size(): " << logs.size() << "\n" << std::flush;

        reply = json::make_json_content(request["id"], logs);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << "\n";
        reply = json::make_json_error(request["id"], e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception\n";
        reply = json::make_json_error(request["id"], "unexpected exception");
    }

    co_await tx->close();// RAII not (yet) available with coroutines
    co_return;
}

asio::awaitable<Roaring> EthereumRpcApi::get_topics_bitmap(core::rawdb::DatabaseReader& db_reader, FilterTopics& topics, uint64_t start, uint64_t end) {
    //SILKRPC_INFO << "#topics: " << topics.size() << " start: " << start << " end: " << end << "\n" << std::flush;
    Roaring result_bitmap;
    for (auto subtopics : topics) {
        //SILKRPC_INFO << "#subtopics: " << subtopics.size() << "\n" << std::flush;
        Roaring subtopic_bitmap;
        for (auto topic : subtopics) {
            silkworm::Bytes topic_key{std::begin(topic.bytes), std::end(topic.bytes)};
            //SILKRPC_INFO << "topic: " << topic << "\n" << std::flush;
            auto bitmap = co_await ethdb::bitmap::get(db_reader, silkworm::db::table::kLogTopicIndex.name, topic_key, start, end);
            //SILKRPC_INFO << "bitmap: " << bitmap.toString() << "\n" << std::flush;
            subtopic_bitmap |= bitmap;
            //SILKRPC_INFO << "subtopic_bitmap: " << subtopic_bitmap.toString() << "\n" << std::flush;
        }
        if (result_bitmap.isEmpty()) {
            result_bitmap = subtopic_bitmap;
        } else {
            result_bitmap &= subtopic_bitmap;
        }
        //SILKRPC_INFO << "result_bitmap: " << result_bitmap.toString() << "\n" << std::flush;
    }
    co_return result_bitmap;
}

asio::awaitable<Roaring> EthereumRpcApi::get_addresses_bitmap(core::rawdb::DatabaseReader& db_reader, FilterAddresses& addresses, uint64_t start, uint64_t end) {
    //SILKRPC_INFO << "#addresses: " << addresses.size() << " start: " << start << " end: " << end << "\n" << std::flush;
    Roaring result_bitmap;
    for (auto address : addresses) {
        silkworm::Bytes address_key{std::begin(address.bytes), std::end(address.bytes)};
        auto bitmap = co_await ethdb::bitmap::get(db_reader, silkworm::db::table::kLogAddressIndex.name, address_key, start, end);
        //SILKRPC_INFO << "bitmap: " << bitmap.toString() << "\n" << std::flush;
        result_bitmap |= bitmap;
    }
    //SILKRPC_INFO << "result_bitmap: " << result_bitmap.toString() << "\n" << std::flush;
    co_return result_bitmap;
}

asio::awaitable<Receipts> EthereumRpcApi::get_receipts(core::rawdb::DatabaseReader& db_reader, uint64_t number, evmc::bytes32 hash) {
    auto cached_receipts = co_await core::rawdb::read_receipts(db_reader, hash, number);
    if (!cached_receipts.empty()) {
        co_return cached_receipts;
    }

    // If not already present, retrieve receipts by executing transactions
    auto block = co_await core::rawdb::read_block(db_reader, hash, number);
    // TODO: implement
    SILKRPC_WARN << "retrieve receipts by executing transactions NOT YET IMPLEMENTED\n" << std::flush;
    co_return Receipts{};
}

std::vector<Log> EthereumRpcApi::filter_logs(std::vector<Log>& logs, const Filter& filter) {
    std::vector<Log> filtered_logs;

    auto addresses = filter.addresses;
    auto topics = filter.topics;
    //SILKRPC_INFO << "filter.addresses: " << filter.addresses << "\n" << std::flush;
    for (auto log : logs) {
        //SILKRPC_INFO << "log: " << log << "\n" << std::flush;
        if (addresses.has_value() && std::find(addresses.value().begin(), addresses.value().end(), log.address) == addresses.value().end()) {
            //SILKRPC_INFO << "skipped log for address: 0x" << silkworm::to_hex(log.address) << "\n" << std::flush;
            continue;
        }
        auto matches = true;
        if (topics.has_value()) {
            if (topics.value().size() > log.topics.size()) {
                //SILKRPC_INFO << "#topics: " << topics.value().size() << " #log.topics: " << log.topics.size() << "\n" << std::flush;
                continue;
            }
            for (size_t i{0}; i < topics.value().size(); i++) {
                //SILKRPC_INFO << "log.topics[i]: " << log.topics[i] << "\n" << std::flush;
                auto subtopics = topics.value()[i];
                auto matches_subtopics = subtopics.empty(); // empty rule set == wildcard
                //SILKRPC_INFO << "matches_subtopics: " << std::boolalpha << matches_subtopics << "\n" << std::flush;
                for (auto topic : subtopics) {
                    //SILKRPC_INFO << "topic: " << topic << "\n" << std::flush;
                    if (log.topics[i] == topic) {
                        matches_subtopics = true;
                        break;
                    }
                }
                if (!matches_subtopics) {
                    matches = false;
                    break;
                }
            }
        }
        if (matches) {
            filtered_logs.push_back(log);
        }
    }
    return filtered_logs;
}

} // namespace silkrpc::commands