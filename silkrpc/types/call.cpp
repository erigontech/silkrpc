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

#include "call.hpp"

#include <silkworm/common/util.hpp>
#include <silkrpc/common/util.hpp>

namespace silkrpc {

std::ostream& operator<<(std::ostream& out, const Call& call) {
    out << "from: " << call.from.value_or(evmc::address{}) << " "
    << "to: " << call.to.value_or(evmc::address{}) << " "
    << "gas: " << call.gas.value_or(0) << " "
    << "gas_price: " << silkworm::to_hex(silkworm::rlp::big_endian(call.gas_price.value_or(intx::uint256{}))) << " "
    << "value: " << silkworm::to_hex(silkworm::rlp::big_endian(call.value.value_or(intx::uint256{}))) << " "
    << "data: " << silkworm::to_hex(call.data.value_or(silkworm::Bytes{}));
    return out;
}

} // namespace silkrpc
