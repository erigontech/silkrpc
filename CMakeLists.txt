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

cmake_minimum_required(VERSION 3.13)

# Check that submodules initialization has been done
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/silkworm/.git)
  message(FATAL_ERROR "Git submodules not initialized, execute:\n  git submodule update --init --recursive")
endif()

# Use default toolchain file if not specified on the command line
if(NOT CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/silkworm/cmake/toolchain.cmake CACHE FILEPATH "" FORCE)
endif()

include(silkworm/evmone/cmake/cable/HunterGate.cmake)
HunterGate(
  URL "https://github.com/cpp-pm/hunter/archive/v0.23.293.tar.gz"
  SHA1 "e8e5470652db77149d9b38656db2a6c0b7642693"
  FILEPATH "${CMAKE_SOURCE_DIR}/cmake/Hunter/config.cmake"
)

project(silkrpc)

include(cmake/Hunter/packages.cmake)

# Silkworm
find_package(ethash CONFIG REQUIRED)
find_package(intx CONFIG REQUIRED)

add_subdirectory(silkworm)

# Silkrpc itself
add_subdirectory(silkrpc)
add_subdirectory(cmd)
