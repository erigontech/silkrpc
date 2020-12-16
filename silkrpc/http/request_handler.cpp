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

#include "request_handler.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "methods.hpp"
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"

namespace silkrpc::http {

std::map<std::string, RequestHandler::HandleMethod> RequestHandler::handlers_ = {
    {method::k_eth_blockNumber, &RequestHandler::handle_eth_block_number},
};

void RequestHandler::handle_request(const Request& request, Reply& reply) {
    try {
        std::cout << "RequestHandler thread: " << std::this_thread::get_id() << "\n" << std::flush;

        if (request.content.empty()) {
            reply.content = "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":\"content missing\"}";
            reply.status = Reply::no_content;
        }

        auto request_json = nlohmann::json::parse(request.content);
        if (!request_json.contains("method")) {
            reply.content = "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":\"method missing\"}";
            reply.status = Reply::bad_request;
        }

        auto method = request_json["method"].get<std::string>();
        if (RequestHandler::handlers_.find(method) == RequestHandler::handlers_.end()) {
            reply.content = "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":\"method not implemented\"}";
            reply.status = Reply::not_implemented;
        }

        nlohmann::json reply_json;
        auto handle_method = RequestHandler::handlers_[method];
        std::invoke(handle_method, this, request_json, reply_json);

        reply.content = reply_json.dump();
        reply.status = Reply::ok;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";

        std::stringstream rc_stream;
        rc_stream << "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":\"" << e.what() << "\"}";
        reply.content = rc_stream.str();
        reply.status = Reply::internal_server_error;
    }

    reply.headers.resize(2);
    reply.headers.emplace_back(Header{"Content-Length", std::to_string(reply.content.size())});
    reply.headers.emplace_back(Header{"Content-Type", "application/json"});
    //reply.headers[0].name = "Content-Length";
    //reply.headers[0].value = std::to_string(reply.content.size());
    //reply.headers[1].name = "Content-Type";
    //reply.headers[1].value = "application/json";
}

void RequestHandler::handle_eth_block_number(const nlohmann::json& request, nlohmann::json& reply) const {
    // TODO use Silkworm to retrieve the latest block number

    // TODO: define namespace for JSON RPC structs (eth::jsonrpc), then use arbitrary type conv
    reply = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x3a9e4b\"}";
}

} // namespace silkrpc::http
