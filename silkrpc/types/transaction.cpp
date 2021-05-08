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

#include "transaction.hpp"

#include <iomanip>

#include <silkrpc/common/util.hpp>

namespace silkrpc {

std::ostream& operator<<(std::ostream& out, const Transaction& t) {
    out << " block_hash: " << t.block_hash;
    out << " block_number: " << t.block_number;
    out << " transaction_index: " << t.transaction_index;

    return out;
}

} // namespace silkrpc
