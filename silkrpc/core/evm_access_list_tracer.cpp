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

#include <memory>

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

    evmc::address recipient(msg.recipient);
    evmc::address sender(msg.sender);
    SILKRPC_DEBUG << "on_execution_start: "
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
       add_storage(recipient, address);
    } else if (opcode_name == "SSTORE" && execution_state.stack.size() > 1) {
       const auto address = silkworm::bytes32_from_hex(intx::hex(execution_state.stack[0]));
       add_storage(recipient, address);
    } else if ((opcode_name == "EXTCODECOPY" || opcode_name == "EXTCODEHASH" || opcode_name == "EXTCODESIZE" || opcode_name == "BALANCE" || opcode_name == "SELFDESTRUCT") &&
                execution_state.stack.size() > 1) {
       const auto address = silkworm::bytes32_from_hex(intx::hex(execution_state.stack[0]));
       if (!exclude(recipient))
          add_address(recipient);
    } else if ((opcode_name == "DELEGATECALL" || opcode_name == "CALL" || opcode_name == "STATICCALL" || opcode_name == "CALLCODE") &&
                execution_state.stack.size() >= 5) {
       const auto address = silkworm::bytes32_from_hex(intx::hex(execution_state.stack[1]));
       if (!exclude(recipient))
          add_address(recipient);
    }
}

inline bool AccessListTracer::exclude(const evmc::address& address) {
       // return (address != from_ && address != to_ && !is_precompiled(address)); // ADD precompiled
       return (address != from_ && address != to_);
}

void AccessListTracer::add_storage(const evmc::address& address, const evmc::bytes32& storage) {
    std::cout << "add_storage:: Address: " << address << " Storage: " << storage << "\n";
    for (int i = 0; i < access_list_.size(); i++) {
       if (access_list_[i].account == address) {
          for (int j = 0; j < access_list_[i].storage_keys.size(); j++) {
            if (access_list_[i].storage_keys[j] == storage) {
               return;
            }
          }
          access_list_[i].storage_keys.push_back(storage);
          return;
       }
    }
    silkworm::AccessListEntry item;
    item.account = address;
    item.storage_keys.push_back(storage);
    access_list_.push_back(item);
}

void AccessListTracer::add_address(const evmc::address& address) {
    for (int i = 0; i < access_list_.size(); i++) {
       if (access_list_[i].account == address) {
          return;
       }
    }
    silkworm::AccessListEntry item;
    item.account = address;
    access_list_.push_back(item);
}

void AccessListTracer::on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept {
    SILKRPC_DEBUG << "on_execution_end:"
        << " result.status_code: " << result.status_code
        << " gas_left: " << std::dec << result.gas_left
        << "\n";
}

void AccessListTracer::add_local_access_list(const std::vector<silkworm::AccessListEntry> input_access_list) {
    for (int i = 0; i < input_access_list.size(); i++) {
       if (!exclude(input_access_list[i].account))
       if (input_access_list[i].account != from_ &&
            input_access_list[i].account != to_) {  // && !is _precompiled(input_access_list[i].account)
          add_address(input_access_list[i].account);
       }
       for (int z = 0; z < input_access_list[i].storage_keys.size(); z++) {
          add_storage(input_access_list[i].account, input_access_list[i].storage_keys[z]);
       }
    }
}

void AccessListTracer::dump(const std::string str, const std::vector<silkworm::AccessListEntry>& acl) {
   std::cout << str << "\n";
   for (int i = 0; i < acl.size(); i++) {
          std::cout << "AccessList Address: " << acl[i].account << "\n";
   }
}

bool AccessListTracer::compare(const std::vector<silkworm::AccessListEntry>& acl1, const std::vector<silkworm::AccessListEntry>& acl2) {
    std::cout << "entering compare\n";
    if (acl1.size() != acl2.size()) {
       std::cout << "exiting compare1\n";
       return false;
    }
    for (int i = 0; i < acl1.size(); i++) {
       bool match_address = false;
       std::cout << "AccessList Address: " << acl1[i].account << "\n";
       for (int j = 0; j < acl2.size(); j++) {
          std::cout << "other->AccessList Address: " << acl2[j].account << "\n";
          if (acl2[j].account == acl1[i].account) {
             match_address = true;
             if (acl2[j].storage_keys.size() != acl1[i].storage_keys.size()) {
                std::cout << "exiting compare2\n";
                return false;
             }
             bool match_storage = false;
             for (int z = 0; z < acl1[i].storage_keys.size(); z++) {
                for (int t = 0; t < acl2[j].storage_keys.size(); t++) {
                   if (acl2[j].storage_keys[t] == acl1[i].storage_keys[z]) {
                      match_storage = true;
                      break;
                   }
                }
                if (!match_storage) {
                   std::cout << "exiting compare3\n";
                   return false;
                }
             }
             break;
          }
       }
       if (!match_address) {
          std::cout << "exiting compare4\n";
          return false;
       }
    }

    std::cout << "exiting compare true\n";
    return true;
}

#ifdef notdef

void AccessListTracer::dump(std::string str) {
   std::cout << str << "\n";
   for (int i = 0; i < access_list_.size(); i++) {
          std::cout << "AccessList Address: " << access_list_[i].account << "\n";
   }
}

bool AccessListTracer::compare(std::shared_ptr<silkrpc::access_list::AccessListTracer> other) {
    std::cout << "entering compare\n";
    if (access_list_.size() != other->access_list_.size()) {
       std::cout << "exiting compare1\n";
       return false;
    }

    for (int i = 0; i < access_list_.size(); i++) {
       bool match_address = false;
       std::cout << "AccessList Address: " << access_list_[i].account << "\n";
       for (int j = 0; j < other->access_list_.size(); j++) {
          std::cout << "other->AccessList Address: " << other->access_list_[j].account << "\n";
          if (other->access_list_[j].account == access_list_[i].account) {
             match_address = true;
             if (other->access_list_[j].storage_keys.size() != access_list_[i].storage_keys.size()) {
                std::cout << "exiting compare2\n";
                return false;
             }
             bool match_storage = false;
             for (int z = 0; z < access_list_[i].storage_keys.size(); z++) {
                for (int t = 0; t < other->access_list_[j].storage_keys.size(); t++) {
                   if (other->access_list_[j].storage_keys[t] == access_list_[i].storage_keys[z]) {
                      match_storage = true;
                      break;
                   }
                }
                if (!match_storage) {
                   std::cout << "exiting compare3\n";
                   return false;
                }
             }
             break;
          }
       }
       if (!match_address) {
          std::cout << "exiting compare4\n";
          return false;
       }
    }

    std::cout << "exiting compare true\n";
    return true;
}
#endif

} // namespace silkrpc::access_list
