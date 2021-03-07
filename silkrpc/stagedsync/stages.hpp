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

#ifndef SILKRPC_STAGEDSYNC_STAGES_HPP_
#define SILKRPC_STAGEDSYNC_STAGES_HPP_

#include <string>

#include <silkrpc/config.hpp>

#include <asio/awaitable.hpp>

#include <silkworm/common/base.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/core/rawdb/accessors.hpp>

namespace silkrpc::stages {

using silkworm::Bytes, silkworm::bytes_of_string;

/* The synchronization stages */
const Bytes kHeaders             = bytes_of_string("Headers");             // Downloads headers, verifying their POW validity and chaining
const Bytes kBlockHashes         = bytes_of_string("BlockHashes");         // Writes header numbers, fills blockHash => number table
const Bytes kBodies              = bytes_of_string("Bodies");              // Downloads block bodies, TxHash and UncleHash are getting verified
const Bytes kSenders             = bytes_of_string("Senders");             // "From" recovered from signatures, bodies re-written
const Bytes kExecution           = bytes_of_string("Execution");           // Executing each block w/o building a trie
const Bytes kIntermediateHashes  = bytes_of_string("IntermediateHashes");  // Generate intermediate hashes, calculate the state root hash
const Bytes kHashState           = bytes_of_string("HashState");           // Apply Keccak256 to all the keys in the state
const Bytes kAccountHistoryIndex = bytes_of_string("AccountHistoryIndex"); // Generating history index for accounts
const Bytes kStorageHistoryIndex = bytes_of_string("StorageHistoryIndex"); // Generating history index for storage
const Bytes kLogIndex            = bytes_of_string("LogIndex");            // Generating logs index (from receipts)
const Bytes kTxLookup            = bytes_of_string("TxLookup");            // Generating transactions lookup index
const Bytes kTxPool              = bytes_of_string("TxPool");              // Starts Backend
const Bytes kFinish              = bytes_of_string("Finish");              // Nominal stage after all other stages

asio::awaitable<uint64_t> get_sync_stage_progress(const core::rawdb::DatabaseReader& database, const Bytes& stage_key);

} // namespace silkrpc::stages

#endif  // SILKRPC_STAGEDSYNC_STAGES_HPP_
