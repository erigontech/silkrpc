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

#ifndef SILKRPC_KV_DATABASE_H_
#define SILKRPC_KV_DATABASE_H_

#include <silkworm/common/util.hpp>
#include <silkrpc/kv/client.hpp>
#include <silkrpc/kv/cursor.hpp>

namespace silkrpc::kv {

class Database {
public:
    Database(Client& client) : client_(client) {}

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    Cursor cursor() { return Cursor{client_}; }

private:
    Client& client_;
};

} // namespace silkrpc::kv

#endif  // SILKRPC_KV_DATABASE_H_
