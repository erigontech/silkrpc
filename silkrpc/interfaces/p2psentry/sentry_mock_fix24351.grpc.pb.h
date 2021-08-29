// Manually created to overcome grpcpp issue 24351 (https://github.com/grpc/grpc/issues/24351)

#include "p2psentry/sentry_mock.grpc.pb.h"

namespace sentry {

class FixIssue24351_MockSentryStub : public MockSentryStub {
 public:
  MOCK_METHOD3(AsyncPenalizePeer, ::grpc::ClientAsyncResponseReaderInterface< ::google::protobuf::Empty>*(::grpc::ClientContext* context, const ::sentry::PenalizePeerRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(PrepareAsyncPenalizePeer, ::grpc::ClientAsyncResponseReaderInterface< ::google::protobuf::Empty>*(::grpc::ClientContext* context, const ::sentry::PenalizePeerRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(AsyncPeerMinBlock, ::grpc::ClientAsyncResponseReaderInterface< ::google::protobuf::Empty>*(::grpc::ClientContext* context, const ::sentry::PeerMinBlockRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(PrepareAsyncPeerMinBlock, ::grpc::ClientAsyncResponseReaderInterface< ::google::protobuf::Empty>*(::grpc::ClientContext* context, const ::sentry::PeerMinBlockRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(AsyncSendMessageByMinBlock, ::grpc::ClientAsyncResponseReaderInterface< ::sentry::SentPeers>*(::grpc::ClientContext* context, const ::sentry::SendMessageByMinBlockRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(PrepareAsyncSendMessageByMinBlock, ::grpc::ClientAsyncResponseReaderInterface< ::sentry::SentPeers>*(::grpc::ClientContext* context, const ::sentry::SendMessageByMinBlockRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(AsyncSendMessageById, ::grpc::ClientAsyncResponseReaderInterface< ::sentry::SentPeers>*(::grpc::ClientContext* context, const ::sentry::SendMessageByIdRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(PrepareAsyncSendMessageById, ::grpc::ClientAsyncResponseReaderInterface< ::sentry::SentPeers>*(::grpc::ClientContext* context, const ::sentry::SendMessageByIdRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(AsyncSendMessageToRandomPeers, ::grpc::ClientAsyncResponseReaderInterface< ::sentry::SentPeers>*(::grpc::ClientContext* context, const ::sentry::SendMessageToRandomPeersRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(PrepareAsyncSendMessageToRandomPeers, ::grpc::ClientAsyncResponseReaderInterface< ::sentry::SentPeers>*(::grpc::ClientContext* context, const ::sentry::SendMessageToRandomPeersRequest& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(AsyncSendMessageToAll, ::grpc::ClientAsyncResponseReaderInterface< ::sentry::SentPeers>*(::grpc::ClientContext* context, const ::sentry::OutboundMessageData& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(PrepareAsyncSendMessageToAll, ::grpc::ClientAsyncResponseReaderInterface< ::sentry::SentPeers>*(::grpc::ClientContext* context, const ::sentry::OutboundMessageData& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(AsyncSetStatus, ::grpc::ClientAsyncResponseReaderInterface< ::sentry::SetStatusReply>*(::grpc::ClientContext* context, const ::sentry::StatusData& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD3(PrepareAsyncSetStatus, ::grpc::ClientAsyncResponseReaderInterface< ::sentry::SetStatusReply>*(::grpc::ClientContext* context, const ::sentry::StatusData& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD2(ReceiveMessages, ::grpc::ClientReaderInterface< ::sentry::InboundMessage>*(::grpc::ClientContext* context, const ::google::protobuf::Empty& request));
  MOCK_METHOD4(AsyncReceiveMessages, ::grpc::ClientAsyncReaderInterface< ::sentry::InboundMessage>*(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq, void* tag));
  MOCK_METHOD3(PrepareAsyncReceiveMessages, ::grpc::ClientAsyncReaderInterface< ::sentry::InboundMessage>*(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD2(ReceiveUploadMessages, ::grpc::ClientReaderInterface< ::sentry::InboundMessage>*(::grpc::ClientContext* context, const ::google::protobuf::Empty& request));
  MOCK_METHOD4(AsyncReceiveUploadMessages, ::grpc::ClientAsyncReaderInterface< ::sentry::InboundMessage>*(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq, void* tag));
  MOCK_METHOD3(PrepareAsyncReceiveUploadMessages, ::grpc::ClientAsyncReaderInterface< ::sentry::InboundMessage>*(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq));
  MOCK_METHOD2(ReceiveTxMessages, ::grpc::ClientReaderInterface< ::sentry::InboundMessage>*(::grpc::ClientContext* context, const ::google::protobuf::Empty& request));
  MOCK_METHOD4(AsyncReceiveTxMessages, ::grpc::ClientAsyncReaderInterface< ::sentry::InboundMessage>*(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq, void* tag));
  MOCK_METHOD3(PrepareAsyncReceiveTxMessages, ::grpc::ClientAsyncReaderInterface< ::sentry::InboundMessage>*(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq));
};

} // namespace sentry

