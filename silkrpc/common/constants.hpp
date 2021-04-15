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
#include <string>

#include <boost/format.hpp>

namespace silkrpc::common {

constexpr const char* kOsName{
#ifdef _WIN32
    "Windows 32-bit"
#elif _WIN64
    "Windows 64-bit"
#elif __APPLE__ || __MACH__
    "Mac OSX"
#elif __linux__
    "Linux"
#elif __FreeBSD__
    "FreeBSD"
#elif __unix || __unix__
    "Unix"
#else
    "Other"
#endif
};

constexpr const char* kAddressPortSeparator{":"};

constexpr const char* kEmptyChainData{""};
constexpr const char* kDefaultLocal{"localhost:8545"};
constexpr const char* kDefaultTarget{"localhost:9090"};
constexpr const std::chrono::milliseconds kDefaultTimeout{10000};

constexpr const char* kNodeName{"TurboGeth"};
constexpr const char* kVersion{"2021.03.2-alpha"};
const std::string kEthereumNodeName = boost::str(boost::format("%1%/v%2%/%3%") % kNodeName % kVersion % kOsName); // NOLINT

constexpr uint64_t kETH66{66};

}  // namespace silkrpc::common

#endif  // SILKRPC_COMMON_CONSTANTS_HPP_
