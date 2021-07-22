/*
    Copyright 2020 The Silkrpc Author           hp88niomomjhijmpoonbp0kj.0
    kjmm,<<<<<<<<<<<<<<<9iy3

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

#include "transaction.hpp"

#include <iomanip>

#include <silkrpc/common/util.hpp>

namespace silkrpc {

intx::uint256 Transaction::effective_gas_price() const {
    return silkworm::Transaction::effective_gas_price(block_base_fee_per_gas.value_or(0));
}

std::ostream& operator<<(std::ostream& out, const Transaction& t) {
    out << " #access_list: " << t.access_list.size();
    out << " block_hash: " << t.block_hash;
    out << " block_number: " << t.block_number;
    out << " block_base_fee_per_gas: " << silkworm::to_hex(silkworm::rlp::big_endian(t.block_base_fee_per_gas.value_or(0)));
    if (t.chain_id) {
        out << " chain_id: " << silkworm::to_hex(silkworm::rlp::big_endian(*t.chain_id));
    } else {
        out << " chain_id: null";
    }
    out << " data: " << silkworm::to_hex(t.data);
    if (t.from) {
        out << " from: " << silkworm::to_hex(*t.from);
    } else {
        out << " from: null";
    }
    out << " nonce: " << t.nonce;
    out << " max_priority_fee_per_gas: " << silkworm::to_hex(silkworm::rlp::big_endian(t.max_priority_fee_per_gas));
    out << " max_fee_per_gas: " << silkworm::to_hex(silkworm::rlp::big_endian(t.max_fee_per_gas));
    out << " gas_price: " << silkworm::to_hex(silkworm::rlp::big_endian(t.effective_gas_price()));
    out << " gas_limit: " << t.gas_limit;
    out << " odd_y_parity: " << t.odd_y_parity;

    out << " r: " << silkworm::to_hex(silkworm::rlp::big_endian(t.r));
    out << " s: " << silkworm::to_hex(silkworm::rlp::big_endian(t.s));

    if (t.to) {
        out << " to: " << silkworm::to_hex(*t.to);
    } else {
        out << " to: null";
    }
    out << " transaction_index: " << t.transaction_index;
    if (t.type) {
        out << " type: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(*t.type);
    } else {
        out << " type: null";
    }
    out << " value: " << silkworm::to_hex(silkworm::rlp::big_endian(t.value));

    return out;
}

} // namespace silkrpc
