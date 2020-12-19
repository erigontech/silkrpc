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

#ifndef HTTP_SERVER2_HPP
#define HTTP_SERVER2_HPP

//#include <coroutine>
#include <string>

#define ASIO_HAS_CO_AWAIT
#define ASIO_HAS_STD_COROUTINE
#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <asio/use_awaitable.hpp>

#include <silkrpc/coro/coroutine.hpp>
//#include <silkrpc/coro/task.hpp>

#include "connection.hpp"
#include "connection_manager.hpp"
#include "request_handler.hpp"

namespace silkrpc::http {

/// The top-level class of the HTTP server.
class Server
{
public:
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /// Construct the server to listen on the specified TCP address and port, and
    /// serve up files from the given directory.
    explicit Server(asio::io_context& io_context, const std::string& address, const std::string& port, const std::string& target);

    asio::awaitable<void> start();

    void stop();

private:
  /// The io_context used to perform asynchronous operations.
  asio::io_context& io_context_;

  /// The signal_set is used to register for process termination notifications.
  //asio::signal_set& signals_;

  /// Acceptor used to listen for incoming connections.
  asio::ip::tcp::acceptor acceptor_;

  /// The connection manager which owns all live connections.
  ConnectionManager connection_manager_;

  /// The handler for all incoming requests.
  RequestHandler request_handler_;
};

} // namespace silkrpc::http

#endif // HTTP_SERVER2_HPP
