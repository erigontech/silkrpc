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

#ifndef SILKRPC_KV_ASYNC_OPERATION_HPP
#define SILKRPC_KV_ASYNC_OPERATION_HPP

#include <asio/detail/handler_tracking.hpp>

namespace silkrpc::kv {

// Base class for gRPC async operations using Asio completion tokens.
template<typename R, typename... Args>
class async_operation ASIO_INHERIT_TRACKED_HANDLER
{
public:
    typedef async_operation operation_type;

    void complete(void* owner, Args... args) {
        func_(owner, this, args...);
    }

    void destroy() {
        func_(0, this);
    }

protected:
    typedef R (*func_type)(void*, async_operation*, Args...);

    async_operation(func_type func) : func_(func) {}

    // Prevents deletion through this type.
    ~async_operation() {}

private:
    func_type func_;
};

} // namespace silkrpc::kv

#endif // SILKRPC_KV_ASYNC_OPERATION_HPP
