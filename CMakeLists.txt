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

cmake_minimum_required(VERSION 3.16)

# Check that submodules initialization has been done
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/silkworm/.git)
  message(FATAL_ERROR "Git submodules not initialized, execute:\n  git submodule update --init --recursive")
endif()

# Use default toolchain file if not specified on the command line
if(NOT CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/silkworm/cmake/toolchain/cxx20.cmake CACHE FILEPATH "" FORCE)
endif()

include(silkworm/third_party/evmone/cmake/cable/bootstrap.cmake)
include(CableBuildInfo)
include(silkworm/third_party/evmone/cmake/cable/HunterGate.cmake)
HunterGate(
  URL "https://github.com/cpp-pm/hunter/archive/v0.24.3.tar.gz"
  SHA1 "10738b59e539818a01090e64c2d09896247530c7"
  FILEPATH "${CMAKE_SOURCE_DIR}/cmake/Hunter/config.cmake"
)

project(silkrpc)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_C_COMPILER_VERSION VERSION_LESS 10.3.0)
    message(FATAL_ERROR "required gcc version >= 10.3.0")
  endif()
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.3.0)
    message(FATAL_ERROR "required g++ version >= 10.3.0")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  if(CMAKE_C_COMPILER_VERSION VERSION_LESS 10.0.0)
    message(FATAL_ERROR "required clang version >= 10.0.0")
  endif()
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0.0)
    message(FATAL_ERROR "required clang++ version >= 10.0.0")
  endif()
endif()

include(cmake/Hunter/packages.cmake)

# Silkworm
find_package(ethash CONFIG REQUIRED)
find_package(intx CONFIG REQUIRED)

add_subdirectory(silkworm)

# Silkrpc itself
option(SILKRPC_CLANG_COVERAGE "Clang instrumentation for code coverage reports" OFF)

if(SILKRPC_CLANG_COVERAGE)

  add_compile_options(-fprofile-instr-generate -fcoverage-mapping -DBUILD_COVERAGE)
  add_link_options(-fprofile-instr-generate -fcoverage-mapping)
endif()

add_subdirectory(silkrpc)
add_subdirectory(cmd)
add_subdirectory(examples)
