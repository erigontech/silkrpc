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

#include <future>
#include <system_error>

#include <asio/co_spawn.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/io_context.hpp>
#include <catch2/catch.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/test/grpc_actions.hpp>
#include <silkrpc/test/grpc_matcher.hpp>
#include <silkrpc/test/kv_test_base.hpp>

namespace silkrpc::ethdb::kv {

TEST_CASE("StateChangesStream::set_registration_interval", "[silkrpc][ethdb][kv][state_changes_stream]") {
    CHECK(StateChangesStream::registration_interval() == kDefaultRegistrationInterval);
    constexpr boost::posix_time::milliseconds new_registration_interval{5'000};
    CHECK_NOTHROW(StateChangesStream::set_registration_interval(new_registration_interval));
    CHECK(StateChangesStream::registration_interval() == new_registration_interval);
}

struct StateChangesStreamTest : test::KVTestBase {
    StateChangesStream stream_{context_, stub_.get()};
};

static remote::StateChangeBatch make_batch() {
    static uint64_t block_height{14'000'010};

    remote::StateChangeBatch state_changes{};
    remote::StateChange* latest_change = state_changes.add_changebatch();
    latest_change->set_blockheight(++block_height);

    return state_changes;
}

TEST_CASE_METHOD(StateChangesStreamTest, "StateChangesStream::open", "[silkrpc][ethdb][kv][state_changes_stream]") {
    StateChangesStream::set_registration_interval(boost::posix_time::milliseconds{10});

    SECTION("stream closed-by-peer/reopened/cancelled") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncStateChangesRaw calls succeed
        EXPECT_CALL(*stub_, AsyncStateChangesRaw)
            .WillOnce([&](auto&&, auto&, auto&&, void* tag) {
                //agrpc::process_grpc_tag(grpc_context_, tag, true);
                asio::post(io_context_, [&, tag]() { agrpc::process_grpc_tag(grpc_context_, tag, true); });

                // 2. AsyncReader<remote::StateChangeBatch>::Read 1st/2nd/3rd calls succeed, 4th fails
                EXPECT_CALL(*statechanges_reader_, Read)
                    .WillOnce(test::read_success_with(grpc_context_, make_batch()))
                    .WillOnce(test::read_success_with(grpc_context_, make_batch()))
                    .WillOnce(test::read_success_with(grpc_context_, make_batch()))
                    .WillOnce(test::read_failure(grpc_context_));
                // 3. AsyncReader<remote::StateChangeBatch>::Finish call succeeds w/ status OK
                EXPECT_CALL(*statechanges_reader_, Finish)
                    .WillOnce(test::finish_streaming_ok(grpc_context_));

                return statechanges_reader_ptr_.release();
            })
            .WillOnce([&](auto&&, auto&, auto&&, void* tag) {
                //agrpc::process_grpc_tag(grpc_context_, tag, true);
                asio::post(io_context_, [&, tag]() { agrpc::process_grpc_tag(grpc_context_, tag, true); });

                // Recreate mocked reader for StateChanges RPC
                statechanges_reader_ptr_ = std::make_unique<StrictMockKVStateChangesAsyncReader>();
                statechanges_reader_ = statechanges_reader_ptr_.get();

                // 2. AsyncReader<remote::StateChangeBatch>::Read 1st/2nd/3rd calls succeed, 4th fails
                EXPECT_CALL(*statechanges_reader_, Read)
                    .WillOnce(test::read_success_with(grpc_context_, make_batch()))
                    .WillOnce(test::read_success_with(grpc_context_, make_batch()))
                    .WillOnce(test::read_success_with(grpc_context_, make_batch()))
                    .WillOnce(test::read_failure(grpc_context_));
                // 3. AsyncReader<remote::StateChangeBatch>::Finish call succeeds w/ status cancelled
                EXPECT_CALL(*statechanges_reader_, Finish)
                    .WillOnce(test::finish_streaming_cancelled(grpc_context_));

                return statechanges_reader_ptr_.release();
            });

        // Execute the test: running the stream should succeed until finishes
        CHECK_NOTHROW(spawn_and_wait(stream_.run()));
    }
    SECTION("failure in first read") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncStateChangesRaw call succeeds
        expect_request_async_statechanges(/*.ok=*/true);
        // 2. AsyncReader<remote::StateChangeBatch>::Read call fails
        EXPECT_CALL(*statechanges_reader_, Read).WillOnce(test::read_failure(grpc_context_));
        // 3. AsyncReader<remote::StateChangeBatch>::Finish call succeeds w/ status cancelled
        EXPECT_CALL(*statechanges_reader_, Finish).WillOnce(test::finish_streaming_cancelled(grpc_context_));

        // Execute the test: running the stream should succeed until finishes
        CHECK_NOTHROW(spawn_and_wait(stream_.run()));
    }
    SECTION("failure in second read") {
        // Set the call expectations:
        // 1. remote::KV::StubInterface::AsyncStateChangesRaw call succeeds
        expect_request_async_statechanges(/*.ok=*/true);
        // 2. AsyncReader<remote::StateChangeBatch>::Read 1st call succeeds, 2nd call fails
        EXPECT_CALL(*statechanges_reader_, Read)
            .WillOnce(test::read_success_with(grpc_context_, remote::StateChangeBatch{}))
            .WillOnce(test::read_failure(grpc_context_));
        // 3. AsyncReader<remote::StateChangeBatch>::Finish call succeeds w/ status cancelled
        EXPECT_CALL(*statechanges_reader_, Finish).WillOnce(test::finish_streaming_cancelled(grpc_context_));

        // Execute the test: running the stream should succeed until finishes
        CHECK_NOTHROW(spawn_and_wait(stream_.run()));
    }
}

TEST_CASE_METHOD(StateChangesStreamTest, "StateChangesStream::close", "[silkrpc][ethdb][kv][state_changes_stream]") {
}

} // namespace silkrpc::ethdb::kv
