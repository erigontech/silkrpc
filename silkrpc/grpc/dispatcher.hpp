/*
    Copyright 2022 The Silkrpc Authors

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

#ifndef SILKRPC_GRPC_DISPATCHER_HPP_
#define SILKRPC_GRPC_DISPATCHER_HPP_

#include <silkrpc/config.hpp>

#include <utility>

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/experimental/append.hpp>

namespace silkrpc {

namespace detail {

template<typename Executor>
struct ExecutorDispatcher {
    Executor executor_;

    template<typename CompletionToken, typename... Args>
    void dispatch(CompletionToken&& token, Args&&... args) {
        boost::asio::dispatch(
            boost::asio::bind_executor(executor_,
                boost::asio::experimental::append(std::forward<CompletionToken>(token), std::forward<Args>(args)...)));
    }
};

struct InlineDispatcher {
    template<typename CompletionToken, typename... Args>
    void dispatch(CompletionToken&& token, Args&&... args) {
        std::forward<CompletionToken>(token)(std::forward<Args>(args)...);
    }
};
} // namespace detail

} // namespace silkrpc

#endif // SILKRPC_GRPC_DISPATCHER_HPP_
