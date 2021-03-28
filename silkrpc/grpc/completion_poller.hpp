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

#ifndef SILKRPC_GRPC_COMPLETION_POLLER_HPP_
#define SILKRPC_GRPC_COMPLETION_POLLER_HPP_

#include <atomic>
#include <thread>

#include <asio/io_context.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/log.hpp>

namespace silkrpc::grpc {

class CompletionPoller {
public:
    CompletionPoller(::grpc::CompletionQueue& queue, asio::io_context& io_context)
    : queue_(queue), io_context_(io_context) {}

    CompletionPoller(const CompletionPoller&) = delete;
    CompletionPoller& operator=(const CompletionPoller&) = delete;

    void start();

    void stop();

private:
    void run();

    ::grpc::CompletionQueue& queue_;
    asio::io_context& io_context_;
    std::thread thread_;
};

} // namespace silkrpc::grpc

#endif  // SILKRPC_GRPC_COMPLETION_POLLER_HPP_
