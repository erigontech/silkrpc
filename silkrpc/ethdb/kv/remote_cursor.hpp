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

#ifndef SILKRPC_ETHDB_KV_REMOTE_CURSOR_HPP_
#define SILKRPC_ETHDB_KV_REMOTE_CURSOR_HPP_

#include <silkrpc/config.hpp>

#include <memory>
#include <string>
#include <utility>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <silkworm/common/util.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/ethdb/kv/awaitables.hpp>
#include <silkrpc/ethdb/cursor.hpp>
#include <silkrpc/grpc/bidi_streaming_rpc.hpp>
#include <silkrpc/interfaces/remote/kv.grpc.pb.h>

namespace silkrpc::ethdb::kv {

class RemoteCursor : public CursorDupSort {
public:
    explicit RemoteCursor(KvAsioAwaitable<boost::asio::io_context::executor_type>& kv_awaitable)
    : kv_awaitable_(kv_awaitable), cursor_id_{0} {}

    RemoteCursor(const RemoteCursor&) = delete;
    RemoteCursor& operator=(const RemoteCursor&) = delete;

    uint32_t cursor_id() const override { return cursor_id_; };

    boost::asio::awaitable<void> open_cursor(const std::string& table_name) override;

    boost::asio::awaitable<KeyValue> seek(silkworm::ByteView key) override;

    boost::asio::awaitable<KeyValue> seek_exact(silkworm::ByteView key) override;

    boost::asio::awaitable<KeyValue> next() override;

    boost::asio::awaitable<void> close_cursor() override;

    boost::asio::awaitable<silkworm::Bytes> seek_both(silkworm::ByteView key, silkworm::ByteView value) override;

    boost::asio::awaitable<KeyValue> seek_both_exact(silkworm::ByteView key, silkworm::ByteView value) override;

private:
    KvAsioAwaitable<boost::asio::io_context::executor_type>& kv_awaitable_;
    uint32_t cursor_id_;
};

using KVStreamingRpc = silkrpc::BidiStreamingRpc<&remote::KV::StubInterface::PrepareAsyncTx>;

class RemoteCursor2 : public CursorDupSort {
public:
    explicit RemoteCursor2(KVStreamingRpc& streaming_rpc)
    : streaming_rpc_(streaming_rpc), cursor_id_{0} {}

    uint32_t cursor_id() const override { return cursor_id_; };

    boost::asio::awaitable<void> open_cursor(const std::string& table_name) override;

    boost::asio::awaitable<KeyValue> seek(silkworm::ByteView key) override;

    boost::asio::awaitable<KeyValue> seek_exact(silkworm::ByteView key) override;

    boost::asio::awaitable<KeyValue> next() override;

    boost::asio::awaitable<void> close_cursor() override;

    boost::asio::awaitable<silkworm::Bytes> seek_both(silkworm::ByteView key, silkworm::ByteView value) override;

    boost::asio::awaitable<KeyValue> seek_both_exact(silkworm::ByteView key, silkworm::ByteView value) override;

private:
    KVStreamingRpc& streaming_rpc_;
    uint32_t cursor_id_;
};

} // namespace silkrpc::ethdb::kv

#endif  // SILKRPC_ETHDB_KV_REMOTE_CURSOR_HPP_
