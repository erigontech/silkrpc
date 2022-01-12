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

#include "execution_payload.hpp"

#include <silkrpc/common/util.hpp>
#include <silkworm/common/endian.hpp>
#include <silkworm/core/silkworm/rlp/encode_vector.hpp>
#include <silkrpc/core/blocks.hpp>

namespace silkrpc {

std::ostream& operator<<(std::ostream& out, const ExecutionPayload& p) {
    out << "parent_hash: " << p.parent_hash;
    out << " state_root: " << p.state_root;
    out << " receipts_root: " << p.receipts_root;
    out << " logs_bloom: " << silkworm::to_hex(full_view(p.logs_bloom));
    out << " number: " << p.number;
    out << " gas_limit: " << p.gas_limit;
    out << " gas_used: " << p.gas_used;
    out << " timestamp: " << p.timestamp;
    out << " extra_data: " << silkworm::to_hex(p.extra_data);
    out << " random: " << p.random;
    out << " #transactions: " << p.transactions.size();
    out << " hash: " << p.block_hash;
    return out;
}

} // namespace silkrpc
