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

#include "ethash.hpp"

namespace silkrpc::ethash {

BlockReward compute_reward(const ChainConfig& config, const silkworm::Block& block) {
    return BlockReward{};
}

std::ostream& operator<<(std::ostream& out, const BlockReward& reward) {
    out << "miner_reward: " << intx::to_string(reward.miner_reward) << " "
        << "ommer_rewards: [";
    for (auto i{0}; i < reward.ommer_rewards.size(); i++) {
        out << intx::to_string(reward.ommer_rewards[i]);
        if (i != reward.ommer_rewards.size() - 1) {
            out << " ";
        }
    }
    out << "]";
    return out;
}

} // namespace silkrpc::ethash
