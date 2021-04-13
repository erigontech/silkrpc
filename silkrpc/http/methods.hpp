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

#ifndef SILKRPC_HTTP_METHODS_HPP_
#define SILKRPC_HTTP_METHODS_HPP_

#include <string>

#include "header.hpp"

namespace silkrpc::http::method {

constexpr const char* k_eth_blockNumber{"eth_blockNumber"};
constexpr const char* k_eth_getBlockByHash{"eth_getBlockByHash"};
constexpr const char* k_eth_getBlockByNumber{"eth_getBlockByNumber"};
constexpr const char* k_eth_getLogs{"eth_getLogs"};

constexpr const char* k_net_listening{"net_listening"};
constexpr const char* k_net_peerCount{"net_peerCount"};
constexpr const char* k_net_version{"net_version"};

constexpr const char* k_web3_clientVersion{"web3_clientVersion"};
constexpr const char* k_web3_sha3{"web3_sha3"};

} // namespace silkrpc::http::method

#endif // SILKRPC_HTTP_METHODS_HPP_
