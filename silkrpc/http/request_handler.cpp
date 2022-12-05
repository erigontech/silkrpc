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
#include <vector>

#include <jwt-cpp/jwt.h>
#include <boost/asio/write.hpp>
#include <nlohmann/json.hpp>

#include <silkrpc/common/clock_time.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/http/header.hpp>

namespace silkrpc::http {

boost::asio::awaitable<void> RequestHandler::handle_request(const http::Request& request) {
    auto start = clock_time::now();

    http::Reply reply;
    if (request.content.empty()) {
        reply.content = "";
        reply.status = http::StatusType::no_content;
    } else {
        SILKRPC_DEBUG << "handle_request content: " << request.content << "\n";

        const auto request_json = nlohmann::json::parse(request.content);
        auto request_id = request_json["id"].get<uint32_t>();
        if (!request_json.contains("method")) {
            reply.content = make_json_error(request_id, -32600, "method missing").dump() + "\n";
            reply.status = http::StatusType::bad_request;
        } else {
            const auto error = co_await is_request_authorized(request_id, request);
            if (error.has_value()) {
                reply.content = make_json_error(request_id, 403, error.value()).dump() + "\n";
                reply.status = http::StatusType::unauthorized;
            } else {
                co_await handle_request(request_json, reply);
            }
        }
    }

    co_await do_write(reply);

    SILKRPC_INFO << "handle_request t=" << clock_time::since(start) << "ns\n";
}

boost::asio::awaitable<std::optional<std::string>> RequestHandler::is_request_authorized(uint32_t request_id, const http::Request& request) {
    if (!jwt_secret_.has_value()) {
        co_return std::nullopt;
    }

    SILKRPC_LOG << "JWT jwt_secret_: " << jwt_secret_.value() << "\n";

    const auto it = std::find_if(request.headers.begin(), request.headers.end(), [&](const Header& h){
        return h.name == "Authorization";
    });

    if (it == request.headers.end()) {
        SILKRPC_ERROR << "JWT request without Authorization in auth connection\n";
        co_return "missing Authorization Header";
    }

    std::string client_token;
    if (it->value.substr(0, 7) == "Bearer ") {
        client_token = it->value.substr(7);
    } else {
        SILKRPC_ERROR << "JWT client request without token\n";
        co_return "missing token";
    }

    SILKRPC_LOG << "JWT client_token: " << client_token << "\n";

    try {
        // Parse token
        auto decoded_client_token = jwt::decode(client_token);
        if (decoded_client_token.has_issued_at() == 0) {
            SILKRPC_ERROR << "JWT iat (Issued At) not defined: \n";
            co_return "iat(Issued At) not defined";
        }
        // Validate token
        auto verifier = jwt::verify().allow_algorithm(jwt::algorithm::hs256{*jwt_secret_});

        SILKRPC_TRACE << "jwt client token: " << client_token << " jwt_secret: " << *jwt_secret_ << "\n";
        verifier.verify(decoded_client_token);
    } catch (const std::system_error& se) {
        SILKRPC_ERROR << "JWT invalid token: " << se.what() << "\n";
        co_return "invalid token";
    } catch (const std::exception& se) {
        SILKRPC_ERROR << "JWT invalid token: " << se.what() << "\n";
        co_return "invalid token";
    }

    co_return std::nullopt;
}

boost::asio::awaitable<void> RequestHandler::handle_request(const nlohmann::json& request_json, http::Reply& reply) {
    const auto method = request_json["method"].get<std::string>();
    const auto json_handler_opt = rpc_api_table_.find_json_handler(method);
    if (json_handler_opt) {
        const auto json_handler = json_handler_opt.value();

        nlohmann::json reply_json;
        co_await (rpc_api_.*json_handler)(request_json, reply_json);

        reply.content = reply_json.dump(
            /*indent=*/-1, /*indent_char=*/' ', /*ensure_ascii=*/false, nlohmann::json::error_handler_t::replace) + "\n";
        reply.status = http::StatusType::ok;
        co_return;
    }

    const auto stream_handler_opt = rpc_api_table_.find_stream_handler(method);
    if (stream_handler_opt) {
        const auto stream_handler = stream_handler_opt.value();

        json::Stream stream(socket_);
        co_await (rpc_api_.*stream_handler)(request_json, stream);
        co_return;
    }

    auto request_id = request_json["id"].get<uint32_t>();
    reply.content = make_json_error(request_id, -32601, "the method " + method + " does not exist/is not available").dump() + "\n";
    reply.status = http::StatusType::not_implemented;

    co_return;
}

boost::asio::awaitable<void> RequestHandler::handle_request(silkrpc::commands::RpcApiTable::HandleJson handler, const nlohmann::json& request_json, http::Reply& reply) {
    auto request_id = request_json["id"].get<uint32_t>();
    try {
        nlohmann::json reply_json;
        co_await (rpc_api_.*handler)(request_json, reply_json);

        reply.content = reply_json.dump(
            /*indent=*/-1, /*indent_char=*/' ', /*ensure_ascii=*/false, nlohmann::json::error_handler_t::replace) + "\n";
        reply.status = http::StatusType::ok;
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << "\n";
        reply.content = make_json_error(request_id, 100, e.what()).dump() + "\n";
        reply.status = http::StatusType::internal_server_error;
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception\n";
        reply.content = make_json_error(request_id, 100, "unexpected exception").dump() + "\n";
        reply.status = http::StatusType::internal_server_error;
    }

    co_return;
}

boost::asio::awaitable<void> RequestHandler::handle_request(silkrpc::commands::RpcApiTable::HandleStream handler, const nlohmann::json& request_json, http::Reply& reply) {
    auto request_id = request_json["id"].get<uint32_t>();
    try {
        json::Stream stream(socket_);

        co_await write_headers();
        co_await (rpc_api_.*handler)(request_json, stream);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << "\n";
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception\n";
    }

    co_return;
}

boost::asio::awaitable<void> RequestHandler::do_write(Reply &reply) {
    try {
        SILKRPC_DEBUG << "RequestHandler::do_write reply: " << reply.content << "\n" << std::flush;

        reply.headers.reserve(2);
        reply.headers.emplace_back(http::Header{"Content-Length", std::to_string(reply.content.size())});
        reply.headers.emplace_back(http::Header{"Content-Type", "application/json"});

        const auto bytes_transferred = co_await boost::asio::async_write(socket_, reply.to_buffers(), boost::asio::use_awaitable);
        SILKRPC_TRACE << "RequestHandler::do_write bytes_transferred: " << bytes_transferred << "\n" << std::flush;
    } catch (const std::system_error& se) {
        std::rethrow_exception(std::make_exception_ptr(se));
    } catch (const std::exception& e) {
        std::rethrow_exception(std::make_exception_ptr(e));
    }
}

boost::asio::awaitable<void> RequestHandler::write_headers() {
    try {
        std::vector<http::Header> headers;
        headers.reserve(2);
        headers.emplace_back(http::Header{"Content-Length", 0});
        headers.emplace_back(http::Header{"Content-Type", "application/json"});

        const auto bytes_transferred = co_await boost::asio::async_write(socket_, http::to_buffers(headers), boost::asio::use_awaitable);
    } catch (const std::system_error& se) {
        std::rethrow_exception(std::make_exception_ptr(se));
    } catch (const std::exception& e) {
        std::rethrow_exception(std::make_exception_ptr(e));
    }
}

} // namespace silkrpc::http
