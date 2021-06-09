// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: remote/kv.proto
#ifndef GRPC_remote_2fkv_2eproto__INCLUDED
#define GRPC_remote_2fkv_2eproto__INCLUDED

#include "remote/kv.pb.h"

#include <functional>
#include <grpc/impl/codegen/port_platform.h>
#include <grpcpp/impl/codegen/async_generic_service.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/client_context.h>
#include <grpcpp/impl/codegen/completion_queue.h>
#include <grpcpp/impl/codegen/message_allocator.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/impl/codegen/rpc_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/impl/codegen/stub_options.h>
#include <grpcpp/impl/codegen/sync_stream.h>

namespace remote {

// Provides methods to access key-value data
class KV final {
 public:
  static constexpr char const* service_full_name() {
    return "remote.KV";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    // Version returns the service version number
    virtual ::grpc::Status Version(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::types::VersionReply* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::types::VersionReply>> AsyncVersion(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::types::VersionReply>>(AsyncVersionRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::types::VersionReply>> PrepareAsyncVersion(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::types::VersionReply>>(PrepareAsyncVersionRaw(context, request, cq));
    }
    // Tx exposes read-only transactions for the key-value store
    std::unique_ptr< ::grpc::ClientReaderWriterInterface< ::remote::Cursor, ::remote::Pair>> Tx(::grpc::ClientContext* context) {
      return std::unique_ptr< ::grpc::ClientReaderWriterInterface< ::remote::Cursor, ::remote::Pair>>(TxRaw(context));
    }
    std::unique_ptr< ::grpc::ClientAsyncReaderWriterInterface< ::remote::Cursor, ::remote::Pair>> AsyncTx(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderWriterInterface< ::remote::Cursor, ::remote::Pair>>(AsyncTxRaw(context, cq, tag));
    }
    std::unique_ptr< ::grpc::ClientAsyncReaderWriterInterface< ::remote::Cursor, ::remote::Pair>> PrepareAsyncTx(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderWriterInterface< ::remote::Cursor, ::remote::Pair>>(PrepareAsyncTxRaw(context, cq));
    }
    std::unique_ptr< ::grpc::ClientReaderInterface< ::remote::StateChange>> ReceiveStateChanges(::grpc::ClientContext* context, const ::google::protobuf::Empty& request) {
      return std::unique_ptr< ::grpc::ClientReaderInterface< ::remote::StateChange>>(ReceiveStateChangesRaw(context, request));
    }
    std::unique_ptr< ::grpc::ClientAsyncReaderInterface< ::remote::StateChange>> AsyncReceiveStateChanges(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderInterface< ::remote::StateChange>>(AsyncReceiveStateChangesRaw(context, request, cq, tag));
    }
    std::unique_ptr< ::grpc::ClientAsyncReaderInterface< ::remote::StateChange>> PrepareAsyncReceiveStateChanges(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderInterface< ::remote::StateChange>>(PrepareAsyncReceiveStateChangesRaw(context, request, cq));
    }
    class experimental_async_interface {
     public:
      virtual ~experimental_async_interface() {}
      // Version returns the service version number
      virtual void Version(::grpc::ClientContext* context, const ::google::protobuf::Empty* request, ::types::VersionReply* response, std::function<void(::grpc::Status)>) = 0;
      virtual void Version(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::types::VersionReply* response, std::function<void(::grpc::Status)>) = 0;
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      virtual void Version(::grpc::ClientContext* context, const ::google::protobuf::Empty* request, ::types::VersionReply* response, ::grpc::ClientUnaryReactor* reactor) = 0;
      #else
      virtual void Version(::grpc::ClientContext* context, const ::google::protobuf::Empty* request, ::types::VersionReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) = 0;
      #endif
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      virtual void Version(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::types::VersionReply* response, ::grpc::ClientUnaryReactor* reactor) = 0;
      #else
      virtual void Version(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::types::VersionReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) = 0;
      #endif
      // Tx exposes read-only transactions for the key-value store
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      virtual void Tx(::grpc::ClientContext* context, ::grpc::ClientBidiReactor< ::remote::Cursor,::remote::Pair>* reactor) = 0;
      #else
      virtual void Tx(::grpc::ClientContext* context, ::grpc::experimental::ClientBidiReactor< ::remote::Cursor,::remote::Pair>* reactor) = 0;
      #endif
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      virtual void ReceiveStateChanges(::grpc::ClientContext* context, ::google::protobuf::Empty* request, ::grpc::ClientReadReactor< ::remote::StateChange>* reactor) = 0;
      #else
      virtual void ReceiveStateChanges(::grpc::ClientContext* context, ::google::protobuf::Empty* request, ::grpc::experimental::ClientReadReactor< ::remote::StateChange>* reactor) = 0;
      #endif
    };
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    typedef class experimental_async_interface async_interface;
    #endif
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    async_interface* async() { return experimental_async(); }
    #endif
    virtual class experimental_async_interface* experimental_async() { return nullptr; }
  private:
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::types::VersionReply>* AsyncVersionRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::types::VersionReply>* PrepareAsyncVersionRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientReaderWriterInterface< ::remote::Cursor, ::remote::Pair>* TxRaw(::grpc::ClientContext* context) = 0;
    virtual ::grpc::ClientAsyncReaderWriterInterface< ::remote::Cursor, ::remote::Pair>* AsyncTxRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) = 0;
    virtual ::grpc::ClientAsyncReaderWriterInterface< ::remote::Cursor, ::remote::Pair>* PrepareAsyncTxRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientReaderInterface< ::remote::StateChange>* ReceiveStateChangesRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request) = 0;
    virtual ::grpc::ClientAsyncReaderInterface< ::remote::StateChange>* AsyncReceiveStateChangesRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq, void* tag) = 0;
    virtual ::grpc::ClientAsyncReaderInterface< ::remote::StateChange>* PrepareAsyncReceiveStateChangesRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel);
    ::grpc::Status Version(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::types::VersionReply* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::types::VersionReply>> AsyncVersion(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::types::VersionReply>>(AsyncVersionRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::types::VersionReply>> PrepareAsyncVersion(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::types::VersionReply>>(PrepareAsyncVersionRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientReaderWriter< ::remote::Cursor, ::remote::Pair>> Tx(::grpc::ClientContext* context) {
      return std::unique_ptr< ::grpc::ClientReaderWriter< ::remote::Cursor, ::remote::Pair>>(TxRaw(context));
    }
    std::unique_ptr<  ::grpc::ClientAsyncReaderWriter< ::remote::Cursor, ::remote::Pair>> AsyncTx(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderWriter< ::remote::Cursor, ::remote::Pair>>(AsyncTxRaw(context, cq, tag));
    }
    std::unique_ptr<  ::grpc::ClientAsyncReaderWriter< ::remote::Cursor, ::remote::Pair>> PrepareAsyncTx(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderWriter< ::remote::Cursor, ::remote::Pair>>(PrepareAsyncTxRaw(context, cq));
    }
    std::unique_ptr< ::grpc::ClientReader< ::remote::StateChange>> ReceiveStateChanges(::grpc::ClientContext* context, const ::google::protobuf::Empty& request) {
      return std::unique_ptr< ::grpc::ClientReader< ::remote::StateChange>>(ReceiveStateChangesRaw(context, request));
    }
    std::unique_ptr< ::grpc::ClientAsyncReader< ::remote::StateChange>> AsyncReceiveStateChanges(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncReader< ::remote::StateChange>>(AsyncReceiveStateChangesRaw(context, request, cq, tag));
    }
    std::unique_ptr< ::grpc::ClientAsyncReader< ::remote::StateChange>> PrepareAsyncReceiveStateChanges(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncReader< ::remote::StateChange>>(PrepareAsyncReceiveStateChangesRaw(context, request, cq));
    }
    class experimental_async final :
      public StubInterface::experimental_async_interface {
     public:
      void Version(::grpc::ClientContext* context, const ::google::protobuf::Empty* request, ::types::VersionReply* response, std::function<void(::grpc::Status)>) override;
      void Version(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::types::VersionReply* response, std::function<void(::grpc::Status)>) override;
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      void Version(::grpc::ClientContext* context, const ::google::protobuf::Empty* request, ::types::VersionReply* response, ::grpc::ClientUnaryReactor* reactor) override;
      #else
      void Version(::grpc::ClientContext* context, const ::google::protobuf::Empty* request, ::types::VersionReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) override;
      #endif
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      void Version(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::types::VersionReply* response, ::grpc::ClientUnaryReactor* reactor) override;
      #else
      void Version(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::types::VersionReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) override;
      #endif
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      void Tx(::grpc::ClientContext* context, ::grpc::ClientBidiReactor< ::remote::Cursor,::remote::Pair>* reactor) override;
      #else
      void Tx(::grpc::ClientContext* context, ::grpc::experimental::ClientBidiReactor< ::remote::Cursor,::remote::Pair>* reactor) override;
      #endif
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      void ReceiveStateChanges(::grpc::ClientContext* context, ::google::protobuf::Empty* request, ::grpc::ClientReadReactor< ::remote::StateChange>* reactor) override;
      #else
      void ReceiveStateChanges(::grpc::ClientContext* context, ::google::protobuf::Empty* request, ::grpc::experimental::ClientReadReactor< ::remote::StateChange>* reactor) override;
      #endif
     private:
      friend class Stub;
      explicit experimental_async(Stub* stub): stub_(stub) { }
      Stub* stub() { return stub_; }
      Stub* stub_;
    };
    class experimental_async_interface* experimental_async() override { return &async_stub_; }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    class experimental_async async_stub_{this};
    ::grpc::ClientAsyncResponseReader< ::types::VersionReply>* AsyncVersionRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::types::VersionReply>* PrepareAsyncVersionRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientReaderWriter< ::remote::Cursor, ::remote::Pair>* TxRaw(::grpc::ClientContext* context) override;
    ::grpc::ClientAsyncReaderWriter< ::remote::Cursor, ::remote::Pair>* AsyncTxRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) override;
    ::grpc::ClientAsyncReaderWriter< ::remote::Cursor, ::remote::Pair>* PrepareAsyncTxRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientReader< ::remote::StateChange>* ReceiveStateChangesRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request) override;
    ::grpc::ClientAsyncReader< ::remote::StateChange>* AsyncReceiveStateChangesRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq, void* tag) override;
    ::grpc::ClientAsyncReader< ::remote::StateChange>* PrepareAsyncReceiveStateChangesRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_Version_;
    const ::grpc::internal::RpcMethod rpcmethod_Tx_;
    const ::grpc::internal::RpcMethod rpcmethod_ReceiveStateChanges_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    // Version returns the service version number
    virtual ::grpc::Status Version(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::types::VersionReply* response);
    // Tx exposes read-only transactions for the key-value store
    virtual ::grpc::Status Tx(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::remote::Pair, ::remote::Cursor>* stream);
    virtual ::grpc::Status ReceiveStateChanges(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::grpc::ServerWriter< ::remote::StateChange>* writer);
  };
  template <class BaseClass>
  class WithAsyncMethod_Version : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_Version() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_Version() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Version(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::types::VersionReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestVersion(::grpc::ServerContext* context, ::google::protobuf::Empty* request, ::grpc::ServerAsyncResponseWriter< ::types::VersionReply>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithAsyncMethod_Tx : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_Tx() {
      ::grpc::Service::MarkMethodAsync(1);
    }
    ~WithAsyncMethod_Tx() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Tx(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::remote::Pair, ::remote::Cursor>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestTx(::grpc::ServerContext* context, ::grpc::ServerAsyncReaderWriter< ::remote::Pair, ::remote::Cursor>* stream, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncBidiStreaming(1, context, stream, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithAsyncMethod_ReceiveStateChanges : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_ReceiveStateChanges() {
      ::grpc::Service::MarkMethodAsync(2);
    }
    ~WithAsyncMethod_ReceiveStateChanges() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ReceiveStateChanges(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::grpc::ServerWriter< ::remote::StateChange>* /*writer*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestReceiveStateChanges(::grpc::ServerContext* context, ::google::protobuf::Empty* request, ::grpc::ServerAsyncWriter< ::remote::StateChange>* writer, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncServerStreaming(2, context, request, writer, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_Version<WithAsyncMethod_Tx<WithAsyncMethod_ReceiveStateChanges<Service > > > AsyncService;
  template <class BaseClass>
  class ExperimentalWithCallbackMethod_Version : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithCallbackMethod_Version() {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::Service::
    #else
      ::grpc::Service::experimental().
    #endif
        MarkMethodCallback(0,
          new ::grpc_impl::internal::CallbackUnaryHandler< ::google::protobuf::Empty, ::types::VersionReply>(
            [this](
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
                   ::grpc::CallbackServerContext*
    #else
                   ::grpc::experimental::CallbackServerContext*
    #endif
                     context, const ::google::protobuf::Empty* request, ::types::VersionReply* response) { return this->Version(context, request, response); }));}
    void SetMessageAllocatorFor_Version(
        ::grpc::experimental::MessageAllocator< ::google::protobuf::Empty, ::types::VersionReply>* allocator) {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::internal::MethodHandler* const handler = ::grpc::Service::GetHandler(0);
    #else
      ::grpc::internal::MethodHandler* const handler = ::grpc::Service::experimental().GetHandler(0);
    #endif
      static_cast<::grpc_impl::internal::CallbackUnaryHandler< ::google::protobuf::Empty, ::types::VersionReply>*>(handler)
              ->SetMessageAllocator(allocator);
    }
    ~ExperimentalWithCallbackMethod_Version() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Version(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::types::VersionReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    virtual ::grpc::ServerUnaryReactor* Version(
      ::grpc::CallbackServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::types::VersionReply* /*response*/)
    #else
    virtual ::grpc::experimental::ServerUnaryReactor* Version(
      ::grpc::experimental::CallbackServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::types::VersionReply* /*response*/)
    #endif
      { return nullptr; }
  };
  template <class BaseClass>
  class ExperimentalWithCallbackMethod_Tx : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithCallbackMethod_Tx() {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::Service::
    #else
      ::grpc::Service::experimental().
    #endif
        MarkMethodCallback(1,
          new ::grpc_impl::internal::CallbackBidiHandler< ::remote::Cursor, ::remote::Pair>(
            [this](
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
                   ::grpc::CallbackServerContext*
    #else
                   ::grpc::experimental::CallbackServerContext*
    #endif
                     context) { return this->Tx(context); }));
    }
    ~ExperimentalWithCallbackMethod_Tx() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Tx(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::remote::Pair, ::remote::Cursor>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    virtual ::grpc::ServerBidiReactor< ::remote::Cursor, ::remote::Pair>* Tx(
      ::grpc::CallbackServerContext* /*context*/)
    #else
    virtual ::grpc::experimental::ServerBidiReactor< ::remote::Cursor, ::remote::Pair>* Tx(
      ::grpc::experimental::CallbackServerContext* /*context*/)
    #endif
      { return nullptr; }
  };
  template <class BaseClass>
  class ExperimentalWithCallbackMethod_ReceiveStateChanges : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithCallbackMethod_ReceiveStateChanges() {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::Service::
    #else
      ::grpc::Service::experimental().
    #endif
        MarkMethodCallback(2,
          new ::grpc_impl::internal::CallbackServerStreamingHandler< ::google::protobuf::Empty, ::remote::StateChange>(
            [this](
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
                   ::grpc::CallbackServerContext*
    #else
                   ::grpc::experimental::CallbackServerContext*
    #endif
                     context, const ::google::protobuf::Empty* request) { return this->ReceiveStateChanges(context, request); }));
    }
    ~ExperimentalWithCallbackMethod_ReceiveStateChanges() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ReceiveStateChanges(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::grpc::ServerWriter< ::remote::StateChange>* /*writer*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    virtual ::grpc::ServerWriteReactor< ::remote::StateChange>* ReceiveStateChanges(
      ::grpc::CallbackServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/)
    #else
    virtual ::grpc::experimental::ServerWriteReactor< ::remote::StateChange>* ReceiveStateChanges(
      ::grpc::experimental::CallbackServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/)
    #endif
      { return nullptr; }
  };
  #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
  typedef ExperimentalWithCallbackMethod_Version<ExperimentalWithCallbackMethod_Tx<ExperimentalWithCallbackMethod_ReceiveStateChanges<Service > > > CallbackService;
  #endif

  typedef ExperimentalWithCallbackMethod_Version<ExperimentalWithCallbackMethod_Tx<ExperimentalWithCallbackMethod_ReceiveStateChanges<Service > > > ExperimentalCallbackService;
  template <class BaseClass>
  class WithGenericMethod_Version : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_Version() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_Version() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Version(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::types::VersionReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithGenericMethod_Tx : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_Tx() {
      ::grpc::Service::MarkMethodGeneric(1);
    }
    ~WithGenericMethod_Tx() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Tx(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::remote::Pair, ::remote::Cursor>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithGenericMethod_ReceiveStateChanges : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_ReceiveStateChanges() {
      ::grpc::Service::MarkMethodGeneric(2);
    }
    ~WithGenericMethod_ReceiveStateChanges() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ReceiveStateChanges(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::grpc::ServerWriter< ::remote::StateChange>* /*writer*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithRawMethod_Version : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_Version() {
      ::grpc::Service::MarkMethodRaw(0);
    }
    ~WithRawMethod_Version() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Version(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::types::VersionReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestVersion(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawMethod_Tx : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_Tx() {
      ::grpc::Service::MarkMethodRaw(1);
    }
    ~WithRawMethod_Tx() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Tx(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::remote::Pair, ::remote::Cursor>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestTx(::grpc::ServerContext* context, ::grpc::ServerAsyncReaderWriter< ::grpc::ByteBuffer, ::grpc::ByteBuffer>* stream, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncBidiStreaming(1, context, stream, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawMethod_ReceiveStateChanges : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_ReceiveStateChanges() {
      ::grpc::Service::MarkMethodRaw(2);
    }
    ~WithRawMethod_ReceiveStateChanges() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ReceiveStateChanges(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::grpc::ServerWriter< ::remote::StateChange>* /*writer*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestReceiveStateChanges(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncWriter< ::grpc::ByteBuffer>* writer, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncServerStreaming(2, context, request, writer, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class ExperimentalWithRawCallbackMethod_Version : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithRawCallbackMethod_Version() {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::Service::
    #else
      ::grpc::Service::experimental().
    #endif
        MarkMethodRawCallback(0,
          new ::grpc_impl::internal::CallbackUnaryHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
                   ::grpc::CallbackServerContext*
    #else
                   ::grpc::experimental::CallbackServerContext*
    #endif
                     context, const ::grpc::ByteBuffer* request, ::grpc::ByteBuffer* response) { return this->Version(context, request, response); }));
    }
    ~ExperimentalWithRawCallbackMethod_Version() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Version(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::types::VersionReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    virtual ::grpc::ServerUnaryReactor* Version(
      ::grpc::CallbackServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/, ::grpc::ByteBuffer* /*response*/)
    #else
    virtual ::grpc::experimental::ServerUnaryReactor* Version(
      ::grpc::experimental::CallbackServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/, ::grpc::ByteBuffer* /*response*/)
    #endif
      { return nullptr; }
  };
  template <class BaseClass>
  class ExperimentalWithRawCallbackMethod_Tx : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithRawCallbackMethod_Tx() {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::Service::
    #else
      ::grpc::Service::experimental().
    #endif
        MarkMethodRawCallback(1,
          new ::grpc_impl::internal::CallbackBidiHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
                   ::grpc::CallbackServerContext*
    #else
                   ::grpc::experimental::CallbackServerContext*
    #endif
                     context) { return this->Tx(context); }));
    }
    ~ExperimentalWithRawCallbackMethod_Tx() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Tx(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::remote::Pair, ::remote::Cursor>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    virtual ::grpc::ServerBidiReactor< ::grpc::ByteBuffer, ::grpc::ByteBuffer>* Tx(
      ::grpc::CallbackServerContext* /*context*/)
    #else
    virtual ::grpc::experimental::ServerBidiReactor< ::grpc::ByteBuffer, ::grpc::ByteBuffer>* Tx(
      ::grpc::experimental::CallbackServerContext* /*context*/)
    #endif
      { return nullptr; }
  };
  template <class BaseClass>
  class ExperimentalWithRawCallbackMethod_ReceiveStateChanges : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithRawCallbackMethod_ReceiveStateChanges() {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::Service::
    #else
      ::grpc::Service::experimental().
    #endif
        MarkMethodRawCallback(2,
          new ::grpc_impl::internal::CallbackServerStreamingHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
                   ::grpc::CallbackServerContext*
    #else
                   ::grpc::experimental::CallbackServerContext*
    #endif
                     context, const::grpc::ByteBuffer* request) { return this->ReceiveStateChanges(context, request); }));
    }
    ~ExperimentalWithRawCallbackMethod_ReceiveStateChanges() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ReceiveStateChanges(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::grpc::ServerWriter< ::remote::StateChange>* /*writer*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    virtual ::grpc::ServerWriteReactor< ::grpc::ByteBuffer>* ReceiveStateChanges(
      ::grpc::CallbackServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/)
    #else
    virtual ::grpc::experimental::ServerWriteReactor< ::grpc::ByteBuffer>* ReceiveStateChanges(
      ::grpc::experimental::CallbackServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/)
    #endif
      { return nullptr; }
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_Version : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithStreamedUnaryMethod_Version() {
      ::grpc::Service::MarkMethodStreamed(0,
        new ::grpc::internal::StreamedUnaryHandler<
          ::google::protobuf::Empty, ::types::VersionReply>(
            [this](::grpc_impl::ServerContext* context,
                   ::grpc_impl::ServerUnaryStreamer<
                     ::google::protobuf::Empty, ::types::VersionReply>* streamer) {
                       return this->StreamedVersion(context,
                         streamer);
                  }));
    }
    ~WithStreamedUnaryMethod_Version() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status Version(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::types::VersionReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedVersion(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::google::protobuf::Empty,::types::VersionReply>* server_unary_streamer) = 0;
  };
  typedef WithStreamedUnaryMethod_Version<Service > StreamedUnaryService;
  template <class BaseClass>
  class WithSplitStreamingMethod_ReceiveStateChanges : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithSplitStreamingMethod_ReceiveStateChanges() {
      ::grpc::Service::MarkMethodStreamed(2,
        new ::grpc::internal::SplitServerStreamingHandler<
          ::google::protobuf::Empty, ::remote::StateChange>(
            [this](::grpc_impl::ServerContext* context,
                   ::grpc_impl::ServerSplitStreamer<
                     ::google::protobuf::Empty, ::remote::StateChange>* streamer) {
                       return this->StreamedReceiveStateChanges(context,
                         streamer);
                  }));
    }
    ~WithSplitStreamingMethod_ReceiveStateChanges() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status ReceiveStateChanges(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::grpc::ServerWriter< ::remote::StateChange>* /*writer*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with split streamed
    virtual ::grpc::Status StreamedReceiveStateChanges(::grpc::ServerContext* context, ::grpc::ServerSplitStreamer< ::google::protobuf::Empty,::remote::StateChange>* server_split_streamer) = 0;
  };
  typedef WithSplitStreamingMethod_ReceiveStateChanges<Service > SplitStreamedService;
  typedef WithStreamedUnaryMethod_Version<WithSplitStreamingMethod_ReceiveStateChanges<Service > > StreamedService;
};

}  // namespace remote


#endif  // GRPC_remote_2fkv_2eproto__INCLUDED
