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

#ifndef SILKRPC_CORO_TASK_HPP
#define SILKRPC_CORO_TASK_HPP

//#include <coroutine>
#include <functional>
#include <thread>
#include <variant>

#include <silkrpc/coro/coroutine.hpp>

namespace silkrpc::coro {

template<typename T>
struct task {
    struct promise_type {
        std::variant<std::monostate, T, std::exception_ptr> result;
        std::coroutine_handle<void> caller_coro;
        std::function<void()> completion_handler;

        task get_return_object() { return {this}; }
        auto initial_suspend() { return std::suspend_always{}; }
        auto final_suspend() {
            struct Awaiter {
                promise_type* self;
                bool await_ready() { return false; }
                void await_suspend(std::coroutine_handle<void>) {
                    if (self->caller_coro) {
                        self->caller_coro.resume();
                    } else if (self->completion_handler) {
                        self->completion_handler();
                    }
                }
                void await_resume() {}
            };
            return Awaiter{this};
        }
        template <typename U>
        void return_value(U&& value) {
            result.template emplace<1>(std::forward<U>(value));
        }
        void set_exception(std::exception_ptr eptr) {
            result.template emplace<2>(std::move(eptr));
        }
        void unhandled_exception() noexcept { std::terminate(); }
    };

    task(task&& rhs) noexcept : coro(rhs.coro) { rhs.coro = nullptr; }
    task(const task&) = delete;
    task& operator=(const task&) = delete;
    task& operator=(task&&) = delete;

    ~task() {
        if (coro) coro.destroy();
    }

    bool await_ready() { return false; }

    auto await_suspend(std::coroutine_handle<void> caller_coro) {
        coro.promise().caller_coro = caller_coro;
        coro.resume();
    }

    T await_resume() {
        if (coro.promise().result.index() == 2) {
            std::rethrow_exception(std::get<2>(coro.promise().result));
        }
        return std::get<1>(coro.promise().result);
    }

    void start(std::function<void()> completion_handler = {}) {
        coro.promise().completion_handler = completion_handler;
        coro.resume();
    }

private:
    task(promise_type* p) noexcept : coro(std::coroutine_handle<promise_type>::from_promise(*p)) {}

    std::coroutine_handle<promise_type> coro;
};

template<>
struct task<void> {
    struct promise_type {
        std::coroutine_handle<void> caller_coro;
        std::function<void()> completion_handler;

        task get_return_object() { return {this}; }
        auto initial_suspend() { return std::suspend_always{}; }
        auto final_suspend() {
            struct Awaiter {
                promise_type* self;
                bool await_ready() { return false; }
                void await_suspend(std::coroutine_handle<void>) {
                    if (self->caller_coro) {
                        self->caller_coro.resume();
                    } else if (self->completion_handler) {
                        self->completion_handler();
                    }
                }
                void await_resume() {}
            };
            return Awaiter{this};
        }
        void return_void() {}
        void unhandled_exception() noexcept { std::terminate(); }
    };

    task(task&& rhs) : coro(rhs.coro) { rhs.coro = nullptr; }
    task(const task&) = delete;
    task& operator=(const task&) = delete;
    task& operator=(task&&) = delete;

    ~task() {
        if (coro) coro.destroy();
    }

    bool await_ready() { return false; }

    auto await_suspend(std::coroutine_handle<void> caller_coro) {
        coro.promise().caller_coro = caller_coro;
        coro.resume();
    }

    void await_resume() {}

    void start(std::function<void()> completion_handler = {}) {
        coro.promise().completion_handler = completion_handler;
        coro.resume();
    }

private:
    task(promise_type* p) : coro(std::coroutine_handle<promise_type>::from_promise(*p)) {}

    std::coroutine_handle<promise_type> coro;
};

} // namespace silkrpc::coro

#endif // SILKRPC_CORO_TASK_HPP
