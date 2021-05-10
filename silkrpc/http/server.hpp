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

#include <silkrpc/config.hpp>

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>

#include <silkrpc/context_pool.hpp>

namespace silkrpc::http {

/// The top-level class of the HTTP server.
class Server {
public:
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // Construct the server to listen on the specified TCP address and port
    explicit Server(const std::string& address, const std::string& port, ContextPool& context_pool);

    void start();

    void stop();

private:
    asio::awaitable<void> run();

    // The context pool used to perform asynchronous operations
    ContextPool& context_pool_;

    // The acceptor used to listen for incoming TCP connections
    asio::ip::tcp::acceptor acceptor_;
};

} // namespace silkrpc::http

#endif // SILKRPC_HTTP_SERVER_HPP_
