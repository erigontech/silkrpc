/*
    Copyright 2022 The Silkrpc Authors

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

#include "state_changes_stream.hpp"

#include <ostream>

#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/util.hpp>

namespace remote {

inline std::ostream& operator<<(std::ostream& out, const remote::StateChangeBatch& batch) {
    out << "changebatch_size=" << batch.changebatch_size()
        << " databaseviewid=" << batch.databaseviewid()
        << " pendingblockbasefee=" << batch.pendingblockbasefee()
        << " blockgaslimit=" << batch.blockgaslimit();
    return out;
}

} // namespace remote

namespace silkrpc::ethdb::kv {

AsyncStateChangesCall::AsyncStateChangesCall(
    grpc::CompletionQueue* queue,
    remote::KV::StubInterface* stub,
    StateCache* cache,
    TerminationHook termination_hook)
    : AsyncServerStreamingCall(queue, stub), cache_(cache), termination_hook_(termination_hook) {
}

void AsyncStateChangesCall::handle_read() {
    SILKRPC_INFO << "StateChanges batch received: " << reply_ << "\n";
    cache_->on_new_block(reply_);
}

void AsyncStateChangesCall::handle_finish() {
    switch (status_.error_code()) {
        case grpc::StatusCode::OK:
            SILKRPC_INFO << "StateChanges completed status: " << status_ << "\n";
        break;
        case grpc::StatusCode::CANCELLED:
            SILKRPC_INFO << "StateChanges cancelled status: " << status_ << "\n";
        break;
        default:
            SILKRPC_ERROR << "StateChanges failed: " << status_ << "\n";
        break;
    }
    termination_hook_(status_.ok());
}

boost::posix_time::milliseconds StateChangesStream::registration_interval_{kDefaultRegistrationInterval};

void StateChangesStream::set_registration_interval(boost::posix_time::milliseconds registration_interval) {
    StateChangesStream::registration_interval_ = registration_interval;
}

StateChangesStream::StateChangesStream(
    asio::io_context& scheduler,
    grpc::CompletionQueue* queue,
    remote::KV::StubInterface* stub,
    StateCache* cache)
    : queue_(queue), stub_(stub), cache_(cache), registration_timer_{scheduler} {
}

void StateChangesStream::open() {
    state_changes_call_ = std::make_unique<AsyncStateChangesCall>(queue_, stub_, cache_, [&](bool) {
        state_changes_call_ = nullptr;
        SILKRPC_DEBUG << "State changes stream closed, schedule reopen\n";
        schedule_open();
    });
    state_changes_call_->start(request_);
    SILKRPC_INFO << "Registration for state changes started\n";
}

void StateChangesStream::close() {
    registration_timer_.cancel();
    SILKRPC_DEBUG << "Registration timer cancelled\n";

    if (state_changes_call_ && !cancelled_) {
        state_changes_call_->cancel();
        cancelled_ = true;
        SILKRPC_INFO << "Registration for state changes cancelled\n";
    }
}

void StateChangesStream::schedule_open() {
    registration_timer_.expires_from_now(registration_interval_);
    registration_timer_.async_wait([&](const auto& /*ec*/) { open(); });
}

} // namespace silkrpc::ethdb::kv
