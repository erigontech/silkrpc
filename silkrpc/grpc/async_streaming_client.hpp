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

#ifndef SILKRPC_GRPC_ASYNC_STREAMING_CLIENT_HPP_
#define SILKRPC_GRPC_ASYNC_STREAMING_CLIENT_HPP_

#include <functional>

#include <grpcpp/grpcpp.h>

namespace silkrpc {

template<typename Request, typename Response>
class AsyncStreamingClient {
public:
    virtual void start_call(std::function<void(const grpc::Status&)> start_completed) = 0;

    virtual void end_call(std::function<void(const grpc::Status&)> end_completed) = 0;

    virtual void read_start(std::function<void(const grpc::Status&, const Response &)> read_completed) = 0;

    virtual void write_start(const Request& request, std::function<void(const grpc::Status&)> write_completed) = 0;
};

} // namespace silkrpc

#endif // SILKRPC_GRPC_ASYNC_STREAMING_CLIENT_HPP_
