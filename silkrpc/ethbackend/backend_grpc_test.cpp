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

#include "backend_grpc.hpp"

#include <string>
#include <system_error>
#include <thread>
#include <utility>

#include <boost/asio/io_context.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_future.hpp>
#include <agrpc/test.hpp>
#include <catch2/catch.hpp>
#include <evmc/evmc.hpp>
#include <gmock/gmock.h>
#include <grpcpp/grpcpp.h>
#include <boost/endian/conversion.hpp>

#include <silkrpc/test/api_test_base.hpp>
#include <silkrpc/test/grpc_actions.hpp>
#include <silkrpc/test/grpc_responder.hpp>
#include <silkrpc/context_pool.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/interfaces/remote/ethbackend_mock.grpc.pb.h>

namespace silkrpc {

using Catch::Matchers::Message;
using evmc::literals::operator""_address;
using evmc::literals::operator""_bytes32;

::types::H160* make_h160(uint64_t hi_hi, uint64_t hi_lo, uint32_t lo) {
    auto h128_ptr{new ::types::H128()};
    h128_ptr->set_hi(hi_hi);
    h128_ptr->set_lo(hi_lo);
    auto h160_ptr{new ::types::H160()};
    h160_ptr->set_allocated_hi(h128_ptr);
    h160_ptr->set_lo(lo);
    return h160_ptr;
}

::types::H256* make_h256(uint64_t hi_hi, uint64_t hi_lo, uint64_t lo_hi, uint64_t lo_lo) {
    auto h256_ptr{new ::types::H256()};
    auto hi_ptr{new ::types::H128()};
    hi_ptr->set_hi(hi_hi);
    hi_ptr->set_lo(hi_lo);
    auto lo_ptr{new ::types::H128()};
    lo_ptr->set_hi(lo_hi);
    lo_ptr->set_lo(lo_lo);
    h256_ptr->set_allocated_hi(hi_ptr);
    h256_ptr->set_allocated_lo(lo_ptr);
    return h256_ptr;
}

bool h160_equal_address(const ::types::H160& h160, const evmc::address& address) {
    return h160.hi().hi() == boost::endian::load_big_u64(address.bytes) &&
        h160.hi().lo() == boost::endian::load_big_u64(address.bytes + 8) &&
        h160.lo() == boost::endian::load_big_u32(address.bytes + 16);
}

bool h256_equal_bytes32(const ::types::H256& h256, const evmc::bytes32& hash) {
    return h256.hi().hi() == boost::endian::load_big_u64(hash.bytes) &&
        h256.hi().lo() == boost::endian::load_big_u64(hash.bytes + 8) &&
        h256.lo().hi() == boost::endian::load_big_u64(hash.bytes + 16) &&
        h256.lo().lo() == boost::endian::load_big_u64(hash.bytes + 24);
}

bool h2048_equal_bloom(const ::types::H2048& h2048, const silkworm::Bloom& bloom) {
    // Fragment the H2048 in 8 H256 and verify each of them
    ::types::H256 fragments[] = {h2048.hi().hi().hi(), h2048.hi().hi().lo(),
                                h2048.hi().lo().hi(), h2048.hi().lo().lo(),
                                h2048.lo().hi().hi(), h2048.lo().hi().lo(),
                                h2048.lo().lo().hi(), h2048.lo().lo().lo()};
    uint64_t pos{0};
    for (const auto& h256 : fragments) {
        // Take bloom segment and convert it to evmc::bytes32
        evmc::bytes32 bloom_segment_bytes32;
        std::memcpy(bloom_segment_bytes32.bytes, &bloom[pos], 32);
        pos += 32;
        if (!h256_equal_bytes32(h256, bloom_segment_bytes32)) {
            return false;
        }
    }

    return true;
}

class EmptyBackEndService : public ::remote::ETHBACKEND::Service {
public:
    ::grpc::Status Etherbase(::grpc::ServerContext* context, const ::remote::EtherbaseRequest* request, ::remote::EtherbaseReply* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status ProtocolVersion(::grpc::ServerContext* context, const ::remote::ProtocolVersionRequest* request, ::remote::ProtocolVersionReply* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status NetVersion(::grpc::ServerContext* context, const ::remote::NetVersionRequest* request, ::remote::NetVersionReply* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status ClientVersion(::grpc::ServerContext *context, const ::remote::ClientVersionRequest *request, ::remote::ClientVersionReply *response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status NetPeerCount(::grpc::ServerContext* context, const ::remote::NetPeerCountRequest* request, ::remote::NetPeerCountReply* response) override {
        return ::grpc::Status::OK;
    }

    ::grpc::Status EngineGetPayloadV1(::grpc::ServerContext* context, const ::remote::EngineGetPayloadRequest* request, ::types::ExecutionPayload* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status EngineNewPayloadV1(::grpc::ServerContext* context, const ::types::ExecutionPayload* request, ::remote::EnginePayloadStatus* response) override {
        return ::grpc::Status::OK;
    }
    ::grpc::Status EngineForkChoiceUpdatedV1(::grpc::ServerContext* context,
        const ::remote::EngineForkChoiceUpdatedRequest* request, ::remote::EngineForkChoiceUpdatedReply* response) override {
        return ::grpc::Status::OK;
    }
};

class FailureBackEndService : public ::remote::ETHBACKEND::Service {
public:
    ::grpc::Status Etherbase(::grpc::ServerContext* context, const ::remote::EtherbaseRequest* request, ::remote::EtherbaseReply* response) override {
        return ::grpc::Status::CANCELLED;
    }
    ::grpc::Status ProtocolVersion(::grpc::ServerContext* context, const ::remote::ProtocolVersionRequest* request, ::remote::ProtocolVersionReply* response) override {
        return ::grpc::Status::CANCELLED;
    }
    ::grpc::Status NetVersion(::grpc::ServerContext* context, const ::remote::NetVersionRequest* request, ::remote::NetVersionReply* response) override {
        return ::grpc::Status::CANCELLED;
    }
    ::grpc::Status ClientVersion(::grpc::ServerContext *context, const ::remote::ClientVersionRequest *request, ::remote::ClientVersionReply *response) override {
        return ::grpc::Status::CANCELLED;
    }
    ::grpc::Status NetPeerCount(::grpc::ServerContext* context, const ::remote::NetPeerCountRequest* request, ::remote::NetPeerCountReply* response) override {
        return ::grpc::Status::CANCELLED;
    }
    ::grpc::Status EngineGetPayloadV1(::grpc::ServerContext* context, const ::remote::EngineGetPayloadRequest* request, ::types::ExecutionPayload* response) override {
        return ::grpc::Status::CANCELLED;
    }
    ::grpc::Status EngineNewPayloadV1(::grpc::ServerContext* context, const ::types::ExecutionPayload* request, ::remote::EnginePayloadStatus* response) override {
        return ::grpc::Status::CANCELLED;
    }
    ::grpc::Status EngineForkChoiceUpdatedV1(::grpc::ServerContext* context,
        const ::remote::EngineForkChoiceUpdatedRequest* request, ::remote::EngineForkChoiceUpdatedReply* response) override {
        return ::grpc::Status::CANCELLED;
    }
};

class TestBackEndService : public ::remote::ETHBACKEND::Service {
public:
    ::grpc::Status Etherbase(::grpc::ServerContext* context, const ::remote::EtherbaseRequest* request, ::remote::EtherbaseReply* response) override {
        auto address{make_h160(0xAAAAEEFFFFEEAAAA, 0x11DDBBAAAABBDD11, 0xCCDDDDCC)};
        response->set_allocated_address(address);
        return ::grpc::Status::OK;
    }
    ::grpc::Status ProtocolVersion(::grpc::ServerContext* context, const ::remote::ProtocolVersionRequest* request, ::remote::ProtocolVersionReply* response) override {
        response->set_id(15);
        return ::grpc::Status::OK;
    }
    ::grpc::Status NetVersion(::grpc::ServerContext* context, const ::remote::NetVersionRequest* request, ::remote::NetVersionReply* response) override {
        response->set_id(66);
        return ::grpc::Status::OK;
    }
    ::grpc::Status ClientVersion(::grpc::ServerContext *context, const ::remote::ClientVersionRequest *request, ::remote::ClientVersionReply *response) override {
        response->set_nodename("erigon");
        return ::grpc::Status::OK;
    }
    ::grpc::Status NetPeerCount(::grpc::ServerContext* context, const ::remote::NetPeerCountRequest* request, ::remote::NetPeerCountReply* response) override {
        response->set_count(20);
        return ::grpc::Status::OK;
    }
    ::grpc::Status EngineGetPayloadV1(::grpc::ServerContext* context, const ::remote::EngineGetPayloadRequest* request, ::types::ExecutionPayload* response) override {
        response->set_allocated_coinbase(make_h160(0xa94f5374fce5edbc, 0x8e2a8697c1533167, 0x7e6ebf0b));
        response->set_allocated_blockhash(make_h256(0x3559e851470f6e7b, 0xbed1db474980683e, 0x8c315bfce99b2a6e, 0xf47c057c04de7858));
        response->set_allocated_basefeepergas(make_h256(0x0, 0x0, 0x0, 0x7));
        response->set_allocated_stateroot(make_h256(0xca3149fa9e37db08, 0xd1cd49c9061db100, 0x2ef1cd58db2210f2, 0x115c8c989b2bdf45));
        response->set_allocated_receiptroot(make_h256(0x56e81f171bcc55a6, 0xff8345e692c0f86e, 0x5b48e01b996cadc0, 0x01622fb5e363b421));
        response->set_allocated_parenthash(make_h256(0x3b8fb240d288781d, 0x4aac94d3fd16809e, 0xe413bc99294a0857, 0x98a589dae51ddd4a));
        response->set_allocated_prevrandao(make_h256(0x0, 0x0, 0x0, 0x1));
        response->set_blocknumber(0x1);
        response->set_gaslimit(0x1c9c380);
        response->set_timestamp(0x5);
        auto tx_bytes{*silkworm::from_hex("0xf92ebdeab45d368f6354e8c5a8ac586c")};
        response->add_transactions(tx_bytes.data(), tx_bytes.size());
        // Assembling for logs_bloom
        auto hi_hi_hi_logsbloom{make_h256(0x1000000000000000, 0x0, 0x0, 0x0)};
        auto hi_hi_logsbloom{new ::types::H512()};
        hi_hi_logsbloom->set_allocated_hi(hi_hi_hi_logsbloom);
        auto hi_logsbloom{new ::types::H1024()};
        hi_logsbloom->set_allocated_hi(hi_hi_logsbloom);
        auto logsbloom{new ::types::H2048()};
        logsbloom->set_allocated_hi(hi_logsbloom);
        response->set_allocated_logsbloom(logsbloom);

        return ::grpc::Status::OK;
    }

    ::grpc::Status EngineNewPayloadV1(::grpc::ServerContext* context, const ::types::ExecutionPayload* request, ::remote::EnginePayloadStatus* response) override {
        silkworm::Bloom bloom;
        bloom.fill(0);
        bloom[0] = 0x12;
        auto transaction{*silkworm::from_hex("0xf92ebdeab45d368f6354e8c5a8ac586c")};
        // Check that payload was converted to protobuf correctly
        CHECK(request->timestamp() == 0x5);
        CHECK(request->gaslimit() == 0x1c9c380);
        CHECK(request->gasused() == 0x9);
        CHECK(h160_equal_address(request->coinbase(), 0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b_address));
        CHECK(h256_equal_bytes32(request->stateroot(), 0xca3149fa9e37db08d1cd49c9061db1002ef1cd58db2210f2115c8c989b2bdf43_bytes32));
        CHECK(h256_equal_bytes32(request->receiptroot(), 0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421_bytes32));
        CHECK(h256_equal_bytes32(request->parenthash(), 0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a_bytes32));
        CHECK(h256_equal_bytes32(request->prevrandao(), 0x0000000000000000000000000000000000000000000000000000000000000001_bytes32));
        CHECK(h256_equal_bytes32(request->basefeepergas(), 0x0000000000000000000000000000000000000000000000000000000000000007_bytes32));
        CHECK(h2048_equal_bloom(request->logsbloom(), bloom));
        CHECK(request->transactions(0) == std::string(reinterpret_cast<char*>(&transaction[0]), 16));
        // Assemble response
        response->set_allocated_latestvalidhash(make_h256(0, 0, 0, 0x40));
        response->set_validationerror("some error");
        // Clean way to test each decoding of messages without writing a switch
        response->set_status(static_cast<::remote::EngineStatus>(request->blocknumber()));
        return ::grpc::Status::OK;
    }

    ::grpc::Status EngineForkChoiceUpdatedV1(::grpc::ServerContext* context,
        const ::remote::EngineForkChoiceUpdatedRequest* request, ::remote::EngineForkChoiceUpdatedReply* response) override {
        // check if we have payloadattributes
        CHECK(request->has_payloadattributes());
        ::remote::EnginePayloadAttributes payload_attributes_grpc = request->payloadattributes();
        ::remote::EngineForkChoiceState fork_choice_state_grpc = request->forkchoicestate();
        // check forkchoice state was converted into protobuf correctly
        CHECK(h256_equal_bytes32(fork_choice_state_grpc.headblockhash(), 0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a_bytes32));
        CHECK(h256_equal_bytes32(fork_choice_state_grpc.safeblockhash(), 0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a_bytes32));
        CHECK(h256_equal_bytes32(fork_choice_state_grpc.finalizedblockhash(), 0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a_bytes32));
        // check if payload attributes was converted into protobuf correctly
        CHECK(payload_attributes_grpc.timestamp() == 0x1);
        CHECK(h256_equal_bytes32(payload_attributes_grpc.prevrandao(), 0x0000000000000000000000000000000000000000000000000000000000000001_bytes32));
        CHECK(h160_equal_address(payload_attributes_grpc.suggestedfeerecipient(), 0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b_address));
        // assemble payload status
        ::remote::EnginePayloadStatus *payload_status_grpc = new ::remote::EnginePayloadStatus();
        payload_status_grpc->set_allocated_latestvalidhash(make_h256(0, 0, 0, 0x40));
        payload_status_grpc->set_validationerror("some error");
        payload_status_grpc->set_status(static_cast<::remote::EngineStatus>(0));
        response->set_allocated_payloadstatus(payload_status_grpc);
        // assemble payload id
        response->set_payloadid(1);
        return ::grpc::Status::OK;
    }
};

static auto create_channel = []() {
    return grpc::CreateChannel("localhost:12345", grpc::InsecureChannelCredentials());
};

class ClientServerTestBox {
public:
    explicit ClientServerTestBox(grpc::Service* service) : context_{create_channel, std::make_shared<BlockCache>()} {
        server_address_ << "localhost:" << 12345; // TODO(canepat): grpc_pick_unused_port_or_die
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address_.str(), grpc::InsecureServerCredentials());
        builder.RegisterService(service);
        server_ = builder.BuildAndStart();
        context_thread_ = std::thread([&]() { context_.execute_loop(); });
    }

    template<auto method, typename T>
    auto make_method_proxy() {
        const auto channel = grpc::CreateChannel(server_address_.str(), grpc::InsecureChannelCredentials());
        std::unique_ptr<T> target{std::make_unique<T>(*context_.io_context(), channel, *context_.grpc_context())};
        return [target = std::move(target)](auto&&... args) {
            return (target.get()->*method)(std::forward<decltype(args)>(args)...);
        };
    }

    ~ClientServerTestBox() {
        server_->Shutdown();
        context_.stop();
        if (context_thread_.joinable()) {
            context_thread_.join();
        }
    }

private:
    Context context_;
    std::ostringstream server_address_;
    std::unique_ptr<grpc::Server> server_;
    std::thread context_thread_;
};

template<typename T, auto method, typename R, typename ...Args>
boost::asio::awaitable<R> test_comethod(::remote::ETHBACKEND::Service* service, Args... args) {
    ClientServerTestBox test_box{service};
    auto method_proxy{test_box.make_method_proxy<method, T>()};
    co_return co_await method_proxy(args...);
}

auto test_etherbase = test_comethod<ethbackend::BackEndGrpc, &ethbackend::BackEndGrpc::etherbase, evmc::address>;
auto test_protocol_version = test_comethod<ethbackend::BackEndGrpc, &ethbackend::BackEndGrpc::protocol_version, uint64_t>;
auto test_net_version = test_comethod<ethbackend::BackEndGrpc, &ethbackend::BackEndGrpc::net_version, uint64_t>;
auto test_client_version = test_comethod<ethbackend::BackEndGrpc, &ethbackend::BackEndGrpc::client_version, std::string>;
auto test_net_peer_count = test_comethod<ethbackend::BackEndGrpc, &ethbackend::BackEndGrpc::net_peer_count, uint64_t>;
auto test_engine_get_payload_v1 = test_comethod<ethbackend::BackEndGrpc, &ethbackend::BackEndGrpc::engine_get_payload_v1, ExecutionPayload, uint64_t>;
auto test_engine_new_payload_v1 = test_comethod<ethbackend::BackEndGrpc, &ethbackend::BackEndGrpc::engine_new_payload_v1, PayloadStatus, ExecutionPayload>;
auto test_engine_forkchoice_updated_v1 = test_comethod<ethbackend::BackEndGrpc, &ethbackend::BackEndGrpc::engine_forkchoice_updated_v1, ForkchoiceUpdatedReply, ForkchoiceUpdatedRequest>;


using StrictMockEthBackendStub = testing::StrictMock<::remote::MockETHBACKENDStub>;

struct EthBackendTest : test::ApiTestBase<ethbackend::BackEndGrpc, StrictMockEthBackendStub>
{
};

TEST_CASE_METHOD(EthBackendTest, "BackEnd::etherbase", "[silkrpc][ethbackend][backend]") {
    test::StrictMockAsyncResponseReader<::remote::EtherbaseReply> reader;
    EXPECT_CALL(*stub_, AsyncEtherbaseRaw).WillOnce(testing::Return(&reader));

    SECTION("call etherbase and get address") {
        ::remote::EtherbaseReply response;
        response.set_allocated_address(make_h160(0xAAAAEEFFFFEEAAAA, 0x11DDBBAAAABBDD11, 0xCCDDDDCC));
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_with(grpc_context_, std::move(response)));
        auto etherbase = run<&ethbackend::BackEndGrpc::etherbase>();
        CHECK(etherbase == 0xaaaaeeffffeeaaaa11ddbbaaaabbdd11ccddddcc_address);
    }

    SECTION("call etherbase and get empty address") {
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_ok(grpc_context_));
        auto etherbase = run<&ethbackend::BackEndGrpc::etherbase>();
        CHECK(etherbase == evmc::address{});
    }

    SECTION("call etherbase and get error") {
        EXPECT_CALL(reader, Finish).WillOnce(test::finish_cancelled(grpc_context_));
        CHECK_THROWS_AS((run<&ethbackend::BackEndGrpc::etherbase>()), boost::system::system_error);
    }
}

TEST_CASE("BackEnd::protocol_version", "[silkrpc][ethbackend][backend]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("call protocol_version and get version") {
        TestBackEndService service;
        boost::asio::io_context io_context;
        auto version{boost::asio::co_spawn(io_context, test_protocol_version(&service), boost::asio::use_future)};
        io_context.run();
        CHECK(version.get() == 15);
    }

    SECTION("call protocol_version and get empty version") {
        EmptyBackEndService service;
        boost::asio::io_context io_context;
        auto version{boost::asio::co_spawn(io_context, test_protocol_version(&service), boost::asio::use_future)};
        io_context.run();
        CHECK(version.get() == 0);
    }

    SECTION("call protocol_version and get error") {
        FailureBackEndService service;
        boost::asio::io_context io_context;
        auto version{boost::asio::co_spawn(io_context, test_protocol_version(&service), boost::asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(version.get(), boost::system::system_error);
    }
}

TEST_CASE("BackEnd::net_version", "[silkrpc][ethbackend][backend]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("call net_version and get version") {
        TestBackEndService service;
        boost::asio::io_context io_context;
        auto version{boost::asio::co_spawn(io_context, test_net_version(&service), boost::asio::use_future)};
        io_context.run();
        CHECK(version.get() == 66);
    }

    SECTION("call net_version and get empty version") {
        EmptyBackEndService service;
        boost::asio::io_context io_context;
        auto version{boost::asio::co_spawn(io_context, test_net_version(&service), boost::asio::use_future)};
        io_context.run();
        CHECK(version.get() == 0);
    }

    SECTION("call net_version and get error") {
        FailureBackEndService service;
        boost::asio::io_context io_context;
        auto version{boost::asio::co_spawn(io_context, test_net_version(&service), boost::asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(version.get(), boost::system::system_error);
    }
}

TEST_CASE("BackEnd::client_version", "[silkrpc][ethbackend][backend]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("call client_version and get version") {
        TestBackEndService service;
        boost::asio::io_context io_context;
        auto version{boost::asio::co_spawn(io_context, test_client_version(&service), boost::asio::use_future)};
        io_context.run();
        CHECK(version.get() == "erigon");
    }

    SECTION("call client_version and get empty version") {
        EmptyBackEndService service;
        boost::asio::io_context io_context;
        auto version{boost::asio::co_spawn(io_context, test_client_version(&service), boost::asio::use_future)};
        io_context.run();
        CHECK(version.get() == "");
    }

    SECTION("call client_version and get error") {
        FailureBackEndService service;
        boost::asio::io_context io_context;
        auto version{boost::asio::co_spawn(io_context, test_client_version(&service), boost::asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(version.get(), boost::system::system_error);
    }
}

TEST_CASE("BackEnd::net_peer_count", "[silkrpc][ethbackend][backend]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("call net_peer_count and get peer count") {
        TestBackEndService service;
        boost::asio::io_context io_context;
        auto peer_count{boost::asio::co_spawn(io_context, test_net_peer_count(&service), boost::asio::use_future)};
        io_context.run();
        CHECK(peer_count.get() == 20);
    }

    SECTION("call net_peer_count and get empty peer count") {
        EmptyBackEndService service;
        boost::asio::io_context io_context;
        auto peer_count{boost::asio::co_spawn(io_context, test_net_peer_count(&service), boost::asio::use_future)};
        io_context.run();
        CHECK(peer_count.get() == 0);
    }

    SECTION("call net_peer_count and get error") {
        FailureBackEndService service;
        boost::asio::io_context io_context;
        auto peer_count{boost::asio::co_spawn(io_context, test_net_peer_count(&service), boost::asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(peer_count.get(), boost::system::system_error);
    }
}

TEST_CASE("BackEnd::engine_get_payload_v1", "[silkrpc][ethbackend][backend]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("call engine_get_payload_v1 and get payload") {
        TestBackEndService service;
        boost::asio::io_context io_context;
        auto reply{boost::asio::co_spawn(io_context, test_engine_get_payload_v1(&service, 0), boost::asio::use_future)};
        io_context.run();
        auto payload{reply.get()};
        CHECK(payload.number == 0x1);
        CHECK(payload.gas_limit == 0x1c9c380);
        CHECK(payload.suggested_fee_recipient == 0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b_address);
        CHECK(payload.state_root == 0xca3149fa9e37db08d1cd49c9061db1002ef1cd58db2210f2115c8c989b2bdf45_bytes32);
        CHECK(payload.receipts_root == 0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421_bytes32);
        CHECK(payload.parent_hash == 0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a_bytes32);
        CHECK(payload.block_hash == 0x3559e851470f6e7bbed1db474980683e8c315bfce99b2a6ef47c057c04de7858_bytes32);
        CHECK(payload.prev_randao == 0x0000000000000000000000000000000000000000000000000000000000000001_bytes32);
        CHECK(payload.base_fee == 0x7);
        CHECK(payload.transactions.size() == 1);
        CHECK(silkworm::to_hex(payload.transactions[0]) == "f92ebdeab45d368f6354e8c5a8ac586c");
        silkworm::Bloom expected_bloom{0};
        expected_bloom[0] = 0x10;
        CHECK(payload.logs_bloom == expected_bloom);
    }

    SECTION("call engine_get_payload_v1 and get empty peer count") {
        EmptyBackEndService service;
        boost::asio::io_context io_context;
        auto payload{boost::asio::co_spawn(io_context, test_engine_get_payload_v1(&service, 0), boost::asio::use_future)};
        io_context.run();
        CHECK(payload.get().number == 0);
    }

    SECTION("call engine_get_payload_v1 and get error") {
        FailureBackEndService service;
        boost::asio::io_context io_context;
        auto payload{boost::asio::co_spawn(io_context, test_engine_get_payload_v1(&service, 0), boost::asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(payload.get(), boost::system::system_error);
    }
}

TEST_CASE("BackEnd::engine_new_payload_v1", "[silkrpc][ethbackend][backend]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    silkworm::Bloom bloom;
    bloom.fill(0);
    bloom[0] = 0x12;
    auto transaction{*silkworm::from_hex("0xf92ebdeab45d368f6354e8c5a8ac586c")};
    silkrpc::ExecutionPayload execution_payload{
        .timestamp = 0x5,
        .gas_limit = 0x1c9c380,
        .gas_used = 0x9,
        .suggested_fee_recipient = 0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b_address,
        .state_root = 0xca3149fa9e37db08d1cd49c9061db1002ef1cd58db2210f2115c8c989b2bdf43_bytes32,
        .receipts_root = 0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421_bytes32,
        .parent_hash = 0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a_bytes32,
        .block_hash = 0x3559e851470f6e7bbed1db474980683e8c315bfce99b2a6ef47c057c04de7858_bytes32,
        .prev_randao = 0x0000000000000000000000000000000000000000000000000000000000000001_bytes32,
        .base_fee = 0x7,
        .logs_bloom = bloom,
        .transactions = {transaction},
    };

    SECTION("call engine_new_payload_v1 and get VALID status") {
        TestBackEndService service;
        boost::asio::io_context io_context;
        auto reply{boost::asio::co_spawn(io_context, test_engine_new_payload_v1(&service, execution_payload), boost::asio::use_future)};
        io_context.run();
        auto payload_status{reply.get()};
        CHECK(payload_status.status == "VALID");
        CHECK(payload_status.latest_valid_hash == 0x0000000000000000000000000000000000000000000000000000000000000040_bytes32);
        CHECK(payload_status.validation_error == "some error");
    }

    SECTION("call engine_new_payload_v1 and get INVALID status") {
        execution_payload.number = 1;
        TestBackEndService service;
        boost::asio::io_context io_context;
        auto reply{boost::asio::co_spawn(io_context, test_engine_new_payload_v1(&service, execution_payload), boost::asio::use_future)};
        io_context.run();
        CHECK(reply.get().status == "INVALID");
    }

    SECTION("call engine_new_payload_v1 and get SYNCYNG status") {
        execution_payload.number = 2;
        TestBackEndService service;
        boost::asio::io_context io_context;
        auto reply{boost::asio::co_spawn(io_context, test_engine_new_payload_v1(&service, execution_payload), boost::asio::use_future)};
        io_context.run();
        CHECK(reply.get().status == "SYNCING");
    }

    SECTION("call engine_new_payload_v1 and get ACCEPTED status") {
        execution_payload.number = 3;
        TestBackEndService service;
        boost::asio::io_context io_context;
        auto reply{boost::asio::co_spawn(io_context, test_engine_new_payload_v1(&service, execution_payload), boost::asio::use_future)};
        io_context.run();
        CHECK(reply.get().status == "ACCEPTED");
    }

    SECTION("call engine_new_payload_v1 and get INVALID_BLOCK_HASH status") {
        execution_payload.number = 4;
        TestBackEndService service;
        boost::asio::io_context io_context;
        auto reply{boost::asio::co_spawn(io_context, test_engine_new_payload_v1(&service, execution_payload), boost::asio::use_future)};
        io_context.run();
        CHECK(reply.get().status == "INVALID_BLOCK_HASH");
    }

    SECTION("call engine_new_payload_v1 and get INVALID_TERMINAL_BLOCK status") {
        execution_payload.number = 5;
        TestBackEndService service;
        boost::asio::io_context io_context;
        auto reply{boost::asio::co_spawn(io_context, test_engine_new_payload_v1(&service, execution_payload), boost::asio::use_future)};
        io_context.run();
        CHECK(reply.get().status == "INVALID_TERMINAL_BLOCK");
    }

    SECTION("call engine_new_payload_v1 and get empty peer count") {
        EmptyBackEndService service;
        boost::asio::io_context io_context;
        auto reply{boost::asio::co_spawn(io_context, test_engine_new_payload_v1(&service, execution_payload), boost::asio::use_future)};
        io_context.run();
        auto payload_status{reply.get()};
        CHECK(payload_status.status == "VALID"); // Default value in interfaces is Valid
        CHECK(payload_status.latest_valid_hash == std::nullopt);
        CHECK(payload_status.validation_error == std::nullopt);
    }

    SECTION("call engine_new_payload_v1 and get error") {
        FailureBackEndService service;
        boost::asio::io_context io_context;
        auto reply{boost::asio::co_spawn(io_context, test_engine_new_payload_v1(&service, execution_payload), boost::asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(reply.get(), boost::system::system_error);
    }
}

TEST_CASE("Backend::engine_forkchoice_updated_v1", "[silkrpc][ethbackend][backend]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);
    silkrpc::PayloadAttributes payload_attributes{
        .timestamp = 0x1,
        .prev_randao = 0x0000000000000000000000000000000000000000000000000000000000000001_bytes32,
        .suggested_fee_recipient = 0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b_address
    };
    silkrpc::ForkchoiceUpdatedRequest forkchoice_request{
        .forkchoice_state = silkrpc::ForkchoiceState{
            .head_block_hash = 0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a_bytes32,
            .safe_block_hash = 0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a_bytes32,
            .finalized_block_hash = 0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a_bytes32
        },
        .payload_attributes = std::optional<silkrpc::PayloadAttributes>{payload_attributes}
    };
    SECTION("call engine_forkchoice_updated_v1 and get VALID status") {
        TestBackEndService service;
        boost::asio::io_context io_context;
        auto reply{boost::asio::co_spawn(io_context, test_engine_forkchoice_updated_v1(&service, forkchoice_request), boost::asio::use_future)};
        io_context.run();
        auto forkchoice_reply{reply.get()};
        silkrpc::PayloadStatus payload_status = forkchoice_reply.payload_status;
        CHECK(payload_status.status == "VALID");
        CHECK(payload_status.latest_valid_hash == 0x0000000000000000000000000000000000000000000000000000000000000040_bytes32);
        CHECK(payload_status.validation_error == "some error");
    }

    SECTION("call engine_forkchoice_updated_v1 and get error") {
        EmptyBackEndService service;
        boost::asio::io_context io_context;
        auto reply{boost::asio::co_spawn(io_context, test_engine_forkchoice_updated_v1(&service, forkchoice_request), boost::asio::use_future)};
        io_context.run();
        auto forkchoice_reply{reply.get()};
        silkrpc::PayloadStatus payload_status = forkchoice_reply.payload_status;
        CHECK(payload_status.status == "VALID"); // defaults to valid status
        CHECK(payload_status.latest_valid_hash == std::nullopt);
        CHECK(payload_status.validation_error == std::nullopt);
    }

    SECTION("call engine_forkchoice_updated_v1 and get VALID status with empty service") {
        FailureBackEndService service;
        boost::asio::io_context io_context;
        auto reply{boost::asio::co_spawn(io_context, test_engine_forkchoice_updated_v1(&service, forkchoice_request), boost::asio::use_future)};
        io_context.run();
        CHECK_THROWS_AS(reply.get(), boost::system::system_error);
    }
}
} // namespace silkrpc
