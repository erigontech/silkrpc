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

#ifndef SILKRPC_COMMANDS_RPC_API_HPP_
#define SILKRPC_COMMANDS_RPC_API_HPP_

#include <memory>

#include <silkrpc/commands/eth_api.hpp>
#include <silkrpc/commands/web3_api.hpp>
#include <silkrpc/ethdb/kv/database.hpp>

namespace silkrpc::http { class RequestHandler; }

namespace silkrpc::commands {

class RpcApi : protected EthereumRpcApi, Web3RpcApi {
public:
    explicit RpcApi(std::unique_ptr<ethdb::kv::Database>& database)
    : EthereumRpcApi{database}, Web3RpcApi{database} {}
    virtual ~RpcApi() {}

    RpcApi(const RpcApi&) = delete;
    RpcApi& operator=(const RpcApi&) = delete;

    friend class silkrpc::http::RequestHandler;
};

} // namespace silkrpc::commands

#endif  // SILKRPC_COMMANDS_RPC_API_HPP_
