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

#include "database.hpp"

#include <climits>
#include <vector>

#include <boost/endian/conversion.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>

namespace silkrpc::ethdb::bitmap {

asio::awaitable<Roaring> get(core::rawdb::DatabaseReader& db_reader, const std::string& table, silkworm::Bytes& key, uint32_t from_block, uint32_t to_block) {
    using namespace silkworm;
    std::vector<const Roaring*> chuncks;

    silkworm::Bytes from_key{key.begin(), key.end()};
    from_key.resize(key.size() + sizeof(uint32_t));
    boost::endian::store_big_u32(&from_key[key.size()], from_block);
    SILKRPC_INFO << "key: " << key << " from_key: " << from_key << "\n" << std::flush;

    Roaring chunck{};
    core::rawdb::Walker walker = [&](const silkworm::Bytes& k, const silkworm::Bytes& v) {
        SILKRPC_INFO << "k: " << k << " v: " << v << "\n" << std::flush;
        chunck = std::move(Roaring::readSafe(reinterpret_cast<const char*>(v.data()), v.size()));
        chuncks.push_back(&chunck);
        auto block = boost::endian::load_big_u32(&k[k.size() - sizeof(uint32_t)]);
        return block < to_block;
    };
    co_await db_reader.walk(table, from_key, key.size() * CHAR_BIT, walker);

    co_return Roaring::fastunion(chuncks.size(), chuncks.data());
}

} // silkrpc::ethdb::bitmap
