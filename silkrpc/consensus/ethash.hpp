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

#ifndef SILKRPC_CONSENSUS_ETHASH_HPP_
#define SILKRPC_CONSENSUS_ETHASH_HPP_

#include <iostream>
#include <string>
#include <vector>

#include <intx/intx.hpp>
#include <silkworm/types/block.hpp>

#include <silkrpc/types/chain_config.hpp>

namespace silkrpc::ethash {

struct BlockReward {
    intx::uint256 miner_reward;
    std::vector<intx::uint256> ommer_rewards;
};

BlockReward compute_reward(const ChainConfig& config, const silkworm::Block& block);

std::ostream& operator<<(std::ostream& out, const BlockReward& reward);

} // namespace silkrpc::ethash

#endif  // SILKRPC_CONSENSUS_ETHASH_HPP_
