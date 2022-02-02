/*
   Copyright 2021 The Silkrpc Authors

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

#ifndef SILKRPC_ETHBACKEND_BACKEND_INTERFACE_HPP_
#define SILKRPC_ETHBACKEND_BACKEND_INTERFACE_HPP_

#include <string>

#include <asio/io_context.hpp>
#include <asio/use_awaitable.hpp>
#include <evmc/evmc.hpp>
#include <silkrpc/types/execution_payload.hpp>

namespace silkrpc::ethbackend {


class BackEndInterface {
    public:
    
    virtual ~BackEndInterface() = default;
    virtual asio::awaitable<evmc::address> etherbase() = 0;
    virtual asio::awaitable<uint64_t> protocol_version() = 0;
    virtual asio::awaitable<uint64_t> net_version() = 0;
    virtual asio::awaitable<std::string> client_version() = 0;
    virtual asio::awaitable<uint64_t> net_peer_count() = 0;
    virtual asio::awaitable<ExecutionPayload> engine_get_payload_v1(uint64_t payload_id) = 0;
};

} // namespace silkrpc::ethbackend

# endif