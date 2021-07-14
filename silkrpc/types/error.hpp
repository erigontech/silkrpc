/*
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
*/

#ifndef SILKRPC_TYPES_ERROR_HPP_
#define SILKRPC_TYPES_ERROR_HPP_

#include <iostream>
#include <string>

#include <evmc/evmc.hpp>

#include <silkworm/common/base.hpp>

namespace silkrpc {

struct Error {
    int32_t code{0};
    std::string message;
};

std::ostream& operator<<(std::ostream& out, const Error& error);

struct RevertError : public Error {
    silkworm::Bytes data;
};

std::ostream& operator<<(std::ostream& out, const RevertError& error);

} // namespace silkrpc

#endif  // SILKRPC_TYPES_ERROR_HPP_
