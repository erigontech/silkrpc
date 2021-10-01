// Manually created to overcome grpcpp issue 24351 (https://github.com/grpc/grpc/issues/24351)

#include "remote/kv_mock.grpc.pb.h"

namespace remote {

class FixIssue24351_MockKVStub : public remote::MockKVStub {
 public:
  MOCK_METHOD3(AsyncVersion, ::grpc::ClientAsyncResponseReaderInterface< ::types::VersionReply>*(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(PrepareAsyncVersion, ::grpc::ClientAsyncResponseReaderInterface< ::types::VersionReply>*(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD1(Tx, ::grpc::ClientReaderWriterInterface< ::remote::Cursor, ::remote::Pair>*(::grpc::ClientContext* context));
  MOCK_METHOD3(AsyncTx, ::grpc::ClientAsyncReaderWriterInterface<::remote::Cursor, ::remote::Pair>*(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag));
  MOCK_METHOD2(PrepareAsyncTx, ::grpc::ClientAsyncReaderWriterInterface<::remote::Cursor, ::remote::Pair>*(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq));
  MOCK_METHOD2(ReceiveStateChanges, ::grpc::ClientReaderInterface< ::remote::StateChange>*(::grpc::ClientContext* context, const ::google::protobuf::Empty& request));
  MOCK_METHOD4(AsyncReceiveStateChanges, ::grpc::ClientAsyncReaderInterface< ::remote::StateChange>*(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq, void* tag));
  MOCK_METHOD3(PrepareAsyncReceiveStateChanges, ::grpc::ClientAsyncReaderInterface< ::remote::StateChange>*(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq));
};

} // namespace remote
