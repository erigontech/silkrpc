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

#include <memory>
#include <string>
#include <utility>

#include <asio/co_spawn.hpp>
#include <asio/ip/tcp.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/http/request_handler.hpp>

namespace silkrpc::http {

Server::Server(io_context_pool& io_context_pool, const std::string& address, const std::string& port, std::unique_ptr<ethdb::kv::Database>& database)
: io_context_pool_(io_context_pool), acceptor_{io_context_pool_.get_io_context()}, database_(database) {
    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    asio::ip::tcp::resolver resolver{acceptor_.get_executor()};
    asio::ip::tcp::endpoint endpoint = *resolver.resolve(address, port).begin();
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
}

void Server::start() {
    asio::co_spawn(acceptor_.get_executor(), run(), [&](std::exception_ptr eptr) {
        if (eptr) std::rethrow_exception(eptr);
    });
}

asio::awaitable<void> Server::run() {
    acceptor_.listen();

    while (acceptor_.is_open()) {
        SILKRPC_DEBUG << "Server::start accepting...\n" << std::flush;

        auto& new_io_context = io_context_pool_.get_io_context();

        auto new_connection = std::make_shared<Connection>(new_io_context, connection_manager_, database_);
        co_await acceptor_.async_accept(new_connection->socket(), asio::use_awaitable);
        new_connection->socket().set_option(asio::ip::tcp::socket::keep_alive(true));
        SILKRPC_TRACE << "Server::start new socket: " << &new_connection->socket() << "\n";
        if (!acceptor_.is_open()) {
            SILKRPC_TRACE << "Server::start returning...\n";
            co_return;
        }

        asio::co_spawn(new_io_context, connection_manager_.start(new_connection), [&](std::exception_ptr eptr) {
            if (eptr) std::rethrow_exception(eptr);
        });
    }
    SILKRPC_DEBUG << "Server::start exiting...\n" << std::flush;
}

void Server::stop() {
    // The server is stopped by cancelling all outstanding asynchronous operations.
    acceptor_.close(); // TODO: should not be necessary
    connection_manager_.stop_all();
}

} // namespace silkrpc::http
