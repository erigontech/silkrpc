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
    explicit AccessListTracer(AccessListResult& access_list_result) : access_list_result_(access_list_result) {}

    AccessListTracer(const AccessListTracer&) = delete;
    AccessListTracer& operator=(const AccessListTracer&) = delete;

    void on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept override;
    void on_instruction_start(uint32_t pc, const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept override;
    void on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept override;

private:
    void addStorage(const evmc::address& address, const evmc::bytes32& storage);

    AccessListResult& access_list_result_;

    const char* const* opcode_names_ = nullptr;
    std::int64_t start_gas_{0};
};


} // namespace silkrpc::access_list

#endif  // SILKRPC_CORE_EVM_ACCESS_LIST_TRACER_HPP_
