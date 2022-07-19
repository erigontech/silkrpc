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

#include <chrono>
#include <functional>
#include <memory>

#include <silkrpc/config.hpp>

#include <asio/awaitable.hpp>
#ifndef ASIO_HAS_BOOST_DATE_TIME
#define ASIO_HAS_BOOST_DATE_TIME
#endif
#include <asio/cancellation_signal.hpp>
#include <asio/deadline_timer.hpp>
#include <asio/io_context.hpp>

#include <silkrpc/concurrency/context_pool.hpp>
#include <silkrpc/ethdb/kv/rpc.hpp>
#include <silkrpc/ethdb/kv/state_cache.hpp>

namespace silkrpc::ethdb::kv {

//! The default registration interval.
constexpr boost::posix_time::milliseconds kDefaultRegistrationInterval{10'000};

class StateChangesStream {
public:
    static void set_registration_interval(boost::posix_time::milliseconds registration_interval);

    explicit StateChangesStream(Context& context, remote::KV::StubInterface* stub);

    void open();

    void close();

private:
    asio::awaitable<void> run();

    static boost::posix_time::milliseconds registration_interval_;

    asio::io_context& scheduler_;

    agrpc::GrpcContext& grpc_context_;

    remote::KV::StubInterface* stub_;

    StateCache* cache_;

    asio::cancellation_signal cancellation_signal_;

    //! The state changes options.
    remote::StateChangeRequest request_;

    //! The timer to reschedule retries for state changes.
    asio::deadline_timer retry_timer_;
};

} // namespace silkrpc::ethdb::kv

#endif // SILKRPC_ETHDB_KV_STATE_CHANGES_STREAM_HPP_
