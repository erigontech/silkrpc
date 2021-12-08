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

#ifndef SILKRPC_BLOCK_CACHE_HPP_
#define SILKRPC_BLOCK_CACHE_HPP_

#include <boost/compute/detail/lru_cache.hpp>
#include <boost/thread/mutex.hpp>

#include <evmc/evmc.hpp>
#include <silkworm/chain/config.hpp>
#include <silkworm/common/util.hpp>
#include <silkworm/common/base.hpp>
#include <silkworm/execution/address.hpp>
#include <silkworm/db/util.hpp>
#include <silkworm/types/receipt.hpp>
#include <silkworm/types/transaction.hpp>


namespace silkrpc {

class BlockCache { 
public:
    BlockCache(int capacity, bool shared_cache) : block_cache_(capacity), shared_cache_(shared_cache) {}

public:
    boost::optional <silkworm::BlockWithHash> get (const evmc::bytes32 &key) {
       if (shared_cache_)
           mtx_.lock();
       auto block =  block_cache_.get(key);
       if (shared_cache_)
           mtx_.unlock();
       return block;
    }

    void insert (const evmc::bytes32 &key, silkworm::BlockWithHash& block) {
       if (shared_cache_)
          mtx_.lock();
       block_cache_.insert(key, block);
       if (shared_cache_)
          mtx_.unlock();
    }

private:
    boost::compute::detail::lru_cache<evmc::bytes32, silkworm::BlockWithHash> block_cache_;
    boost::mutex mtx_;
    bool shared_cache_;
};

} // namespace silkrpc

#endif // SILKRPC_BLOCK_CACHE_HPP_
