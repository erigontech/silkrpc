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

#include "local_database.hpp"

#include <silkrpc/common/log.hpp>
#include <silkrpc/ethdb/file/local_transaction.hpp>

namespace silkrpc::ethdb::file {

LocalDatabase::LocalDatabase(std::string local_database) {
    SILKRPC_TRACE << "LocalDatabase::ctor " << this << "\n";
}

LocalDatabase::~LocalDatabase() {
    SILKRPC_TRACE << "LocalDatabase::dtor " << this << "\n";
}

boost::asio::awaitable<std::unique_ptr<Transaction>> LocalDatabase::begin() {
    SILKRPC_TRACE << "LocalDatabase::begin " << this << " start\n";
    auto txn = std::make_unique<LocalTransaction>();
    co_await txn->open();
    SILKRPC_TRACE << "LocalDatabase::begin " << this << " txn: " << txn.get() << " end\n";
    co_return txn;
}

} // namespace silkrpc::ethdb::file