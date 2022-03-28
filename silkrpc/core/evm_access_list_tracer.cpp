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

#include "evm_access_list_tracer.hpp"

#include <evmc/hex.hpp>
#include <evmc/instructions.h>
#include <intx/intx.hpp>
#include <silkworm/third_party/evmone/lib/evmone/execution_state.hpp>
#include <silkworm/third_party/evmone/lib/evmone/instructions.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/core/evm_executor.hpp>

namespace silkrpc::access_list {

std::string get_opcode_name(const char* const* names, std::uint8_t opcode) {
    const auto name = names[opcode];
    return (name != nullptr) ?name : "opcode 0x" + evmc::hex(opcode) + " not defined";
}

void AccessListTracer::on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept {
    if (opcode_names_ == nullptr) {
        opcode_names_ = evmc_get_instruction_names_table(rev);
    }

    start_gas_ = msg.gas;
    evmc::address recipient(msg.recipient);
    evmc::address sender(msg.sender);
    SILKRPC_DEBUG << "on_execution_start: gas: " << std::dec << msg.gas
        << " depth: " << msg.depth
        << " recipient: " << recipient
        << " sender: " << sender
        << "\n";
}

void AccessListTracer::on_instruction_start(uint32_t pc, const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept {
    assert(execution_state.msg);
    evmc::address recipient(execution_state.msg->recipient);
    evmc::address sender(execution_state.msg->sender);

    const auto opcode = execution_state.code[pc];
    auto opcode_name = get_opcode_name(opcode_names_, opcode);

    SILKRPC_DEBUG << "on_instruction_start:"
        << " pc: " << std::dec << pc
        << " opcode: 0x" << std::hex << evmc::hex(opcode)
        << " opcode_name: " << opcode_name
        << " recipient: " << recipient
        << " sender: " << sender
        << " execution_state: {"
        << "   gas_left: " << std::dec << execution_state.gas_left
        << "   status: " << execution_state.status
        << "   msg.gas: " << std::dec << execution_state.msg->gas
        << "   msg.depth: " << std::dec << execution_state.msg->depth
        << "}\n";

    if (opcode_name == "SLOAD" && execution_state.stack.size() > 0) {
       const auto address = silkworm::bytes32_from_hex(intx::hex(execution_state.stack[0]));
       addStorage(recipient, address);

    } else if (opcode_name == "SSTORE" && execution_state.stack.size() > 1) {
       const auto address = silkworm::bytes32_from_hex(intx::hex(execution_state.stack[0]));
       addStorage(recipient, address);
    }

    //log.gas = execution_state.gas_left;
}


void AccessListTracer::addStorage(const evmc::address& address, const evmc::bytes32& storage) {
    for (int i = 0; i < access_list_.access_list.size(); i++) {
       if (access_list_.access_list[i].account == address) {
          for (int j = 0; j < access_list_.access_list[i].storage_keys.size(); j++) {
            if (access_list_.access_list[i].storage_keys[j] == storage) {
               return;
            }
          }
          access_list_.access_list[i].storage_keys.push_back(storage);
          return;
       }
    }
    silkworm::AccessListEntry item;
    item.account = address;
    item.storage_keys.push_back(storage);
    access_list_.access_list.push_back(item);
}

void AccessListTracer::on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept {
    SILKRPC_DEBUG << "on_execution_end:"
        << " result.status_code: " << result.status_code
        << " start_gas: " << std::dec << start_gas_
        << " gas_left: " << std::dec << result.gas_left
        << "\n";
}

} // namespace silkrpc::access_list
