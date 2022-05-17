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

#include <silkrpc/txpool/transaction_pool.hpp>
#include <silkrpc/common/block_cache.hpp>
#include <silkrpc/ethbackend/backend.hpp>
#include <silkrpc/ethdb/database.hpp>
#include <silkrpc/txpool/miner.hpp>
//#include <silkworm/rpc/completion_end_point.hpp>
#include <silkrpc/common/log.hpp>

// TODO(canepat) Temporary modified copy of CompletionEndPoint just for prototyping
namespace silkworm::rpc {

//! Application end-point dedicated to read completion notifications from one gRPC completion queue.
class CompletionEndPoint {
  public:
    explicit CompletionEndPoint(grpc::CompletionQueue& queue) : queue_(queue) {}

    CompletionEndPoint(const CompletionEndPoint&) = delete;
    CompletionEndPoint& operator=(const CompletionEndPoint&) = delete;

    //! Run at most one execution cycle polling gRPC completion queue for one event.
    int poll_one() {
        SILKRPC_TRACE << "CompletionEndPoint::poll_one START\n";

        int num_completed{0}; // returned when next_status == grpc::CompletionQueue::TIMEOUT

        void* tag{nullptr};
        bool ok{false};
        const auto next_status = queue_.AsyncNext(&tag, &ok, gpr_time_0(GPR_CLOCK_MONOTONIC));
        if (next_status == grpc::CompletionQueue::GOT_EVENT) {
            num_completed = 1;
            // Handle the event completion on the calling thread (*must* be the io_context scheduler).
            CompletionTag completion_tag{reinterpret_cast<TagProcessor*>(tag), ok};
            SILKRPC_DEBUG << "CompletionEndPoint::poll_one post operation: " << completion_tag.processor << "\n";
            (*completion_tag.processor)(completion_tag.ok);
        } else if (next_status == grpc::CompletionQueue::SHUTDOWN) {
            num_completed = -1;
        }

        SILKRPC_TRACE << "CompletionEndPoint::poll_one next_status=" << next_status << " END\n";
        return num_completed;
    }

    //! Post at most one execution task to scheduler polling gRPC completion queue for one event.
    bool post_one(asio::io_context& scheduler) {
        void* tag{nullptr};
        bool ok{false};
        const auto got_event = queue_.Next(&tag, &ok);
        if (got_event) {
            // Post the event completion on the passed io_context scheduler.
            CompletionTag completion_tag{reinterpret_cast<TagProcessor*>(tag), ok};
            SILKRPC_DEBUG << "CompletionEndPoint::post_one post operation: " << completion_tag.processor << "\n";
            scheduler.post([completion_tag]() { (*completion_tag.processor)(completion_tag.ok); });
        } else {
            SILKRPC_DEBUG << "CompletionEndPoint::run shutdown\n";
        }
        return !got_event;
    }

    //! Shutdown and drain the gRPC completion queue.
    void shutdown() {
        SILKRPC_TRACE << "CompletionEndPoint::shutdown START\n";
        queue_.Shutdown();
        SILKRPC_DEBUG << "CompletionEndPoint::shutdown draining...\n";
        void* ignored_tag;
        bool ignored_ok;
        while (queue_.Next(&ignored_tag, &ignored_ok)) {
        }
        SILKRPC_TRACE << "CompletionEndPoint::shutdown END\n";
    }

  private:
    //! The gRPC completion queue to read async completion notifications from.
    grpc::CompletionQueue& queue_;
};

} // namespace silkworm::rpc

namespace silkrpc {

struct WaitStrategy {
    virtual ~WaitStrategy() = default;

    virtual void wait_once(uint32_t executed_count) = 0;
};

struct SleepingWaitStrategy : public WaitStrategy {
    void wait_once(uint32_t executed_count) override {
        if (executed_count > 0) {
            if (counter_ != kRetries) {
                counter_ = kRetries;
            }
            return;
        }

        if (counter_ > 100) {
            --counter_;
        } else if (counter_ > 0) {
            --counter_;
            std::this_thread::yield();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(0));
        }
    }

  private:
    inline static const uint32_t kRetries{200};

    uint32_t counter_{kRetries};
};

struct YieldingWaitStrategy : public WaitStrategy {
    void wait_once(uint32_t executed_count) override {
        if (executed_count > 0) {
            if (counter_ != kSpinTries) {
                counter_ = kSpinTries;
            }
            return;
        }

        if (counter_ == 0) {
            std::this_thread::yield();
        } else {
            --counter_;
        }
    }

  private:
    inline static const uint32_t kSpinTries{100};

    uint32_t counter_{kSpinTries};
};

struct SpinWaitWaitStrategy : public WaitStrategy {
    void wait_once(uint32_t executed_count) override {
        if (executed_count > 0) {
            if (counter_ != 0) {
                counter_ = 0;
            }
            return;
        }

        if (counter_ > kYieldThreshold) {
            auto delta = counter_ - kYieldThreshold;
            if (delta % kSleep1EveryHowManyTimes == kSleep1EveryHowManyTimes - 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            } else if (delta % kSleep0EveryHowManyTimes == kSleep0EveryHowManyTimes - 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(0));
            } else {
                std::this_thread::yield();
            }
        } else {
            for (auto i{0}; i < (4 << counter_); i++) {
                spin_wait();
            }
        }
    }

  private:
    void spin_wait() {
        asm volatile
        (
            "rep\n"
            "nop"
        );
    }

    inline static const uint32_t kYieldThreshold{10};
    inline static const uint32_t kSleep0EveryHowManyTimes{5};
    inline static const uint32_t kSleep1EveryHowManyTimes{20};

    uint32_t counter_{0};
};

struct BusySpinWaitStrategy : public WaitStrategy {
    void wait_once(uint32_t /*executed_count*/) override {
    }
};

enum class WaitMode {
    blocking,
    sleeping,
    yielding,
    spin_wait,
    busy_spin
};

std::unique_ptr<WaitStrategy> make_wait_strategy(WaitMode wait_mode);

using ChannelFactory = std::function<std::shared_ptr<grpc::Channel>()>;

//! Asynchronous client scheduler running an execution loop.
class Context {
  public:
    explicit Context(ChannelFactory create_channel, std::shared_ptr<BlockCache> block_cache, WaitMode wait_mode = WaitMode::blocking);

    asio::io_context* io_context() const noexcept { return io_context_.get(); }
    grpc::CompletionQueue* grpc_queue() const noexcept { return queue_.get(); }
    silkworm::rpc::CompletionEndPoint* rpc_end_point() noexcept { return rpc_end_point_.get(); }
    std::unique_ptr<ethdb::Database>& database() noexcept { return database_; }
    std::unique_ptr<ethbackend::BackEnd>& backend() noexcept { return backend_; }
    std::unique_ptr<txpool::Miner>& miner() noexcept { return miner_; }
    std::unique_ptr<txpool::TransactionPool>& tx_pool() noexcept { return tx_pool_; }
    std::shared_ptr<BlockCache>& block_cache() noexcept { return block_cache_; }

    //! Execute the scheduler loop until stopped.
    void execute_loop();

    //! Stop the execution loop.
    void stop();

  private:
    //! Execute single-threaded loop until stopped.
    void execute_loop_single_threaded();

    //! Execute single-threaded loop until stopped.
    void execute_loop_double_threaded();

    //! The asynchronous event loop scheduler.
    std::shared_ptr<asio::io_context> io_context_;

    //! The work-tracking executor that keep the scheduler running.
    asio::execution::any_executor<> work_;

    std::unique_ptr<grpc::CompletionQueue> queue_;
    std::unique_ptr<silkworm::rpc::CompletionEndPoint> rpc_end_point_;
    std::unique_ptr<ethdb::Database> database_;
    std::unique_ptr<ethbackend::BackEnd> backend_;
    std::unique_ptr<txpool::Miner> miner_;
    std::unique_ptr<txpool::TransactionPool> tx_pool_;
    std::shared_ptr<BlockCache> block_cache_;
    WaitMode wait_mode_;
};

std::ostream& operator<<(std::ostream& out, Context& c);

class ContextPool {
public:
    explicit ContextPool(std::size_t pool_size, ChannelFactory create_channel, WaitMode wait_mode = WaitMode::blocking);
    ~ContextPool();

    ContextPool(const ContextPool&) = delete;
    ContextPool& operator=(const ContextPool&) = delete;

    void start();

    void join();

    void stop();

    void run();

    Context& next_context();

    asio::io_context& next_io_context();

private:
    // The pool of contexts
    std::vector<Context> contexts_;

    //! The pool of threads running the execution contexts.
    asio::detail::thread_group context_threads_;

    // The next index to use for a context
    std::size_t next_index_;

    //! Flag indicating if pool has been stopped.
    bool stopped_{false};
};

} // namespace silkrpc

#endif // SILKRPC_CONTEXT_POOL_HPP_
