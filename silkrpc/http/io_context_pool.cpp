//
// io_context_pool.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "io_context_pool.hpp"

#include <stdexcept>
#include <thread>

#include <boost/bind/bind.hpp>

namespace silkrpc::http {

io_context_pool::io_context_pool(std::size_t pool_size) : next_io_context_(0) {
    if (pool_size == 0) {
        throw std::runtime_error("io_context_pool size is 0");
    }

    // Give all the io_contexts work to do so that their run() functions will not
    // exit until they are explicitly stopped.
    for (std::size_t i{0}; i < pool_size; ++i) {
        io_context_ptr io_context(new asio::io_context);
        io_contexts_.push_back(io_context);
        work_.push_back(asio::require(io_context->get_executor(), asio::execution::outstanding_work.tracked));
    }
}

void io_context_pool::run() {
    // Create a pool of threads to run all of the io_contexts.
    asio::detail::thread_group workers{};
    for (auto i{0}; i < io_contexts_.size(); i++) {
        workers.create_thread([&, i]() { io_contexts_[i]->run(); });
    }

    // Wait for all threads in the pool to exit.
    workers.join();

    // Create a pool of threads to run all of the io_contexts.
    /*std::vector<std::shared_ptr<std::thread>> threads;
    for (std::size_t i{0}; i < io_contexts_.size(); ++i) {
        std::shared_ptr<std::thread> new_thread(new std::thread([&, i]() { io_contexts_[i]->run(); }));
        threads.push_back(new_thread);
    }

    // Wait for all threads in the pool to exit.
    for (std::size_t i = 0; i < threads.size(); ++i)
        threads[i]->join();*/
}

void io_context_pool::stop() {
    // Explicitly stop all io_contexts.
    for (std::size_t i{0}; i < io_contexts_.size(); ++i) {
        io_contexts_[i]->stop();
    }
}

asio::io_context& io_context_pool::get_io_context() {
    // Use a round-robin scheme to choose the next io_context to use.
    asio::io_context& io_context = *io_contexts_[next_io_context_];
    ++next_io_context_;
    if (next_io_context_ == io_contexts_.size()) {
        next_io_context_ = 0;
    }
    return io_context;
}

} // namespace silkrpc::http
