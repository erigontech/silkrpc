//
// Reply.cpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <string>

#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>

#include "reply.hpp"

namespace silkrpc::http {

namespace status_strings {

const std::string ok = "HTTP/1.1 200 OK\r\n";                                       // NOLINT(runtime/string)
const std::string created = "HTTP/1.1 201 Created\r\n";                             // NOLINT(runtime/string)
const std::string accepted = "HTTP/1.1 202 Accepted\r\n";                           // NOLINT(runtime/string)
const std::string no_content = "HTTP/1.1 204 No Content\r\n";                       // NOLINT(runtime/string)
const std::string multiple_choices = "HTTP/1.1 300 Multiple Choices\r\n";           // NOLINT(runtime/string)
const std::string moved_permanently = "HTTP/1.1 301 Moved Permanently\r\n";         // NOLINT(runtime/string)
const std::string moved_temporarily = "HTTP/1.1 302 Moved Temporarily\r\n";         // NOLINT(runtime/string)
const std::string not_modified = "HTTP/1.1 304 Not Modified\r\n";                   // NOLINT(runtime/string)
const std::string bad_request = "HTTP/1.1 400 Bad Request\r\n";                     // NOLINT(runtime/string)
const std::string unauthorized = "HTTP/1.1 401 Unauthorized\r\n";                   // NOLINT(runtime/string)
const std::string forbidden = "HTTP/1.1 403 Forbidden\r\n";                         // NOLINT(runtime/string)
const std::string not_found = "HTTP/1.1 404 Not Found\r\n";                         // NOLINT(runtime/string)
const std::string internal_server_error = "HTTP/1.1 500 Internal Server Error\r\n"; // NOLINT(runtime/string)
const std::string not_implemented = "HTTP/1.1 501 Not Implemented\r\n";             // NOLINT(runtime/string)
const std::string bad_gateway = "HTTP/1.1 502 Bad Gateway\r\n";                     // NOLINT(runtime/string)
const std::string service_unavailable = "HTTP/1.1 503 Service Unavailable\r\n";     // NOLINT(runtime/string)
const std::string processing_continue = "HTTP/1.1 100 Continue\r\n";                // NOLINT(runtime/string)

boost::asio::const_buffer to_buffer(Reply::StatusType status) {
    switch (status) {
        case Reply::ok:
            return boost::asio::buffer(ok);
        case Reply::created:
            return boost::asio::buffer(created);
        case Reply::accepted:
            return boost::asio::buffer(accepted);
        case Reply::no_content:
            return boost::asio::buffer(no_content);
        case Reply::multiple_choices:
            return boost::asio::buffer(multiple_choices);
        case Reply::moved_permanently:
            return boost::asio::buffer(moved_permanently);
        case Reply::moved_temporarily:
            return boost::asio::buffer(moved_temporarily);
        case Reply::not_modified:
            return boost::asio::buffer(not_modified);
        case Reply::bad_request:
            return boost::asio::buffer(bad_request);
        case Reply::unauthorized:
            return boost::asio::buffer(unauthorized);
        case Reply::forbidden:
            return boost::asio::buffer(forbidden);
        case Reply::not_found:
            return boost::asio::buffer(not_found);
        case Reply::internal_server_error:
            return boost::asio::buffer(internal_server_error);
        case Reply::not_implemented:
            return boost::asio::buffer(not_implemented);
        case Reply::bad_gateway:
            return boost::asio::buffer(bad_gateway);
        case Reply::service_unavailable:
            return boost::asio::buffer(service_unavailable);
        case Reply::processing_continue:
            return boost::asio::buffer(processing_continue);
        default:
            return boost::asio::buffer(internal_server_error);
    }
}

} // namespace status_strings

namespace misc_strings {

const char name_value_separator[] = { ':', ' ' };
const char crlf[] = { '\r', '\n' };
const char lf[] = { '\n' };

} // namespace misc_strings

std::vector<boost::asio::const_buffer> Reply::to_buffers() {
    std::vector<boost::asio::const_buffer> buffers;
    buffers.reserve(1+headers.size()*4+2);
    buffers.push_back(status_strings::to_buffer(status));
    for (std::size_t i = 0; i < headers.size(); ++i) {
        Header& h = headers[i];
        buffers.push_back(boost::asio::buffer(h.name));
        buffers.push_back(boost::asio::buffer(misc_strings::name_value_separator));
        buffers.push_back(boost::asio::buffer(h.value));
        buffers.push_back(boost::asio::buffer(misc_strings::crlf));
    }
    buffers.push_back(boost::asio::buffer(misc_strings::crlf));
    buffers.push_back(boost::asio::buffer(content));
    SILKRPC_TRACE << "Reply::to_buffers buffers: " << buffers << "\n";
    return buffers;
}

namespace stock_replies {

const char ok[] = "";
const char processing_continue[] = "";
const char created[] =
    "<html>"
    "<head><title>Created</title></head>"
    "<body><h1>201 Created</h1></body>"
    "</html>";
const char accepted[] =
    "<html>"
    "<head><title>Accepted</title></head>"
    "<body><h1>202 Accepted</h1></body>"
    "</html>";
const char no_content[] =
    "<html>"
    "<head><title>No Content</title></head>"
    "<body><h1>204 Content</h1></body>"
    "</html>";
const char multiple_choices[] =
    "<html>"
    "<head><title>Multiple Choices</title></head>"
    "<body><h1>300 Multiple Choices</h1></body>"
    "</html>";
const char moved_permanently[] =
    "<html>"
    "<head><title>Moved Permanently</title></head>"
    "<body><h1>301 Moved Permanently</h1></body>"
    "</html>";
const char moved_temporarily[] =
    "<html>"
    "<head><title>Moved Temporarily</title></head>"
    "<body><h1>302 Moved Temporarily</h1></body>"
    "</html>";
const char not_modified[] =
    "<html>"
    "<head><title>Not Modified</title></head>"
    "<body><h1>304 Not Modified</h1></body>"
    "</html>";
const char bad_request[] =
    "<html>"
    "<head><title>Bad Request</title></head>"
    "<body><h1>400 Bad Request</h1></body>"
    "</html>";
const char unauthorized[] =
    "<html>"
    "<head><title>Unauthorized</title></head>"
    "<body><h1>401 Unauthorized</h1></body>"
    "</html>";
const char forbidden[] =
    "<html>"
    "<head><title>Forbidden</title></head>"
    "<body><h1>403 Forbidden</h1></body>"
    "</html>";
const char not_found[] =
    "<html>"
    "<head><title>Not Found</title></head>"
    "<body><h1>404 Not Found</h1></body>"
    "</html>";
const char internal_server_error[] =
    "<html>"
    "<head><title>Internal Server Error</title></head>"
    "<body><h1>500 Internal Server Error</h1></body>"
    "</html>";
const char not_implemented[] =
    "<html>"
    "<head><title>Not Implemented</title></head>"
    "<body><h1>501 Not Implemented</h1></body>"
    "</html>";
const char bad_gateway[] =
    "<html>"
    "<head><title>Bad Gateway</title></head>"
    "<body><h1>502 Bad Gateway</h1></body>"
    "</html>";
const char service_unavailable[] =
    "<html>"
    "<head><title>Service Unavailable</title></head>"
    "<body><h1>503 Service Unavailable</h1></body>"
    "</html>";

std::string to_string(Reply::StatusType status) {
    switch (status) {
        case Reply::processing_continue:
            return processing_continue;
        case Reply::ok:
            return ok;
        case Reply::created:
            return created;
        case Reply::accepted:
            return accepted;
        case Reply::no_content:
            return no_content;
        case Reply::multiple_choices:
            return multiple_choices;
        case Reply::moved_permanently:
            return moved_permanently;
        case Reply::moved_temporarily:
            return moved_temporarily;
        case Reply::not_modified:
            return not_modified;
        case Reply::bad_request:
            return bad_request;
        case Reply::unauthorized:
            return unauthorized;
        case Reply::forbidden:
            return forbidden;
        case Reply::not_found:
            return not_found;
        case Reply::internal_server_error:
            return internal_server_error;
        case Reply::not_implemented:
            return not_implemented;
        case Reply::bad_gateway:
            return bad_gateway;
        case Reply::service_unavailable:
            return service_unavailable;
        default:
            return internal_server_error;
    }
}

} // namespace stock_replies

Reply Reply::stock_reply(Reply::StatusType status) {
    Reply rep;
    rep.status = status;
    rep.content = stock_replies::to_string(status);

    if (status != processing_continue) {
       rep.headers.reserve(2);
       rep.headers.emplace_back(Header{"Content-Length", std::to_string(rep.content.size())});
       rep.headers.emplace_back(Header{"Content-Type", "text/html"});
    }
    return rep;
}

} // namespace silkrpc::http
