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

#ifndef SILKRPC_ETHDB_BITMAP_HPP_
#define SILKRPC_ETHDB_BITMAP_HPP_

#include <string>

#include <silkrpc/config.hpp>

#include <boost/asio/awaitable.hpp>

#include <silkworm/common/util.hpp>
#include <silkrpc/core/rawdb/accessors.hpp>
#include <silkrpc/croaring/roaring.hh>

namespace silkrpc::ethdb::bitmap {

boost::asio::awaitable<roaring::Roaring> get(core::rawdb::DatabaseReader& db_reader, const std::string& table, silkworm::Bytes& key, uint32_t from_block, uint32_t to_block);

} // silkrpc::ethdb::bitmap

#endif  // SILKRPC_ETHDB_BITMAP_HPP_
