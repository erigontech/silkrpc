#[[
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
]]

find_package(absl CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(intx CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

# This doesn't work - need to install buf, need a target?
# add_custom_command(OUTPUT "protos/*.proto" COMMAND buf ARGS check lint)

file(GLOB_RECURSE DAEMON_SOURCES CONFIGURE_DEPENDS "*.cpp" "*.hpp")
list(FILTER DAEMON_SOURCES EXCLUDE REGEX "_test\.cpp$")

file(GLOB_RECURSE GENERATED_SOURCES CONFIGURE_DEPENDS "*.cc" "*.c" "*.h")

add_executable(silkrpc ${DAEMON_SOURCES} ${GENERATED_SOURCES})
target_include_directories(silkrpc PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(silkrpc absl::flat_hash_map absl::flat_hash_set absl::btree intx::intx gRPC::grpc++ protobuf::libprotobuf silkworm)
