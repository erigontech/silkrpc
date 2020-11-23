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

# It would be nice to use `buf` to give us protobuffer linting. Currently
# doesn't work because I need to figure out how to install it, need a target?
# add_custom_command(OUTPUT "protos/*.proto" COMMAND buf ARGS check lint)

# A few useful paths
set(PROTO_ROOT ${CMAKE_SOURCE_DIR}/silkrpc/protos/)
set(DEST_ROOT ${CMAKE_BINARY_DIR}/)

# Find all the protos in the repo (except for the kv repo which is handled differently)
file(GLOB_RECURSE PROTO_FILES CONFIGURE_DEPENDS "*.proto")
list(FILTER PROTO_FILES EXCLUDE REGEX "kv/remote/")

foreach(PROTO_FILE ${PROTO_FILES})
    # The path for the current proto file
    get_filename_component(PROTO_PATH ${PROTO_FILE} PATH)
    string(APPEND PROTO_PATH "/")

    # Build paths to the generated files (there are four for each proto file)
    string(REPLACE ${PROTO_ROOT} ${DEST_ROOT} TEMP ${PROTO_FILE})
    string(REPLACE ".proto" ".pb.cc"      GEN_SOURCE  ${TEMP})
    string(REPLACE ".proto" ".pb.h"       GEN_HEADER  ${TEMP})
    string(REPLACE ".proto" ".grpc.pb.cc" GRPC_SOURCE ${TEMP})
    string(REPLACE ".proto" ".grpc.pb.h"  GRPC_HEADER ${TEMP})

    # For debugging. Leave this here until we no longer need it
    # message(STATUS "PROTO_ROOT:      " ${PROTO_ROOT})
    # message(STATUS "DEST_ROOT:       " ${DEST_ROOT})
    # message(STATUS "PROTO_FILE:      " ${PROTO_FILE})
    # message(STATUS "PROTO_PATH:      " ${PROTO_PATH})
    # message(STATUS "TEMP:            " ${TEMP})
    # message(STATUS "GEN_SOURCE:      " ${GEN_SOURCE})
    # message(STATUS "GEN_HEADER:      " ${GEN_HEADER})
    # message(STATUS "GRPC_SOURCE:     " ${GRPC_SOURCE})
    # message(STATUS "GRPC_HEADER:     " ${GRPC_HEADER})
    # message(STATUS "PROTOBUF_PROTOC: " ${PROTOBUF_PROTOC})
    # message(STATUS "GRPC_CPP_PLUGIN: " ${GRPC_CPP_PLUGIN_EXECUTABLE})
    # message(STATUS "CURRENT_SOURCE:  " ${CMAKE_CURRENT_SOURCE_DIR})
    # message(STATUS "CURRENT_BIN:     " ${CMAKE_CURRENT_BINARY_DIR})
    # message(STATUS "DEST_ROOT:       " ${DEST_ROOT})
    # message(STATUS "----------- FINISHED: ${PROTO_FILE} -----------------------")
    # message(STATUS "")

    add_custom_command(
        OUTPUT "${GEN_SOURCE}" "${GEN_HEADER}" "${GRPC_SOURCE}" "${GRPC_HEADER}"
        COMMAND ${PROTOBUF_PROTOC}
        ARGS
            --grpc_out="${DEST_ROOT}"
            --cpp_out="${DEST_ROOT}"
            --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_EXECUTABLE}"
            -I="${PROTO_ROOT}" # we need both, order matters
            -I="${PROTO_PATH}"
            "${PROTO_FILE}"
        DEPENDS ${PROTO_FILE}
    )

    # Save the .cc files for the build
    list(APPEND GENERATED_SOURCES ${GEN_SOURCE})
    list(APPEND GENERATED_SOURCES ${GRPC_SOURCE})
endforeach(PROTO_FILE)

# Collect the sources for the daemon
file(GLOB_RECURSE DAEMON_SOURCES CONFIGURE_DEPENDS "*.cpp" "*.hpp")
list(FILTER DAEMON_SOURCES EXCLUDE REGEX "_test\.cpp$")

# Build the executable
add_executable(silkrpc ${DAEMON_SOURCES} ${GENERATED_SOURCES})
target_include_directories(silkrpc PUBLIC ${DEST_ROOT})
target_include_directories(silkrpc PUBLIC "/Users/jrush/Development/silkrpc/build/")
target_link_libraries(silkrpc absl::flat_hash_map absl::flat_hash_set absl::btree intx::intx gRPC::grpc++ protobuf::libprotobuf silkworm)
