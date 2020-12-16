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

#include "connection.hpp"

#include <utility>
#include <vector>

#include "connection_manager.hpp"
#include "request_handler.hpp"

namespace silkrpc::http {

Connection::Connection(asio::ip::tcp::socket socket, ConnectionManager& manager, RequestHandler& handler)
  : socket_(std::move(socket)), connection_manager_(manager), request_handler_(handler) {}

void Connection::start() {
    do_read();
}

void Connection::stop() {
    socket_.close();
}

void Connection::do_read() {
    auto self(shared_from_this());

    socket_.async_read_some(
        asio::buffer(buffer_),
        [this, self](std::error_code ec, std::size_t bytes_transferred) {
            if (!ec) {
                RequestParser::ResultType result;
                std::tie(result, std::ignore) = request_parser_.parse(
                    request_, buffer_.data(), buffer_.data() + bytes_transferred
                );

                if (result == RequestParser::good) {
                    request_handler_.handle_request(request_, reply_);
                    do_write();
                } else if (result == RequestParser::bad) {
                    reply_ = Reply::stock_reply(Reply::bad_request);
                    do_write();
                } else {
                    do_read();
                }
            } else if (ec != asio::error::operation_aborted) {
                connection_manager_.stop(shared_from_this());
            }
        }
    );
}

void Connection::do_write() {
    auto self(shared_from_this());
    socket_.async_write_some(reply_.to_buffers(),
    //asio::async_write(socket_, reply_.to_buffers(),
        [this, self](std::error_code ec, std::size_t) {
            if (!ec) {
                // Initiate graceful connection closure.
                asio::error_code ignored_ec;
                socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
            }

            if (ec != asio::error::operation_aborted) {
                connection_manager_.stop(shared_from_this());
            }
        }
    );
}

} // namespace silkrpc::http
