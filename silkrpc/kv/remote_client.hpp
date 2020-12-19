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

#ifndef SILKRPC_KV_REMOTE_CLIENT_HPP
#define SILKRPC_KV_REMOTE_CLIENT_HPP

//#include <coroutine>
#include <memory>

#include <asio/io_context.hpp>

#include <silkworm/common/util.hpp>
#include <silkrpc/coro/coroutine.hpp>
#include <silkrpc/coro/task.hpp>
#include <silkrpc/kv/awaitables.hpp>
#include <silkrpc/kv/client_callback_reactor.hpp>
#include <silkrpc/kv/remote/kv.grpc.pb.h>

namespace silkrpc::kv {

class RemoteClient {
public:
    explicit RemoteClient(asio::io_context& context, std::shared_ptr<grpc::Channel> channel)
    : context_(context), reactor_{channel} {}

    coro::task<uint32_t> open_cursor(const std::string& table_name) {
        auto cursor_id = co_await KvOpenCursorAwaitable{context_, reactor_, table_name};
        co_return cursor_id;
    }

    coro::task<silkworm::ByteView> seek(uint32_t cursor_id, const silkworm::Bytes& seek_key) {
        auto value_bytes = co_await KvSeekAwaitable{context_, reactor_, cursor_id, seek_key};
        co_return value_bytes;
    }

    coro::task<void> close_cursor(uint32_t cursor_id) {
        co_await KvCloseCursorAwaitable{context_, reactor_, cursor_id};
    }

private:
    asio::io_context& context_;
    ClientCallbackReactor reactor_;
};

} // namespace silkrpc::kv

#endif // SILKRPC_KV_REMOTE_CLIENT_HPP
