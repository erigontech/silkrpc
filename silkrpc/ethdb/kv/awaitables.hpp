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

#ifndef SILKRPC_ETHDB_KV_AWAITABLES_HPP_
#define SILKRPC_ETHDB_KV_AWAITABLES_HPP_

#include <silkrpc/config.hpp>

#include <functional>
#include <string>
#include <system_error>
#include <thread>
#include <utility>

#include <asio/async_result.hpp>
#include <asio/detail/non_const_lvalue.hpp>
#include <asio/error.hpp>
#include <asio/io_context.hpp>
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
#include <silkrpc/ethdb/kv/error.hpp>
#include <silkrpc/ethdb/kv/streaming_client.hpp>
#include <silkrpc/grpc/awaitables.hpp>
#include <silkrpc/interfaces/remote/kv.grpc.pb.h>

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
        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        typedef silkrpc::ethdb::kv::async_start<WaitHandler, Executor> op;
        typename op::ptr p = {asio::detail::addressof(handler2.value), op::ptr::allocate(handler2.value), 0};
        wrapper_ = new op(handler2.value, self_->context_.get_executor());

        self_->client_.start_call([this](const grpc::Status& status) {
            typedef silkrpc::ethdb::kv::async_start<WaitHandler, Executor> op;
            auto start_op = static_cast<op*>(wrapper_);
            if (status.ok()) {
                start_op->complete(this, {});
            } else {
                start_op->complete(this, KVError::rpc_start_stream_failed);
            }
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
        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        typedef silkrpc::ethdb::kv::async_open_cursor<WaitHandler, Executor> op;
        typename op::ptr p = {asio::detail::addressof(handler2.value), op::ptr::allocate(handler2.value), 0};
        wrapper_ = new op(handler2.value, self_->context_.get_executor());

        auto open_message = remote::Cursor{};
        open_message.set_op(remote::Op::OPEN);
        open_message.set_bucketname(table_name_);
        self_->client_.write_start(open_message, [this](const grpc::Status& status) {
            if (!status.ok()) {
                auto open_cursor_op = static_cast<op*>(wrapper_);
                open_cursor_op->complete(this, KVError::rpc_open_cursor_write_stream_failed, 0);
                return;
            }
            self_->client_.read_start([this](const grpc::Status& status, remote::Pair open_pair) {
                auto cursor_id = open_pair.cursorid();

                typedef silkrpc::ethdb::kv::async_open_cursor<WaitHandler, Executor> op;
                auto open_cursor_op = static_cast<op*>(wrapper_);
                if (status.ok()) {
                    open_cursor_op->complete(this, {}, cursor_id);
                } else {
                    open_cursor_op->complete(this, KVError::rpc_open_cursor_read_stream_failed, 0);
                }
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

    explicit initiate_async_seek(KvAsioAwaitable<Executor>* self, uint32_t cursor_id, const silkworm::ByteView& key, bool exact)
    : self_(self), cursor_id_(cursor_id), key_(std::move(key)), exact_(exact) {}

    executor_type get_executor() const noexcept { return self_->get_executor(); }

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        typedef silkrpc::ethdb::kv::async_seek<WaitHandler, Executor> op;
        typename op::ptr p = {asio::detail::addressof(handler2.value), op::ptr::allocate(handler2.value), 0};
        wrapper_ = new op(handler2.value, self_->context_.get_executor());

        auto seek_message = remote::Cursor{};
        seek_message.set_op(exact_ ? remote::Op::SEEK_EXACT : remote::Op::SEEK);
        seek_message.set_cursor(cursor_id_);
        seek_message.set_k(key_.data(), key_.length());
        self_->client_.write_start(seek_message, [this](const grpc::Status& status) {
            if (!status.ok()) {
                auto seek_op = static_cast<op*>(wrapper_);
                seek_op->complete(this, KVError::rpc_seek_write_stream_failed, {});
                return;
            }
            self_->client_.read_start([this](const grpc::Status& status, remote::Pair seek_pair) {
                typedef silkrpc::ethdb::kv::async_seek<WaitHandler, Executor> op;
                auto seek_op = static_cast<op*>(wrapper_);
                if (status.ok()) {
                    seek_op->complete(this, {}, seek_pair);
                } else {
                    seek_op->complete(this, KVError::rpc_seek_read_stream_failed, {});
                }
            });
        });
    }

private:
    KvAsioAwaitable<Executor>* self_;
    uint32_t cursor_id_;
    const silkworm::ByteView key_;
    bool exact_;
    void* wrapper_;
};

template<typename Executor>
class initiate_async_seek_both {
public:
    typedef Executor executor_type;

    explicit initiate_async_seek_both(KvAsioAwaitable<Executor>* self, uint32_t cursor_id, const silkworm::ByteView& key, const silkworm::ByteView& value, bool exact)
    : self_(self), cursor_id_(cursor_id), key_(std::move(key)), value_(std::move(value)), exact_(exact) {}

    executor_type get_executor() const noexcept { return self_->get_executor(); }

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        typedef silkrpc::ethdb::kv::async_seek<WaitHandler, Executor> op;
        typename op::ptr p = {asio::detail::addressof(handler2.value), op::ptr::allocate(handler2.value), 0};
        wrapper_ = new op(handler2.value, self_->context_.get_executor());

        auto seek_message = remote::Cursor{};
        seek_message.set_op(exact_ ? remote::Op::SEEK_BOTH_EXACT : remote::Op::SEEK_BOTH);
        seek_message.set_cursor(cursor_id_);
        seek_message.set_k(key_.data(), key_.length());
        seek_message.set_v(value_.data(), value_.length());
        self_->client_.write_start(seek_message, [this](const grpc::Status& status) {
            if (!status.ok()) {
                auto seek_op = static_cast<op*>(wrapper_);
                seek_op->complete(this, KVError::rpc_seek_both_write_stream_failed, {});
                return;
            }
            self_->client_.read_start([this](const grpc::Status& status, remote::Pair seek_pair) {
                typedef silkrpc::ethdb::kv::async_seek<WaitHandler, Executor> op;
                auto seek_op = static_cast<op*>(wrapper_);
                if (status.ok()) {
                    seek_op->complete(this, {}, seek_pair);
                } else {
                    seek_op->complete(this, KVError::rpc_seek_both_read_stream_failed, {});
                }
            });
        });
    }

private:
    KvAsioAwaitable<Executor>* self_;
    uint32_t cursor_id_;
    const silkworm::ByteView key_;
    const silkworm::ByteView value_;
    bool exact_;
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
        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        typedef silkrpc::ethdb::kv::async_next<WaitHandler, Executor> op;
        typename op::ptr p = {asio::detail::addressof(handler2.value), op::ptr::allocate(handler2.value), 0};
        wrapper_ = new op(handler2.value, self_->context_.get_executor());

        auto next_message = remote::Cursor{};
        next_message.set_op(remote::Op::NEXT);
        next_message.set_cursor(cursor_id_);
        self_->client_.write_start(next_message, [this](const grpc::Status& status) {
            if (!status.ok()) {
                auto next_op = static_cast<op*>(wrapper_);
                next_op->complete(this, KVError::rpc_next_write_stream_failed, {});
                return;
            }
            self_->client_.read_start([this](const grpc::Status& status, remote::Pair next_pair) {
                typedef silkrpc::ethdb::kv::async_next<WaitHandler, Executor> op;
                auto next_op = static_cast<op*>(wrapper_);
                if (status.ok()) {
                    next_op->complete(this, {}, next_pair);
                } else {
                    next_op->complete(this, KVError::rpc_next_read_stream_failed, {});
                }
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
        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        typedef silkrpc::ethdb::kv::async_close_cursor<WaitHandler, Executor> op;
        typename op::ptr p = {asio::detail::addressof(handler2.value), op::ptr::allocate(handler2.value), 0};
        wrapper_ = new op(handler2.value, self_->context_.get_executor());

        auto close_message = remote::Cursor{};
        close_message.set_op(remote::Op::CLOSE);
        close_message.set_cursor(cursor_id_);
        self_->client_.write_start(close_message, [this](const grpc::Status& status) {
            if (!status.ok()) {
                auto close_cursor_op = static_cast<op*>(wrapper_);
                close_cursor_op->complete(this, KVError::rpc_close_cursor_write_stream_failed, 0);
                return;
            }
            self_->client_.read_start([this](const grpc::Status& status, remote::Pair close_pair) {
                auto cursor_id = close_pair.cursorid();

                typedef silkrpc::ethdb::kv::async_close_cursor<WaitHandler, Executor> op;
                auto close_cursor_op = static_cast<op*>(wrapper_);
                if (status.ok()) {
                    close_cursor_op->complete(this, {}, cursor_id);
                } else {
                    close_cursor_op->complete(this, KVError::rpc_close_cursor_read_stream_failed, 0);
                }
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
        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        typedef silkrpc::ethdb::kv::async_end<WaitHandler, Executor> op;
        typename op::ptr p = {asio::detail::addressof(handler2.value), op::ptr::allocate(handler2.value), 0};
        wrapper_ = new op(handler2.value, self_->context_.get_executor());

        self_->client_.end_call([this](const grpc::Status& status) {
            typedef silkrpc::ethdb::kv::async_end<WaitHandler, Executor> op;
            auto end_op = static_cast<op*>(wrapper_);
            if (status.ok()) {
                end_op->complete(this, {});
            } else {
                end_op->complete(this, KVError::rpc_end_stream_failed);
            }
        });
    }

private:
    KvAsioAwaitable<Executor>* self_;
    void* wrapper_;
};

template<typename Executor>
struct KvAsioAwaitable {
    typedef Executor executor_type;

    explicit KvAsioAwaitable(asio::io_context& context, StreamingClient& client)
    : context_(context), client_(client) {}

    template<typename WaitHandler>
    auto async_start(WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(asio::error_code)>(initiate_async_start{this}, handler);
    }

    template<typename WaitHandler>
    auto async_open_cursor(const std::string& table_name, WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(asio::error_code, uint32_t)>(initiate_async_open_cursor{this, table_name}, handler);
    }

    template<typename WaitHandler>
    auto async_seek(uint32_t cursor_id, const silkworm::ByteView& key, WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(asio::error_code, remote::Pair)>(initiate_async_seek{this, cursor_id, key, false}, handler);
    }

    template<typename WaitHandler>
    auto async_seek_exact(uint32_t cursor_id, const silkworm::ByteView& key, WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(asio::error_code, remote::Pair)>(initiate_async_seek{this, cursor_id, key, true}, handler);
    }

    template<typename WaitHandler>
    auto async_seek_both(uint32_t cursor_id, const silkworm::ByteView& key, const silkworm::ByteView& value, WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(asio::error_code, remote::Pair)>(initiate_async_seek_both{this, cursor_id, key, value, false}, handler);
    }

    template<typename WaitHandler>
    auto async_seek_both_exact(uint32_t cursor_id, const silkworm::ByteView& key, const silkworm::ByteView& value, WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(asio::error_code, remote::Pair)>(initiate_async_seek_both{this, cursor_id, key, value, true}, handler);
    }

    template<typename WaitHandler>
    auto async_next(uint32_t cursor_id, WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(asio::error_code, remote::Pair)>(initiate_async_next{this, cursor_id}, handler);
    }

    template<typename WaitHandler>
    auto async_close_cursor(uint32_t cursor_id, WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(asio::error_code, uint32_t)>(initiate_async_close_cursor{this, cursor_id}, handler);
    }

    template<typename WaitHandler>
    auto async_end(WaitHandler&& handler) {
        return asio::async_initiate<WaitHandler, void(asio::error_code)>(initiate_async_end{this}, handler);
        //return asio::async_initiate<WaitHandler, void(asio::error_code)>(initiate_unary_async<Executor, UnaryClient, async_noreply_operation, void>{this}, handler);
    }

    asio::io_context& context_;
    StreamingClient& client_;
};

} // namespace silkrpc::ethdb::kv

#endif // SILKRPC_ETHDB_KV_AWAITABLES_HPP_
