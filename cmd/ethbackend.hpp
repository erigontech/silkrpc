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

#ifndef CMD_ETHBACKEND_HPP_
#define CMD_ETHBACKEND_HPP_

#include <iomanip>
#include <iostream>

#include <grpcpp/grpcpp.h>

#include <silkrpc/interfaces/types/types.pb.h>

std::ostream& operator<<(std::ostream& out, const types::H160& address) {
    out << "address=" << address.has_hi();
    if (address.has_hi()) {
        auto hi_half = address.hi();
        out << std::hex << hi_half.hi() << hi_half.lo();
    } else {
        auto lo_half = address.lo();
        out << std::hex << lo_half;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const grpc::Status& status) {
    out << "status=" << (status.ok() ? "OK" : "KO");
    if (!status.ok()) {
        out << " error_code=" << status.error_code()
            << " error_message=" << status.error_message()
            << " error_details=" << status.error_details();
    }
    return out;
}

#endif // CMD_ETHBACKEND_HPP_
