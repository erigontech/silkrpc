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

#include <functional>
#include <string>
#include <system_error>
#include <thread>

#include <asio/async_result.hpp>
#include <asio/detail/non_const_lvalue.hpp>
#include <asio/error.hpp>
#include <asio/io_context.hpp>
#include <asio/post.hpp> // TBR
#include <grpcpp/grpcpp.h>

#include <silkworm/common/util.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/coro/coroutine.hpp>
#include <silkrpc/kv/async_close_cursor.hpp>
#include <silkrpc/kv/async_open_cursor.hpp>
#include <silkrpc/kv/async_seek.hpp>
#include <silkrpc/kv/client_callback_reactor.hpp>
#include <silkrpc/kv/remote/kv.grpc.pb.h>

namespace silkrpc::kv {

template<typename Executor>
struct KvAsyncAwaitable;

template<typename Executor>
class initiate_async_open_cursor {
public:
    typedef Executor executor_type;

    explicit initiate_async_open_cursor(KvAsyncAwaitable<Executor>* self, const std::string& table_name)
    : self_(self), table_name_(table_name) {}

    executor_type get_executor() const noexcept { return self_->get_executor(); }

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        ASIO_WAIT_HANDLER_CHECK(WaitHandler, handler) type_check;

        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        typedef silkrpc::kv::async_open_cursor<WaitHandler, Executor> op;
        typename op::ptr p = {asio::detail::addressof(handler2.value), op::ptr::allocate(handler2.value), 0};
        wrapper_ = new op(handler2.value, self_->context_.get_executor());

        auto open_message = remote::Cursor{};
        open_message.set_op(remote::Op::OPEN);
        open_message.set_bucketname(table_name_);
        self_->reactor_.write_start(&open_message, [this](bool ok) {
            if (!ok) {
                throw std::system_error{std::make_error_code(std::errc::io_error), "write failed in OPEN cursor"};
            }
            self_->reactor_.read_start([this](bool ok, remote::Pair open_pair) {
                if (!ok) {
                    throw std::system_error{std::make_error_code(std::errc::io_error), "read failed in OPEN cursor"};
                }
                auto cursor_id = open_pair.cursorid();

                typedef silkrpc::kv::async_open_cursor<WaitHandler, Executor> op;
                auto open_cursor_op = static_cast<op*>(wrapper_);

                // Make the io_context thread execute the operation completion
                self_->context_.post([this, open_cursor_op, cursor_id]() {
                    open_cursor_op->complete(this, cursor_id);
                });
            });
        });
    }

private:
    KvAsyncAwaitable<Executor>* self_;
    const std::string& table_name_;
    void* wrapper_;
};

template<typename Executor>
class initiate_async_seek {
public:
    typedef Executor executor_type;

    explicit initiate_async_seek(KvAsyncAwaitable<Executor>* self, uint32_t cursor_id, const silkworm::Bytes& seek_key_bytes)
    : self_(self), cursor_id_(cursor_id), seek_key_bytes_(std::move(seek_key_bytes)) {}

    executor_type get_executor() const noexcept { return self_->get_executor(); }

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        ASIO_WAIT_HANDLER_CHECK(WaitHandler, handler) type_check;

        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        typedef silkrpc::kv::async_seek<WaitHandler, Executor> op;
        typename op::ptr p = {asio::detail::addressof(handler2.value), op::ptr::allocate(handler2.value), 0};
        wrapper_ = new op(handler2.value, self_->context_.get_executor());

        auto seek_message = remote::Cursor{};
        seek_message.set_op(remote::Op::SEEK);
        seek_message.set_cursor(cursor_id_);
        seek_message.set_k(seek_key_bytes_.c_str(), seek_key_bytes_.length());
        self_->reactor_.write_start(&seek_message, [this](bool ok) {
            if (!ok) {
                throw std::system_error{std::make_error_code(std::errc::io_error), "write failed in SEEK"};
            }
            self_->reactor_.read_start([this](bool ok, remote::Pair seek_pair) {
                if (!ok) {
                    throw std::system_error{std::make_error_code(std::errc::io_error), "read failed in SEEK"};
                }

                typedef silkrpc::kv::async_seek<WaitHandler, Executor> op;
                auto seek_op = static_cast<op*>(wrapper_);

                // Make the io_context thread execute the operation completion
                self_->context_.post([this, seek_op, seek_pair]() {
                    seek_op->complete(this, seek_pair);
                });
            });
        });
    }

private:
    KvAsyncAwaitable<Executor>* self_;
    uint32_t cursor_id_;
    const silkworm::Bytes seek_key_bytes_;
    void* wrapper_;
};

template<typename Executor>
class initiate_async_close_cursor {
public:
    typedef Executor executor_type;

    explicit initiate_async_close_cursor(KvAsyncAwaitable<Executor>* self, uint32_t cursor_id)
    : self_(self), cursor_id_(cursor_id) {}

    executor_type get_executor() const noexcept { return self_->get_executor(); }

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        ASIO_WAIT_HANDLER_CHECK(WaitHandler, handler) type_check;

        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        typedef silkrpc::kv::async_close_cursor<WaitHandler, Executor> op;
        typename op::ptr p = {asio::detail::addressof(handler2.value), op::ptr::allocate(handler2.value), 0};
        wrapper_ = new op(handler2.value, self_->context_.get_executor());

        auto close_message = remote::Cursor{};
        close_message.set_op(remote::Op::CLOSE);
        close_message.set_cursor(cursor_id_);
        self_->reactor_.write_start(&close_message, [this](bool ok) {
            if (!ok) {
                throw std::system_error{std::make_error_code(std::errc::io_error), "write failed in CLOSE cursor"};
            }
            self_->reactor_.read_start([this](bool ok, remote::Pair close_pair) {
                if (!ok) {
                    throw std::system_error{std::make_error_code(std::errc::io_error), "read failed in CLOSE cursor"};
                }
                auto cursor_id = close_pair.cursorid();

                typedef silkrpc::kv::async_close_cursor<WaitHandler, Executor> op;
                auto close_cursor_op = static_cast<op*>(wrapper_);

                // Make the io_context thread execute the operation completion
                self_->context_.post([this, close_cursor_op, cursor_id]() {
                    close_cursor_op->complete(this, cursor_id);
                });
            });
        });
    }

private:
    KvAsyncAwaitable<Executor>* self_;
    uint32_t cursor_id_;
    void* wrapper_;
};

template<typename Executor>
struct KvAsyncAwaitable {
    typedef Executor executor_type;

    explicit KvAsyncAwaitable(asio::io_context& context, ClientCallbackReactor& reactor)
    : context_(context), reactor_(reactor) {}

    template<typename WaitHandler>
    auto async_open_cursor(const std::string& table_name, WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(uint32_t)>(initiate_async_open_cursor{this, table_name}, handler);
    }

    template<typename WaitHandler>
    auto async_seek(uint32_t cursor_id, const silkworm::Bytes& seek_key_bytes, WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(remote::Pair)>(initiate_async_seek{this, cursor_id, seek_key_bytes}, handler);
    }

    template<typename WaitHandler>
    auto async_close_cursor(uint32_t cursor_id, WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(uint32_t)>(initiate_async_close_cursor{this, cursor_id}, handler);
    }

    asio::io_context& context_;
    ClientCallbackReactor& reactor_;
};

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
