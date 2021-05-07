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

#ifndef SILKRPC_HTTP_CONNECTION_MANAGER_HPP_
#define SILKRPC_HTTP_CONNECTION_MANAGER_HPP_

#include <memory>
#include <mutex>
#include <set>

#include <silkrpc/config.hpp>

#include <asio/awaitable.hpp>

#include "connection.hpp"

namespace silkrpc::http {

/// Manages open connections so that they may be cleanly stopped when the server
/// needs to shut down.
class ConnectionManager {
public:
    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    // Construct a connection manager.
    ConnectionManager() = default;

    // Add the specified connection to the manager and start it
    asio::awaitable<void> start(std::shared_ptr<Connection> c);

    // Stop the specified connection
    void stop(std::shared_ptr<Connection> c);

    // Stop all connections
    void stop_all();

private:
    // The managed connections
    std::set<std::shared_ptr<Connection>> connections_;

    // The mutual exclusion for thread-safe access to managed connections
    std::mutex connections_mutex_;
};

} // namespace silkrpc::http

#endif // SILKRPC_HTTP_CONNECTION_MANAGER_HPP_
