// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: remote/ethbackend.proto

#include "remote/ethbackend.pb.h"
#include "remote/ethbackend.grpc.pb.h"

#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/sync_stream.h>
#include <gmock/gmock.h>
namespace remote {

class MockETHBACKENDStub : public ETHBACKEND::StubInterface {
 public:
  MOCK_METHOD3(Etherbase, ::grpc::Status(::grpc::ClientContext* context, const ::remote::EtherbaseRequest& request, ::remote::EtherbaseReply* response));
  MOCK_METHOD3(AsyncEtherbaseRaw, ::grpc::ClientAsyncResponseReaderInterface< ::remote::EtherbaseReply>*(::grpc::ClientContext* context, const ::remote::EtherbaseRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(PrepareAsyncEtherbaseRaw, ::grpc::ClientAsyncResponseReaderInterface< ::remote::EtherbaseReply>*(::grpc::ClientContext* context, const ::remote::EtherbaseRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(NetVersion, ::grpc::Status(::grpc::ClientContext* context, const ::remote::NetVersionRequest& request, ::remote::NetVersionReply* response));
  MOCK_METHOD3(AsyncNetVersionRaw, ::grpc::ClientAsyncResponseReaderInterface< ::remote::NetVersionReply>*(::grpc::ClientContext* context, const ::remote::NetVersionRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(PrepareAsyncNetVersionRaw, ::grpc::ClientAsyncResponseReaderInterface< ::remote::NetVersionReply>*(::grpc::ClientContext* context, const ::remote::NetVersionRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(Version, ::grpc::Status(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::types::VersionReply* response));
  MOCK_METHOD3(AsyncVersionRaw, ::grpc::ClientAsyncResponseReaderInterface< ::types::VersionReply>*(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(PrepareAsyncVersionRaw, ::grpc::ClientAsyncResponseReaderInterface< ::types::VersionReply>*(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(ProtocolVersion, ::grpc::Status(::grpc::ClientContext* context, const ::remote::ProtocolVersionRequest& request, ::remote::ProtocolVersionReply* response));
  MOCK_METHOD3(AsyncProtocolVersionRaw, ::grpc::ClientAsyncResponseReaderInterface< ::remote::ProtocolVersionReply>*(::grpc::ClientContext* context, const ::remote::ProtocolVersionRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(PrepareAsyncProtocolVersionRaw, ::grpc::ClientAsyncResponseReaderInterface< ::remote::ProtocolVersionReply>*(::grpc::ClientContext* context, const ::remote::ProtocolVersionRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(ClientVersion, ::grpc::Status(::grpc::ClientContext* context, const ::remote::ClientVersionRequest& request, ::remote::ClientVersionReply* response));
  MOCK_METHOD3(AsyncClientVersionRaw, ::grpc::ClientAsyncResponseReaderInterface< ::remote::ClientVersionReply>*(::grpc::ClientContext* context, const ::remote::ClientVersionRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(PrepareAsyncClientVersionRaw, ::grpc::ClientAsyncResponseReaderInterface< ::remote::ClientVersionReply>*(::grpc::ClientContext* context, const ::remote::ClientVersionRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD2(SubscribeRaw, ::grpc::ClientReaderInterface< ::remote::SubscribeReply>*(::grpc::ClientContext* context, const ::remote::SubscribeRequest& request));
  MOCK_METHOD4(AsyncSubscribeRaw, ::grpc::ClientAsyncReaderInterface< ::remote::SubscribeReply>*(::grpc::ClientContext* context, const ::remote::SubscribeRequest& request, ::grpc::CompletionQueue* cq, void* tag));
  MOCK_METHOD3(PrepareAsyncSubscribeRaw, ::grpc::ClientAsyncReaderInterface< ::remote::SubscribeReply>*(::grpc::ClientContext* context, const ::remote::SubscribeRequest& request, ::grpc::CompletionQueue* cq));
};

} // namespace remote

