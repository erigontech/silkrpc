/*
   Copyright 2022 The Silkrpc Authors

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

#ifndef SILKRPC_TEST_DUMMY_TRANSACTION_HPP_
#define SILKRPC_TEST_DUMMY_TRANSACTION_HPP_

#include <memory>
#include <string>

#include <asio/awaitable.hpp>

#include <silkrpc/common/util.hpp>
#include <silkrpc/ethdb/cursor.hpp>
#include <silkrpc/ethdb/transaction.hpp>

namespace silkrpc::test {

//! This dummy transaction just gives you the same cursor over and over again.
class DummyTransaction : public ethdb::Transaction {
public:
    explicit DummyTransaction(std::shared_ptr<ethdb::Cursor> cursor) : cursor_(cursor) {}

    uint64_t tx_id() const override { return 0; }

    asio::awaitable<void> open() override { co_return; }

    asio::awaitable<std::shared_ptr<ethdb::Cursor>> cursor(const std::string& /*table*/) override {
        co_return cursor_;
    }

    asio::awaitable<std::shared_ptr<ethdb::CursorDupSort>> cursor_dup_sort(const std::string& /*table*/) override {
        co_return nullptr;
    }

    asio::awaitable<void> close() override { co_return; }

private:
    std::shared_ptr<ethdb::Cursor> cursor_;
};

}  // namespace silkrpc::test

#endif  // SILKRPC_TEST_DUMMY_TRANSACTION_HPP_
