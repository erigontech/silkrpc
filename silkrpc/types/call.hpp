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

#ifndef SILKRPC_TYPES_CALL_HPP_
#define SILKRPC_TYPES_CALL_HPP_

#include <iostream>
#include <optional>

#include <evmc/evmc.hpp>
#include <intx/intx.hpp>

#include <silkworm/common/util.hpp>
#include <silkworm/types/transaction.hpp>

namespace silkrpc {

struct Call {
    std::optional<evmc::address> from;
    std::optional<evmc::address> to;
    std::optional<uint64_t> gas;
    std::optional<intx::uint256> gas_price;
    std::optional<intx::uint256> max_priority_fee_per_gas;
    std::optional<intx::uint256> max_fee_per_gas;
    std::optional<intx::uint256> value;
    std::optional<silkworm::Bytes> data;

    silkworm::Transaction to_transaction() const {
        silkworm::Transaction txn{};
        txn.from = from;
        txn.to = to;
        txn.gas_limit = gas.value_or(0);
        if (gas_price) {
            txn.max_priority_fee_per_gas = gas_price.value();
            txn.max_fee_per_gas = gas_price.value();
        } else {
            txn.max_priority_fee_per_gas = max_priority_fee_per_gas.value_or(intx::uint256{0});
            txn.max_fee_per_gas = max_fee_per_gas.value_or(intx::uint256{0});
        }
        txn.value = value.value_or(intx::uint256{0});
        txn.data = data.value_or(silkworm::Bytes{});
        return txn;
    }
};

std::ostream& operator<<(std::ostream& out, const Call& call);

} // namespace silkrpc

#endif  // SILKRPC_TYPES_CALL_HPP_
