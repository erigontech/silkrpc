/*
    Copyright 2020-2021 The Silkrpc Authors

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

#ifndef SILKRPC_COMMANDS_RPC_API_HANDLER_HPP_
#define SILKRPC_COMMANDS_RPC_API_HANDLER_HPP_

#include <map>
#include <memory>
#include <string>

#include <silkrpc/config.hpp>

#include <asio/awaitable.hpp>
#include <asio/thread_pool.hpp>

#include <silkrpc/context_pool.hpp>
#include <silkrpc/commands/rpc_api.hpp>
#include <silkrpc/commands/rpc_api_table.hpp>
#include <silkrpc/http/reply.hpp>
#include <silkrpc/http/request.hpp>
#include <silkrpc/http/request_handler.hpp>

namespace silkrpc::commands {

class RpcApiHandler : public http::RequestHandler {
public:
    RpcApiHandler(const RpcApiHandler&) = delete;
    RpcApiHandler& operator=(const RpcApiHandler&) = delete;

    RpcApiHandler(Context& context, asio::thread_pool& workers, const RpcApiTable& rpc_api_table)
        : rpc_api_{context, workers}, rpc_api_table_(rpc_api_table) {}

    virtual ~RpcApiHandler() {}

    asio::awaitable<void> handle_request(const http::Request& request, http::Reply& reply) override;

private:
    RpcApi rpc_api_;
    const RpcApiTable& rpc_api_table_;
};

class RpcApiHandlerFactory : public http::RequestHandlerFactory {
public:
    RpcApiHandlerFactory(const RpcApiHandlerFactory&) = delete;
    RpcApiHandlerFactory& operator=(const RpcApiHandlerFactory&) = delete;

    explicit RpcApiHandlerFactory(const RpcApiTable& rpc_api_table) : rpc_api_table_(rpc_api_table) {}
    virtual ~RpcApiHandlerFactory() {}

    std::unique_ptr<http::RequestHandler> make_request_handler(Context& context, asio::thread_pool& workers) override;

private:
    const RpcApiTable& rpc_api_table_;
};

} // namespace silkrpc::commands

#endif // SILKRPC_COMMANDS_RPC_API_HANDLER_HPP_
