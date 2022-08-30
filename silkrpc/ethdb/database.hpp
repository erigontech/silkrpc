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

#ifndef SILKRPC_ETHDB_DATABASE_HPP_
#define SILKRPC_ETHDB_DATABASE_HPP_

#include <memory>

#include <silkrpc/config.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>

#include <silkrpc/ethdb/transaction.hpp>

namespace silkrpc::ethdb {

class Database {
public:
    Database() = default;
    virtual ~Database() = default;

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    virtual boost::asio::awaitable<std::unique_ptr<Transaction>> begin() = 0;
};

} // namespace silkrpc::ethdb

#endif  // SILKRPC_ETHDB_DATABASE_HPP_
