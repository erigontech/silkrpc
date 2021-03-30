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

#include <silkrpc/config.hpp>

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
#include <silkrpc/ethdb/kv/async_close_cursor.hpp>
#include <silkrpc/ethdb/kv/async_end.hpp>
#include <silkrpc/ethdb/kv/async_next.hpp>
#include <silkrpc/ethdb/kv/async_open_cursor.hpp>
#include <silkrpc/ethdb/kv/async_seek.hpp>
#include <silkrpc/ethdb/kv/async_start.hpp>
#include <silkrpc/ethdb/kv/client_callback_reactor.hpp>
#include <silkrpc/ethdb/kv/util.hpp>
#include <silkrpc/ethdb/kv/remote/kv.grpc.pb.h>

namespace silkrpc::ethdb::kv {

template<typename Executor>
struct KvAsioAwaitable;

template<typename Executor>
class initiate_async_start {
public:
    typedef Executor executor_type;

    explicit initiate_async_start(KvAsioAwaitable<Executor>* self)
    : self_(self) {}

    executor_type get_executor() const noexcept { return self_->get_executor(); }

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        ASIO_WAIT_HANDLER_CHECK(WaitHandler, handler) type_check;

        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        typedef silkrpc::ethdb::kv::async_start<WaitHandler, Executor> op;
        typename op::ptr p = {asio::detail::addressof(handler2.value), op::ptr::allocate(handler2.value), 0};
        wrapper_ = new op(handler2.value, self_->context_.get_executor());

        self_->reactor_.start_call([this](const ::grpc::Status& status) {
            typedef silkrpc::ethdb::kv::async_start<WaitHandler, Executor> op;
            auto start_op = static_cast<op*>(wrapper_);
            if (!status.ok()) {
                throw make_system_error(status, "RPC failed in START_CALL");
            }
            start_op->complete(this, 0);
        });
    }

private:
    KvAsioAwaitable<Executor>* self_;
    void* wrapper_;
};

template<typename Executor>
class initiate_async_open_cursor {
public:
    typedef Executor executor_type;

    explicit initiate_async_open_cursor(KvAsioAwaitable<Executor>* self, const std::string& table_name)
    : self_(self), table_name_(table_name) {}

    executor_type get_executor() const noexcept { return self_->get_executor(); }

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        ASIO_WAIT_HANDLER_CHECK(WaitHandler, handler) type_check;

        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        typedef silkrpc::ethdb::kv::async_open_cursor<WaitHandler, Executor> op;
        typename op::ptr p = {asio::detail::addressof(handler2.value), op::ptr::allocate(handler2.value), 0};
        wrapper_ = new op(handler2.value, self_->context_.get_executor());

        auto open_message = remote::Cursor{};
        open_message.set_op(remote::Op::OPEN);
        open_message.set_bucketname(table_name_);
        self_->reactor_.write_start(open_message, [this](const ::grpc::Status& status) {
            if (!status.ok()) {
                throw make_system_error(status, "write failed in OPEN_CURSOR");
                return;
            }
            self_->reactor_.read_start([this](const ::grpc::Status& status, remote::Pair open_pair) {
                auto cursor_id = open_pair.cursorid();

                typedef silkrpc::ethdb::kv::async_open_cursor<WaitHandler, Executor> op;
                auto open_cursor_op = static_cast<op*>(wrapper_);
                if (!status.ok()) {
                    throw make_system_error(status, "read failed in OPEN_CURSOR");
                }
                open_cursor_op->complete(this, cursor_id);
            });
        });
    }

private:
    KvAsioAwaitable<Executor>* self_;
    const std::string& table_name_;
    void* wrapper_;
};

template<typename Executor>
class initiate_async_seek {
public:
    typedef Executor executor_type;

    explicit initiate_async_seek(KvAsioAwaitable<Executor>* self, uint32_t cursor_id, const silkworm::Bytes& seek_key_bytes)
    : self_(self), cursor_id_(cursor_id), seek_key_bytes_(std::move(seek_key_bytes)) {}

    executor_type get_executor() const noexcept { return self_->get_executor(); }

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        ASIO_WAIT_HANDLER_CHECK(WaitHandler, handler) type_check;

        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        typedef silkrpc::ethdb::kv::async_seek<WaitHandler, Executor> op;
        typename op::ptr p = {asio::detail::addressof(handler2.value), op::ptr::allocate(handler2.value), 0};
        wrapper_ = new op(handler2.value, self_->context_.get_executor());

        auto seek_message = remote::Cursor{};
        seek_message.set_op(remote::Op::SEEK);
        seek_message.set_cursor(cursor_id_);
        seek_message.set_k(seek_key_bytes_.c_str(), seek_key_bytes_.length());
        self_->reactor_.write_start(seek_message, [this](const ::grpc::Status& status) {
            if (!status.ok()) {
                throw make_system_error(status, "write failed in SEEK");
                return;
            }
            self_->reactor_.read_start([this](const ::grpc::Status& status, remote::Pair seek_pair) {
                typedef silkrpc::ethdb::kv::async_seek<WaitHandler, Executor> op;
                auto seek_op = static_cast<op*>(wrapper_);
                if (!status.ok()) {
                    throw make_system_error(status, "read failed in SEEK");
                }
                seek_op->complete(this, seek_pair);
            });
        });
    }

private:
    KvAsioAwaitable<Executor>* self_;
    uint32_t cursor_id_;
    const silkworm::Bytes seek_key_bytes_;
    void* wrapper_;
};

template<typename Executor>
class initiate_async_next {
public:
    typedef Executor executor_type;

    explicit initiate_async_next(KvAsioAwaitable<Executor>* self, uint32_t cursor_id)
    : self_(self), cursor_id_(cursor_id) {}

    executor_type get_executor() const noexcept { return self_->get_executor(); }

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        ASIO_WAIT_HANDLER_CHECK(WaitHandler, handler) type_check;

        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        typedef silkrpc::ethdb::kv::async_next<WaitHandler, Executor> op;
        typename op::ptr p = {asio::detail::addressof(handler2.value), op::ptr::allocate(handler2.value), 0};
        wrapper_ = new op(handler2.value, self_->context_.get_executor());

        auto next_message = remote::Cursor{};
        next_message.set_op(remote::Op::NEXT);
        next_message.set_cursor(cursor_id_);
        self_->reactor_.write_start(next_message, [this](const ::grpc::Status& status) {
            if (!status.ok()) {
                throw make_system_error(status, "write failed in NEXT");
                return;
            }
            self_->reactor_.read_start([this](const ::grpc::Status& status, remote::Pair next_pair) {
                typedef silkrpc::ethdb::kv::async_next<WaitHandler, Executor> op;
                auto next_op = static_cast<op*>(wrapper_);
                if (!status.ok()) {
                    throw std::system_error{std::make_error_code(std::errc::io_error), "read failed in NEXT"};
                }
                next_op->complete(this, next_pair);
            });
        });
    }

private:
    KvAsioAwaitable<Executor>* self_;
    uint32_t cursor_id_;
    const silkworm::Bytes next_key_bytes_;
    void* wrapper_;
};

template<typename Executor>
class initiate_async_close_cursor {
public:
    typedef Executor executor_type;

    explicit initiate_async_close_cursor(KvAsioAwaitable<Executor>* self, uint32_t cursor_id)
    : self_(self), cursor_id_(cursor_id) {}

    executor_type get_executor() const noexcept { return self_->get_executor(); }

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        ASIO_WAIT_HANDLER_CHECK(WaitHandler, handler) type_check;

        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        typedef silkrpc::ethdb::kv::async_close_cursor<WaitHandler, Executor> op;
        typename op::ptr p = {asio::detail::addressof(handler2.value), op::ptr::allocate(handler2.value), 0};
        wrapper_ = new op(handler2.value, self_->context_.get_executor());

        auto close_message = remote::Cursor{};
        close_message.set_op(remote::Op::CLOSE);
        close_message.set_cursor(cursor_id_);
        self_->reactor_.write_start(close_message, [this](const ::grpc::Status& status) {
            if (!status.ok()) {
                throw make_system_error(status, "write failed in CLOSE_CURSOR");
                return;
            }
            self_->reactor_.read_start([this](const ::grpc::Status& status, remote::Pair close_pair) {
                auto cursor_id = close_pair.cursorid();

                typedef silkrpc::ethdb::kv::async_close_cursor<WaitHandler, Executor> op;
                auto close_cursor_op = static_cast<op*>(wrapper_);
                if (!status.ok()) {
                    throw make_system_error(status, "read failed in CLOSE_CURSOR");
                }
                close_cursor_op->complete(this, cursor_id);
            });
        });
    }

private:
    KvAsioAwaitable<Executor>* self_;
    uint32_t cursor_id_;
    void* wrapper_;
};

template<typename Executor>
class initiate_async_end {
public:
    typedef Executor executor_type;

    explicit initiate_async_end(KvAsioAwaitable<Executor>* self)
    : self_(self) {}

    executor_type get_executor() const noexcept { return self_->get_executor(); }

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        ASIO_WAIT_HANDLER_CHECK(WaitHandler, handler) type_check;

        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        typedef silkrpc::ethdb::kv::async_end<WaitHandler, Executor> op;
        typename op::ptr p = {asio::detail::addressof(handler2.value), op::ptr::allocate(handler2.value), 0};
        wrapper_ = new op(handler2.value, self_->context_.get_executor());

        self_->reactor_.end_call([this](const ::grpc::Status& status) {
            typedef silkrpc::ethdb::kv::async_end<WaitHandler, Executor> op;
            auto end_op = static_cast<op*>(wrapper_);
            if (!status.ok()) {
                throw make_system_error(status, "RPC failed in END_CALL");
            }
            end_op->complete(this, 0);
        });
    }

private:
    KvAsioAwaitable<Executor>* self_;
    void* wrapper_;
};

template<typename Executor>
struct KvAsioAwaitable {
    typedef Executor executor_type;

    explicit KvAsioAwaitable(asio::io_context& context, ClientCallbackReactor& reactor)
    : context_(context), reactor_(reactor) {}

    template<typename WaitHandler>
    auto async_start(WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(uint32_t)>(initiate_async_start{this}, handler);
    }

    template<typename WaitHandler>
    auto async_open_cursor(const std::string& table_name, WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(uint32_t)>(initiate_async_open_cursor{this, table_name}, handler);
    }

    template<typename WaitHandler>
    auto async_seek(uint32_t cursor_id, const silkworm::Bytes& seek_key_bytes, WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(remote::Pair)>(initiate_async_seek{this, cursor_id, seek_key_bytes}, handler);
    }

    template<typename WaitHandler>
    auto async_next(uint32_t cursor_id, WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(remote::Pair)>(initiate_async_next{this, cursor_id}, handler);
    }

    template<typename WaitHandler>
    auto async_close_cursor(uint32_t cursor_id, WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(uint32_t)>(initiate_async_close_cursor{this, cursor_id}, handler);
    }

    template<typename WaitHandler>
    auto async_end(WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(uint32_t)>(initiate_async_end{this}, handler);
    }

    asio::io_context& context_;
    ClientCallbackReactor& reactor_;
};

} // namespace silkrpc::ethdb::kv

#endif // SILKRPC_KV_AWAITABLES_HPP
