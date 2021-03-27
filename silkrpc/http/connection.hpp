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

#ifndef SILKRPC_HTTP_CONNECTION_HPP_
#define SILKRPC_HTTP_CONNECTION_HPP_

#include <array>
#include <memory>

#include <silkrpc/config.hpp>

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

#include "reply.hpp"
#include "request.hpp"
#include "request_handler.hpp"
#include "request_parser.hpp"

namespace silkrpc::http {

class ConnectionManager;

/// Represents a single connection from a client.
class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    /// Construct a connection with the given socket.
    explicit Connection(asio::io_context& io_context, ConnectionManager& manager, std::unique_ptr<ethdb::kv::Database>& database);

    ~Connection();

    asio::ip::tcp::socket& socket() { return socket_; };

    /// Start the first asynchronous operation for the connection.
    asio::awaitable<void> start();

    /// Stop all asynchronous operations associated with the connection.
    void stop();

private:
    /// Perform an asynchronous read operation.
    asio::awaitable<void> do_read();

    /// Perform an asynchronous write operation.
    asio::awaitable<void> do_write();

    /// Socket for the connection.
    asio::ip::tcp::socket socket_;

    /// The manager for this connection.
    ConnectionManager& connection_manager_;

    /// The handler used to process the incoming request.
    RequestHandler request_handler_;

    /// Buffer for incoming data.
    std::array<char, 8192> buffer_;

    /// The incoming request.
    Request request_;

    /// The parser for the incoming request.
    RequestParser request_parser_;

    /// The reply to be sent back to the client.
    Reply reply_;
};

} // namespace silkrpc::http

#endif // SILKRPC_HTTP_CONNECTION_HPP_
