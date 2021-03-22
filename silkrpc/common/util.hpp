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

#ifndef SILKRPC_COMMON_UTIL_HPP_
#define SILKRPC_COMMON_UTIL_HPP_

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <asio/buffer.hpp>
#include <evmc/evmc.hpp>

#include <silkworm/common/base.hpp>
#include <silkworm/common/util.hpp>

namespace silkrpc::common {

struct KeyValue {
    silkworm::Bytes key;
    silkworm::Bytes value;
};

} // namespace silkrpc::common

namespace silkworm {

inline ByteView byte_view_of_string(const std::string& s) {
    return {reinterpret_cast<const uint8_t*>(s.data()), s.length()};
}

inline Bytes bytes_of_string(const std::string& s) {
    return Bytes(s.begin(), s.end());
}

} // namespace silkworm

inline std::ostream& operator<<(std::ostream& out, const silkworm::ByteView& bytes) {
    for (const auto& b : bytes) {
        out << std::hex << std::setw(2) << std::setfill('0') << int(b);
    }
    out << std::dec;
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const evmc::address& addr) {
    out << silkworm::to_hex(addr);
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const evmc::bytes32& b32) {
    out << silkworm::to_hex(b32);
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const asio::const_buffer& buffer) {
    out << std::string{static_cast<const char*>(buffer.data()), buffer.size()};
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const std::vector<asio::const_buffer>& buffers) {
    for (const auto buffer : buffers) {
        out << buffer;
    }
    return out;
}

#endif // SILKRPC_COMMON_UTIL_HPP_
