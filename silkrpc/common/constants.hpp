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

#ifndef SILKRPC_COMMON_CONSTANTS_HPP_
#define SILKRPC_COMMON_CONSTANTS_HPP_

#include <chrono>
#include <cstddef>

namespace silkrpc {

constexpr const char* kDebugApiNamespace{"debug"};
constexpr const char* kEthApiNamespace{"eth"};
constexpr const char* kNetApiNamespace{"net"};
constexpr const char* kParityApiNamespace{"parity"};
constexpr const char* kTgApiNamespace{"tg"};
constexpr const char* kTraceApiNamespace{"trace"};
constexpr const char* kWeb3ApiNamespace{"web3"};

constexpr const char* kAddressPortSeparator{":"};
constexpr const char* kApiSpecSeparator{","};

constexpr const char* kEmptyChainData{""};
constexpr const char* kDefaultLocal{"localhost:8545"};
constexpr const char* kDefaultTarget{"localhost:9090"};
constexpr const char* kDefaultApiSpec{"debug,eth,net,parity,tg,trace,web3"};
constexpr const std::chrono::milliseconds kDefaultTimeout{10000};

constexpr const std::size_t kHttpIncomingBufferSize{8192};

constexpr const std::size_t kRequestContentInitialCapacity{1024};
constexpr const std::size_t kRequestHeadersInitialCapacity{8};
constexpr const std::size_t kRequestMethodInitialCapacity{64};
constexpr const std::size_t kRequestUriInitialCapacity{64};

} // namespace silkrpc

#endif  // SILKRPC_COMMON_CONSTANTS_HPP_
