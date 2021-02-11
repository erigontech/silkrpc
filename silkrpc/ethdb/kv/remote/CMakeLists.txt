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

# gRPC proto file
get_filename_component(KV_PROTO "kv.proto" ABSOLUTE)
get_filename_component(KV_PROTO_PATH "${KV_PROTO}" PATH)

# Generate sources
set(KV_PROTO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/kv.pb.cc")
set(KV_PROTO_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/kv.pb.h")
set(KV_GRPC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/kv.grpc.pb.cc")
set(KV_GRPC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/kv.grpc.pb.h")

add_custom_command(
    OUTPUT "${KV_PROTO_SOURCES}" "${KV_PROTO_HEADERS}" "${KV_GRPC_SOURCES}" "${KV_GRPC_HEADERS}"
    COMMAND ${PROTOBUF_PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_SOURCE_DIR}"
      --cpp_out "${CMAKE_CURRENT_SOURCE_DIR}"
      -I "${KV_PROTO_PATH}"
      --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_EXECUTABLE}"
      "${KV_PROTO}"
    DEPENDS "${KV_PROTO}"
)

add_custom_target(
    generate_kv_grpc
    DEPENDS "${KV_PROTO_SOURCES}" "${KV_PROTO_HEADERS}" "${KV_GRPC_SOURCES}" "${KV_GRPC_HEADERS}"
)
