/*
   Copyright 2022 The Silkrpc Authors

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

#ifndef SILKRPC_ETHBACKEND_BACKEND_TEST_HPP_
#define SILKRPC_ETHBACKEND_BACKEND_TEST_HPP_

#include <string>

#include <silkrpc/ethbackend/backend_interface.hpp>
#include <evmc/evmc.hpp>

namespace silkrpc::ethbackend {

using evmc::literals::operator""_address;

constexpr evmc::address kEtherbaseTest = 0xD6f2Ce894ea1A181E07040615F9a6598A76380CD_address;
constexpr uint64_t kProtocolVersionTest = 1;
constexpr uint64_t kNetVersionTest = 2;
static const char* kClientVersionTest = "6.0.0";
constexpr uint64_t kNetPeerCountTest = 5;
static const ExecutionPayload kGetPayloadTest = ExecutionPayload{1}; // Empty payload with block number 1


class TestBackEnd : public BackEndInterface {
public:
    asio::awaitable<evmc::address> etherbase() { co_return kEtherbaseTest; }
    asio::awaitable<uint64_t> protocol_version() { co_return kProtocolVersionTest; }
    asio::awaitable<uint64_t> net_version() { co_return kNetVersionTest; }
    asio::awaitable<std::string> client_version() { co_return kClientVersionTest; }
    asio::awaitable<uint64_t> net_peer_count() { co_return kNetPeerCountTest; }
    asio::awaitable<ExecutionPayload> engine_get_payload_v1(uint64_t payload_id) { co_return kGetPayloadTest; }
};

} // namespace silkrpc::ethbackend

#endif // SILKRPC_ETHBACKEND_BACKEND_TEST_HPP_
