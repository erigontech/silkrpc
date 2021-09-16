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

#include <exception>
#include <system_error>
#include <string_view>
#include <utility>
#include <vector>

#include <asio/write.hpp>
#include <asio/use_awaitable.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/ethdb/database.hpp>
#include "request_handler.hpp"

namespace silkrpc::http {

Connection::Connection(Context& context, asio::thread_pool& workers) : socket_{*context.io_context}, request_handler_{context, workers} {
    request_.content.reserve(1024);
    request_.headers.reserve(8);
    request_.method.reserve(64);
    request_.uri.reserve(64);
    SILKRPC_DEBUG << "Connection::Connection socket " << &socket_ << " created\n";
}

Connection::~Connection() {
    socket_.close();
    SILKRPC_DEBUG << "Connection::~Connection socket " << &socket_ << " deleted\n";
}

asio::awaitable<void> Connection::start() {
    co_await do_read();
}

asio::awaitable<void> Connection::do_read() {
    try {
        SILKRPC_DEBUG << "Connection::do_read going to read...\n" << std::flush;
        std::size_t bytes_read = co_await socket_.async_read_some(asio::buffer(buffer_), asio::use_awaitable);
        SILKRPC_DEBUG << "Connection::do_read bytes_read: " << bytes_read << "\n";
        SILKRPC_TRACE << "Connection::do_read buffer: " << std::string_view{static_cast<const char*>(buffer_.data()), bytes_read} << "\n";


        RequestParser::ResultType result = request_parser_.parse(request_, buffer_.data(), buffer_.data() + bytes_read);

        if (result == RequestParser::good) {
            co_await request_handler_.handle_request(request_, reply_);
            co_await do_write();
        } else if (result == RequestParser::bad) {
            reply_ = Reply::stock_reply(Reply::bad_request);
            co_await do_write();
        }

        // Read next chunck (result == RequestParser::indeterminate) or next request
        co_await do_read();
    } catch (const std::system_error& se) {
        if (se.code() == asio::error::eof || se.code() == asio::error::connection_reset || se.code() == asio::error::broken_pipe) {
            SILKRPC_DEBUG << "Connection::do_read close from client with code: " << se.code() << "\n" << std::flush;
        } else if (se.code() != asio::error::operation_aborted) {
            SILKRPC_ERROR << "Connection::do_read system_error: " << se.what() << "\n" << std::flush;
            std::rethrow_exception(std::make_exception_ptr(se));
        } else {
            SILKRPC_DEBUG << "Connection::do_read operation_aborted: " << se.what() << "\n" << std::flush;
        }
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "Connection::do_read exception: " << e.what() << "\n" << std::flush;
        std::rethrow_exception(std::make_exception_ptr(e));
    }
}

asio::awaitable<void> Connection::do_write() {
    try {
        SILKRPC_DEBUG << "Connection::do_write reply: " << reply_.content << "\n" << std::flush;
        const auto bytes_transferred = co_await asio::async_write(socket_, reply_.to_buffers(), asio::use_awaitable);
        SILKRPC_TRACE << "Connection::do_write bytes_transferred: " << bytes_transferred << "\n" << std::flush;
        request_.reset();
        request_parser_.reset();
        reply_.reset();
    } catch (const std::system_error& se) {
        std::rethrow_exception(std::make_exception_ptr(se));
    } catch (const std::exception& e) {
        std::rethrow_exception(std::make_exception_ptr(e));
    }
}

} // namespace silkrpc::http
