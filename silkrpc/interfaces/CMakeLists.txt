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

# Find Protobuf installation
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
find_program(PROTOBUF_PROTOC protoc REQUIRED)

# Find gRPC installation
find_package(gRPC CONFIG REQUIRED)
find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin REQUIRED)

# gRPC proto files
set(IF_PROTO_PATH "${CMAKE_SOURCE_DIR}/interfaces")

# Generate sources
set(KV_PROTO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/remote/kv.pb.cc")
set(KV_PROTO_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/remote/kv.pb.h")
set(KV_GRPC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/remote/kv.grpc.pb.cc")
set(KV_GRPC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/remote/kv.grpc.pb.h")
set(TYPES_PROTO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/types/types.pb.cc")
set(TYPES_PROTO_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/types/types.pb.h")

add_custom_command(
    OUTPUT "${KV_PROTO_SOURCES}" "${KV_PROTO_HEADERS}" "${KV_GRPC_SOURCES}" "${KV_GRPC_HEADERS}"
    COMMAND ${PROTOBUF_PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_SOURCE_DIR}"
      --cpp_out "${CMAKE_CURRENT_SOURCE_DIR}"
      --proto_path "${IF_PROTO_PATH}"
      --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_EXECUTABLE}"
      "${IF_PROTO_PATH}/**/ethbackend.proto"
      "${IF_PROTO_PATH}/**/kv.proto"
      "${IF_PROTO_PATH}/**/types.proto"
)

add_custom_target(
    generate_kv_grpc
    DEPENDS "${KV_PROTO_SOURCES}" "${KV_PROTO_HEADERS}" "${KV_GRPC_SOURCES}" "${KV_GRPC_HEADERS}"
)
