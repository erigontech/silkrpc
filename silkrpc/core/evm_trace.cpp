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
    json["idx"] = std::to_string(trace_op.idx);
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
    json["gas"] = trace_action.gas;
    if (trace_action.input.size() > 0) {
        json["input"] = "0x" + silkworm::to_hex(trace_action.input);
    }
    if (trace_action.init.size() > 0) {
        json["init"] = "0x" + silkworm::to_hex(trace_action.init);
    }
    json["value"] = "0x" + silkworm::to_hex(trace_action.value);
}

void to_json(nlohmann::json& json, const TraceResult& trace_result) {
    json["address"] = trace_result.address;
    json["code"] = "0x" + silkworm::to_hex(trace_result.code);
    json["gasUsed"] = trace_result.gas_used;
}

void to_json(nlohmann::json& json, const Trace& trace) {
    json["action"] = trace.trace_action;
    if (trace.trace_result) {
        json["result"] = trace.trace_result.value();
    }
    json["subtraces"] = trace.sub_traces;
    json["traceAddress"] = trace.trace_address;
    if (trace.error) {
        json["error"] = trace.error.value();
    }
    json["type"] = trace.type;
}

void to_json(nlohmann::json& json, const DiffBalanceEntry& dfe) {
    json["from"] = dfe.from;
    json["to"] = dfe.to;
}

void to_json(nlohmann::json& json, const DiffCodeEntry& dce) {
    json["from"] = dce.from;
    json["to"] = dce.to;
}

void to_json(nlohmann::json& json, const StateDiffEntry& state_diff) {
    json["balance"] = state_diff.balance;
    json["code"] = "=";
    json["nonce"] = "=";
    json["storage"] = state_diff.storage;
}

void to_json(nlohmann::json& json, const TraceCallTraces& result) {
    json["output"] = result.output;
    if (result.state_diff) {
        json["stateDiff"] = result.state_diff.value();
    } else {
        json["stateDiff"] = nlohmann::json::value_t::null;
    }
    if (result.trace) {
        json["trace"] = result.trace.value();
    } else {
        json["trace"] = nlohmann::json::value_t::null;
    }
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
    return (name != nullptr) ?name : "opcode 0x" + evmc::hex(opcode) + " not defined";
}

void VmTraceTracer::on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept {
    if (opcode_names_ == nullptr) {
        opcode_names_ = evmc_get_instruction_names_table(rev);
    }
    start_gas_.push(msg.gas);

    SILKRPC_DEBUG << "VmTraceTracer::on_execution_start:"
        << " depth: " << msg.depth
        << " gas: " << std::dec << msg.gas
        << " recipient: " << evmc::address{msg.recipient}
        << " sender: " << evmc::address{msg.sender}
        << " code: " << silkworm::to_hex(code)
        << " code_address: " << evmc::address{msg.code_address}
        << " input_size: " << msg.input_size
        << "\n";

    if (msg.depth == 0) {
        vm_trace_.code = "0x" + silkworm::to_hex(code);
        trace_stack_.push(vm_trace_);
    } else if (vm_trace_.ops.size() > 0) {
        auto& op = vm_trace_.ops[vm_trace_.ops.size() - 1];
        op.sub = std::make_shared<VmTrace>();
        trace_stack_.push(*op.sub);
        op.sub->code = "0x" + silkworm::to_hex(code);
    }
}

void VmTraceTracer::on_instruction_start(uint32_t pc , const intx::uint256 *stack_top, const int stack_height,
              const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept {
    const auto op_code = execution_state.code[pc];
    auto op_name = get_op_name(opcode_names_, op_code);

    SILKRPC_DEBUG << "VmTraceTracer::on_instruction_start:"
        << " pc: " << std::dec << pc
        << " opcode: 0x" << std::hex << evmc::hex(op_code)
        << " opcode_name: " << op_name
        << " execution_state: {"
        << "   gas_left: " << std::dec << execution_state.gas_left
        << "   status: " << execution_state.status
        << "   msg.gas: " << std::dec << execution_state.msg->gas
        << "   msg.depth: " << std::dec << execution_state.msg->depth
        << "}\n";

    auto& vm_trace = trace_stack_.top().get();

    if (vm_trace.ops.size() > 0) {
        auto& op = vm_trace.ops[vm_trace.ops.size() - 1];
        op.gas_cost = op.gas_cost - execution_state.gas_left;
        op.trace_ex.used = execution_state.gas_left;

        copy_memory(execution_state.memory, op.trace_ex.memory);
        copy_stack(op.op_code, stack_top, op.trace_ex.stack);
    }

    TraceOp trace_op;
    trace_op.gas_cost = execution_state.gas_left;
    trace_op.idx = next_index_++;
    trace_op.op_code = op_code;
    trace_op.op_name = op_name == "KECCAK256" ? "SHA3" : op_name; // TODO(sixtysixter) for RPCDAEMON compatibility
    trace_op.pc = pc;

    copy_memory_offset_len(op_code, stack_top, trace_op.trace_ex.memory);
    copy_store(op_code, stack_top, trace_op.trace_ex.storage);

    vm_trace.ops.push_back(trace_op);
}

void VmTraceTracer::on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept {
    auto& vm_trace = trace_stack_.top().get();
    trace_stack_.pop();

    std::uint64_t start_gas = start_gas_.top();
    start_gas_.pop();

    SILKRPC_LOG << "VmTraceTracer::on_execution_end:"
        << " result.status_code: " << result.status_code
        << " start_gas: " << std::dec << start_gas
        << " gas_left: " << std::dec << result.gas_left
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

    SILKRPC_DEBUG << "TraceTracer::on_execution_start: gas: " << std::dec << msg.gas
        << " depth: " << msg.depth
        << " recipient: " << evmc::address{msg.recipient}
        << " sender: " << evmc::address{msg.sender}
        << "\n";
}

void TraceTracer::on_instruction_start(uint32_t pc , const intx::uint256 *stack_top, const int stack_height,
              const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept {
    const auto opcode = execution_state.code[pc];
    auto opcode_name = get_op_name(opcode_names_, opcode);

    SILKRPC_DEBUG << "TraceTracer::on_instruction_start:"
        << " pc: " << std::dec << pc
        << " opcode: 0x" << std::hex << evmc::hex(opcode)
        << " opcode_name: " << opcode_name
        << " recipient: " << evmc::address{execution_state.msg->recipient}
        << " sender: " << evmc::address{execution_state.msg->sender}
        << " execution_state: {"
        << "   gas_left: " << std::dec << execution_state.gas_left
        << "   status: " << execution_state.status
        << "   msg.gas: " << std::dec << execution_state.msg->gas
        << "   msg.depth: " << std::dec << execution_state.msg->depth
        << "}\n";
}

void TraceTracer::on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept {
    SILKRPC_DEBUG << "TraceTracer::on_execution_end:"
        << " result.status_code: " << result.status_code
        << " start_gas: " << std::dec << start_gas_
        << " gas_left: " << std::dec << result.gas_left
        << "\n";
}

void StateDiffTracer::on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept {
    if (opcode_names_ == nullptr) {
        opcode_names_ = evmc_get_instruction_names_table(rev);
    }

    SILKRPC_DEBUG << "StateDiffTracer::on_execution_start: gas: " << std::dec << msg.gas
        << " depth: " << msg.depth
        << " recipient: " << evmc::address{msg.recipient}
        << " sender: " << evmc::address{msg.sender}
        << "\n";
}

void StateDiffTracer::on_instruction_start(uint32_t pc , const intx::uint256 *stack_top, const int stack_height,
              const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept {
    const auto opcode = execution_state.code[pc];
    auto opcode_name = get_op_name(opcode_names_, opcode);

    SILKRPC_DEBUG << "StateDiffTracer::on_instruction_start:"
        << " pc: " << std::dec << pc
        << " opcode: 0x" << std::hex << evmc::hex(opcode)
        << " opcode_name: " << opcode_name
        << " recipient: " << evmc::address{execution_state.msg->recipient}
        << " sender: " << evmc::address{execution_state.msg->sender}
        << " execution_state: {"
        << "   gas_left: " << std::dec << execution_state.gas_left
        << "   status: " << execution_state.status
        << "   msg.gas: " << std::dec << execution_state.msg->gas
        << "   msg.depth: " << std::dec << execution_state.msg->depth
        << "}\n";
}

void StateDiffTracer::on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept {
    SILKRPC_DEBUG << "StateDiffTracer::on_execution_end:"
        << " result.status_code: " << result.status_code
        << " start_gas: " << std::dec << start_gas_
        << " gas_left: " << std::dec << result.gas_left
        << "\n";
}

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

    EVMExecutor<WorldState, VM> executor{context_, database_reader_, *chain_config_ptr, workers_, block_number};

    for (auto idx = 0; idx < index; idx++) {
        silkrpc::Transaction txn{block.transactions[idx]};

        txn.recover_sender();
        const auto execution_result = co_await executor.call(block, txn);
    }

    silkrpc::Tracers tracers;

    TraceCallResult result;
    TraceCallTraces& traces = result.traces;
    if (config_.vm_trace) {
        traces.vm_trace.emplace();
        std::shared_ptr<silkworm::EvmTracer> tracer = std::make_shared<trace::VmTraceTracer>(traces.vm_trace.value());
        tracers.push_back(tracer);
    }
    if (config_.trace) {
        traces.trace.emplace();
        std::shared_ptr<silkworm::EvmTracer> tracer = std::make_shared<trace::TraceTracer>(traces.trace.value());
        tracers.push_back(tracer);
    }
    if (config_.state_diff) {
        traces.state_diff.emplace();
        std::shared_ptr<silkworm::EvmTracer> tracer = std::make_shared<trace::StateDiffTracer>(traces.state_diff.value());
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
