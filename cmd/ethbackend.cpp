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

#include <iomanip>
#include <iostream>
#include <string>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <grpcpp/grpcpp.h>
#include <silkworm/common/util.hpp>

#include <cmd/ethbackend.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/interfaces/remote/ethbackend.grpc.pb.h>
#include <silkrpc/interfaces/types/types.pb.h>

ABSL_FLAG(std::string, target, silkrpc::common::kDefaultTarget, "server location as string <address>:<port>");

int main(int argc, char* argv[]) {
    absl::SetProgramUsageMessage("Query Erigon/Silkworm ETHBACKEND remote interface");
    absl::ParseCommandLine(argc, argv);

    auto target{absl::GetFlag(FLAGS_target)};
    if (target.empty() || target.find(":") == std::string::npos) {
        std::cerr << "Parameter target is invalid: [" << target << "]\n";
        std::cerr << "Use --target flag to specify the location of Erigon running instance\n";
        return -1;
    }

    // Create ETHBACKEND stub using insecure channel to target
    grpc::Status status;

    const auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    const auto stub = remote::ETHBACKEND::NewStub(channel);

    grpc::ClientContext eb_context;
    remote::EtherbaseReply eb_reply;
    std::cout << "ETHBACKEND Etherbase ->\n";
    status = stub->Etherbase(&eb_context, remote::EtherbaseRequest{}, &eb_reply);
    if (status.ok()) {
        std::cout << "ETHBACKEND Etherbase <- " << status << " address: " << eb_reply.address() << "\n";
    } else {
        std::cout << "ETHBACKEND Etherbase <- " << status << "\n";
    }

    grpc::ClientContext nv_context;
    remote::NetVersionReply nv_reply;
    std::cout << "ETHBACKEND NetVersion ->\n";
    status = stub->NetVersion(&nv_context, remote::NetVersionRequest{}, &nv_reply);
    if (status.ok()) {
        std::cout << "ETHBACKEND NetVersion <- " << status << " id: " << nv_reply.id() << "\n";
    } else {
        std::cout << "ETHBACKEND NetVersion <- " << status << "\n";
    }

    grpc::ClientContext v_context;
    types::VersionReply v_reply;
    std::cout << "ETHBACKEND Version ->\n";
    status = stub->Version(&v_context, google::protobuf::Empty{}, &v_reply);
    if (status.ok()) {
        std::cout << "ETHBACKEND Version <- " << status << " major.minor.patch: " << v_reply.major() << "." << v_reply.minor() << "." << v_reply.patch() << "\n";
    } else {
        std::cout << "ETHBACKEND Version <- " << status << "\n";
    }

    grpc::ClientContext pv_context;
    remote::ProtocolVersionReply pv_reply;
    std::cout << "ETHBACKEND ProtocolVersion ->\n";
    status = stub->ProtocolVersion(&pv_context, remote::ProtocolVersionRequest{}, &pv_reply);
    if (status.ok()) {
        std::cout << "ETHBACKEND ProtocolVersion <- " << status << " id: " << pv_reply.id() << "\n";
    } else {
        std::cout << "ETHBACKEND ProtocolVersion <- " << status << "\n";
    }

    grpc::ClientContext cv_context;
    remote::ClientVersionReply cv_reply;
    std::cout << "ETHBACKEND ClientVersion ->\n";
    status = stub->ClientVersion(&cv_context, remote::ClientVersionRequest{}, &cv_reply);
    if (status.ok()) {
        std::cout << "ETHBACKEND ClientVersion <- " << status << " nodename: " << cv_reply.nodename() << "\n";
    } else {
        std::cout << "ETHBACKEND ClientVersion <- " << status << "\n";
    }

    return 0;
}
