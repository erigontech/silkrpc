//
// reply.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SILKRPC_HTTP_REPLY_HPP_
#define SILKRPC_HTTP_REPLY_HPP_

#include <string>
#include <vector>
#include <boost/asio/buffer.hpp>

#include "header.hpp"

namespace silkrpc::http {

/// The status of the reply.
enum StatusType {
    processing_continue = 100,
    ok = 200,
    created = 201,
    accepted = 202,
    no_content = 204,
    multiple_choices = 300,
    moved_permanently = 301,
    moved_temporarily = 302,
    not_modified = 304,
    bad_request = 400,
    unauthorized = 401,
    forbidden = 403,
    not_found = 404,
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503
};

boost::asio::const_buffer to_buffer(StatusType status);
std::vector<boost::asio::const_buffer> to_buffers(const std::vector<Header>& headers);
std::vector<boost::asio::const_buffer> to_buffers(StatusType status, const std::vector<Header>& headers);

/// A reply to be sent to a client.
struct Reply {
    /// The status of the reply.
    StatusType status;

    /// The headers to be included in the reply.
    std::vector<Header> headers;

    /// The content to be sent in the reply.
    std::string content;

    /// Convert the reply into a vector of buffers. The buffers do not own the
    /// underlying memory blocks, therefore the reply object must remain valid and
    /// not be changed until the write operation has completed.
    std::vector<boost::asio::const_buffer> to_buffers() const;

    /// Get a stock reply.
    static Reply stock_reply(StatusType status);

    // reset Reply data
    void reset() {
        headers.resize(0);
        content.resize(0);
    }
};

} // namespace silkrpc::http

#endif // SILKRPC_HTTP_REPLY_HPP_
