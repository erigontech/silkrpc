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

#ifndef SILKRPC_ETHBACKEND_ASYNC_PROTOCOL_VERSION_HPP_
#define SILKRPC_ETHBACKEND_ASYNC_PROTOCOL_VERSION_HPP_

#include <asio/detail/config.hpp>
#include <asio/detail/bind_handler.hpp>
#include <asio/detail/memory.hpp>

#include <silkrpc/grpc/async_operation.hpp>
#include <silkrpc/interfaces/remote/ethbackend.grpc.pb.h>

namespace silkrpc::ethbackend {

template <typename Handler, typename IoExecutor>
using async_protocolVersion = async_reply_operation<Handler, IoExecutor, remote::ProtocolVersionReply>;

} // namespace silkrpc::ethbackend

#endif // SILKRPC_ETHBACKEND_ASYNC_PROTOCOL_VERSION_HPP_
