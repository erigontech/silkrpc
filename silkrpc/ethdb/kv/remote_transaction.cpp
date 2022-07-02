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

#include <utility>

#include <silkrpc/config.hpp>

#include <boost/asio/use_awaitable.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/awaitables.hpp>
#include <silkrpc/ethdb/kv/remote_transaction.hpp>

namespace silkrpc::ethdb::kv {

RemoteTransaction2::RemoteTransaction2(remote::KV::StubInterface& stub, agrpc::GrpcContext& grpc_context)
    : streaming_rpc_{stub, grpc_context} {
        SILKRPC_TRACE << "RemoteTransaction::ctor " << this << " start\n";
        SILKRPC_TRACE << "RemoteTransaction::ctor " << this << " end\n";
    }

RemoteTransaction2::~RemoteTransaction2() {
    SILKRPC_TRACE << "RemoteTransaction::dtor " << this << " start\n";
    SILKRPC_TRACE << "RemoteTransaction::dtor " << this << " end\n";
}

boost::asio::awaitable<void> RemoteTransaction2::open() {
    tx_id_ = (co_await streaming_rpc_.request_and_read()).txid();
}

boost::asio::awaitable<std::shared_ptr<Cursor>> RemoteTransaction2::cursor(const std::string& table) {
    co_return co_await get_cursor(table);
}

boost::asio::awaitable<std::shared_ptr<CursorDupSort>> RemoteTransaction2::cursor_dup_sort(const std::string& table) {
    co_return co_await get_cursor(table);
}

boost::asio::awaitable<void> RemoteTransaction2::close() {
    co_await streaming_rpc_.writes_done_and_finish();
    cursors_.clear();
    tx_id_ = 0;
}

boost::asio::awaitable<std::shared_ptr<CursorDupSort>> RemoteTransaction2::get_cursor(const std::string& table) {
    co_await silkrpc::continue_on(streaming_rpc_.get_executor());
    auto cursor_it = cursors_.find(table);
    if (cursor_it != cursors_.end()) {
        co_return cursor_it->second;
    }
    auto cursor = std::make_shared<RemoteCursor2>(streaming_rpc_);
    co_await cursor->open_cursor(table);
    cursors_[table] = cursor;
    co_return cursor;
}

} // namespace silkrpc::ethdb::kv

