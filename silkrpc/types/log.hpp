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

#ifndef SILKRPC_TYPES_LOG_H_
#define SILKRPC_TYPES_LOG_H_

#include <vector>

#include <evmc/evmc.hpp>

#include <silkworm/common/base.hpp>

namespace silkrpc {

struct Log {
    /* raw fields */
    evmc::address address;
    std::vector<evmc::bytes32> topics;
    silkworm::Bytes data;

    /* derived fields */
    uint64_t block_number;
    evmc::bytes32 tx_hash;
    uint32_t tx_index;
    evmc::bytes32 block_hash;
    uint32_t index;
    bool removed;
};

typedef std::vector<Log> Logs;

} // silkrpc

#endif  // SILKRPC_TYPES_LOG_H_
