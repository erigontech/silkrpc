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

#include "server.hpp"

#include <string>

#include <asio/ip/tcp.hpp>
#include <asio/steady_timer.hpp>

namespace silkrpc::http {

Server::Server(asio::io_context& io_context, const std::string& address, const std::string& port, const std::string& target)
  : io_context_(io_context), acceptor_{io_context_}, request_handler_{io_context, target} {
    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    asio::ip::tcp::resolver resolver{io_context_};
    asio::ip::tcp::endpoint endpoint = *resolver.resolve(address, port).begin();
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
}

asio::awaitable<void> Server::start() {
    acceptor_.listen();

    while (acceptor_.is_open()) {
        auto socket = co_await acceptor_.async_accept(asio::use_awaitable);
        if (!acceptor_.is_open()) {
            co_return;
        }

        auto new_connection = std::make_shared<Connection>(std::move(socket), connection_manager_, request_handler_);
        connection_manager_.start(new_connection);

        asio::steady_timer timer{io_context_, asio::chrono::seconds(1)};
        co_await timer.async_wait(asio::use_awaitable);
    }
}

void Server::stop() {
    // The server is stopped by cancelling all outstanding asynchronous operations.
    acceptor_.close();
    connection_manager_.stop_all();
}

} // namespace silkrpc::http
