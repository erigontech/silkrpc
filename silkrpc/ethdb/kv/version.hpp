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

#ifndef SILKRPC_ETHDB_KV_VERSION_HPP_
#define SILKRPC_ETHDB_KV_VERSION_HPP_

#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

#include <grpcpp/grpcpp.h>

#include <silkrpc/interfaces/remote/kv.grpc.pb.h>
#include <silkrpc/interfaces/types/types.pb.h>

namespace silkrpc::ethdb::kv {

struct ProtocolVersion {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
};

std::ostream& operator<<(std::ostream& out, const ProtocolVersion& v) {
    out << v.major << "." << v.minor << "." << v.patch;
    return out;
}

struct ProtocolVersionResult {
    bool compatible;
    std::string result;
};

using ProtocolVersionCheck = std::optional<ProtocolVersionResult>;

ProtocolVersionCheck check_protocol_version(const std::shared_ptr<grpc::Channel>& channel, const ProtocolVersion& client_version) {
    const auto stub = remote::KV::NewStub(channel);

    grpc::ClientContext context;
    ::types::VersionReply version_reply;
    const auto status = stub->Version(&context, google::protobuf::Empty{}, &version_reply);
    if (!status.ok()) {
        return std::nullopt;
    }
    ProtocolVersion server_version{version_reply.major(), version_reply.minor(), version_reply.patch()};

    std::stringstream vv_stream;
    vv_stream << "client: " << client_version << " server: " << server_version;
    if (client_version.major != server_version.major) {
        return ProtocolVersionResult{false, "KV incompatible interface versions: " + vv_stream.str()};
    } else if (client_version.minor != server_version.minor) {
        return ProtocolVersionResult{false, "KV incompatible interface versions: " + vv_stream.str()};
    } else {
        return ProtocolVersionResult{true, "KV compatible interface versions: " + vv_stream.str()};
    }
}

} // namespace silkrpc::ethdb::kv

#endif // SILKRPC_ETHDB_KV_VERSION_HPP_
