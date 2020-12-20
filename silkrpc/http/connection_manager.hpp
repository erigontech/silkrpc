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

#ifndef HTTP_ConnectionManager_HPP
#define HTTP_ConnectionManager_HPP

#include <set>

#include "connection.hpp"

#define ASIO_HAS_CO_AWAIT
#define ASIO_HAS_STD_COROUTINE
#include <asio/awaitable.hpp>

namespace silkrpc::http {

/// Manages open connections so that they may be cleanly stopped when the server
/// needs to shut down.
class ConnectionManager
{
public:
  ConnectionManager(const ConnectionManager&) = delete;
  ConnectionManager& operator=(const ConnectionManager&) = delete;

  /// Construct a connection manager.
  ConnectionManager();

  /// Add the specified connection to the manager and start it.
  asio::awaitable<void> start(std::shared_ptr<Connection> c);

  /// Stop the specified connection.
  void stop(std::shared_ptr<Connection> c);

  /// Stop all connections.
  void stop_all();

private:
  /// The managed connections.
  std::set<std::shared_ptr<Connection>> connections_;
};

} // namespace silkrpc::http

#endif // HTTP_ConnectionManager_HPP
