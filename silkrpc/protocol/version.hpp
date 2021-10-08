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

#ifndef SILKRPC_PROTOCOL_VERSION_HPP_
#define SILKRPC_PROTOCOL_VERSION_HPP_

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <grpcpp/grpcpp.h>

#include <silkrpc/interfaces/remote/ethbackend.grpc.pb.h>
#include <silkrpc/interfaces/remote/kv.grpc.pb.h>
#include <silkrpc/interfaces/types/types.pb.h>

namespace silkrpc {

struct ProtocolVersion {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
};

constexpr auto KV_SERVICE_API_VERSION = ProtocolVersion{4, 0, 0};
constexpr auto ETHBACKEND_SERVICE_API_VERSION = ProtocolVersion{2, 1, 0};

std::ostream& operator<<(std::ostream& out, const ProtocolVersion& v) {
    out << v.major << "." << v.minor << "." << v.patch;
    return out;
}

struct ProtocolVersionResult {
    bool compatible;
    std::string result;
};

ProtocolVersionResult wait_for_kv_protocol_check(const std::unique_ptr<remote::KV::StubInterface>& stub) {
    grpc::ClientContext context;
    context.set_wait_for_ready(true);

    types::VersionReply version_reply;
    const auto status = stub->Version(&context, google::protobuf::Empty{}, &version_reply);
    if (!status.ok()) {
        return ProtocolVersionResult{false, "KV incompatible interface: " + status.error_message() + " [" + status.error_details() + "]"};
    }
    ProtocolVersion server_version{version_reply.major(), version_reply.minor(), version_reply.patch()};

    std::stringstream vv_stream;
    vv_stream << "client: " << KV_SERVICE_API_VERSION << " server: " << server_version;
    if (KV_SERVICE_API_VERSION.major != server_version.major) {
        return ProtocolVersionResult{false, "KV incompatible interface: " + vv_stream.str()};
    } else if (KV_SERVICE_API_VERSION.minor != server_version.minor) {
        return ProtocolVersionResult{false, "KV incompatible interface: " + vv_stream.str()};
    } else {
        return ProtocolVersionResult{true, "KV compatible interface: " + vv_stream.str()};
    }
}

ProtocolVersionResult wait_for_kv_protocol_check(const std::shared_ptr<grpc::Channel>& channel) {
    return wait_for_kv_protocol_check(remote::KV::NewStub(channel));
}

ProtocolVersionResult wait_for_ethbackend_protocol_check(const std::unique_ptr<remote::ETHBACKEND::StubInterface>& stub) {
    grpc::ClientContext context;
    context.set_wait_for_ready(true);

    types::VersionReply version_reply;
    const auto status = stub->Version(&context, google::protobuf::Empty{}, &version_reply);
    if (!status.ok()) {
        return ProtocolVersionResult{false, "ETHBACKEND incompatible interface: " + status.error_message() + " [" + status.error_details() + "]"};
    }
    ProtocolVersion server_version{version_reply.major(), version_reply.minor(), version_reply.patch()};

    std::stringstream vv_stream;
    vv_stream << "client: " << ETHBACKEND_SERVICE_API_VERSION << " server: " << server_version;
    if (ETHBACKEND_SERVICE_API_VERSION.major != server_version.major) {
        return ProtocolVersionResult{false, "ETHBACKEND incompatible interface: " + vv_stream.str()};
    } else if (ETHBACKEND_SERVICE_API_VERSION.minor != server_version.minor) {
        return ProtocolVersionResult{false, "ETHBACKEND incompatible interface: " + vv_stream.str()};
    } else {
        return ProtocolVersionResult{true, "ETHBACKEND compatible interface: " + vv_stream.str()};
    }
}

ProtocolVersionResult wait_for_ethbackend_protocol_check(const std::shared_ptr<grpc::Channel>& channel) {
    return wait_for_ethbackend_protocol_check(remote::ETHBACKEND::NewStub(channel));
}

} // namespace silkrpc

#endif // SILKRPC_PROTOCOL_VERSION_HPP_
