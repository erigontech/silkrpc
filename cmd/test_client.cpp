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

#if 1
#include <iostream>
int main(int argc, char* argv[]) {
    std::cout << "In the client" << std::endl;
    return 0;
}
#else
#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>

#ifdef BAZEL_BUILD
#include "examples/protos/eth.grpc.pb.h"
#else
#include "eth.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using rpcdaemon::EthService;
using rpcdaemon::GetBlockRequest;
using rpcdaemon::GetBlockResponse;
using rpcdaemon::GetTransRequest;
using rpcdaemon::GetTransResponse;

//----------------------------------------------------------------------------------------------
class EthServiceClient {
  public:
    explicit EthServiceClient(std::shared_ptr<Channel> channel);
    std::string GetBlock(uint64_t f, uint64_t l = UINT_MAX, uint64_t s = 1);
    std::string GetTransaction(uint64_t b, uint64_t id);

  private:
    std::unique_ptr<EthService::Stub> stub_;
};

//----------------------------------------------------------------------------------------------
EthServiceClient::EthServiceClient(std::shared_ptr<Channel> channel) : stub_(EthService::NewStub(channel)) {}

//----------------------------------------------------------------------------------------------
std::string EthServiceClient::GetBlock(uint64_t f, uint64_t l, uint64_t s) {
    GetBlockRequest request;
    request.set_first(f);
    request.set_last(l == UINT_MAX ? f : l);
    request.set_step(s);

    GetBlockResponse reply;
    ClientContext context;
    Status status = stub_->GetBlock(&context, request, &reply);
    return (status.ok() ? reply.message() : "RPC failed");
}

//----------------------------------------------------------------------------------------------
std::string EthServiceClient::GetTransaction(uint64_t b, uint64_t id) {
    GetTransRequest request;
    request.set_block(b);
    request.set_tx_id(id);

    GetTransResponse reply;
    ClientContext context;
    Status status = stub_->GetTransaction(&context, request, &reply);
    return (status.ok() ? reply.message() : "RPC failed");
}

//----------------------------------------------------------------------------------------------
int main(int argc, char** argv) {
    auto credentials = grpc::InsecureChannelCredentials();
    auto channel = grpc::CreateChannel("localhost:50051", credentials);

    EthServiceClient client(channel);

    std::cout << std::string(120, '~') << std::endl;
    std::cout << "{\"blocks\":[";
    for (int i = 0; i < 1; i++) {
        if (i != 0) std::cout << ",";
        std::cout << client.GetBlock(i) << std::endl;
        std::cerr << "received block: " << i << std::endl;
        std::cerr.flush();
    }
    std::cout << "],\"txs\":[";
    for (int i = 0; i < 1; i++) {
        if (i != 0) std::cout << ",";
        std::cout << client.GetBlock(i) << std::endl;
//        std::cout << client.GetTransaction(i, 0) << std::endl;
        std::cerr << "received transaction: " << i << std::endl;
        std::cerr.flush();
    }
    std::cout << "]}";
    std::cout << std::string(120, '~') << std::endl;
    std::cout << std::endl;

    return 0;
}
#endif
