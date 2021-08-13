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

add_compile_options(-std=c++2a)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-fcoroutines)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-stdlib=libc++)
endif()

add_executable(kv_seek_async_asio kv_seek_async_asio.cpp)
target_include_directories(kv_seek_async_asio PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(kv_seek_async_asio absl::flags_parse gRPC::grpc++_unsecure protobuf::libprotobuf silkworm_core silkworm_db silkrpc)

add_executable(kv_seek_async_callback kv_seek_async_callback.cpp)
target_include_directories(kv_seek_async_callback PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(kv_seek_async_callback absl::flags_parse gRPC::grpc++_unsecure protobuf::libprotobuf silkworm_core silkworm_db silkrpc)

add_executable(kv_seek_async_coroutines kv_seek_async_coroutines.cpp)
target_include_directories(kv_seek_async_coroutines PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(kv_seek_async_coroutines absl::flags_parse gRPC::grpc++_unsecure protobuf::libprotobuf silkworm_core silkworm_db silkrpc)

add_executable(kv_seek_async kv_seek_async.cpp)
target_include_directories(kv_seek_async PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(kv_seek_async absl::flags_parse gRPC::grpc++_unsecure protobuf::libprotobuf silkworm_core silkworm_db silkrpc)

add_executable(kv_seek kv_seek.cpp)
target_include_directories(kv_seek PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(kv_seek absl::flags_parse gRPC::grpc++_unsecure protobuf::libprotobuf silkworm_core silkworm_db silkrpc)

add_executable(kv_seek_both kv_seek_both.cpp)
target_include_directories(kv_seek_both PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(kv_seek_both absl::flags_parse gRPC::grpc++_unsecure protobuf::libprotobuf silkworm_core silkworm_db silkrpc)

add_executable(ethbackend ethbackend.cpp)
target_include_directories(ethbackend PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ethbackend absl::flags_parse gRPC::grpc++_unsecure protobuf::libprotobuf silkworm_core silkworm_db silkrpc)

add_executable(ethbackend_async ethbackend_async.cpp)
target_include_directories(ethbackend_async PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ethbackend_async absl::flags_parse gRPC::grpc++_unsecure protobuf::libprotobuf silkworm_core silkworm_db silkrpc)

add_executable(ethbackend_coroutines ethbackend_coroutines.cpp)
target_include_directories(ethbackend_coroutines PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ethbackend_coroutines absl::flags_parse gRPC::grpc++_unsecure protobuf::libprotobuf silkworm_core silkworm_db silkrpc)

# Unit tests
enable_testing()

find_package(Catch2 CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

file(GLOB_RECURSE SILKRPC_TESTS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/silkrpc/*_test.cpp")
add_executable(unit_test unit_test.cpp ${SILKRPC_TESTS})
target_link_libraries(unit_test silkrpc Catch2::Catch2 GTest::gmock)

include(CTest)
include(Catch)
catch_discover_tests(unit_test)
