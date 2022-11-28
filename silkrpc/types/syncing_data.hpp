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

#ifndef SILKRPC_TYPES_SYNCING_DATA_HPP_
#define SILKRPC_TYPES_SYNCING_DATA_HPP_

#include <vector>
#include <string>

namespace silkrpc {

struct StageData {
     std::string stage_name;
     std::string block_number;
};

struct SyncingData {
     std::string current_block;
     std::string highest_block;
     std::vector<struct StageData> stages;
};

} // namespace silkrpc

#endif  // SILKRPC_TYPES_SYNCING_DATA_HPP_