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

#include <cstring>
#include <memory>
#include <string>
#include <utility>

#include <asio/co_spawn.hpp>
#include <asio/dispatch.hpp>
#include <asio/use_awaitable.hpp>

#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/http/connection.hpp>
#include <silkrpc/http/methods.hpp>

namespace silkrpc::http {

std::tuple<std::string, std::string> Server::parse_endpoint(const std::string& tcp_end_point) {
    const auto host = tcp_end_point.substr(0, tcp_end_point.find(kAddressPortSeparator));
    const auto port = tcp_end_point.substr(tcp_end_point.find(kAddressPortSeparator) + 1, std::string::npos);
    return {host, port};
}

void Server::build_handlers(const std::string& api_spec) {
    auto start = 0u;
    auto end = api_spec.find(kApiSpecSeparator);
    while (end != std::string::npos) {
        add_handlers(api_spec.substr(start, end - start));
        start = end + std::strlen(kApiSpecSeparator);
        end = api_spec.find(kApiSpecSeparator, start);
    }
    add_handlers(api_spec.substr(start, end));
}

void Server::add_handlers(const std::string& api_namespace) {
    if (api_namespace == kDebugApiNamespace) {
        RequestHandler::add_debug_handlers();
    }
    else if (api_namespace == kEthApiNamespace) {
        RequestHandler::add_eth_handlers();
    }
    else if (api_namespace == kNetApiNamespace) {
        RequestHandler::add_net_handlers();
    }
    else if (api_namespace == kParityApiNamespace) {
        RequestHandler::add_parity_handlers();
    }
    else if (api_namespace == kTgApiNamespace) {
        RequestHandler::add_tg_handlers();
    }
    else if (api_namespace == kTraceApiNamespace) {
        RequestHandler::add_trace_handlers();
    }
    else if (api_namespace == kWeb3ApiNamespace) {
        RequestHandler::add_web3_handlers();
    } else {
        SILKRPC_WARN << "Server::add_handlers invalid namespace [" << api_namespace << "] ignored\n";
    }
}

Server::Server(const std::string& end_point, const std::string& api_spec, ContextPool& context_pool, std::size_t num_workers)
: context_pool_(context_pool), acceptor_{context_pool.get_io_context()}, workers_{num_workers} {
    const auto [host, port] = parse_endpoint(end_point);

    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    asio::ip::tcp::resolver resolver{acceptor_.get_executor()};
    asio::ip::tcp::endpoint endpoint = *resolver.resolve(host, port).begin();
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);

    // Activate JSON RPC API handlers based on the API namespace specification
    build_handlers(api_spec);
}

void Server::start() {
    asio::co_spawn(acceptor_.get_executor(), run(), [&](std::exception_ptr eptr) {
        if (eptr) std::rethrow_exception(eptr);
    });
}

asio::awaitable<void> Server::run() {
    acceptor_.listen();

    try {
        while (acceptor_.is_open()) {
            // Get the next context to use chosen round-robin, then get both io_context *and* database from it
            auto& context = context_pool_.get_context();
            auto& io_context = context.io_context;

            SILKRPC_DEBUG << "Server::start accepting using io_context " << io_context << "...\n" << std::flush;

            auto new_connection = std::make_shared<Connection>(context, workers_);
            co_await acceptor_.async_accept(new_connection->socket(), asio::use_awaitable);
            if (!acceptor_.is_open()) {
                SILKRPC_TRACE << "Server::start returning...\n";
                co_return;
            }

            new_connection->socket().set_option(asio::ip::tcp::socket::keep_alive(true));

            SILKRPC_TRACE << "Server::start starting connection for socket: " << &new_connection->socket() << "\n";
            auto new_connection_starter = [=]() -> asio::awaitable<void> { co_await new_connection->start(); };

            // https://github.com/chriskohlhoff/asio/issues/552
            asio::dispatch(*io_context, [=]() mutable {
                asio::co_spawn(*io_context, new_connection_starter, [&](std::exception_ptr eptr) {
                    if (eptr) std::rethrow_exception(eptr);
                });
            });
        }
    } catch (const std::system_error& se) {
        if (se.code() != asio::error::operation_aborted) {
            SILKRPC_ERROR << "Server::start system_error: " << se.what() << "\n" << std::flush;
            std::rethrow_exception(std::make_exception_ptr(se));
        } else {
            SILKRPC_DEBUG << "Server::start operation_aborted: " << se.what() << "\n" << std::flush;
        }
    }
    SILKRPC_DEBUG << "Server::start exiting...\n" << std::flush;
}

void Server::stop() {
    // The server is stopped by cancelling all outstanding asynchronous operations.
    SILKRPC_DEBUG << "Server::stop started...\n";
    acceptor_.close();
    SILKRPC_DEBUG << "Server::stop completed\n" << std::flush;
}

} // namespace silkrpc::http
