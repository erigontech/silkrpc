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

#ifndef SILKRPC_TEST_KV_TEST_BASE_HPP_
#define SILKRPC_TEST_KV_TEST_BASE_HPP_

#include <memory>

#include <agrpc/test.hpp>

#include <silkrpc/config.hpp>
#include <silkrpc/test/context_test_base.hpp>
#include <silkrpc/test/grpc_responder.hpp>
#include <silkrpc/interfaces/remote/kv_mock.grpc.pb.h>

namespace silkrpc::test {

struct KVTestBase : test::ContextTestBase {
    testing::Expectation expect_request_async_tx() {
        return EXPECT_CALL(*stub_, AsyncTxRaw).WillOnce([&](auto&&, auto&&, void* tag) {
            agrpc::process_grpc_tag(grpc_context_, tag, true);
            return reader_writer_ptr_.release();
        });
    }

    using StrictMockKVStub = testing::StrictMock<remote::MockKVStub>;
    using StrictMockKVAsyncReaderWriter = test::StrictMockAsyncReaderWriter<remote::Cursor, remote::Pair>;

    std::unique_ptr<StrictMockKVStub> stub_{std::make_unique<StrictMockKVStub>()};
    std::unique_ptr<StrictMockKVAsyncReaderWriter> reader_writer_ptr_{
        std::make_unique<StrictMockKVAsyncReaderWriter>()};
    StrictMockKVAsyncReaderWriter& reader_writer_{*reader_writer_ptr_};
};

}  // namespace silkrpc::test

#endif  // SILKRPC_TEST_KV_TEST_BASE_HPP_
