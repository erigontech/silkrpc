//
// mime_types.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SILKRPC_HTTP_MIME_TYPES_HPP_
#define SILKRPC_HTTP_MIME_TYPES_HPP_

#include <string>

namespace silkrpc::http::mime_types {

/// Convert a file extension into a MIME type.
std::string extension_to_type(const std::string& extension);

} // namespace silkrpc::http::mime_types

#endif // SILKRPC_HTTP_MIME_TYPES_HPP_
