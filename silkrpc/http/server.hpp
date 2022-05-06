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
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SILKRPC_HTTP_SERVER_HPP_
#define SILKRPC_HTTP_SERVER_HPP_

#include <string>
#include <tuple>
#include <vector>

#include <silkrpc/config.hpp>

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/thread_pool.hpp>

#include <silkrpc/context_pool.hpp>
#include <silkrpc/http/request_handler.hpp>

#include <silkrpc/commands/rpc_api_table.hpp>

namespace silkrpc::http {

/// The top-level class of the HTTP server.
class Server {
public:
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // Construct the server to listen on the specified local TCP end-point
    explicit Server(const std::string& end_point, const std::string& api_spec, ContextPool& context_pool, asio::thread_pool& workers, std::string jwt_token);

    void start();

    void stop();

private:
    static std::tuple<std::string, std::string> parse_endpoint(const std::string& tcp_end_point);

    asio::awaitable<void> run();

    // The repository of API request handlers
    commands::RpcApiTable handler_table_;

    // The context pool used to perform asynchronous operations
    ContextPool& context_pool_;

    // The acceptor used to listen for incoming TCP connections
    asio::ip::tcp::acceptor acceptor_;

    asio::thread_pool& workers_;

    // jwt token
    std::string jwt_token_;
};

} // namespace silkrpc::http

#endif // SILKRPC_HTTP_SERVER_HPP_
