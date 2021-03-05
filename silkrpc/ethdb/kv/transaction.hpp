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

#ifndef SILKRPC_KV_TRANSACTION_H_
#define SILKRPC_KV_TRANSACTION_H_

#include <memory>
#include <string>

#include <silkrpc/config.hpp>

#include <asio/awaitable.hpp>

#include <silkworm/common/util.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/ethdb/kv/cursor.hpp>

namespace silkrpc::ethdb::kv {

class Transaction {
public:
    Transaction() = default;

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    virtual ~Transaction() = default;

    virtual std::unique_ptr<Cursor> cursor() = 0;

    virtual asio::awaitable<std::shared_ptr<Cursor>> cursor(const std::string& table) = 0;

    virtual asio::awaitable<void> close() = 0;
};

} // namespace silkrpc::ethdb::kv

#endif  // SILKRPC_KV_TRANSACTION_H_
