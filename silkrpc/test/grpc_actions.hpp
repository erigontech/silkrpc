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

#ifndef SILKRPC_TEST_GRPC_ACTIONS_HPP_
#define SILKRPC_TEST_GRPC_ACTIONS_HPP_

#include <utility>

#include <grpcpp/grpcpp.h>

#include <agrpc/grpcContext.hpp>
#include <agrpc/test.hpp>

namespace silkrpc::test {

inline auto finish_with_status(agrpc::GrpcContext& grpc_context, grpc::Status status) {
    return [&grpc_context, status](auto&&, ::grpc::Status* status_ptr, void* tag) {
        *status_ptr = status;
        agrpc::process_grpc_tag(grpc_context, tag, true);
    };
}

inline auto finish_ok(agrpc::GrpcContext& grpc_context) { return finish_with_status(grpc_context, grpc::Status::OK); }

inline auto finish_cancelled(agrpc::GrpcContext& grpc_context) {
    return finish_with_status(grpc_context, grpc::Status::CANCELLED);
}

template <typename Reply>
auto finish_with(agrpc::GrpcContext& grpc_context, Reply&& reply) {
    return [&grpc_context, reply = std::forward<Reply>(reply)](auto* reply_ptr, ::grpc::Status* status,
                                                               void* tag) mutable {
        *reply_ptr = std::move(reply);
        finish_with_status(grpc_context, grpc::Status::OK)(reply_ptr, status, tag);
    };
}

inline auto write(agrpc::GrpcContext& grpc_context, bool ok) {
    return [&grpc_context, ok](auto&&, void* tag) { agrpc::process_grpc_tag(grpc_context, tag, ok); };
}

inline auto write_success(agrpc::GrpcContext& grpc_context) { return write(grpc_context, true); }

inline auto write_failure(agrpc::GrpcContext& grpc_context) { return write(grpc_context, false); }

inline auto writes_done(agrpc::GrpcContext& grpc_context, bool ok) {
    return [&grpc_context, ok](void* tag) { agrpc::process_grpc_tag(grpc_context, tag, ok); };
}

inline auto writes_done_success(agrpc::GrpcContext& grpc_context) { return writes_done(grpc_context, true); }

inline auto writes_done_failure(agrpc::GrpcContext& grpc_context) { return writes_done(grpc_context, false); }

template <typename Reply>
auto read_success_with(agrpc::GrpcContext& grpc_context, Reply&& reply) {
    return [&grpc_context, reply = std::forward<Reply>(reply)](auto* reply_ptr, void* tag) mutable {
        *reply_ptr = std::move(reply);
        agrpc::process_grpc_tag(grpc_context, tag, true);
    };
}

inline auto read_failure(agrpc::GrpcContext& grpc_context) {
    return [&grpc_context](auto*, void* tag) { agrpc::process_grpc_tag(grpc_context, tag, false); };
}

inline auto finish_streaming_with_status(agrpc::GrpcContext& grpc_context, grpc::Status status) {
    return [&grpc_context, status](::grpc::Status* status_ptr, void* tag) {
        *status_ptr = status;
        agrpc::process_grpc_tag(grpc_context, tag, true);
    };
}

}  // namespace silkrpc::test

#endif  // SILKRPC_TEST_GRPC_ACTIONS_HPP_
