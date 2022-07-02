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
#include <utility>

#include <silkrpc/common/log.hpp>
#include <silkrpc/ethbackend/remote_backend.hpp>
#include <silkrpc/ethdb/kv/remote_database.hpp>

namespace silkrpc {

std::ostream& operator<<(std::ostream& out, Context& c) {
    out << "io_context: " << c.io_context() << " queue: " << c.grpc_queue();
    return out;
}

Context::Context(
    ChannelFactory create_channel,
    std::shared_ptr<BlockCache> block_cache,
    std::shared_ptr<ethdb::kv::StateCache> state_cache,
    WaitMode wait_mode)
    : io_context_{std::make_shared<asio::io_context>()},
      io_context_work_{asio::make_work_guard(*io_context_)},
      grpc_context_{std::make_unique<agrpc::GrpcContext>(std::make_unique<grpc::CompletionQueue>())},
      grpc_context_work_{asio::make_work_guard(grpc_context_->get_executor())},
      block_cache_(block_cache),
      state_cache_(state_cache),
      wait_mode_(wait_mode) {
    std::shared_ptr<grpc::Channel> channel = create_channel();
    database_ = std::make_unique<ethdb::kv::RemoteDatabase<>>(*io_context_, channel, grpc_queue(), state_cache_.get());
    backend_ = std::make_unique<ethbackend::RemoteBackEnd>(*io_context_, channel, grpc_queue());
    miner_ = std::make_unique<txpool::Miner>(*io_context_, channel, grpc_queue());
    tx_pool_ = std::make_unique<txpool::TransactionPool>(*io_context_, channel, grpc_queue());
}

template <typename WaitStrategy>
void Context::execute_loop_single_threaded(WaitStrategy&& wait_strategy) {
    SILKRPC_DEBUG << "Single-thread execution loop start [" << this << "]\n";
    io_context_->restart();
    grpc_context_->reset();
    while (!io_context_->stopped()) {
        int work_count = grpc_context_->poll_completion_queue();
        work_count += io_context_->poll();
        wait_strategy.idle(work_count);
    }
    SILKRPC_DEBUG << "Single-thread execution loop end [" << this << "]\n";
}

void Context::execute_loop_multi_threaded() {
    SILKRPC_DEBUG << "Multi-thread execution loop start [" << this << "]\n";
    io_context_->restart();
    grpc_context_->reset();
    std::thread completion_runner_thread{[&]() {
        grpc_context_->run_completion_queue();
    }};
    io_context_->run();
    grpc_context_->stop();
    grpc_queue()->Shutdown();
    completion_runner_thread.join();
    SILKRPC_DEBUG << "Multi-thread execution loop end [" << this << "]\n";
}

void Context::execute_loop() {
    switch (wait_mode_) {
        case WaitMode::blocking:
            execute_loop_multi_threaded();
        break;
        case WaitMode::yielding:
            execute_loop_single_threaded(YieldingWaitStrategy{});
        break;
        case WaitMode::sleeping:
            execute_loop_single_threaded(SleepingWaitStrategy{});
        break;
        case WaitMode::spin_wait:
            execute_loop_single_threaded(SpinWaitWaitStrategy{});
        break;
        case WaitMode::busy_spin:
            execute_loop_single_threaded(BusySpinWaitStrategy{});
        break;
    }
}

void Context::stop() {
    io_context_->stop();
    SILKRPC_DEBUG << "Context::stop io_context " << io_context_ << " [" << this << "]\n";
}

ContextPool::ContextPool(std::size_t pool_size, ChannelFactory create_channel, WaitMode wait_mode) : next_index_{0} {
    if (pool_size == 0) {
        throw std::logic_error("ContextPool::ContextPool pool_size is 0");
    }
    SILKRPC_INFO << "ContextPool::ContextPool creating pool with size: " << pool_size << "\n";

    // Create the unique block cache to be shared among the execution contexts
    auto block_cache = std::make_shared<BlockCache>();

    // Create the unique state cache to be shared among the execution contexts
    auto state_cache = std::make_shared<ethdb::kv::CoherentStateCache>();

    // Create as many execution contexts as required by the pool size
    for (std::size_t i{0}; i < pool_size; ++i) {
        contexts_.emplace_back(Context{create_channel, block_cache, state_cache, wait_mode});
        SILKRPC_DEBUG << "ContextPool::ContextPool context[" << i << "] " << contexts_[i] << "\n";
    }
}

ContextPool::~ContextPool() {
    SILKRPC_TRACE << "ContextPool::~ContextPool started " << this << "\n";
    stop();
    SILKRPC_TRACE << "ContextPool::~ContextPool completed " << this << "\n";
}

void ContextPool::start() {
    SILKRPC_TRACE << "ContextPool::start started\n";

    if (stopped_) {
        throw std::logic_error("cannot restart context pool, create another one");
    }

    // Create a pool of threads to run all of the contexts (each one having 1 threads)
    for (std::size_t i{0}; i < contexts_.size(); ++i) {
        auto& context = contexts_[i];
        context_threads_.create_thread([&, i = i]() {
            SILKRPC_DEBUG << "Thread start context[" << i << "] thread_id: " << std::this_thread::get_id() << "\n";
            context.execute_loop();
            SILKRPC_DEBUG << "Thread end context[" << i << "] thread_id: " << std::this_thread::get_id() << "\n";
        });
        SILKRPC_DEBUG << "ContextPool::start context[" << i << "].io_context started: " << &*context.io_context() << "\n";
    }

    SILKRPC_TRACE << "ContextPool::start completed\n";
}

void ContextPool::join() {
    SILKRPC_TRACE << "ContextPool::join started\n";

    // Wait for all threads in the pool to exit.
    SILKRPC_DEBUG << "ContextPool::join joining...\n";
    context_threads_.join();

    SILKRPC_TRACE << "ContextPool::join completed\n";
}

void ContextPool::stop() {
    SILKRPC_TRACE << "ContextPool::stop started\n";

    stopped_ = true;

    // Explicitly stop all scheduler runnable components
    for (std::size_t i{0}; i < contexts_.size(); ++i) {
        contexts_[i].stop();
        SILKRPC_DEBUG << "ContextPool::stop context[" << i << "].io_context stopped: " << &*contexts_[i].io_context() << "\n";
    }
    SILKRPC_TRACE << "ContextPool::stop completed\n";
}

void ContextPool::run() {
    start();
    join();
}

Context& ContextPool::next_context() {
    // Use a round-robin scheme to choose the next context to use
    auto& context = contexts_[next_index_];
    next_index_ = ++next_index_ % contexts_.size();
    return context;
}

asio::io_context& ContextPool::next_io_context() {
    auto& client_context = next_context();
    return *client_context.io_context();
}

} // namespace silkrpc
