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

#include <boost/endian/conversion.hpp>
#include <evmc/evmc.hpp>
#include <silkworm/common/util.hpp>
#include <silkworm/common/base.hpp>
#include <silkworm/core/silkworm/execution/address.hpp>
#include <silkworm/db/util.hpp>

namespace silkrpc::commands {

// checkTxFee used to check whether the fee of
// the given transaction is _reasonable_(under the cap).
bool check_tx_fee_less_cap(intx::uint256 max_fee_per_gas, uint64_t gas_limit) {
     const float ether = silkworm::kEther;
     const float cap = 1; // TBD

     // Short circuit if there is no cap for transaction fee at all.
     if (cap == 0) {
         return true;
     }
     float fee_eth = ((uint64_t)max_fee_per_gas * gas_limit) / ether;
     if (fee_eth > cap) {
         return false;
     }
     return true;
}

bool is_replay_protected(const silkworm::Transaction& txn) {
     if (txn.type != silkworm::Transaction::Type::kLegacy) {
         return false;
     }
     intx::uint256 v = txn.v();
     if (v != 27 && v != 28 && v != 0 && v != 1) {
         return true;
     }
     return false;
}

std::string decoding_result_to_string(silkworm::rlp::DecodingResult decode_result) {
    switch (decode_result) {
        case silkworm::rlp::DecodingResult::kOverflow:
            return "rlp: uint overflow";
        case silkworm::rlp::DecodingResult::kLeadingZero:
            return "rlp: leading Zero";
        case silkworm::rlp::DecodingResult::kInputTooShort:
            return "rlp: element is larger than containing list";
        case silkworm::rlp::DecodingResult::kNonCanonicalSingleByte:
            return "rlp: non-canonical integer format";
        case silkworm::rlp::DecodingResult::kNonCanonicalSize:
            return "rlp: non-canonical size information";
        case silkworm::rlp::DecodingResult::kUnexpectedLength:
            return "rlp: unexpected Length";
        case silkworm::rlp::DecodingResult::kUnexpectedString:
            return "rlp: unexpected String";
        case silkworm::rlp::DecodingResult::kUnexpectedList:
            return "rlp: element is larger than containing list";
        case silkworm::rlp::DecodingResult::kListLengthMismatch:
            return "rlp: list Length Mismatch";
        case silkworm::rlp::DecodingResult::kInvalidVInSignature:         // v != 27 && v != 28 && v < 35, see EIP-155
            return "rlp: invalid V in signature";
        case silkworm::rlp::DecodingResult::kUnsupportedTransactionType:
            return "rlp: unknown tx type prefix";
        default:
            return "unknownError";
   }
}
} // namespace silkrpc::commands
