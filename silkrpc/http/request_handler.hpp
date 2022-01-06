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

#include <memory>

#include <silkrpc/config.hpp>

#include <asio/awaitable.hpp>
#include <asio/thread_pool.hpp>

#include <silkrpc/context_pool.hpp>

namespace silkrpc::http {

struct Reply;
struct Request;

class RequestHandler {
public:
    RequestHandler() = default;
    virtual ~RequestHandler() {}

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    virtual asio::awaitable<void> handle_request(const Request& request, Reply& reply) = 0;
};

class RequestHandlerFactory {
public:
    RequestHandlerFactory() = default;
    virtual ~RequestHandlerFactory() {}

    virtual std::unique_ptr<RequestHandler> make_request_handler(Context& context, asio::thread_pool& workers) = 0;
};

} // namespace silkrpc::http

#endif // SILKRPC_HTTP_REQUEST_HANDLER_HPP_
