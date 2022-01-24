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

#ifndef SILKRPC_ETHBACKEND_BACKEND_HPP_
#define SILKRPC_ETHBACKEND_BACKEND_HPP_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <silkrpc/config.hpp>

#include <asio/io_context.hpp>
#include <asio/use_awaitable.hpp>
#include <boost/endian/conversion.hpp>
#include <evmc/evmc.hpp>
#include <grpcpp/grpcpp.h>

#include <silkrpc/common/clock_time.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/grpc/awaitables.hpp>
#include <silkrpc/grpc/async_unary_client.hpp>
#include <silkrpc/interfaces/remote/ethbackend.grpc.pb.h>
#include <silkrpc/interfaces/types/types.pb.h>
#include <silkrpc/types/execution_payload.hpp>

namespace silkrpc::ethbackend {

using EtherbaseClient = AsyncUnaryClient<
    ::remote::ETHBACKEND::StubInterface,
    ::remote::EtherbaseRequest,
    ::remote::EtherbaseReply,
    &::remote::ETHBACKEND::StubInterface::PrepareAsyncEtherbase
>;

using ProtocolVersionClient = AsyncUnaryClient<
    ::remote::ETHBACKEND::StubInterface,
    ::remote::ProtocolVersionRequest,
    ::remote::ProtocolVersionReply,
    &::remote::ETHBACKEND::StubInterface::PrepareAsyncProtocolVersion
>;

using NetVersionClient = AsyncUnaryClient<
    ::remote::ETHBACKEND::StubInterface,
    ::remote::NetVersionRequest,
    ::remote::NetVersionReply,
    &::remote::ETHBACKEND::StubInterface::PrepareAsyncNetVersion
>;

using ClientVersionClient = AsyncUnaryClient<
    ::remote::ETHBACKEND::StubInterface,
    ::remote::ClientVersionRequest,
    ::remote::ClientVersionReply,
    &::remote::ETHBACKEND::StubInterface::PrepareAsyncClientVersion
>;

using NetPeerCountClient = AsyncUnaryClient<
    ::remote::ETHBACKEND::StubInterface,
    ::remote::NetPeerCountRequest,
    ::remote::NetPeerCountReply,
    &::remote::ETHBACKEND::StubInterface::PrepareAsyncNetPeerCount
>;

using EngineGetPayloadV1Client = AsyncUnaryClient<
    ::remote::ETHBACKEND::StubInterface,
    ::remote::EngineGetPayloadRequest,
    ::types::ExecutionPayload,
    &::remote::ETHBACKEND::StubInterface::PrepareAsyncEngineGetPayloadV1
>;

using EtherbaseAwaitable = unary_awaitable<
    asio::io_context::executor_type,
    EtherbaseClient,
    ::remote::ETHBACKEND::StubInterface,
    ::remote::EtherbaseRequest,
    ::remote::EtherbaseReply
>;

using ProtocolVersionAwaitable = unary_awaitable<
    asio::io_context::executor_type,
    ProtocolVersionClient,
    ::remote::ETHBACKEND::StubInterface,
    ::remote::ProtocolVersionRequest,
    ::remote::ProtocolVersionReply
>;

using NetVersionAwaitable = unary_awaitable<
    asio::io_context::executor_type,
    NetVersionClient,
    ::remote::ETHBACKEND::StubInterface,
    ::remote::NetVersionRequest,
    ::remote::NetVersionReply
>;

using ClientVersionAwaitable = unary_awaitable<
    asio::io_context::executor_type,
    ClientVersionClient,
    ::remote::ETHBACKEND::StubInterface,
    ::remote::ClientVersionRequest,
    ::remote::ClientVersionReply
>;

using NetPeerCountAwaitable = unary_awaitable<
    asio::io_context::executor_type,
    NetPeerCountClient,
    ::remote::ETHBACKEND::StubInterface,
    ::remote::NetPeerCountRequest,
    ::remote::NetPeerCountReply
>;

using EngineGetPayloadV1Awaitable = unary_awaitable<
    asio::io_context::executor_type,
    EngineGetPayloadV1Client,
    ::remote::ETHBACKEND::StubInterface,
    ::remote::EngineGetPayloadRequest,
    ::types::ExecutionPayload
>;

class BackEnd final {
public:
    explicit BackEnd(asio::io_context& context, std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* queue)
    : BackEnd(context.get_executor(), ::remote::ETHBACKEND::NewStub(channel), queue) {}

    explicit BackEnd(asio::io_context::executor_type executor, std::unique_ptr<::remote::ETHBACKEND::StubInterface> stub, grpc::CompletionQueue* queue)
    : executor_(executor), stub_(std::move(stub)), queue_(queue) {
        SILKRPC_TRACE << "BackEnd::ctor " << this << "\n";
    }

    ~BackEnd() {
        SILKRPC_TRACE << "BackEnd::dtor " << this << "\n";
    }

    asio::awaitable<evmc::address> etherbase();
    asio::awaitable<uint64_t> protocol_version();
    asio::awaitable<uint64_t> net_version();
    asio::awaitable<std::string> client_version();
    asio::awaitable<uint64_t> net_peer_count();
    asio::awaitable<ExecutionPayload> engine_get_payload_v1(uint64_t payload_id);

    // just for testing
    asio::awaitable<::types::ExecutionPayload> execution_payload_to_proto(ExecutionPayload payload);

private:

    evmc::address address_from_H160(const types::H160& h160);
    silkworm::Bytes bytes_from_H128(const types::H128& h128);
    types::H128* H128_from_bytes(const uint8_t* bytes);
    types::H160* H160_from_address(const evmc::address& address);
    types::H256* H256_from_bytes(const uint8_t* bytes);
    silkworm::Bytes bytes_from_H256(const types::H256& h256);
    intx::uint256 uint256_from_H256(const types::H256& h256);
    types::H256* H256_from_uint256(const intx::uint256& n);
    evmc::bytes32 bytes32_from_H256(const types::H256& h256);
    types::H512* H512_from_bytes(const uint8_t* bytes);
    silkworm::Bytes bytes_from_H512(types::H512& h512);
    types::H1024* H1024_from_bytes(const uint8_t* bytes);
    silkworm::Bytes bytes_from_H1024(types::H1024& h1024);
    types::H2048* H2048_from_bytes(const uint8_t* bytes);
    silkworm::Bytes bytes_from_H2048(types::H2048& h2048);

    ExecutionPayload decode_execution_payload(const types::ExecutionPayload& execution_payload_grpc);
    types::ExecutionPayload encode_execution_payload(const ExecutionPayload& execution_payload);

    asio::io_context::executor_type executor_;
    std::unique_ptr<::remote::ETHBACKEND::StubInterface> stub_;
    grpc::CompletionQueue* queue_;
};

} // namespace silkrpc::ethbackend

#endif // SILKRPC_ETHBACKEND_BACKEND_HPP_
