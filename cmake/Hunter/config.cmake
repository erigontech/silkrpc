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

# silkworm configuration
include(${CMAKE_SOURCE_DIR}/silkworm/cmake/Hunter/config.cmake)

hunter_config(
    asio-grpc
    VERSION ${HUNTER_asio-grpc_VERSION}
    CMAKE_ARGS
      ASIO_GRPC_USE_BOOST_CONTAINER=ON
)
