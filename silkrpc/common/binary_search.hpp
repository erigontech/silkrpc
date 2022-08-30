/*
   Copyright 2022 The Silkrpc Authors

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

#ifndef SILKRPC_COMMON_BINARY_SEARCH_HPP_
#define SILKRPC_COMMON_BINARY_SEARCH_HPP_

#include <cstddef>

#include <silkrpc/config.hpp>

#include <absl/functional/function_ref.h>
#include <boost/asio/awaitable.hpp>

namespace silkrpc {

using BinaryPredicate = absl::FunctionRef<boost::asio::awaitable<bool>(std::size_t)>;

boost::asio::awaitable<std::size_t> binary_search(std::size_t n, BinaryPredicate pred);

} // namespace silkrpc

#endif // SILKRPC_COMMON_BINARY_SEARCH_HPP_
