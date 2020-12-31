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

#ifndef HTTP_REQUEST_HANDLER_HPP
#define HTTP_REQUEST_HANDLER_HPP

#include <map>
#include <memory>
#include <string>

#define ASIO_HAS_CO_AWAIT
#define ASIO_HAS_STD_COROUTINE
#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <grpcpp/grpcpp.h>
#include <nlohmann/json.hpp>

#include <silkrpc/kv/remote_client.hpp>

namespace silkrpc::http {

struct Reply;
struct Request;

class RequestHandler {
public:
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    explicit RequestHandler(asio::io_context& io_context, const std::string& target) : io_context_(io_context) {
        grpc_channel_ = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    }
    virtual ~RequestHandler() {}

    asio::awaitable<void> handle_request(const Request& request, Reply& reply);

private:
    asio::awaitable<void> handle_eth_block_number(const nlohmann::json& request, nlohmann::json& reply);

    asio::io_context& io_context_;
    std::shared_ptr<grpc::Channel> grpc_channel_;

    typedef asio::awaitable<void> (RequestHandler::*HandleMethod)(const nlohmann::json&, nlohmann::json&);
    static std::map<std::string, HandleMethod> handlers_;
};

} // namespace silkrpc::http

#endif // HTTP_REQUEST_HANDLER_HPP
