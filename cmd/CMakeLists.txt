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

add_executable(silkrpc_toolbox
    silkrpc_toolbox.cpp
    ethbackend_async.cpp ethbackend_coroutines.cpp ethbackend.cpp
    kv_seek_async_callback.cpp kv_seek_async_coroutines.cpp kv_seek_async.cpp kv_seek.cpp
    kv_seek_both.cpp
)
target_include_directories(silkrpc_toolbox PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(silkrpc_toolbox absl::flags_parse gRPC::grpc++_unsecure protobuf::libprotobuf silkrpc)

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
