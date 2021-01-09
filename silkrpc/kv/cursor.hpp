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

#ifndef SILKRPC_KV_CURSOR_H_
#define SILKRPC_KV_CURSOR_H_

#include <silkrpc/config.hpp>

#include <memory>

#include <asio/awaitable.hpp>

#include <silkworm/common/util.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/kv/client.hpp>

namespace silkrpc::kv {

class Cursor {
public:
    Cursor(std::shared_ptr<Client> client) : client_(client) {}

    Cursor(const Cursor&) = delete;
    Cursor& operator=(const Cursor&) = delete;

    asio::awaitable<common::KeyValue> seek(const std::string& table_name, const silkworm::Bytes& seek_key);

private:
    std::shared_ptr<Client> client_;
};

} // namespace silkrpc::kv

#endif  // SILKRPC_KV_CURSOR_H_
