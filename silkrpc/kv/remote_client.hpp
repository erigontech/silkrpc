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

#define ASIO_HAS_CO_AWAIT
#define ASIO_HAS_STD_COROUTINE
#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <asio/use_awaitable.hpp>

#include <silkrpc/coro/coroutine.hpp>
#include <silkrpc/coro/task.hpp>
#include <silkrpc/kv/awaitables.hpp>
#include <silkrpc/kv/client.hpp>
#include <silkrpc/kv/client_callback_reactor.hpp>
#include <silkrpc/kv/remote/kv.grpc.pb.h>

namespace silkrpc::kv {

using namespace silkworm;

class RemoteClient : public Client {
public:
    explicit RemoteClient(asio::io_context& context, std::shared_ptr<grpc::Channel> channel)
    : context_(context), reactor_{channel}, kv_awaitable_{context_, reactor_} {}

    asio::awaitable<uint32_t> open_cursor(const std::string& table_name) {
        auto cursor_id = co_await kv_awaitable_.async_open_cursor(table_name, asio::use_awaitable);
        co_return cursor_id;
    }

    asio::awaitable<silkrpc::common::KeyValue> seek(uint32_t cursor_id, const silkworm::Bytes& seek_key) {
        auto seek_pair = co_await kv_awaitable_.async_seek(cursor_id, seek_key, asio::use_awaitable);
        const auto k = silkworm::bytes_of_string(seek_pair.k());
        const auto v = silkworm::bytes_of_string(seek_pair.v());
        co_return silkrpc::common::KeyValue{k, v};
    }

    asio::awaitable<void> close_cursor(uint32_t cursor_id) {
        co_await kv_awaitable_.async_close_cursor(cursor_id, asio::use_awaitable);
        co_return;
    }

private:
    asio::io_context& context_;
    ClientCallbackReactor reactor_;
    KvAsioAwaitable<asio::io_context::executor_type> kv_awaitable_;
};

} // namespace silkrpc::kv

#endif // SILKRPC_KV_REMOTE_CLIENT_HPP
