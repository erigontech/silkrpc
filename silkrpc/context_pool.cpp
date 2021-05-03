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

#include "context_pool.hpp"

#include <stdexcept>
#include <thread>

#include <silkrpc/ethdb/kv/remote_database.hpp>

namespace silkrpc {

ContextPool::ContextPool(std::size_t pool_size, ChannelFactory create_channel) : next_index_{0} {
    if (pool_size == 0) {
        throw std::logic_error("ContextPool::ContextPool pool_size is 0");
    }

    // Create all the io_contexts and give them work to do so that their event loop will not exit until they are explicitly stopped.
    for (std::size_t i{0}; i < pool_size; ++i) {
        auto io_context = std::make_shared<asio::io_context>();
        auto grpc_channel = create_channel();
        auto grpc_queue = std::make_unique<::grpc::CompletionQueue>();
        auto grpc_runner = std::make_unique<grpc::CompletionRunner>(*grpc_queue, *io_context);
        auto database = std::make_unique<ethdb::kv::RemoteDatabase>(*io_context, grpc_channel, grpc_queue.get()); // TODO(canepat): move elsewhere
        contexts_.push_back({io_context, std::move(grpc_queue), std::move(grpc_runner), std::move(database)});
        work_.push_back(asio::require(io_context->get_executor(), asio::execution::outstanding_work.tracked));
    }
}

void ContextPool::run() {
    // Create a pool of threads to run all of the contexts (each one having 1+1 threads)
    asio::detail::thread_group workers{};
    for (auto& context : contexts_) {
        context.grpc_runner->start(); // TODO(canepat): move here thread start
        auto& io_context = context.io_context;
        workers.create_thread([&]() { io_context->run(); });
    }

    // Wait for all threads in the pool to exit
    workers.join();
}

void ContextPool::stop() {
    // Explicitly stop all scheduler runnable components
    for (auto& context : contexts_) {
        context.grpc_runner->stop(); // TODO(canepat): move here thread stop
        context.io_context->stop();
    }
}

Context& ContextPool::get_context() {
    // Use a round-robin scheme to choose the next context to use
    auto& context = contexts_[next_index_];
    next_index_ = ++next_index_ % contexts_.size();
    return context;
}

asio::io_context& ContextPool::get_io_context() {
    auto& context = get_context();
    return *context.io_context;
}

} // namespace silkrpc
