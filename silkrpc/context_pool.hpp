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

#ifndef SILKRPC_CONTEXT_POOL_HPP_
#define SILKRPC_CONTEXT_POOL_HPP_

#include <cstddef>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <vector>

#include <asio/io_context.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/ethbackend/backend.hpp>
#include <silkrpc/ethdb/database.hpp>
#include <silkrpc/grpc/completion_runner.hpp>
#include <silkrpc/txpool/miner.hpp>
#include <silkrpc/txpool/transaction_pool.hpp>

namespace silkrpc {

struct Context {
    std::shared_ptr<asio::io_context> io_context;
    std::unique_ptr<grpc::CompletionQueue> grpc_queue;
    std::unique_ptr<CompletionRunner> grpc_runner;
    std::unique_ptr<ethdb::Database> database;
    std::unique_ptr<ethbackend::BackEnd> backend;
    std::unique_ptr<txpool::Miner> miner;
    std::unique_ptr<txpool::TransactionPool> tx_pool;
};

std::ostream& operator<<(std::ostream& out, const Context& c);

using ChannelFactory = std::function<std::shared_ptr<grpc::Channel>()>;

class ContextPool {
public:
    explicit ContextPool(std::size_t pool_size, ChannelFactory create_channel);

    ContextPool(const ContextPool&) = delete;
    ContextPool& operator=(const ContextPool&) = delete;

    void run();

    void stop();

    Context& get_context();

    asio::io_context& get_io_context();

private:
    // The pool of contexts
    std::vector<Context> contexts_;

    // The work-tracking executors that keep the io_contexts running
    std::list<asio::execution::any_executor<>> work_;

    // The next index to use for a context
    std::size_t next_index_;
};

} // namespace silkrpc

#endif // SILKRPC_CONTEXT_POOL_HPP_
