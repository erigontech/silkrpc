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

#ifndef SILKRPC_GRPC_ASYNC_COMPLETION_HANDLER_HPP_
#define SILKRPC_GRPC_ASYNC_COMPLETION_HANDLER_HPP_

#include <atomic>
#include <thread>

#include <asio/io_context.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/log.hpp>

namespace silkrpc {

class AsyncCompletionHandler {
public:
    static auto tag(AsyncCompletionHandler* handler) { return static_cast<void*>(handler); }

    static auto detag(void* tag) { return static_cast<AsyncCompletionHandler*>(tag); }

    virtual void completed(bool ok) = 0;
};

} // namespace silkrpc

#endif  // SILKRPC_GRPC_ASYNC_COMPLETION_HANDLER_HPP_
