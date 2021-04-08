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

#include "completion_poller.hpp"

//#include <chrono>

#include "async_completion_handler.hpp"

namespace silkrpc::grpc {

void CompletionPoller::start() {
    SILKRPC_INFO << "CompletionPoller::start starting...\n";
    thread_ = std::thread{&CompletionPoller::run, this};
}

void CompletionPoller::stop() {
    SILKRPC_INFO << "CompletionPoller::stop shutting down...\n";
    queue_.Shutdown();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void CompletionPoller::run() {
    SILKRPC_INFO << "CompletionPoller::run start\n";
    bool running = true;
    while (running) {
        void* got_tag;
        bool ok;
        //gpr_inf_future(   )
        const auto got_event = queue_.Next(&got_tag, &ok);
        if (got_event) {
            auto operation = grpc::AsyncCompletionHandler::detag(got_tag);
            SILKRPC_TRACE << "CompletionPoller::run post operation: " << operation << "\n";
            io_context_.post([=]() { operation->completed(ok); });
        } else {
            running = false;
            SILKRPC_DEBUG << "CompletionPoller::run shutdown\n";
        }
    }
    SILKRPC_INFO << "CompletionPoller::run end\n";
}

} // namespace silkrpc::grpc
