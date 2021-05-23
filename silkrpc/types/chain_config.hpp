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

#ifndef SILKRPC_TYPES_CHAIN_CONFIG_HPP_
#define SILKRPC_TYPES_CHAIN_CONFIG_HPP_

#include <iostream>
#include <vector>

#include <evmc/evmc.hpp>
#include <nlohmann/json.hpp>

namespace silkrpc {

struct ChainConfig {
    evmc::bytes32 genesis_hash;
    nlohmann::json config;
};

struct Forks {
    const evmc::bytes32& genesis_hash;
    std::vector<uint64_t> block_numbers;

    Forks(const ChainConfig& chain_config) : genesis_hash(chain_config.genesis_hash) {
        block_numbers = {
            chain_config.config["istanbulBlock"],
            chain_config.config["berlinBlock"],
        };
    }
};

std::ostream& operator<<(std::ostream& out, const ChainConfig& chain_config);

} // namespace silkrpc

#endif  // SILKRPC_TYPES_CHAIN_CONFIG_HPP_
