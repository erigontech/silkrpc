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

if(MSVC)
  add_link_options(/STACK:10000000)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  add_link_options(-Wl,-stack_size -Wl,0x1000000)
endif()

find_package(gRPC CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)

file(GLOB_RECURSE SILKRPC_SRC CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/silkrpc/common/*.cpp"
    "${CMAKE_SOURCE_DIR}/silkrpc/common/*.hpp"
    "${CMAKE_SOURCE_DIR}/silkrpc/kv/remote/*.cc"
    "${CMAKE_SOURCE_DIR}/silkrpc/kv/remote/*.h"
)

# kv_seek
add_executable(kv_seek_async_asio kv_seek_async_asio.cpp ${SILKRPC_SRC})
target_include_directories(kv_seek_async_asio PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(kv_seek_async_asio absl::flags_parse gRPC::grpc++_unsecure protobuf::libprotobuf silkworm)

add_executable(kv_seek_async kv_seek_async.cpp ${SILKRPC_SRC})
target_include_directories(kv_seek_async PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(kv_seek_async absl::flags_parse gRPC::grpc++_unsecure protobuf::libprotobuf silkworm)

add_executable(kv_seek kv_seek.cpp ${SILKRPC_SRC})
target_include_directories(kv_seek PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(kv_seek absl::flags_parse gRPC::grpc++_unsecure protobuf::libprotobuf silkworm)

# test only client
#file(GLOB_RECURSE GENERATED_SOURCES CONFIGURE_DEPENDS "${CMAKE_BINARY_DIR}/*.cc")
#message(STATUS "CMAKE_BINARY_DIR: " ${CMAKE_SOURCE_DIR})
#message(STATUS "CMAKE_BINARY_DIR: " ${CMAKE_BINARY_DIR})
#message(STATUS "GENERATED_SOURCES: " ${SILKRPC_SOURCES})
#message(STATUS "GENERATED_SOURCES: " ${GENERATED_SOURCES})
#add_executable(test_client test_client.cpp ${SILKRPC_SRC} ${GENERATED_SOURCES})
#target_include_directories(test_client PUBLIC ${CMAKE_BINARY_DIR})
#target_include_directories(test_client PUBLIC ${CMAKE_SOURCE_DIR})
#target_link_libraries(test_client absl::flags_parse gRPC::grpc++_unsecure protobuf::libprotobuf silkworm)

# Unit tests
enable_testing()

find_package(Catch2 CONFIG REQUIRED)

file(GLOB_RECURSE SILKRPC_TESTS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/silkrpc/*_test.cpp")
add_executable(unit_test unit_test.cpp ${SILKRPC_TESTS})
target_link_libraries(unit_test Catch2::Catch2)

include(CTest)
include(Catch)
catch_discover_tests(unit_test)
