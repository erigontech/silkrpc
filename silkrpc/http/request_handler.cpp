/*
    Copyright 2020-2021 The Silkrpc Authors

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
#include <utility>

#include <nlohmann/json.hpp>

#include <silkrpc/common/clock_time.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/http/header.hpp>

namespace silkrpc::http {

boost::asio::awaitable<void> RequestHandler::handle_request(const http::Request& request, http::Reply& reply) {
    SILKRPC_DEBUG << "handle_request content: " << request.content << "\n";
    auto start = clock_time::now();

    auto request_id{0};
    try {
        if (request.content.empty()) {
            reply.content = "";
            reply.status = http::Reply::no_content;
            reply.headers.reserve(2);
            reply.headers.emplace_back(http::Header{"Content-Length", std::to_string(reply.content.size())});
            reply.headers.emplace_back(http::Header{"Content-Type", "application/json"});
            SILKRPC_INFO << "handle_request t=" << clock_time::since(start) << "ns\n";
            co_return;
        }

        const auto request_json = nlohmann::json::parse(request.content);
        request_id = request_json["id"].get<uint32_t>();
        if (!request_json.contains("method")) {
            reply.content = make_json_error(request_id, -32600, "method missing").dump() + "\n";
            reply.status = http::Reply::bad_request;
            reply.headers.reserve(2);
            reply.headers.emplace_back(http::Header{"Content-Length", std::to_string(reply.content.size())});
            reply.headers.emplace_back(http::Header{"Content-Type", "application/json"});
            SILKRPC_INFO << "handle_request t=" << clock_time::since(start) << "ns\n";
            co_return;
        }

        const auto method = request_json["method"].get<std::string>();
        const auto handle_method_opt = rpc_api_table_.find_handler(method);
        if (!handle_method_opt) {
            reply.content = make_json_error(request_id, -32601, "the method " + method + " does not exist/is not available").dump() + "\n";
            reply.status = http::Reply::not_implemented;
            reply.headers.reserve(2);
            reply.headers.emplace_back(http::Header{"Content-Length", std::to_string(reply.content.size())});
            reply.headers.emplace_back(http::Header{"Content-Type", "application/json"});
            SILKRPC_INFO << "handle_request t=" << clock_time::since(start) << "ns\n";
            co_return;
        }
        const auto handle_method = handle_method_opt.value();

        nlohmann::json reply_json;
        co_await (rpc_api_.*handle_method)(request_json, reply_json);

        reply.content = reply_json.dump(
            /*indent=*/-1, /*indent_char=*/' ', /*ensure_ascii=*/false, nlohmann::json::error_handler_t::replace) + "\n";
        reply.status = http::Reply::ok;
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << "\n";
        reply.content = make_json_error(request_id, 100, e.what()).dump() + "\n";
        reply.status = http::Reply::internal_server_error;
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception\n";
        reply.content = make_json_error(request_id, 100, "unexpected exception").dump() + "\n";
        reply.status = http::Reply::internal_server_error;
    }

    reply.headers.reserve(2);
    reply.headers.emplace_back(http::Header{"Content-Length", std::to_string(reply.content.size())});
    reply.headers.emplace_back(http::Header{"Content-Type", "application/json"});

    SILKRPC_INFO << "handle_request t=" << clock_time::since(start) << "ns\n";
    co_return;
}

} // namespace silkrpc::http
