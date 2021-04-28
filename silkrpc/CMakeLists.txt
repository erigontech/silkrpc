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
find_package(gRPC CONFIG REQUIRED)
find_package(intx CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

file(GLOB_RECURSE SILKRPC_SRC CONFIGURE_DEPENDS "*.cpp" "*.cc" "*.hpp" "*.c" "*.h")
list(FILTER SILKRPC_SRC EXCLUDE REGEX "main\.cpp$|_test\.cpp$|\.pb\.cc|\.pb\.h")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fcoroutines")
endif()

set(SILKRPC_GENERATED_IFC_PATH "${CMAKE_SOURCE_DIR}/silkrpc/interfaces")
add_library(silkinterfaces
    ${SILKRPC_GENERATED_IFC_PATH}/remote/ethbackend.grpc.pb.cc
    ${SILKRPC_GENERATED_IFC_PATH}/remote/ethbackend.pb.cc
    ${SILKRPC_GENERATED_IFC_PATH}/remote/kv.grpc.pb.cc
    ${SILKRPC_GENERATED_IFC_PATH}/remote/kv.pb.cc
    ${SILKRPC_GENERATED_IFC_PATH}/types/types.grpc.pb.cc
    ${SILKRPC_GENERATED_IFC_PATH}/types/types.pb.cc)
target_include_directories(silkinterfaces PUBLIC ${CMAKE_SOURCE_DIR}/silkrpc/interfaces)
target_link_libraries(silkinterfaces
    tcmalloc
    gRPC::grpc++
    protobuf::libprotobuf)

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
