/*
   Copyright 2021 The Silkrpc Authors

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

#ifndef SILKRPC_CORE_EVM_ACCESS_LIST_TRACER_HPP_
#define SILKRPC_CORE_EVM_ACCESS_LIST_TRACER_HPP_

#include <string>
#include <vector>

#include <silkworm/core/silkworm/execution/evm.hpp>
#include <silkworm/state/intra_block_state.hpp>

#include <silkrpc/context_pool.hpp>
#include <silkrpc/core/rawdb/accessors.hpp>
#include <silkrpc/types/block.hpp>
#include <silkrpc/types/call.hpp>
#include <silkrpc/types/transaction.hpp>

namespace silkrpc::access_list {

class AccessListTracer : public silkworm::EvmTracer {
public:
    explicit AccessListTracer(const std::optional<std::vector<silkworm::AccessListEntry>>& input_access_list, const evmc::address& from, const evmc::address& to): from_{from}, to_{to} {
       if (input_access_list)
          add_local_access_list(*input_access_list);
    }
    AccessListTracer(const AccessListTracer&) = delete;
    AccessListTracer& operator=(const AccessListTracer&) = delete;

    void set_access_list(std::vector<silkworm::AccessListEntry> ale) { access_list_ = ale; }
    std::vector<silkworm::AccessListEntry> get_access_list() { return access_list_; }
    bool compare(const std::vector<silkworm::AccessListEntry>& acl1, const std::vector<silkworm::AccessListEntry>& acl2);

    void on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept override;
    void on_instruction_start(uint32_t pc, const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept override;
    void on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept override;
    void dump(const std::string str, const std::vector<silkworm::AccessListEntry>& acl);

    //void dump(const std::string str);
    //bool compare(std::shared_ptr<silkrpc::access_list::AccessListTracer> other);

private:
    inline bool exclude(const evmc::address& address);
    void add_storage(const evmc::address& address, const evmc::bytes32& storage);
    void add_address(const evmc::address& address);
    void add_local_access_list(const std::vector<silkworm::AccessListEntry> input_access_list);


    std::vector<silkworm::AccessListEntry> access_list_;
    evmc::address from_;
    evmc::address to_;

    const char* const* opcode_names_ = nullptr;
};


} // namespace silkrpc::access_list

#endif  // SILKRPC_CORE_EVM_ACCESS_LIST_TRACER_HPP_
