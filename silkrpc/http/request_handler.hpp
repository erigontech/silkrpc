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

#ifndef SILKRPC_HTTP_REQUEST_HANDLER_HPP_
#define SILKRPC_HTTP_REQUEST_HANDLER_HPP_

#include <map>
#include <memory>
#include <string>

#include <silkrpc/config.hpp>

#include <asio/awaitable.hpp>

#include <silkrpc/commands/rpc_api.hpp>
#include <silkrpc/ethdb/database.hpp>

namespace silkrpc::http {

struct Reply;
struct Request;

class RequestHandler {
public:
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    explicit RequestHandler(std::unique_ptr<ethdb::Database>& database) : rpc_api_{database} {}

    virtual ~RequestHandler() {}

    asio::awaitable<void> handle_request(const Request& request, Reply& reply);

private:
    commands::RpcApi rpc_api_;

    typedef asio::awaitable<void> (commands::RpcApi::*HandleMethod)(const nlohmann::json&, nlohmann::json&);
    static std::map<std::string, HandleMethod> handlers_;
};

} // namespace silkrpc::http

#endif // SILKRPC_HTTP_REQUEST_HANDLER_HPP_
