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

#ifndef SILKRPC_KV_AWAITABLES_HPP
#define SILKRPC_KV_AWAITABLES_HPP

//#include <coroutine>
#include <functional>
#include <string>
#include <system_error>
#include <thread>

#include <asio/io_context.hpp>
#include <asio/post.hpp>
#include <grpcpp/grpcpp.h>

#include <silkworm/common/util.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/coro/coroutine.hpp>
#include <silkrpc/kv/client_callback_reactor.hpp>
#include <silkrpc/kv/remote/kv.grpc.pb.h>

namespace silkrpc::kv {

struct KvAwaitable {
    KvAwaitable(asio::io_context& context, ClientCallbackReactor& reactor, uint32_t cursor_id = 0)
    : context_(context), reactor_(reactor), cursor_id_(cursor_id) {}

    bool await_ready() noexcept { return false; }

protected:
    asio::io_context& context_;
    ClientCallbackReactor& reactor_;
    uint32_t cursor_id_;
};

struct KvOpenCursorAwaitable : KvAwaitable {
    KvOpenCursorAwaitable(asio::io_context& context, ClientCallbackReactor& reactor, const std::string& table_name)
    : KvAwaitable{context, reactor}, table_name_(table_name) {}

    auto await_suspend(std::coroutine_handle<void> handle) {
        auto open_message = remote::Cursor{};
        open_message.set_op(remote::Op::OPEN);
        open_message.set_bucketname(table_name_);
        reactor_.write_start(&open_message, [&, handle](bool ok) {
            if (!ok) {
                throw std::system_error{std::make_error_code(std::errc::io_error), "write failed in OPEN cursor"};
            }
            reactor_.read_start([&, handle](bool ok, remote::Pair open_pair) {
                if (!ok) {
                    throw std::system_error{std::make_error_code(std::errc::io_error), "read failed in OPEN cursor"};
                }
                cursor_id_ = open_pair.cursorid();
                asio::post(context_, [&handle]() { handle.resume(); });
            });
        });
    }

    auto await_resume() noexcept { return cursor_id_; }

private:
    const std::string& table_name_;
};

struct KvSeekAwaitable : KvAwaitable {
    KvSeekAwaitable(asio::io_context& context, ClientCallbackReactor& reactor, uint32_t cursor_id, const silkworm::Bytes& seek_key_bytes)
    : KvAwaitable{context, reactor, cursor_id}, seek_key_bytes_(std::move(seek_key_bytes)) {}

    auto await_suspend(std::coroutine_handle<void> handle) {
        auto seek_message = remote::Cursor{};
        seek_message.set_op(remote::Op::SEEK);
        seek_message.set_cursor(cursor_id_);
        seek_message.set_k(seek_key_bytes_.c_str(), seek_key_bytes_.length());
        reactor_.write_start(&seek_message, [&, handle](bool ok) {
            if (!ok) {
                throw std::system_error{std::make_error_code(std::errc::io_error), "write failed in SEEK"};
            }
            reactor_.read_start([&, handle](bool ok, remote::Pair seek_pair) {
                if (!ok) {
                    throw std::system_error{std::make_error_code(std::errc::io_error), "read failed in SEEK"};
                }
                const auto& key_bytes = silkworm::byte_view_of_string(seek_pair.k());
                const auto& value_bytes = silkworm::byte_view_of_string(seek_pair.v());
                value_bytes_ = std::move(value_bytes);
                asio::post(context_, [&handle]() { handle.resume(); });
            });
        });
    }

    auto await_resume() noexcept { return value_bytes_; }

private:
    const silkworm::Bytes seek_key_bytes_;
    silkworm::ByteView value_bytes_;
};

struct KvCloseCursorAwaitable : KvAwaitable {
    KvCloseCursorAwaitable(asio::io_context& context, ClientCallbackReactor& reactor, uint32_t cursor_id)
    : KvAwaitable{context, reactor, cursor_id} {}

    auto await_suspend(std::coroutine_handle<void> handle) {
        auto close_message = remote::Cursor{};
        close_message.set_op(remote::Op::CLOSE);
        close_message.set_cursor(cursor_id_);
        reactor_.write_start(&close_message, [&, handle](bool ok) {
            if (!ok) {
                throw std::system_error{std::make_error_code(std::errc::io_error), "write failed in CLOSE cursor"};
            }
            reactor_.read_start([&, handle](bool ok, remote::Pair close_pair) {
                if (!ok) {
                    throw std::system_error{std::make_error_code(std::errc::io_error), "read failed in CLOSE cursor"};
                }
                cursor_id_ = close_pair.cursorid();
                asio::post(context_, [&handle]() { handle.resume(); });
            });
        });
    }

    auto await_resume() noexcept { return cursor_id_; }
};

} // namespace silkrpc::kv

#endif // SILKRPC_KV_AWAITABLES_HPP
