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

#include "evm_trace.hpp"

#include <algorithm>
#include <memory>
#include <stack>
#include <string>

#include <evmc/hex.hpp>
#include <evmc/instructions.h>
#include <intx/intx.hpp>
#include <silkworm/core/silkworm/common/endian.hpp>
#include <silkworm/third_party/evmone/lib/evmone/execution_state.hpp>
#include <silkworm/third_party/evmone/lib/evmone/instructions.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/core/evm_executor.hpp>
#include <silkrpc/core/rawdb/chain.hpp>
#include <silkrpc/json/types.hpp>

namespace silkrpc::trace {

using evmc::literals::operator""_address;

const std::uint8_t CODE_PUSH1 = evmc_opcode::OP_PUSH1;
const std::uint8_t CODE_DUP1 = evmc_opcode::OP_DUP1;

std::ostream& operator<<(std::ostream& out, const TraceConfig& tc) {
    out << "vmTrace: " << std::boolalpha << tc.vm_trace;
    out << " Trace: " << std::boolalpha << tc.trace;
    out << " stateDiff: " << std::boolalpha << tc.state_diff;

    return out;
}

void to_json(nlohmann::json& json, const VmTrace& vm_trace) {
    json["code"] = vm_trace.code;
    json["ops"] = vm_trace.ops;
}

void to_json(nlohmann::json& json, const TraceOp& trace_op) {
    json["cost"] = trace_op.gas_cost;
    json["ex"] = trace_op.trace_ex;
    json["idx"] = trace_op.idx;
    json["op"] = trace_op.op_name;
    json["pc"] = trace_op.pc;
    if (trace_op.sub) {
        json["sub"] = *trace_op.sub;
    } else {
        json["sub"] = nlohmann::json::value_t::null;
    }
}

void to_json(nlohmann::json& json, const TraceEx& trace_ex) {
    if (trace_ex.memory) {
        const auto& memory = trace_ex.memory.value();
        json["mem"] = memory;
    } else {
        json["mem"] = nlohmann::json::value_t::null;
    }

    json["push"] = trace_ex.stack;
    if (trace_ex.storage) {
        const auto& storage = trace_ex.storage.value();
        json["store"] = storage;
    } else {
        json["store"] = nlohmann::json::value_t::null;
    }
    json["used"] = trace_ex.used;
}

void to_json(nlohmann::json& json, const TraceMemory& trace_memory) {
    json = {
        {"data", trace_memory.data},
        {"off", trace_memory.offset}
    };
}

void to_json(nlohmann::json& json, const TraceStorage& trace_storage) {
    json = {
        {"key", trace_storage.key},
        {"val", trace_storage.value}
    };
}

void to_json(nlohmann::json& json, const TraceAction& trace_action) {
    if (trace_action.call_type) {
        json["callType"] = trace_action.call_type.value();
    }
    json["from"] = trace_action.from;
    if (trace_action.to) {
        json["to"] = trace_action.to.value();
    }
    std::ostringstream ss;
    ss << "0x" << std::hex << trace_action.gas;
    json["gas"] = ss.str();
    if (trace_action.input) {
        json["input"] = "0x" + silkworm::to_hex(trace_action.input.value());
    }
    if (trace_action.init) {
        json["init"] = "0x" + silkworm::to_hex(trace_action.init.value());
    }
    json["value"] = to_quantity(trace_action.value);
}

void to_json(nlohmann::json& json, const TraceResult& trace_result) {
    if (trace_result.address) {
        json["address"] = trace_result.address.value();
    }
    if (trace_result.code) {
        json["code"] = "0x" + silkworm::to_hex(trace_result.code.value());
    }
    if (trace_result.output) {
        json["output"] = "0x" + silkworm::to_hex(trace_result.output.value());
    }
    std::ostringstream ss;
    ss << "0x" << std::hex << trace_result.gas_used;
    json["gasUsed"] = ss.str();
}

void to_json(nlohmann::json& json, const Trace& trace) {
    json["action"] = trace.trace_action;
    if (trace.trace_result) {
        json["result"] = trace.trace_result.value();
    } else {
        json["result"] = nlohmann::json::value_t::null;
    }
    json["subtraces"] = trace.sub_traces;
    json["traceAddress"] = trace.trace_address;
    if (trace.error) {
        json["error"] = trace.error.value();
    }
    json["type"] = trace.type;
}

void to_json(nlohmann::json& json, const DiffValue& dv) {
    if (dv.from && dv.to) {
        json["*"] = {
            {"from", dv.from.value()},
            {"to", dv.to.value()}
        };
    } else if (dv.from) {
        json["-"] = dv.from.value();
    } else if (dv.to) {
        json["+"] = dv.to.value();
    } else {
        json = "=";
    }
}

void to_json(nlohmann::json& json, const StateDiffEntry& state_diff) {
    json["balance"] = state_diff.balance;
    json["code"] = state_diff.code;
    json["nonce"] = state_diff.nonce;
    json["storage"] = state_diff.storage;
}

void to_json(nlohmann::json& json, const TraceCallTraces& result) {
    json["output"] = result.output;
    if (result.state_diff) {
        json["stateDiff"] = result.state_diff.value();
    } else {
        json["stateDiff"] = nlohmann::json::value_t::null;
    }
    json["trace"] = result.trace;
    if (result.vm_trace) {
        json["vmTrace"] = result.vm_trace.value();
    } else {
        json["vmTrace"] = nlohmann::json::value_t::null;
    }
}

int get_stack_count(std::uint8_t op_code) {
    int count = 0;
    switch (op_code) {
        case evmc_opcode::OP_PUSH1 ... evmc_opcode::OP_PUSH32:
            count = 1;
            break;
        case evmc_opcode::OP_SWAP1 ... evmc_opcode::OP_SWAP16:
            count = op_code - evmc_opcode::OP_SWAP1 + 2;
            break;
        case evmc_opcode::OP_DUP1 ... evmc_opcode::OP_DUP16:
            count = op_code - evmc_opcode::OP_DUP1 + 2;
            break;
        case evmc_opcode::OP_CALLDATALOAD:
        case evmc_opcode::OP_SLOAD:
        case evmc_opcode::OP_MLOAD:
        case evmc_opcode::OP_CALLDATASIZE:
        case evmc_opcode::OP_LT:
        case evmc_opcode::OP_GT:
        case evmc_opcode::OP_DIV:
        case evmc_opcode::OP_SDIV:
        case evmc_opcode::OP_SAR:
        case evmc_opcode::OP_AND:
        case evmc_opcode::OP_EQ:
        case evmc_opcode::OP_CALLVALUE:
        case evmc_opcode::OP_ISZERO:
        case evmc_opcode::OP_ADD:
        case evmc_opcode::OP_EXP:
        case evmc_opcode::OP_CALLER:
        case evmc_opcode::OP_KECCAK256:
        case evmc_opcode::OP_SUB:
        case evmc_opcode::OP_ADDRESS:
        case evmc_opcode::OP_GAS:
        case evmc_opcode::OP_MUL:
        case evmc_opcode::OP_RETURNDATASIZE:
        case evmc_opcode::OP_NOT:
        case evmc_opcode::OP_SHR:
        case evmc_opcode::OP_SHL:
        case evmc_opcode::OP_EXTCODESIZE:
        case evmc_opcode::OP_SLT:
        case evmc_opcode::OP_OR:
        case evmc_opcode::OP_NUMBER:
        case evmc_opcode::OP_PC:
        case evmc_opcode::OP_TIMESTAMP:
        case evmc_opcode::OP_BALANCE:
        case evmc_opcode::OP_SELFBALANCE:
        case evmc_opcode::OP_MULMOD:
        case evmc_opcode::OP_ADDMOD:
        case evmc_opcode::OP_BASEFEE:
        case evmc_opcode::OP_BLOCKHASH:
        case evmc_opcode::OP_BYTE:
        case evmc_opcode::OP_XOR:
        case evmc_opcode::OP_ORIGIN:
        case evmc_opcode::OP_CODESIZE:
        case evmc_opcode::OP_MOD:
        case evmc_opcode::OP_SIGNEXTEND:
        case evmc_opcode::OP_GASLIMIT:
        case evmc_opcode::OP_DIFFICULTY:
        case evmc_opcode::OP_SGT:
        case evmc_opcode::OP_GASPRICE:
        case evmc_opcode::OP_MSIZE:
        case evmc_opcode::OP_EXTCODEHASH:
        case evmc_opcode::OP_STATICCALL:
        case evmc_opcode::OP_DELEGATECALL:
        case evmc_opcode::OP_CALL:
        case evmc_opcode::OP_CALLCODE:
        case evmc_opcode::OP_CREATE:
        case evmc_opcode::OP_CREATE2:
            count = 1;
            break;
        default:
            count = 0;
            break;
    }

    return count;
}

void copy_stack(std::uint8_t op_code, const evmone::uint256* stack, std::vector<std::string>& trace_stack) {
    int top = get_stack_count(op_code);
    trace_stack.reserve(top);
    for (int i = top - 1; i >= 0; i--) {
        trace_stack.push_back("0x" + intx::to_string(stack[-i], 16));
    }
}

void copy_memory(const evmone::Memory& memory, std::optional<TraceMemory>& trace_memory) {
    if (trace_memory) {
        TraceMemory& tm = trace_memory.value();
        if (tm.len == 0) {
            trace_memory.reset();
            return;
        }
        tm.data = "0x";
        const auto data = memory.data();
        auto start = tm.offset;
        for (int idx = 0; idx < tm.len; idx++) {
            std::string entry{evmc::hex({data + start + idx, 1})};
            tm.data.append(entry);
        }
    }
}

void copy_store(std::uint8_t op_code, const evmone::uint256* stack, std::optional<TraceStorage>& trace_storage) {
    if (op_code == evmc_opcode::OP_SSTORE) {
        trace_storage = TraceStorage{"0x" + intx::to_string(stack[0], 16), "0x" + intx::to_string(stack[-1], 16)};
    }
}

void copy_memory_offset_len(std::uint8_t op_code, const evmone::uint256* stack, std::optional<TraceMemory>& trace_memory) {
    switch (op_code) {
        case evmc_opcode::OP_MSTORE:
        case evmc_opcode::OP_MLOAD:
            trace_memory = TraceMemory{stack[0][0], 32};
            break;
        case evmc_opcode::OP_MSTORE8:
            trace_memory = TraceMemory{stack[0][0], 1};
            break;
        case evmc_opcode::OP_RETURNDATACOPY:
        case evmc_opcode::OP_CALLDATACOPY:
        case evmc_opcode::OP_CODECOPY:
            trace_memory = TraceMemory{stack[0][0], stack[-2][0]};
            break;
        case evmc_opcode::OP_STATICCALL:
        case evmc_opcode::OP_DELEGATECALL:
            trace_memory = TraceMemory{stack[-4][0], stack[-5][0]};
            break;
        case evmc_opcode::OP_CALL:
        case evmc_opcode::OP_CALLCODE:
            trace_memory = TraceMemory{stack[-5][0], stack[-6][0]};
            break;
        case evmc_opcode::OP_CREATE:
        case evmc_opcode::OP_CREATE2:
            trace_memory = TraceMemory{0, 0};
            break;
        default:
            break;
    }
}

void push_memory_offset_len(std::uint8_t op_code, const evmone::uint256* stack, std::stack<TraceMemory>& tms) {
    switch (op_code) {
        case evmc_opcode::OP_STATICCALL:
        case evmc_opcode::OP_DELEGATECALL:
            tms.push(TraceMemory{stack[-4][0], stack[-5][0]});
            break;
        case evmc_opcode::OP_CALL:
        case evmc_opcode::OP_CALLCODE:
            tms.push(TraceMemory{stack[-5][0], stack[-6][0]});
            break;
        case evmc_opcode::OP_CREATE:
        case evmc_opcode::OP_CREATE2:
            tms.push(TraceMemory{0, 0});
            break;
        default:
            break;
    }
}

std::string get_op_name(const char* const* names, std::uint8_t opcode) {
    const auto name = names[opcode];
    if (name != nullptr) {
        return name;
     }
    auto hex = evmc::hex(opcode);
    if (opcode < 16) {
        hex = hex.substr(1);
    }
    return "opcode 0x" + hex + " not defined";
}

static const char* PADDING = "0x0000000000000000000000000000000000000000000000000000000000000000";
std::string to_string(intx::uint256 value) {
    auto out = intx::to_string(value, 16);
    std::string padding = std::string{PADDING};
    return padding.substr(0, padding.size() - out.size()) + out;
}

void VmTraceTracer::on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept {
    if (opcode_names_ == nullptr) {
        opcode_names_ = evmc_get_instruction_names_table(rev);
    }
    start_gas_.push(msg.gas);

    SILKRPC_DEBUG << "VmTraceTracer::on_execution_start:"
        << " depth: " << msg.depth
        << ", gas: " << std::dec << msg.gas
        << ", recipient: " << evmc::address{msg.recipient}
        << ", sender: " << evmc::address{msg.sender}
        << ", code: " << silkworm::to_hex(code)
        << ", code_address: " << evmc::address{msg.code_address}
        << ", input_size: " << msg.input_size
        << "\n";

    if (msg.depth == 0) {
        vm_trace_.code = "0x" + silkworm::to_hex(code);
        traces_stack_.push(vm_trace_);
        if (transaction_index_ == -1) {
            index_prefix_.push("");
        } else {
            index_prefix_.push(std::to_string(transaction_index_) + "-");
        }
    } else if (vm_trace_.ops.size() > 0) {
        auto& vm_trace = traces_stack_.top().get();

        auto index_prefix = index_prefix_.top();
        index_prefix = index_prefix + std::to_string(vm_trace.ops.size() - 1) + "-";
        index_prefix_.push(index_prefix);

        auto& op = vm_trace.ops[vm_trace.ops.size() - 1];
        op.sub = std::make_shared<VmTrace>();
        traces_stack_.push(*op.sub);
        op.sub->code = "0x" + silkworm::to_hex(code);
    }
}

void VmTraceTracer::on_instruction_start(uint32_t pc , const intx::uint256 *stack_top, const int stack_height,
              const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept {
    const auto op_code = execution_state.code[pc];
    auto op_name = get_op_name(opcode_names_, op_code);

    SILKRPC_DEBUG << "VmTraceTracer::on_instruction_start:"
        << " pc: " << std::dec << pc
        << ", opcode: 0x" << std::hex << evmc::hex(op_code)
        << ", opcode_name: " << op_name
        << ", execution_state: {"
        << "   gas_left: " << std::dec << execution_state.gas_left
        << ",   status: " << execution_state.status
        << ",   msg.gas: " << std::dec << execution_state.msg->gas
        << ",   msg.depth: " << std::dec << execution_state.msg->depth
        << "}\n";

    auto& vm_trace = traces_stack_.top().get();

    if (vm_trace.ops.size() > 0) {
        auto& op = vm_trace.ops[vm_trace.ops.size() - 1];
        if (op.call_gas) {
            op.gas_cost = op.gas_cost - op.call_gas.value();
        } else {
            op.gas_cost = op.gas_cost - execution_state.gas_left;
        }
        op.trace_ex.used = execution_state.gas_left;

        copy_memory(execution_state.memory, op.trace_ex.memory);
        copy_stack(op.op_code, stack_top, op.trace_ex.stack);
    }

    auto index_prefix = index_prefix_.top() + std::to_string(vm_trace.ops.size());

    TraceOp trace_op;
    trace_op.gas_cost = execution_state.gas_left;
    trace_op.idx = index_prefix;
    trace_op.op_code = op_code;
    trace_op.op_name = op_name == "KECCAK256" ? "SHA3" : op_name; // TODO(sixtysixter) for RPCDAEMON compatibility
    trace_op.pc = pc;

    copy_memory_offset_len(op_code, stack_top, trace_op.trace_ex.memory);
    copy_store(op_code, stack_top, trace_op.trace_ex.storage);

    vm_trace.ops.push_back(trace_op);
}

void VmTraceTracer::on_precompiled_run(const evmc::result& result, int64_t gas, const silkworm::IntraBlockState& intra_block_state) noexcept {
    SILKRPC_DEBUG << "VmTraceTracer::on_precompiled_run:"
        << " status: " << result.status_code
        << ", gas: " << std::dec << gas
        << "\n";


    if (vm_trace_.ops.size() > 0) {
        auto& op = vm_trace_.ops[vm_trace_.ops.size() - 1];
        op.call_gas = gas;
        op.sub = std::make_shared<VmTrace>();
        op.sub->code = "0x";
    }
}

void VmTraceTracer::on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept {
    auto& vm_trace = traces_stack_.top().get();
    traces_stack_.pop();

    std::uint64_t start_gas = start_gas_.top();
    start_gas_.pop();

    index_prefix_.pop();

    SILKRPC_DEBUG << "VmTraceTracer::on_execution_end:"
        << " result.status_code: " << result.status_code
        << ", start_gas: " << std::dec << start_gas
        << ", gas_left: " << std::dec << result.gas_left
        << "\n";

    if (vm_trace.ops.size() == 0) {
        return;
    }
    auto& op = vm_trace.ops[vm_trace.ops.size() - 1];

    if (op.op_code == evmc_opcode::OP_STOP && vm_trace.ops.size() == 1) {
        vm_trace.ops.clear();
        return;
    }
    switch (result.status_code) {
    case evmc_status_code::EVMC_REVERT:
    case evmc_status_code::EVMC_OUT_OF_GAS:
        op.trace_ex.used = op.gas_cost;
        op.gas_cost = 0;
        break;

    case evmc_status_code::EVMC_UNDEFINED_INSTRUCTION:
        op.trace_ex.used = op.gas_cost;
        op.gas_cost = start_gas - op.gas_cost;
        op.trace_ex.used -= op.gas_cost;
        break;

    default:
        op.gas_cost = op.gas_cost - result.gas_left;
        op.trace_ex.used = result.gas_left;
        break;
    }
}

void TraceTracer::on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept {
    if (opcode_names_ == nullptr) {
        opcode_names_ = evmc_get_instruction_names_table(rev);
    }

    auto sender = evmc::address{msg.sender};
    auto recipient = evmc::address{msg.recipient};
    auto code_address = evmc::address{msg.code_address};

    current_depth_ = msg.depth;

    auto create = !initial_ibs_.exists(recipient) && created_address_.find(recipient) == created_address_.end();

    start_gas_.push(msg.gas);

    std::uint32_t index = traces_.size();
    traces_.resize(traces_.size() + 1);

    Trace& trace = traces_[index];
    trace.type = create ? "create" : "call";

    TraceAction& trace_action = trace.trace_action;
    trace_action.from = sender;
    trace_action.gas = msg.gas;
    trace_action.value = intx::be::load<intx::uint256>(msg.value);

    trace.trace_result.emplace();
    if (create) {
        created_address_.insert(recipient);
        trace_action.init = code;
        trace.trace_result->code.emplace();
        trace.trace_result->address = recipient;
    } else {
        trace.trace_result->output.emplace();
        trace_action.input = silkworm::ByteView{msg.input_data, msg.input_size};
        trace_action.to = recipient;
        bool in_static_mode = (msg.flags & evmc_flags::EVMC_STATIC) != 0;
        switch (msg.kind) {
            case evmc_call_kind::EVMC_CALL:
                trace_action.call_type = in_static_mode ? "staticcall" : "call";
                break;
            case evmc_call_kind::EVMC_DELEGATECALL:
                trace_action.call_type = "delegatecall";
                break;
            case evmc_call_kind::EVMC_CALLCODE:
                trace_action.call_type = "callcode";
                break;
            case evmc_call_kind::EVMC_CREATE:
            case evmc_call_kind::EVMC_CREATE2:
                break;
        }
    }

    if (msg.depth > 0) {
        if (index_stack_.size() > 0) {
            auto index = index_stack_.top();
            Trace& calling_trace = traces_[index];

            trace.trace_address.push_back(calling_trace.sub_traces);
            calling_trace.sub_traces++;
        }
    } else {
        initial_gas_ = msg.gas;
    }
    index_stack_.push(index);

    SILKRPC_DEBUG << "TraceTracer::on_execution_start: gas: " << std::dec << msg.gas
        << " create: " << create
        << ", depth: " << msg.depth
        << ", sender: " << sender
        << ", recipient: " << recipient << " (created: " << create << ")"
        << ", code_address: " << code_address
        << ", msg.value: " << intx::hex(intx::be::load<intx::uint256>(msg.value))
        << ", code: " << silkworm::to_hex(code)
        << "\n";
}

void TraceTracer::on_instruction_start(uint32_t pc , const intx::uint256 *stack_top, const int stack_height,
              const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept {
    const auto opcode = execution_state.code[pc];
    auto opcode_name = get_op_name(opcode_names_, opcode);

    SILKRPC_DEBUG << "TraceTracer::on_instruction_start:"
        << " pc: " << std::dec << pc
        << ", opcode: 0x" << std::hex << evmc::hex(opcode)
        << ", opcode_name: " << opcode_name
        << ", recipient: " << evmc::address{execution_state.msg->recipient}
        << ", sender: " << evmc::address{execution_state.msg->sender}
        << ", execution_state: {"
        << "   gas_left: " << std::dec << execution_state.gas_left
        << ",   status: " << execution_state.status
        << ",   msg.gas: " << std::dec << execution_state.msg->gas
        << ",   msg.depth: " << std::dec << execution_state.msg->depth
        << "}\n";
}

void TraceTracer::on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept {
    auto index = index_stack_.top();
    index_stack_.pop();

    auto start_gas = start_gas_.top();
    start_gas_.pop();

    Trace& trace = traces_[index];

    if (current_depth_) {
        trace.trace_result->output = silkworm::ByteView{result.output_data, result.output_size};
    }

    current_depth_--;

    switch (result.status_code) {
        case evmc_status_code::EVMC_SUCCESS:
            trace.trace_result->gas_used = start_gas - result.gas_left;
            break;
        break;
        case evmc_status_code::EVMC_REVERT:
            trace.error = "Reverted";
            trace.trace_result.reset();
            break;
        case evmc_status_code::EVMC_OUT_OF_GAS:
        case evmc_status_code::EVMC_STACK_OVERFLOW:
            trace.error = "Out of gas";
            trace.trace_result.reset();
            break;
        case evmc_status_code::EVMC_UNDEFINED_INSTRUCTION:
        case evmc_status_code::EVMC_INVALID_INSTRUCTION:
            trace.error = "Bad instruction";
            trace.trace_result.reset();
            break;
        case evmc_status_code::EVMC_STACK_UNDERFLOW:
            trace.error = "Stack underflow";
            trace.trace_result.reset();
            break;
        case evmc_status_code::EVMC_BAD_JUMP_DESTINATION:
            trace.error = "Bad jump destination";
            trace.trace_result.reset();
            break;
        default:
            trace.error = "";
            trace.trace_result.reset();
            break;
    }

    SILKRPC_DEBUG << "TraceTracer::on_execution_end:"
        << " result.status_code: " << result.status_code
        << " start_gas: " << std::dec << start_gas
        << " gas_left: " << std::dec << result.gas_left
        << "\n";
}

void TraceTracer::on_reward_granted(const silkworm::CallResult& result, const silkworm::IntraBlockState& intra_block_state) noexcept {
    SILKRPC_DEBUG << "TraceTracer::on_reward_granted:"
        << " result.status_code: " << result.status
        << ", result.gas_left: " << result.gas_left
        << ", initial_gas: " << std::dec << initial_gas_
        << ", result.data: " << silkworm::to_hex(result.data)
        << "\n";

    // reward only on firts trace
    if (traces_.size() == 0) {
        return;
    }
    Trace& trace = traces_[0];

    switch (result.status) {
        case evmc_status_code::EVMC_SUCCESS:
            trace.trace_result->gas_used = initial_gas_ - result.gas_left;
            if (result.data.size() > 0) {
                if (trace.trace_result->code) {
                    trace.trace_result->code = result.data;
                } else if (trace.trace_result->output) {
                    trace.trace_result->output = result.data;
                }
            }
            break;
        case evmc_status_code::EVMC_REVERT:
            trace.error = "Reverted";
            trace.trace_result.reset();
            break;
        case evmc_status_code::EVMC_OUT_OF_GAS:
        case evmc_status_code::EVMC_STACK_OVERFLOW:
            trace.error = "Out of gas";
            trace.trace_result.reset();
            break;
        case evmc_status_code::EVMC_UNDEFINED_INSTRUCTION:
        case evmc_status_code::EVMC_INVALID_INSTRUCTION:
            trace.error = "Bad instruction";
            trace.trace_result.reset();
            break;
        case evmc_status_code::EVMC_STACK_UNDERFLOW:
            trace.error = "Stack underflow";
            trace.trace_result.reset();
            break;
        case evmc_status_code::EVMC_BAD_JUMP_DESTINATION:
            trace.error = "Bad jump destination";
            trace.trace_result.reset();
            break;
        default:
            trace.error = "";
            trace.trace_result.reset();
            break;
    }
}

void StateDiffTracer::on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept {
    if (opcode_names_ == nullptr) {
        opcode_names_ = evmc_get_instruction_names_table(rev);
    }

    auto recipient = evmc::address{msg.recipient};
    code_[recipient] = code;

    auto exists = initial_ibs_.exists(recipient);

    SILKRPC_DEBUG << "StateDiffTracer::on_execution_start: gas: " << std::dec << msg.gas
        << ", depth: " << msg.depth
        << ", sender: " << evmc::address{msg.sender}
        << ", recipient: " << recipient << " (exists: " << exists << ")"
        << ", code: " << silkworm::to_hex(code)
        << "\n";
}

void StateDiffTracer::on_instruction_start(uint32_t pc , const intx::uint256 *stack_top, const int stack_height,
              const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept {
    const auto opcode = execution_state.code[pc];
    auto opcode_name = get_op_name(opcode_names_, opcode);

    if (opcode == evmc_opcode::OP_SSTORE) {
        auto key = to_string(stack_top[0]);
        auto value = to_string(stack_top[-1]);
        auto address = evmc::address{execution_state.msg->recipient};
        auto original_value = intra_block_state.get_original_storage(address, silkworm::bytes32_from_hex(key));

        auto& keys = diff_storage_[address];
        keys.insert(key);
    }

    SILKRPC_DEBUG << "StateDiffTracer::on_instruction_start:"
        << " pc: " << std::dec << pc
        << ", opcode_name: " << opcode_name
        << ", recipient: " << evmc::address{execution_state.msg->recipient}
        << ", sender: " << evmc::address{execution_state.msg->sender}
        << ", execution_state: {"
        << "   gas_left: " << std::dec << execution_state.gas_left
        << ",   status: " << execution_state.status
        << ",   msg.gas: " << std::dec << execution_state.msg->gas
        << ",   msg.depth: " << std::dec << execution_state.msg->depth
        << "}\n";
}

void StateDiffTracer::on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept {
    SILKRPC_DEBUG << "StateDiffTracer::on_execution_end:"
        << " result.status_code: " << result.status_code
        << ", gas_left: " << std::dec << result.gas_left
        << "\n";
}

void StateDiffTracer::on_reward_granted(const silkworm::CallResult& result, const silkworm::IntraBlockState& intra_block_state) noexcept {
    SILKRPC_DEBUG << "StateDiffTracer::on_reward_granted:"
        << " result.status_code: " << result.status
        << ", result.gas_left: " << result.gas_left
        << "\n";

    for (const auto& address : intra_block_state.touched()) {
        auto initial_exists = initial_ibs_.exists(address);
        auto exists = intra_block_state.exists(address);
        auto& diff_storage = diff_storage_[address];

        auto address_key = "0x" + silkworm::to_hex(address);
        auto& entry = state_diff_[address_key];
        if (initial_exists) {
            auto initial_balance = initial_ibs_.get_balance(address);
            auto initial_code = initial_ibs_.get_code(address);
            auto initial_nonce = initial_ibs_.get_nonce(address);
            if (exists) {
                bool all_equals = true;
                auto final_balance = intra_block_state.get_balance(address);
                if (initial_balance != final_balance) {
                    all_equals = false;
                    entry.balance = DiffValue {
                        "0x" + intx::to_string(initial_balance, 16),
                        "0x" + intx::to_string(final_balance, 16)
                    };
                }
                auto final_code = intra_block_state.get_code(address);
                if (initial_code != final_code) {
                    all_equals = false;
                    entry.code = DiffValue {
                        "0x" + silkworm::to_hex(initial_code),
                        "0x" + silkworm::to_hex(final_code)
                    };
                }
                auto final_nonce = intra_block_state.get_nonce(address);
                if (initial_nonce != final_nonce) {
                    all_equals = false;
                    entry.nonce = DiffValue {
                        to_quantity(initial_nonce),
                        to_quantity(final_nonce)
                    };
                }
                for (auto& key : diff_storage) {
                    auto key_b32 = silkworm::bytes32_from_hex(key);
                    auto initial_storage = intra_block_state.get_original_storage(address, key_b32);
                    auto final_storage = intra_block_state.get_current_storage(address, key_b32);
                    if (initial_storage != final_storage) {
                        all_equals = false;
                        entry.storage[key] = DiffValue{
                            silkworm::to_hex(intra_block_state.get_original_storage(address, key_b32)),
                            silkworm::to_hex(intra_block_state.get_current_storage(address, key_b32))
                        };
                    }
                }
                if (all_equals) {
                    state_diff_.erase(address_key);
                }
            } else {
                entry.balance = DiffValue {
                    "0x" + intx::to_string(initial_balance, 16)
                };
                entry.code = DiffValue {
                    "0x" + silkworm::to_hex(initial_code)
                };
                entry.nonce = DiffValue {
                    to_quantity(initial_nonce)
                };
                for (auto& key : diff_storage) {
                    auto key_b32 = silkworm::bytes32_from_hex(key);
                    entry.storage[key] = DiffValue{
                        "0x" + silkworm::to_hex(intra_block_state.get_original_storage(address, key_b32))
                    };
                }
            }
        } else if (exists) {
            entry.balance = DiffValue {
                {},
                "0x" + intx::to_string(intra_block_state.get_balance(address), 16)
            };

            auto code = intra_block_state.get_code(address);

            entry.code = DiffValue {
                {},
                "0x" + silkworm::to_hex(code)
            };
            entry.nonce = DiffValue {
                {},
                to_quantity(intra_block_state.get_nonce(address))
            };
            for (auto& key : diff_storage) {
                auto key_b32 = silkworm::bytes32_from_hex(key);
                entry.storage[key] = DiffValue{
                    {},
                    "0x" + silkworm::to_hex(intra_block_state.get_current_storage(address, key_b32))
                };
            }
        }
    }
};

template<typename WorldState, typename VM>
asio::awaitable<TraceCallResult> TraceCallExecutor<WorldState, VM>::execute(const silkworm::Block& block, const silkrpc::Call& call) {
    silkrpc::Transaction transaction{call.to_transaction()};
    auto result = co_await execute(block.header.number, block, transaction, -1);
    co_return result;
}

template<typename WorldState, typename VM>
asio::awaitable<TraceCallResult> TraceCallExecutor<WorldState, VM>::execute(std::uint64_t block_number, const silkworm::Block& block,
    const silkrpc::Transaction& transaction, std::int32_t index) {
    SILKRPC_INFO << "execute: "
        << " block_number: " << block_number
        << " transaction: {" << transaction << "}"
        << " index: " << std::dec << index
        << " config: " << config_
        << "\n";

    const auto chain_id = co_await core::rawdb::read_chain_id(database_reader_);

    const auto chain_config_ptr = silkworm::lookup_chain_config(chain_id);

    EVMExecutor<WorldState, VM> executor{io_context_, database_reader_, *chain_config_ptr, workers_, block_number};

    for (auto idx = 0; idx < index; idx++) {
        silkrpc::Transaction txn{block.transactions[idx]};

        if (!txn.from) {
           txn.recover_sender();
        }
        const auto execution_result = co_await executor.call(block, txn);
    }
    executor.reset();

    state::RemoteState remote_state{io_context_, database_reader_, block_number};
    silkworm::IntraBlockState initial_ibs{remote_state};

    Tracers tracers;
    TraceCallResult result;
    TraceCallTraces& traces = result.traces;
    if (config_.vm_trace) {
        traces.vm_trace.emplace();
        std::shared_ptr<silkworm::EvmTracer> tracer = std::make_shared<trace::VmTraceTracer>(traces.vm_trace.value(), index);
        tracers.push_back(tracer);
    }
    if (config_.trace) {
        std::shared_ptr<silkworm::EvmTracer> tracer = std::make_shared<trace::TraceTracer>(traces.trace, initial_ibs);
        tracers.push_back(tracer);
    }
    if (config_.state_diff) {
        traces.state_diff.emplace();

        std::shared_ptr<silkworm::EvmTracer> tracer = std::make_shared<trace::StateDiffTracer>(traces.state_diff.value(), initial_ibs);
        tracers.push_back(tracer);
    }
    auto execution_result = co_await executor.call(block, transaction, /*refund=*/true, /*gas_bailout=*/true, tracers);

    if (execution_result.pre_check_error) {
        result.pre_check_error = execution_result.pre_check_error.value();
    } else {
        traces.output = "0x" + silkworm::to_hex(execution_result.data);
    }

    co_return result;
}

template class TraceCallExecutor<>;

} // namespace silkrpc::trace
