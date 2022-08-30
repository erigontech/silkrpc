#[[
   Copyright 2020 The SilkRpc Authors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
]]

find_package(absl CONFIG REQUIRED)
find_package(intx CONFIG REQUIRED)

# Find Protobuf installation
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)

# Find asio-gRPC installation
find_package(asio-grpc CONFIG REQUIRED)

# Find mimalloc installation (optional)
if(SILKRPC_USE_MIMALLOC)
    find_package(mimalloc 2.0 REQUIRED)
endif()

# Define gRPC proto files
set(IF_PROTO_PATH "${CMAKE_SOURCE_DIR}/interfaces")

# Generate Protocol Buffers and gRPC bindings
set(PROTO_GENERATED_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto-bindings)

set(BACKEND_PROTO_HEADER "${PROTO_GENERATED_SRC_DIR}/remote/ethbackend.pb.h")
set(BACKEND_GRPC_HEADER "${PROTO_GENERATED_SRC_DIR}/remote/ethbackend.grpc.pb.h")
set(BACKEND_MOCK_GRPC_HEADER "${PROTO_GENERATED_SRC_DIR}/remote/ethbackend_mock.grpc.pb.h")
set(KV_PROTO_HEADER "${PROTO_GENERATED_SRC_DIR}/remote/kv.pb.h")
set(KV_GRPC_HEADER "${PROTO_GENERATED_SRC_DIR}/remote/kv.grpc.pb.h")
set(KV_MOCK_GRPC_HEADER "${PROTO_GENERATED_SRC_DIR}/remote/kv_mock.grpc.pb.h")
set(P2PSENTRY_PROTO_HEADER "${PROTO_GENERATED_SRC_DIR}/p2psentry/sentry.pb.h")
set(P2PSENTRY_GRPC_HEADER "${PROTO_GENERATED_SRC_DIR}/p2psentry/sentry.grpc.pb.h")
set(P2PSENTRY_MOCK_GRPC_HEADER "${PROTO_GENERATED_SRC_DIR}/p2psentry/sentry_mock.grpc.pb.h")
set(MINING_PROTO_HEADER "${PROTO_GENERATED_SRC_DIR}/txpool/mining.pb.h")
set(MINING_GRPC_HEADER "${PROTO_GENERATED_SRC_DIR}/txpool/mining.grpc.pb.h")
set(MINING_MOCK_GRPC_HEADER "${PROTO_GENERATED_SRC_DIR}/txpool/mining_mock.grpc.pb.h")
set(TXPOOL_PROTO_HEADER "${PROTO_GENERATED_SRC_DIR}/txpool/txpool.pb.h")
set(TXPOOL_GRPC_HEADER "${PROTO_GENERATED_SRC_DIR}/txpool/txpool.grpc.pb.h")
set(TXPOOL_MOCK_GRPC_HEADER "${PROTO_GENERATED_SRC_DIR}/txpool/txpool_mock.grpc.pb.h")
set(TYPES_PROTO_HEADER "${PROTO_GENERATED_SRC_DIR}/types/types.pb.h")

set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

# Silkinterfaces library
add_library(silkinterfaces)

asio_grpc_protobuf_generate(
    GENERATE_GRPC GENERATE_MOCK_CODE
    TARGET
    silkinterfaces
    USAGE_REQUIREMENT
    PUBLIC
    OUT_DIR
    "${PROTO_GENERATED_SRC_DIR}"
    IMPORT_DIRS
    "${IF_PROTO_PATH}"
    PROTOS
    "${IF_PROTO_PATH}/remote/ethbackend.proto"
    "${IF_PROTO_PATH}/remote/kv.proto"
    "${IF_PROTO_PATH}/p2psentry/sentry.proto"
    "${IF_PROTO_PATH}/txpool/mining.proto"
    "${IF_PROTO_PATH}/txpool/txpool.proto"
    "${IF_PROTO_PATH}/types/types.proto"
)

target_link_libraries(silkinterfaces PUBLIC
    gRPC::grpc++
    protobuf::libprotobuf)

# Copy generated Protocol Buffers and gRPC headers to Silkrpc source tree
set(IF_GENERATED_HEADER_DIR ${CMAKE_SOURCE_DIR}/silkrpc/interfaces)

set(REMOTE_IF_GENERATED_HEADER_DIR ${IF_GENERATED_HEADER_DIR}/remote)
file(MAKE_DIRECTORY ${REMOTE_IF_GENERATED_HEADER_DIR})
add_custom_command(
    TARGET silkinterfaces POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${BACKEND_PROTO_HEADER}
        ${BACKEND_GRPC_HEADER}
        ${BACKEND_MOCK_GRPC_HEADER}
        ${KV_PROTO_HEADER}
        ${KV_GRPC_HEADER}
        ${KV_MOCK_GRPC_HEADER}
        ${REMOTE_IF_GENERATED_HEADER_DIR}
)

set(P2PSENTRY_IF_GENERATED_HEADER_DIR ${IF_GENERATED_HEADER_DIR}/p2psentry)
file(MAKE_DIRECTORY ${P2PSENTRY_IF_GENERATED_HEADER_DIR})
add_custom_command(
    TARGET silkinterfaces POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${P2PSENTRY_PROTO_HEADER}
        ${P2PSENTRY_GRPC_HEADER}
        ${P2PSENTRY_MOCK_GRPC_HEADER}
        ${P2PSENTRY_IF_GENERATED_HEADER_DIR}
)

set(TXPOOL_IF_GENERATED_HEADER_DIR ${IF_GENERATED_HEADER_DIR}/txpool)
file(MAKE_DIRECTORY ${TXPOOL_IF_GENERATED_HEADER_DIR})
add_custom_command(
    TARGET silkinterfaces POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${MINING_PROTO_HEADER}
        ${MINING_GRPC_HEADER}
        ${MINING_MOCK_GRPC_HEADER}
        ${TXPOOL_IF_GENERATED_HEADER_DIR}
)
add_custom_command(
    TARGET silkinterfaces POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${TXPOOL_PROTO_HEADER}
        ${TXPOOL_GRPC_HEADER}
        ${TXPOOL_MOCK_GRPC_HEADER}
        ${TXPOOL_IF_GENERATED_HEADER_DIR}
)

set(TYPES_IF_GENERATED_HEADER_DIR ${IF_GENERATED_HEADER_DIR}/types)
file(MAKE_DIRECTORY ${TYPES_IF_GENERATED_HEADER_DIR})
add_custom_command(
    TARGET silkinterfaces POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${TYPES_PROTO_HEADER}
        ${TYPES_IF_GENERATED_HEADER_DIR}
)

# Silkrpc library
file(GLOB_RECURSE SILKRPC_SRC CONFIGURE_DEPENDS "*.cpp" "*.cc" "*.hpp" "*.c" "*.h")
list(FILTER SILKRPC_SRC EXCLUDE REGEX "main\.cpp$|_test\.cpp$|\.pb\.cc|\.pb\.h")

set(SILKRPC_LIBRARIES
    absl::flat_hash_map
    absl::flat_hash_set
    absl::btree
    asio-grpc::asio-grpc
    intx::intx
    gRPC::grpc++
    protobuf::libprotobuf
    silkinterfaces
    silkworm_core
    silkworm_node)
if(SILKRPC_USE_MIMALLOC)
    list(APPEND SILKRPC_LIBRARIES mimalloc)
endif()

add_library(silkrpc ${SILKRPC_SRC})
target_include_directories(silkrpc PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(silkrpc PUBLIC ${SILKRPC_LIBRARIES})
target_compile_features(silkrpc PUBLIC cxx_std_20)
target_compile_options(silkrpc PUBLIC $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-fcoroutines>)
