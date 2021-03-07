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

#ifndef SILKRPC_CORO_USE_AWAIT_HPP_
#define SILKRPC_CORO_USE_AWAIT_HPP_

#include <functional>
#include <memory>
#include <tuple>
#include <utility>

#include <silkrpc/config.hpp>

#include <asio/async_result.hpp>

namespace silkrpc::coro {

constexpr struct use_await_t {} use_await;

} // namespace silkrpc::coro

template <typename R, typename... Args>
class asio::async_result<silkrpc::coro::use_await_t, R(Args...)> {
private:
    template <typename Initiation, typename... InitArgs>
    struct awaitable {
        template <typename T>
        class allocator;

        struct handler {
            typedef allocator<void> allocator_type;

            allocator_type get_allocator() const {
                return allocator_type(nullptr);
            }

            void operator()(Args... results) {
                std::tuple<Args...> result(std::move(results)...);
                awaitable_->result_ = &result;
                coro_.resume();
            }

            awaitable* awaitable_;
            std::coroutine_handle<> coro_;
        };

        template <typename T>
        class allocator {
        public:
            typedef T value_type;

            explicit allocator(awaitable* a) noexcept : awaitable_(a) {}

            template <typename U>
            allocator(const allocator<U>& a) noexcept : awaitable_(a.awaitable_) {}

            T* allocate(std::size_t n) {
                return static_cast<T*>(::operator new(sizeof(T) * n));
            }

            void deallocate(T* p, std::size_t) {
                ::operator delete(p);
            }

        private:
            template <typename> friend class allocator;
            awaitable* awaitable_;
        };

        bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(std::coroutine_handle<> h) noexcept {
            std::apply(
                [&](auto&&... a) {
                    initiation_(handler{this, h}, std::forward<decltype(a)>(a)...);
                },
                init_args_);
        }

        std::tuple<Args...> await_resume() {
            return std::move(*static_cast<std::tuple<Args...>*>(result_));
        }

        Initiation initiation_;
        std::tuple<InitArgs...> init_args_;
        void* result_ = nullptr;
    };

public:
    template <typename Initiation, typename... InitArgs>
    static auto initiate(Initiation initiation, silkrpc::coro::use_await_t, InitArgs... init_args) {
        return awaitable<Initiation, InitArgs...>{
            std::move(initiation),
            std::forward_as_tuple(std::move(init_args)...)};
    }
};

#endif // SILKRPC_CORO_USE_AWAIT_HPP_
