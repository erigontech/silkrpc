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

#include <asio/signal_set.hpp>
#include <utility>

namespace silkrpc::http {

Server::Server(asio::io_context& io_context, const std::string& address, const std::string& port)
  : io_context_(io_context), acceptor_(io_context_), connection_manager_(), request_handler_() {
    // Register to handle the signals that indicate when the server should exit.
    // It is safe to register for the same signal multiple times in a program,
    // provided all registration for the specified signal is made through Asio.
    //signals.add(SIGINT);
    //signals.add(SIGTERM);
    //#if defined(SIGQUIT)
    //signals.add(SIGQUIT);
    //#endif // defined(SIGQUIT)

    /*signals.async_wait(
        [this](std::error_code ec, int signo) {
            // The server is stopped by cancelling all outstanding asynchronous operations.
            // Once all operations have finished the io_context::run() call will exit.
            acceptor_.close();
            connection_manager_.stop_all();
        }
    );*/

    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    asio::ip::tcp::resolver resolver(io_context_);
    asio::ip::tcp::endpoint endpoint = *resolver.resolve(address, port).begin();
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    do_accept();
}

/*void server::run()
{
  // The io_context::run() call will block until all asynchronous operations
  // have finished. While the server is running, there is always at least one
  // asynchronous operation outstanding: the asynchronous accept call waiting
  // for new incoming connections.
  io_context_.run();
}*/

void Server::do_accept()
{
    acceptor_.async_accept(
        [this](std::error_code ec, asio::ip::tcp::socket socket) {
            // Check whether the server was stopped by a signal before this completion handler had a chance to run.
            if (!acceptor_.is_open()) {
                return;
            }

            if (!ec) {
                auto new_connection = std::make_shared<Connection>(std::move(socket), connection_manager_, request_handler_);
                connection_manager_.start(new_connection);
            }

            do_accept();
      });
}

void Server::stop() {
    // The server is stopped by cancelling all outstanding asynchronous operations.
    acceptor_.close();
    connection_manager_.stop_all();
}

/*void Server::do_await_stop()
{
    signals_.async_wait(
        [this](std::error_code ec, int signo) {
            // The server is stopped by cancelling all outstanding asynchronous operations.
            // Once all operations have finished the io_context::run() call will exit.
            acceptor_.close();
            connection_manager_.stop_all();
        }
    );
}*/

} // namespace silkrpc::http
