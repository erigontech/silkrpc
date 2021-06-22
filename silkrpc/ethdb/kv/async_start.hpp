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

#ifndef SILKRPC_ETHDB_KV_ASYNC_START_HPP_
#define SILKRPC_ETHDB_KV_ASYNC_START_HPP_

#include <silkrpc/grpc/async_operation.hpp>

namespace silkrpc::ethdb::kv {

template <typename Handler, typename IoExecutor>
using async_start = async_noreply_operation<Handler, IoExecutor>;

} // namespace silkrpc::ethdb::kv

#endif // SILKRPC_ETHDB_KV_ASYNC_START_HPP_
