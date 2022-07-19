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

#include <asio/experimental/as_tuple.hpp>
#include <asio/this_coro.hpp>
#include <grpc/grpc.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/util.hpp>

namespace remote {

inline std::ostream& operator<<(std::ostream& out, const remote::StateChangeBatch& batch) {
    out << "changebatch_size=" << batch.changebatch_size() << " databaseviewid=" << batch.databaseviewid()
        << " pendingblockbasefee=" << batch.pendingblockbasefee() << " blockgaslimit=" << batch.blockgaslimit();
    return out;
}

} // namespace remote

namespace silkrpc::ethdb::kv {

//! Define Asio coroutine-based completion token using error codes instead of exceptions for errors
constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);

boost::posix_time::milliseconds StateChangesStream::registration_interval_{kDefaultRegistrationInterval};

void StateChangesStream::set_registration_interval(boost::posix_time::milliseconds registration_interval) {
    StateChangesStream::registration_interval_ = registration_interval;
}

StateChangesStream::StateChangesStream(Context& context, remote::KV::StubInterface* stub)
    : scheduler_(*context.io_context()),
      grpc_context_(*context.grpc_context()),
      cache_(context.state_cache().get()),
      stub_(stub),
      retry_timer_{scheduler_} {}

void StateChangesStream::open() {
    asio::co_spawn(scheduler_, run(), [&](std::exception_ptr eptr) {
        if (eptr) std::rethrow_exception(eptr);
    });
}

void StateChangesStream::close() {
    retry_timer_.cancel();
    SILKRPC_DEBUG << "Retry timer cancelled\n";

    cancellation_signal_.emit(asio::cancellation_type::all);
    SILKRPC_WARN << "Registration for state changes cancelled\n";
}

asio::awaitable<void> StateChangesStream::run() {
    SILKRPC_TRACE << "StateChangesStream::run state stream START\n";

    auto cancellation_slot = cancellation_signal_.slot();

    bool cancelled{false};
    while (!cancelled) {
        StateChangesRpc state_changes_rpc{*stub_, grpc_context_};

        cancellation_slot.assign([&](asio::cancellation_type /*type*/) {
            state_changes_rpc.cancel();
            SILKRPC_WARN << "State changes stream cancelled\n";
        });

        SILKRPC_INFO << "Registration for state changes started\n";
        const auto [sc] = co_await state_changes_rpc.request_on(scheduler_.get_executor(), request_, use_nothrow_awaitable);
        if (sc) {
            if (sc.value() == grpc::StatusCode::CANCELLED) {
                cancelled = true;
                SILKRPC_DEBUG << "State changes stream cancelled immediately\n";
            } else {
                SILKRPC_WARN << "State changes stream error [" << sc.message() << "], schedule reopen\n";
                retry_timer_.expires_from_now(registration_interval_);
                const auto [ec] = co_await retry_timer_.async_wait(use_nothrow_awaitable);
                if (ec == asio::error::operation_aborted) {
                    cancelled = true;
                    SILKRPC_DEBUG << "State changes wait before retry cancelled\n";
                }
            }
            continue;
        }
        SILKRPC_INFO << "State changes stream opened\n";

        while (true) {
            const auto [sc, reply] = co_await state_changes_rpc.read_on(scheduler_.get_executor(), use_nothrow_awaitable);
            if (sc) {
                if (sc.value() == grpc::StatusCode::CANCELLED) {
                    cancelled = true;
                    SILKRPC_DEBUG << "State changes stream cancelled immediately\n";
                } else {
                    SILKRPC_WARN << "State changes stream error [" << sc.message() << "], schedule reopen\n";
                    retry_timer_.expires_from_now(registration_interval_);
                    const auto [ec] = co_await retry_timer_.async_wait(use_nothrow_awaitable);
                    if (ec == asio::error::operation_aborted) {
                        cancelled = true;
                        SILKRPC_DEBUG << "State changes wait before retry cancelled\n";
                    }
                }
                break;
            }
            SILKRPC_INFO << "State changes batch received: " << reply << "\n";
            cache_->on_new_block(reply);
        }
    }

    SILKRPC_TRACE << "StateChangesStream::run state stream END\n";
}

} // namespace silkrpc::ethdb::kv
