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

#ifndef SILKRPC_CORO_COROUTINE_HPP
#define SILKRPC_CORO_COROUTINE_HPP

#include <silkrpc/config.hpp>

#ifdef SILKRPC_HAS_EXPERIMENTAL_COROUTINES
namespace std {
    template <typename T>
    using coroutine_handle = std::experimental::coroutine_handle<T>;
    using suspend_always = std::experimental::suspend_always;
    using suspend_never = std::experimental::suspend_never;
} // namespace std
#endif // SILKRPC_HAS_EXPERIMENTAL_COROUTINES

#endif // SILKRPC_CORO_COROUTINE_HPP
