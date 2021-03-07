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

#ifndef SILKRPC_TYPES_RECEIPT_HPP_
#define SILKRPC_TYPES_RECEIPT_HPP_

#include <vector>

#include <evmc/evmc.hpp>

#include <silkworm/types/bloom.hpp>

#include "log.hpp"

namespace silkrpc {

struct Receipt {
    /* raw fields */
    bool success{false};
    uint64_t cumulative_gas_used{0};
    silkworm::Bloom bloom;
    Logs logs;

    /* derived fields */
    evmc::bytes32 tx_hash;
    evmc::address contract_address;
    uint64_t gas_used;
    evmc::bytes32 block_hash;
    uint64_t block_number;
    uint32_t tx_index;
};

typedef std::vector<Receipt> Receipts;

} // namespace silkrpc

#endif  // SILKRPC_TYPES_RECEIPT_HPP_
