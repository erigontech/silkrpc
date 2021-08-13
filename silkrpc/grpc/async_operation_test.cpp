/*
   Copyright 2021 The Silkrpc Authors

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

#include "async_operation.hpp"

#include <future>

#include <asio/async_result.hpp>
#include <asio/error_code.hpp>
#include <asio/io_context.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch.hpp>

namespace silkrpc {

using Catch::Matchers::Message;

TEST_CASE("call hook on async operation completion", "[silkrpc][grpc][async_operation]") {
    class AO : public async_operation<void, asio::error_code> {
    public:
        AO() : async_operation<void, asio::error_code>(&AO::do_complete) {}
        static void do_complete(void* owner, async_operation<void, asio::error_code>* base, asio::error_code error = {}) {
            auto self = static_cast<AO*>(base);
            self->called = true;
        }
        bool called{false};
    };
    AO op{};
    op.complete(reinterpret_cast<void*>(1), {});
    CHECK(op.called == true);
}

class initiate_async_noreply_op_immediate {
public:
    using executor_type = asio::io_context::executor_type;

    explicit initiate_async_noreply_op_immediate(executor_type executor) : executor_(executor) {}

    executor_type get_executor() const noexcept { return executor_; }

    template<typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        using async_op = async_noreply_operation<WaitHandler, asio::io_context::executor_type>;
        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        auto op = new async_op(handler2.value, executor_);
        op->complete(op, {});
    }
private:
    executor_type executor_;
};

template<typename WaitHandler>
auto async_noreply_op_immediate(WaitHandler&& handler, asio::io_context& io_context) {
    return asio::async_initiate<WaitHandler, void(asio::error_code)>(initiate_async_noreply_op_immediate{io_context.get_executor()}, handler);
}

TEST_CASE("give future result on async void operation with immediate completion", "[silkrpc][grpc][async_operation]") {
    asio::io_context io_context;
    using use_future_t_lvalue_ref = typename std::add_lvalue_reference<asio::use_future_t<>>::type;
    asio::detail::non_const_lvalue<asio::use_future_t<>> handler(const_cast<use_future_t_lvalue_ref>(asio::use_future));
    auto future_result = async_noreply_op_immediate(asio::use_future, io_context);
    CHECK_NOTHROW(future_result.get());
}

class initiate_async_noreply_op_delayed {
public:
    using executor_type = asio::io_context::executor_type;

    explicit initiate_async_noreply_op_delayed(executor_type executor) : executor_(executor) {}

    executor_type get_executor() const noexcept { return executor_; }

    template<typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        using async_op = async_noreply_operation<WaitHandler, asio::io_context::executor_type>;
        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        auto op = new async_op(handler2.value, executor_);
        auto result = std::async([&]() {
            std::this_thread::yield();
            op->complete(op, {});
        });
    }
private:
    executor_type executor_;
};

template<typename WaitHandler>
auto async_noreply_op_delayed(WaitHandler&& handler, asio::io_context& io_context) {
    return asio::async_initiate<WaitHandler, void(asio::error_code)>(initiate_async_noreply_op_delayed{io_context.get_executor()}, handler);
}

TEST_CASE("give future result on async void operation with delayed completion", "[silkrpc][grpc][async_operation]") {
    asio::io_context io_context;
    using use_future_t_lvalue_ref = typename std::add_lvalue_reference<asio::use_future_t<>>::type;
    asio::detail::non_const_lvalue<asio::use_future_t<>> handler(const_cast<use_future_t_lvalue_ref>(asio::use_future));
    auto future_result = async_noreply_op_delayed(asio::use_future, io_context);
    CHECK_NOTHROW(future_result.get());
}

class initiate_async_op_immediate {
public:
    using executor_type = asio::io_context::executor_type;

    explicit initiate_async_op_immediate(executor_type executor, uint32_t id) : executor_(executor), id_(id) {}

    executor_type get_executor() const noexcept { return executor_; }

    template<typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        using async_op = async_reply_operation<WaitHandler, asio::io_context::executor_type, uint32_t>;
        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        auto op = new async_op(handler2.value, executor_);
        op->complete(op, {}, id_);
    }
private:
    executor_type executor_;
    uint32_t id_;
};

template<typename WaitHandler>
auto async_op_immediate(WaitHandler&& handler, uint32_t id, asio::io_context& io_context) {
    return asio::async_initiate<WaitHandler, void(asio::error_code, uint32_t)>(initiate_async_op_immediate{io_context.get_executor(), id}, handler);
}

TEST_CASE("give future result on async operation with immediate completion", "[silkrpc][grpc][async_operation]") {
    asio::io_context io_context;
    using use_future_t_lvalue_ref = typename std::add_lvalue_reference<asio::use_future_t<>>::type;
    asio::detail::non_const_lvalue<asio::use_future_t<>> handler(const_cast<use_future_t_lvalue_ref>(asio::use_future));
    auto future_result = async_op_immediate(asio::use_future, 18, io_context);
    CHECK_NOTHROW(future_result.get() == 18);
}

class initiate_async_op_delayed {
public:
    using executor_type = asio::io_context::executor_type;

    explicit initiate_async_op_delayed(executor_type executor, uint32_t id) : executor_(executor), id_(id) {}

    executor_type get_executor() const noexcept { return executor_; }

    template<typename WaitHandler>
    void operator()(WaitHandler&& handler) {
        using async_op = async_reply_operation<WaitHandler, asio::io_context::executor_type, uint32_t>;
        asio::detail::non_const_lvalue<WaitHandler> handler2(handler);
        auto op = new async_op(handler2.value, executor_);
        auto result = std::async([&]() {
            std::this_thread::yield();
            op->complete(op, {}, id_);
        });
    }
private:
    executor_type executor_;
    uint32_t id_;
};

template<typename WaitHandler>
auto async_op_delayed(WaitHandler&& handler, uint32_t id, asio::io_context& io_context) {
    return asio::async_initiate<WaitHandler, void(asio::error_code, uint32_t)>(initiate_async_op_delayed{io_context.get_executor(), id}, handler);
}

TEST_CASE("give future result on async operation with delayed completion", "[silkrpc][grpc][async_operation]") {
    asio::io_context io_context;
    using use_future_t_lvalue_ref = typename std::add_lvalue_reference<asio::use_future_t<>>::type;
    asio::detail::non_const_lvalue<asio::use_future_t<>> handler(const_cast<use_future_t_lvalue_ref>(asio::use_future));
    auto future_result = async_op_delayed(asio::use_future, 18, io_context);
    CHECK_NOTHROW(future_result.get() == 18);
}

} // namespace silkrpc
