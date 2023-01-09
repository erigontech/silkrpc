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

#ifndef SILKRPC_TYPES_NODE_INFO_HPP_
#define SILKRPC_TYPES_NODE_INFO_HPP_

#include <optional>
#include <string>

#include <evmc/evmc.hpp>

namespace silkrpc {

struct NodeInfoClique {
    uint64_t period;
    uint64_t epoch;
};

struct NodeInfoConfig {
    std::string chain_name;
    uint64_t chain_id;
    std::string consensus;
    uint64_t homestead_block;
    bool  dao_fork_support;
    uint64_t eip150_block;
    evmc::bytes32 eip150_hash;
    uint64_t eip155_block;
    uint64_t byzantium_block;
    uint64_t constantinople_block;
    uint64_t petersburg_block;
    uint64_t istanbul_block;
    uint64_t berlin_block;
    uint64_t london_block;
    uint64_t terminal_total_difficulty;
    bool  terminal_total_difficulty_passed;
    std::optional<NodeInfoClique> clique;
};

struct NodeInfoPorts{
    uint64_t discovery;
    uint64_t listener;
};

struct NodeInfoEthereum {
    uint64_t network;
    uint64_t difficulty;
    evmc::bytes32 genesis_hash;
    NodeInfoConfig config;
    evmc::bytes32 head;
};

struct NodeInfoProtocols {
    NodeInfoEthereum eth;
};

struct NodeInfo {
    std::string       id;
    std::string       name;
    std::string       enode;
    std::string       enr;
    std::string       listenerAddr;
    NodeInfoProtocols protocols;
    NodeInfoPorts     ports;
};

} // namespace silkrpc

#endif  // SILKRPC_TYPES_NODE_INFO_HPP_
