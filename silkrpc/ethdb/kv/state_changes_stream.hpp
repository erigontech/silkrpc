/*
    Copyright 2020-2022 The Silkrpc Authors

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

#ifndef SILKRPC_ETHDB_KV_STATE_CHANGES_STREAM_HPP_
#define SILKRPC_ETHDB_KV_STATE_CHANGES_STREAM_HPP_

#include <functional>
#include <memory>

#ifndef ASIO_HAS_BOOST_DATE_TIME
#define ASIO_HAS_BOOST_DATE_TIME
#endif
#include <asio/deadline_timer.hpp>
#include <asio/io_context.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/ethdb/kv/state_cache.hpp>
#include <silkworm/common/assert.hpp>
#include <silkworm/rpc/completion_tag.hpp>
#include <silkworm/rpc/client/call.hpp>
#include <remote/kv.grpc.pb.h>

namespace silkrpc::ethdb::kv {

using silkworm::rpc::AsyncServerStreamingCall;

class AsyncStateChangesCall
    : public AsyncServerStreamingCall<remote::StateChangeRequest, remote::StateChangeBatch, remote::KV::StubInterface,
                                      &remote::KV::StubInterface::PrepareAsyncStateChanges> {
  public:
    using TerminationHook = std::function<void(bool)>;

    explicit AsyncStateChangesCall(
        grpc::CompletionQueue* queue,
        remote::KV::StubInterface* stub,
        StateCache* cache,
        TerminationHook termination_hook);

    void handle_read() override;

    void handle_finish() override;

  private:
    StateCache* cache_;
    TerminationHook termination_hook_;
};

//! The registration interval.
constexpr boost::posix_time::milliseconds kDefaultRegistrationInterval{10'000};

class StateChangesStream {
  public:
    static void set_registration_interval(boost::posix_time::milliseconds registration_interval);

    explicit StateChangesStream(
        asio::io_context& scheduler,
        grpc::CompletionQueue* queue,
        remote::KV::StubInterface* stub,
        StateCache* cache);

    void open();

    void close();

  private:
    void schedule_open();

    static boost::posix_time::milliseconds registration_interval_;

    grpc::CompletionQueue* queue_;

    remote::KV::StubInterface* stub_;

    StateCache* cache_;

    std::unique_ptr<AsyncStateChangesCall> state_changes_call_;

    //! The state changes options.
    remote::StateChangeRequest request_;

    //! The timer to reschedule registration for state changes.
    asio::deadline_timer registration_timer_;

    //! Flag indicating if the stream has been cancelled or not.
    bool cancelled_{false};
};

} // namespace silkrpc::ethdb::kv

#endif // SILKRPC_ETHDB_KV_STATE_CHANGES_STREAM_HPP_
