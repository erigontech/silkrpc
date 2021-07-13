/*
    Copyright 2021 The Silkrpc Authors

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

#include "gas_price_oracle.hpp"

#include <algorithm>
#include <utility>

#include <asio/compose.hpp>
#include <asio/post.hpp>
#include <asio/use_awaitable.hpp>
#include <silkrpc/core/blocks.hpp>
#include <silkrpc/core/rawdb/chain.hpp>

#include <silkrpc/common/log.hpp>

namespace silkrpc {

intx::uint256 get_block_base_fee(const silkworm::BlockHeader& block_header) {
    return 0;
}

intx::uint256 get_effective_gas_price(const silkworm::Transaction& transaction, intx::uint256 base_fee) {
    return transaction.gas_price;
}

struct PriceComparator {
    bool operator() (const intx::uint256& p1, const intx::uint256& p2) const {
        return p1 < p2;
    }
};

asio::awaitable<intx::uint256> GasPriceOracle::suggested_price() {
    auto block_number = co_await core::get_block_number(silkrpc::core::kLatestBlockId, db_reader_);
    block_number = 4000000; // TODO(sixtysixter) remove when sync with silkworm is completed
    SILKRPC_DEBUG << "block_number " << block_number << "\n";

    std::vector<intx::uint256> tx_prices;
    tx_prices.reserve(kMaxSamples);
    while (tx_prices.size() < kMaxSamples && block_number > 0) {
        co_await load_block_prices(block_number--, kSamples, tx_prices);
    }

    std::sort(tx_prices.begin(), tx_prices.end(), PriceComparator());

    intx::uint256 price = kDefaultPrice;
    if (tx_prices.size() > 0) {
        auto position = (tx_prices.size() - 1) * kPercentile / 100;
        SILKRPC_INFO << "GasPriceOracle::suggested_price getting price in position: " << position << "\n";
        if (tx_prices.size() > position) {
            price = tx_prices[position];
        }
    }

    if (price > silkrpc::kDefaultMaxPrice) {
        price = silkrpc::kDefaultMaxPrice;
    }

    co_return price;
}

asio::awaitable<void> GasPriceOracle::load_block_prices(uint64_t block_number, uint64_t limit, std::vector<intx::uint256>& tx_prices) {
    SILKRPC_TRACE << "GasPriceOracle::load_block_prices processing block: " << block_number << "\n";

    const auto block_with_hash = co_await core::rawdb::read_block_by_number(db_reader_, block_number);
    const auto base_fee = get_block_base_fee(block_with_hash.block.header);
    const auto coinbase = block_with_hash.block.header.beneficiary;

    SILKRPC_TRACE << "GasPriceOracle::load_block_prices # transactions in block: " << block_with_hash.block.transactions.size() << "\n";
    SILKRPC_TRACE << "GasPriceOracle::load_block_prices # block base_fee: 0x" << silkworm::to_hex(silkworm::rlp::big_endian(base_fee)) << "\n";
    SILKRPC_TRACE << "GasPriceOracle::load_block_prices # block beneficiary: 0x" << coinbase << "\n";

    std::vector<intx::uint256> block_prices;
    block_prices.reserve(block_with_hash.block.transactions.size());
    for (const auto& transaction : block_with_hash.block.transactions) {
        const auto effective_gas_price = get_effective_gas_price(transaction, base_fee);
        if (effective_gas_price < kDefaultMinPrice) {
            continue;
        }
        const auto sender = transaction.from;
        if (sender == coinbase) {
            continue;
        }
        block_prices.push_back(effective_gas_price);
    }

    std::sort(block_prices.begin(), block_prices.end(), PriceComparator());

    for (const auto& effective_gas_price : block_prices) {
        tx_prices.push_back(effective_gas_price);
        if (tx_prices.size() >= limit) {
            break;
        }
    }
}
} // namespace silkrpc
