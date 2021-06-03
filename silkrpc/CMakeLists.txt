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
find_program(PROTOBUF_PROTOC protoc REQUIRED)

# Find gRPC installation
find_package(gRPC CONFIG REQUIRED)
find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin REQUIRED)

# Define gRPC proto files
set(IF_PROTO_PATH "${CMAKE_SOURCE_DIR}/interfaces")

# Generate Protocol Buffers and gRPC bindings
set(PROTO_GENERATED_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto-bindings)
file(MAKE_DIRECTORY ${PROTO_GENERATED_SRC_DIR})

set(BACKEND_PROTO_SOURCE "${PROTO_GENERATED_SRC_DIR}/remote/ethbackend.pb.cc")
set(BACKEND_PROTO_HEADER "${PROTO_GENERATED_SRC_DIR}/remote/ethbackend.pb.h")
set(BACKEND_GRPC_SOURCE "${PROTO_GENERATED_SRC_DIR}/remote/ethbackend.grpc.pb.cc")
set(BACKEND_GRPC_HEADER "${PROTO_GENERATED_SRC_DIR}/remote/ethbackend.grpc.pb.h")
set(KV_PROTO_SOURCE "${PROTO_GENERATED_SRC_DIR}/remote/kv.pb.cc")
set(KV_PROTO_HEADER "${PROTO_GENERATED_SRC_DIR}/remote/kv.pb.h")
set(KV_GRPC_SOURCE "${PROTO_GENERATED_SRC_DIR}/remote/kv.grpc.pb.cc")
set(KV_GRPC_HEADER "${PROTO_GENERATED_SRC_DIR}/remote/kv.grpc.pb.h")
set(TYPES_PROTO_SOURCE "${PROTO_GENERATED_SRC_DIR}/types/types.pb.cc")
set(TYPES_PROTO_HEADER "${PROTO_GENERATED_SRC_DIR}/types/types.pb.h")

set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

add_custom_command(
    OUTPUT "${TYPES_PROTO_HEADER}" "${TYPES_PROTO_SOURCE}"
      "${BACKEND_PROTO_HEADER}" "${BACKEND_PROTO_SOURCE}" "${BACKEND_GRPC_HEADER}" "${BACKEND_GRPC_SOURCE}"
      "${KV_PROTO_SOURCE}" "${KV_PROTO_HEADER}" "${KV_GRPC_SOURCE}" "${KV_GRPC_HEADER}"
    COMMAND ${PROTOBUF_PROTOC}
    ARGS --grpc_out "${PROTO_GENERATED_SRC_DIR}"
      --cpp_out "${PROTO_GENERATED_SRC_DIR}"
      --proto_path "${IF_PROTO_PATH}"
      --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_EXECUTABLE}"
      "${IF_PROTO_PATH}/**/ethbackend.proto"
      "${IF_PROTO_PATH}/**/kv.proto"
      "${IF_PROTO_PATH}/**/types.proto"
    COMMENT "Running C++ gRPC compiler on ${IF_PROTO_PATH}"
)

# Silkinterfaces library
add_library(silkinterfaces
    ${TYPES_PROTO_HEADER} ${TYPES_PROTO_SOURCE}
    ${BACKEND_PROTO_HEADER} ${BACKEND_PROTO_SOURCE} ${BACKEND_GRPC_HEADER} ${BACKEND_GRPC_SOURCE}
    ${KV_PROTO_HEADER} ${KV_PROTO_SOURCE} ${KV_GRPC_HEADER} ${KV_GRPC_SOURCE})
target_include_directories(silkinterfaces PUBLIC ${PROTO_GENERATED_SRC_DIR})
target_link_libraries(silkinterfaces
    tcmalloc
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
        ${KV_PROTO_HEADER}
        ${KV_GRPC_HEADER}
        ${REMOTE_IF_GENERATED_HEADER_DIR}
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

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fcoroutines")
endif()

add_library(silkrpc ${SILKRPC_SRC})
target_include_directories(silkrpc PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(silkrpc
    tcmalloc
    absl::flat_hash_map
    absl::flat_hash_set
    absl::btree
    intx::intx
    gRPC::grpc++
    protobuf::libprotobuf
    silkinterfaces
    silkworm_core
    silkworm_db)

add_executable(silkrpcdaemon main.cpp)
target_include_directories(silkrpcdaemon PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(silkrpcdaemon
    tcmalloc
    absl::btree
    absl::flags_parse
    absl::flat_hash_map
    absl::flat_hash_set
    gRPC::grpc++
    intx::intx
    protobuf::libprotobuf
    silkrpc
    silkinterfaces
    silkworm_core
    silkworm_db)
