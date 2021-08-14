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

#include "block.hpp"

#include <iomanip>

#include <silkrpc/common/util.hpp>

namespace silkrpc {

std::ostream& operator<<(std::ostream& out, const Block& b) {
    out << "parent_hash: " << b.block.header.parent_hash;
    out << " ommers_hash: " << b.block.header.ommers_hash;
    out << " beneficiary: ";
    for (const auto& b : b.block.header.beneficiary.bytes) {
        out << std::hex << std::setw(2) << std::setfill('0') << int(b);
    }
    out << std::dec;
    out << " state_root: " << b.block.header.state_root;
    out << " transactions_root: " << b.block.header.transactions_root;
    out << " receipts_root: " << b.block.header.receipts_root;
    out << " logs_bloom: " << silkworm::to_hex(silkworm::full_view(b.block.header.logs_bloom));
    out << " difficulty: " << silkworm::to_hex(silkworm::rlp::big_endian(b.block.header.difficulty));
    out << " number: " << b.block.header.number;
    out << " gas_limit: " << b.block.header.gas_limit;
    out << " gas_used: " << b.block.header.gas_used;
    out << " timestamp: " << b.block.header.timestamp;
    out << " extra_data: " << silkworm::to_hex(b.block.header.extra_data);
    out << " mix_hash: " << b.block.header.mix_hash;
    out << " nonce: " << silkworm::to_hex({b.block.header.nonce.data(), b.block.header.nonce.size()});
    out << " #transactions: " << b.block.transactions.size();
    out << " #ommers: " << b.block.ommers.size();
    out << " hash: " << b.hash;
    out << " total_difficulty: " << silkworm::to_hex(silkworm::rlp::big_endian(b.total_difficulty));
    out << " full_tx: " << b.full_tx;
    return out;
}

uint64_t Block::get_block_size() const {
   silkworm::rlp::Header rlp_head{true, 0};
   rlp_head.payload_length = silkworm::rlp::length(block.header);
   rlp_head.payload_length += silkworm::rlp::length(block.transactions);
   rlp_head.payload_length += silkworm::rlp::length(block.ommers);
   rlp_head.payload_length += silkworm::rlp::length_of_length(rlp_head.payload_length);
   return rlp_head.payload_length;
}

} // namespace silkrpc
