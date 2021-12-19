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


#ifndef SILKRPC_CORE_ETH_UTIL_API_HPP_
#define SILKRPC_CORE_ETH_UTIL_API_HPP_

#include <silkrpc/config.hpp>

#include <asio/awaitable.hpp>
#include <evmc/evmc.hpp>
#include <intx/intx.hpp>
#include <nlohmann/json.hpp>

#include <silkworm/chain/config.hpp>
#include <silkworm/common/util.hpp>
#include <silkworm/common/base.hpp>
#include <silkworm/types/block.hpp>
#include <silkworm/types/transaction.hpp>

#include <silkrpc/core/rawdb/accessors.hpp>
#include <silkrpc/types/block.hpp>
#include <silkrpc/types/chain_config.hpp>
#include <silkrpc/types/receipt.hpp>
#include <silkrpc/types/log.hpp>

namespace silkrpc::core  {

asio::awaitable<silkworm::BlockWithHash> read_block_by_number(const silkrpc::Context &context, const silkrpc::core::rawdb::DatabaseReader& reader, uint64_t block_number);
asio::awaitable<silkworm::BlockWithHash> read_block_by_hash(const silkrpc::Context &context, const silkrpc::core::rawdb::DatabaseReader& reader, const evmc::bytes32& block_hash);

} // namespace silkrpc::core
#endif // SILKRPC_CORE_ETH_UTIL_API_HPP_

